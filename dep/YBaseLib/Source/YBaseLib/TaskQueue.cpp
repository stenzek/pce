#include "YBaseLib/TaskQueue.h"
#include "YBaseLib/Memory.h"
#include "YBaseLib/String.h"

// Pointer to whether the current thread is a worker thread for a task queue.
// This won't work if a task calls ExecuteQueuedTasks for another queue, but
// if you're doing that, it's probably a "bad idea" anyway.
Y_DECLARE_THREAD_LOCAL(TaskQueue*) s_pCurrentThreadTaskQueue = nullptr;
static inline void BeginThreadTaskQueueProcessing(TaskQueue* pTaskQueue)
{
  DebugAssert(s_pCurrentThreadTaskQueue == nullptr);
  s_pCurrentThreadTaskQueue = pTaskQueue;
}
static inline void EndThreadTaskQueueProcessing(TaskQueue* pTaskQueue)
{
  DebugAssert(s_pCurrentThreadTaskQueue == pTaskQueue);
  s_pCurrentThreadTaskQueue = nullptr;
}

TaskQueue::TaskQueue()
  : m_workerThreadExitFlag(true), m_pThreadPool(nullptr), m_activeThreadPoolTasks(0),
    m_threadPoolYieldToOtherJobs(false), m_activeWorkerThreads(0), m_allocatedBarrierCount(0)
{
}

TaskQueue::~TaskQueue()
{
  // cleanup thread pool tasks
  if (m_pThreadPool != nullptr)
  {
    for (;;)
    {
      m_queueLock.Lock();
      if (m_activeThreadPoolTasks == 0)
        break;

      // loop until empty
      m_queueLock.Unlock();
      Thread::Yield();
      continue;
    }

    // tasks are now done, so kill off the references
    while (m_threadPoolTasks.GetSize() > 0)
    {
      ThreadPoolTask* pTask = m_threadPoolTasks.PopBack();
      pTask->Release();
    }

    // release lock
    m_queueLock.Unlock();
  }
  else if (m_workerThreads.IsEmpty())
  {
    // We are using this as a non-threaded task queue.
    EndThreadTaskQueueProcessing(this);
  }

  // cleanup queue and end threads
  ExitWorkers();

  // the queue should be empty at this point
  Assert(FifoIsEmpty());

  // free barriers
  Assert(m_allocatedBarrierCount == 0);
  for (uint32 i = 0; i < m_allocatedBarrierCount; i++)
    delete m_barrierPool[i];
}

bool TaskQueue::Initialize(uint32 taskQueueSize /* = DefaultQueueSize */, uint32 workerThreadCount /* = 1 */)
{
  DebugAssert(m_workerThreadExitFlag && m_workerThreads.IsEmpty() && m_pThreadPool == nullptr);

  // allocate queue
  AllocateQueue(taskQueueSize);

  // create worker threads
  if (workerThreadCount > 0)
  {
    // prevent threads exiting
    m_workerThreadExitFlag = false;
    MemoryBarrier();

    // create the workers
    for (uint32 i = 0; i < workerThreadCount; i++)
    {
      WorkerThread* pThread = new WorkerThread(this);
      if (!pThread->Start())
      {
        // cleanup the remaining threads
        delete pThread;
        ExitWorkers();
        return false;
      }

      // store thread
      m_workerThreads.Add(pThread);
    }
  }
  else
  {
    // Non-threaded task queue.
    BeginThreadTaskQueueProcessing(this);
  }

  return true;
}

bool TaskQueue::Initialize(ThreadPool* pThreadPool, uint32 taskQueueSize /* = DefaultQueueSize */,
                           bool yieldToOtherJobs /* = false */)
{
  DebugAssert(m_workerThreadExitFlag && m_workerThreads.IsEmpty() && m_pThreadPool == nullptr);
  m_pThreadPool = pThreadPool;

  // allocate queue
  AllocateQueue(taskQueueSize);

  // allocate thread pool tasks, with a kept reference so that we can re-use them.
  uint32 taskCount = m_pThreadPool->GetWorkerThreadCount();
  m_threadPoolTasks.Resize(taskCount);
  for (uint32 i = 0; i < taskCount; i++)
    m_threadPoolTasks[i] = new ThreadPoolTask(this);

  // all done for now
  m_threadPoolYieldToOtherJobs = yieldToOtherJobs;
  return true;
}

