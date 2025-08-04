#include "YBaseLib/ThreadPool.h"
#include "YBaseLib/CPUID.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/Timer.h"
Log_SetChannel(ThreadPool);

ThreadPool::ThreadPool(uint32 workerThreadCount /* = GetDefaultWorkerThreadCount */)
  : m_nWorkerThreads(workerThreadCount), m_bExitThreads(false)
{
  Assert(workerThreadCount > 0);

  // resize and null worker threads
  m_WorkerThreads.Resize(workerThreadCount);
  Y_memzero(m_WorkerThreads.GetBasePointer(), sizeof(ThreadPoolWorkerThread*) * workerThreadCount);

  // start worker threads
  StartWorkerThreads();
}

ThreadPool::~ThreadPool()
{
  StopWorkerThreads();
}

void ThreadPool::StartWorkerThreads()
{
  // spawn worker threads
  for (uint32 i = 0; i < m_nWorkerThreads; i++)
  {
    ThreadPoolWorkerThread* pWorkerThread = new ThreadPoolWorkerThread(this);

    DebugAssert(m_WorkerThreads[i] == nullptr);
    m_WorkerThreads[i] = pWorkerThread;

    pWorkerThread->Start(false);
  }
}

void ThreadPool::StopWorkerThreads()
{
  m_bExitThreads = true;
  MemoryBarrier();

  // shutdown all threads
  for (;;)
  {
    uint32 activeThreads = 0;

    m_WorkQueueConditionVariable.WakeAll();

    for (uint32 i = 0; i < m_nWorkerThreads; i++)
    {
      if (m_WorkerThreads[i] != NULL && m_WorkerThreads[i]->IsRunning())
        activeThreads++;
    }

    if (activeThreads == 0)
      break;

    Thread::Sleep(1);
  }

  // join & delete all threads
  for (uint32 i = 0; i < m_nWorkerThreads; i++)
  {
    if (m_WorkerThreads[i] != nullptr)
    {
      m_WorkerThreads[i]->Join();
      delete m_WorkerThreads[i];
      m_WorkerThreads[i] = nullptr;
    }
  }

  m_bExitThreads = false;
  MemoryBarrier();
}

void ThreadPool::EnqueueWorkItem(ThreadPoolWorkItem* pWorkItem)
{
  m_WorkQueueLock.Lock();

  pWorkItem->AddRef();
  m_WorkItemQueue.Add(pWorkItem);

  m_WorkQueueConditionVariable.Wake();

  m_WorkQueueLock.Unlock();
}

bool ThreadPool::ShouldYieldToOtherTask()
{
  bool result;
  m_WorkQueueLock.Lock();
  result = !m_WorkItemQueue.IsEmpty();
  m_WorkQueueLock.Unlock();
  return result;
}

ThreadPoolWorkItem* ThreadPool::ThreadGetNextWorkItem()
{
  ThreadPoolWorkItem* pWorkItem;

  m_WorkQueueLock.Lock();

  if (m_WorkItemQueue.GetSize() > 0)
  {
    pWorkItem = m_WorkItemQueue.PopFront();
    m_WorkQueueLock.Unlock();
    return pWorkItem;
  }

  if (m_bExitThreads)
  {
    m_WorkQueueLock.Unlock();
    return nullptr;
  }

  // enter condition loop
  for (;;)
  {
    if (m_WorkItemQueue.GetSize() > 0)
    {
      pWorkItem = m_WorkItemQueue.PopFront();
      m_WorkQueueLock.Unlock();
      return pWorkItem;
    }

    if (m_bExitThreads)
    {
      m_WorkQueueLock.Unlock();
      return nullptr;
    }

    m_WorkQueueConditionVariable.SleepAndRelease(&m_WorkQueueLock);
  }
}

uint32 ThreadPool::GetDefaultWorkerThreadCount()
{
  Y_CPUID_RESULT cpuidResult;
  Y_ReadCPUID(&cpuidResult);

  return (cpuidResult.ThreadCount >= 2) ? cpuidResult.ThreadCount - 1 : 1;
}

ThreadPoolWorkerThread::ThreadPoolWorkerThread(ThreadPool* pThreadPool) : m_pThreadPool(pThreadPool) {}

ThreadPoolWorkerThread::~ThreadPoolWorkerThread() {}

int32 ThreadPoolWorkerThread::ThreadEntryPoint()
{
  for (;;)
  {
    ThreadPoolWorkItem* pWorkItem = m_pThreadPool->ThreadGetNextWorkItem();
    if (pWorkItem == NULL)
      break;

#if 0
        Timer timer;
#endif

    pWorkItem->m_iState = ThreadPoolWorkItem::STATE_STARTED;
    pWorkItem->m_iReturnValue = pWorkItem->ProcessWork();
    pWorkItem->m_iState = ThreadPoolWorkItem::STATE_COMPLETED;

    pWorkItem->OnCompleted();
    pWorkItem->Release();

#if 0
        Log_DevPrintf("ThreadPoolWorkerThread: Work item at 0x%p took %.3f msec to run.", pWorkItem, timer.GetTimeMilliseconds());
#endif
  }

  return 0;
}

ThreadPoolWorkItem::ThreadPoolWorkItem() : m_iState(STATE_QUEUED), m_iReturnValue(0xDEADBEEF) {}

ThreadPoolWorkItem::~ThreadPoolWorkItem() {}

int32 ThreadPoolWorkItem::ProcessWork()
{
  return 0;
}

void ThreadPoolWorkItem::OnCompleted() {}

ThreadPoolWorkItemSignaled::ThreadPoolWorkItemSignaled() : m_CompletionEvent(true) {}

ThreadPoolWorkItemSignaled::~ThreadPoolWorkItemSignaled() {}

void ThreadPoolWorkItemSignaled::WaitForCompletion()
{
  m_CompletionEvent.Wait();
}

void ThreadPoolWorkItemSignaled::OnCompleted()
{
  ThreadPoolWorkItem::OnCompleted();
  m_CompletionEvent.Signal();
}