void TaskQueue::AllocateQueue(uint32 size)
{
  if (size > 0)
    m_taskQueueBuffer.ResizeBuffer(size);
}

void TaskQueue::ExitWorkers()
{
  // if there's no workers, just drain the queue
  if (m_workerThreads.IsEmpty())
  {
    ExecuteQueuedTasks();
    return;
  }

  // pause the workers, draining the queue
  PauseWorkers();

  // set the worker exit flag, and wake all workers
  m_workerThreadExitFlag = true;
  m_conditionVariable.WakeAll();
  ResumeWorkers();

  // join each thread
  while (m_workerThreads.GetSize() > 0)
  {
    WorkerThread* pThread = m_workerThreads.PopBack();
    pThread->Join();
    delete pThread;
  }
}

void TaskQueue::PauseWorkers()
{
  if (m_workerThreads.IsEmpty())
  {
    ExecuteQueuedTasks();
    return;
  }

  // drain the queue, then pause the thread by holding the lock
  for (;;)
  {
    m_queueLock.Lock();

    // queue has entries? wait for the worker to sleep
    if (!FifoIsEmpty() || m_activeWorkerThreads > 0)
    {
      if (m_activeWorkerThreads == 0)
      {
        m_conditionVariable.Wake();
        // Log::GetInstance().Write("w", LOGLEVEL_DEV, "Woke CV");
      }
      m_queueLock.Unlock();
      Thread::Yield();
      continue;
    }
    else
    {
      // leave the queue locked
      break;
    }
  }
}

void TaskQueue::ResumeWorkers()
{
  if (m_workerThreads.IsEmpty())
    return;

  // unlock queue, any workers that were woken should be allowed to continue
  m_queueLock.Unlock();
}

void TaskQueue::QueueTask(Task* pTask, uint32 taskSize)
{
  if (m_taskQueueBuffer.GetBufferSize() == 0)
  {
    pTask->Execute();
    return;
  }

  LockQueueForNewTask();

  // write @TODO(this should be a template function calling copy operator..)
  void* task = FifoAllocateTask(taskSize, nullptr);
  std::memcpy(task, pTask, taskSize);

  UnlockQueueForNewTask();
}

void TaskQueue::QueueBlockingTask(Task* pTask, uint32 taskSize)
{
  if (m_taskQueueBuffer.GetBufferSize() == 0 || m_workerThreads.IsEmpty() || IsOnWorkerThread())
  {
    pTask->Execute();
    return;
  }

  // lock before write
  LockQueueForNewTask();

  // write @TODO(this should be a template function calling copy operator..)
  Barrier* barrier;
  void* task = FifoAllocateTask(taskSize, &barrier);
  std::memcpy(task, pTask, taskSize);

  // unlock
  UnlockQueueForNewTask();

  // block
  barrier->Wait();
}

bool TaskQueue::ExecuteQueuedTasks()
{
  // result <- any tasks were executed.
  bool result = false;
  m_queueLock.Lock();

  // loop
  for (;;)
  {
    FifoQueueEntryHeader* taskHdr = FifoGetNextTask();
    if (taskHdr == nullptr)
    {
      // if there's still outstanding tasks, yield and search again
      if (m_activeWorkerThreads > 0 || m_activeThreadPoolTasks > 0)
      {
        m_queueLock.Unlock();
        Thread::Yield();
        m_queueLock.Lock();
        continue;
      }

      // all tasks done
      break;
    }

    // unlock queue while the task runs
    m_queueLock.Unlock();

    // run the task
    Task* pTask = reinterpret_cast<Task*>(taskHdr + 1);
    pTask->Execute();
    pTask->~Task();

    // barrier waits
    if (taskHdr->pBarrier != nullptr)
      taskHdr->pBarrier->Wait();

    // re-lock queue
    m_queueLock.Lock();

    // push barrier back to pool
    if (taskHdr->pBarrier != nullptr)
      ReleaseBarrier(taskHdr->pBarrier);

    // pop the task off the queue
    FifoReleaseTask(taskHdr);
    result = true;
  }

  m_queueLock.Unlock();
  return result;
}

bool TaskQueue::IsOnWorkerThread() const
{
  // Simply test the per-thread pointer if we're pointed to our task queue.
  return (s_pCurrentThreadTaskQueue == this);
}

TaskQueue::WorkerThread::WorkerThread(TaskQueue* pParent) : m_this(pParent) {}

int TaskQueue::WorkerThread::ThreadEntryPoint()
{
  // initialize thread name
  Thread::SetDebugName(String::FromFormat("Task Queue %p Worker", m_this));
  BeginThreadTaskQueueProcessing(m_this);

  // start with it locked
  m_this->m_queueLock.Lock();
  m_this->m_activeWorkerThreads++;
  MemoryBarrier();

  // loop
  for (;;)
  {
    // get the next task
    FifoQueueEntryHeader* taskHdr = m_this->FifoGetNextTask();
    if (taskHdr == nullptr)
    {
      // no next task, are we exiting?
      if (m_this->m_workerThreadExitFlag)
        break;

      // this thread is now inactive
      m_this->m_activeWorkerThreads--;
      MemoryBarrier();

      // wait until there is a new event
      m_this->m_conditionVariable.SleepAndRelease(&m_this->m_queueLock);

      // this thread is now active
      m_this->m_activeWorkerThreads++;
      MemoryBarrier();
      continue;
    }

    // release queue lock
    m_this->m_queueLock.Unlock();

    // run the task
    Task* pTask = reinterpret_cast<Task*>(taskHdr + 1);
    pTask->Execute();
    pTask->~Task();

    // handle blocking events
    if (taskHdr->pBarrier != nullptr)
      taskHdr->pBarrier->Wait();

    // hold lock again
    m_this->m_queueLock.Lock();

    // push barrier back to pool
    if (taskHdr->pBarrier != nullptr)
      m_this->ReleaseBarrier(taskHdr->pBarrier);

    // destruct the task
    m_this->FifoReleaseTask(taskHdr);
  }

  m_this->m_activeWorkerThreads--;
  MemoryBarrier();

  m_this->m_queueLock.Unlock();
  EndThreadTaskQueueProcessing(m_this);
  return 0;
}

TaskQueue::ThreadPoolTask::ThreadPoolTask(TaskQueue* pParent) : ThreadPoolWorkItem(), m_this(pParent), m_active(false)
{
}

int32 TaskQueue::ThreadPoolTask::ProcessWork()
{
  BeginThreadTaskQueueProcessing(m_this);
  m_this->m_queueLock.Lock();

  // loop until there are no tasks left
  for (;;)
  {
    // get the next task
    FifoQueueEntryHeader* taskHdr = m_this->FifoGetNextTask();
    if (taskHdr == nullptr)
      break;

    // release queue lock
    m_this->m_queueLock.Unlock();

    // run the task
    Task* pTask = reinterpret_cast<Task*>(taskHdr + 1);
    pTask->Execute();
    pTask->~Task();

    // handle blocking events
    if (taskHdr->pBarrier != nullptr)
      taskHdr->pBarrier->Wait();

    // hold lock again
    m_this->m_queueLock.Lock();

    // push barrier back to pool
    if (taskHdr->pBarrier != nullptr)
      m_this->ReleaseBarrier(taskHdr->pBarrier);

    // destruct the task
    m_this->FifoReleaseTask(taskHdr);

    // check if we should yield
    if (m_this->m_threadPoolYieldToOtherJobs && m_this->m_pThreadPool->ShouldYieldToOtherTask())
      break;
  }

  // this task is no longer active
  DebugAssert(m_this->m_activeThreadPoolTasks > 0);
  m_this->m_activeThreadPoolTasks--;
  m_active = false;

  // end of this task's work
  m_this->m_queueLock.Unlock();
  EndThreadTaskQueueProcessing(m_this);
  return 0;
}

void TaskQueue::LockQueueForNewTask()
{
  m_queueLock.Lock();
}

void TaskQueue::UnlockQueueForNewTask()
{
  DebugAssert(m_taskQueueBuffer.GetBufferSize() > 0);

  if (m_pThreadPool != nullptr)
  {
    bool queueTask = (m_activeThreadPoolTasks < m_threadPoolTasks.GetSize());
    if (queueTask)
    {
      // find a free task
      for (uint32 i = 0; i < m_threadPoolTasks.GetSize(); i++)
      {
        ThreadPoolTask* pTask = m_threadPoolTasks[i];
        if (!pTask->IsActive())
        {
          // enqueue it
          pTask->SetActive();
          m_activeThreadPoolTasks++;
          m_pThreadPool->EnqueueWorkItem(pTask);
          break;
        }
      }
    }

    // release lock
    m_queueLock.Unlock();
  }
  else
  {
    if (m_activeWorkerThreads < m_workerThreads.GetSize())
      m_conditionVariable.Wake();

    m_queueLock.Unlock();
  }
}

void* TaskQueue::FifoAllocateTask(uint32 size, Barrier** ppBarrier)
{
  void* pWritePointer;
  size_t requiredSpace = sizeof(FifoQueueEntryHeader) + size;
  size_t freeSpace = requiredSpace;

  while (!m_taskQueueBuffer.GetWritePointer(&pWritePointer, &freeSpace) || freeSpace < requiredSpace)
  {
    // need to wait until the queue is empty @TODO find a more efficient way of doing this that doesn't spin
    freeSpace = requiredSpace;
    m_queueLock.Unlock();
    Thread::Yield();
    m_queueLock.Lock();
  }

  // move buffer forward
  m_taskQueueBuffer.MoveWritePointer(requiredSpace);

  // fill in header details
  FifoQueueEntryHeader* hdr = reinterpret_cast<FifoQueueEntryHeader*>(pWritePointer);
  hdr->Size = sizeof(FifoQueueEntryHeader) + size;
  hdr->AcceptedFlag = false;
  hdr->CompletedFlag = false;

  // need a barrier?
  if (ppBarrier != nullptr)
  {
    // allocate a barrier object
    hdr->pBarrier = AllocateBarrier();
    *ppBarrier = hdr->pBarrier;
  }
  else
  {
    // nope
    hdr->pBarrier = nullptr;
  }

  // return pointer to the data
  return (hdr + 1);
}

bool TaskQueue::FifoIsEmpty() const
{
  return m_taskQueueBuffer.GetBufferUsed() == 0;
}

TaskQueue::FifoQueueEntryHeader* TaskQueue::FifoGetNextTask()
{
  void* pReadPointer;
  size_t availableBytes;
  if (!m_taskQueueBuffer.GetReadPointer(const_cast<const void**>(&pReadPointer), &availableBytes))
    return nullptr;

  FifoQueueEntryHeader* hdr = reinterpret_cast<FifoQueueEntryHeader*>(pReadPointer);
  while (availableBytes > 0)
  {
    DebugAssert(availableBytes >= hdr->Size);
    availableBytes -= hdr->Size;
    if (!hdr->AcceptedFlag)
    {
      hdr->AcceptedFlag = true;
      return hdr;
    }

    // move to next entry
    hdr = reinterpret_cast<FifoQueueEntryHeader*>(reinterpret_cast<byte*>(hdr) + hdr->Size);
  }

  // Somehow got woken but all tasks are in progress
  return nullptr;
}

void TaskQueue::FifoReleaseTask(FifoQueueEntryHeader* taskHdr)
{
  // flag as completed
  taskHdr->CompletedFlag = true;

  // re-obtain the head of the queue
  const void* pReadPointer;
  size_t availableBytes;
  if (!m_taskQueueBuffer.GetReadPointer(&pReadPointer, &availableBytes))
    Panic("FIFO corruption");

  // are we at the front of the queue?
  if (pReadPointer == taskHdr)
  {
    // squish this task, and remove as many as possible
    size_t bytesToRemove = 0;
    DebugAssert(taskHdr->CompletedFlag);
    while (taskHdr->CompletedFlag)
    {
      DebugAssert((bytesToRemove + taskHdr->Size) <= availableBytes);
      bytesToRemove += taskHdr->Size;
      if (bytesToRemove == availableBytes)
        break;

      taskHdr = reinterpret_cast<FifoQueueEntryHeader*>(reinterpret_cast<byte*>(taskHdr) + taskHdr->Size);
    }

    // free from the buffer
    DebugAssert(bytesToRemove <= availableBytes);
    m_taskQueueBuffer.MoveReadPointer(bytesToRemove);
  }
}

Barrier* TaskQueue::AllocateBarrier()
{
  m_allocatedBarrierCount++;
  if (m_barrierPool.GetSize() > 0)
    return m_barrierPool.PopBack();
  else
    return new Barrier(2);
}

void TaskQueue::ReleaseBarrier(Barrier* pBarrier)
{
  DebugAssert(m_allocatedBarrierCount > 0);
  m_allocatedBarrierCount--;

  m_barrierPool.Add(pBarrier);
}
