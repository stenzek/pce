#pragma once
#include "YBaseLib/Barrier.h"
#include "YBaseLib/CircularBuffer.h"
#include "YBaseLib/Common.h"
#include "YBaseLib/RecursiveMutex.h"
#include "YBaseLib/Thread.h"
#include "YBaseLib/ThreadPool.h"

// Thread-safe task queue

class TaskQueue
{
public:
  // 1MiB default queue size
  static const uint32 DefaultQueueSize = 1048576;

  // To create a class task, inherit from this class
  struct Task
  {
    virtual ~Task() {}
    virtual void Execute() = 0;
  };

private:
  template<class T>
  struct LamdaTask : public Task
  {
    T m_callback;

    LamdaTask(const T& callback) : m_callback(callback) {}
    LamdaTask(T&& callback) : m_callback(std::move(callback)) {}
    virtual ~LamdaTask() {}

    virtual void Execute() { m_callback(); }
  };

public:
  TaskQueue();
  ~TaskQueue();

  // Initialize the command queue
  // If workerThreadCount is set to zero, the calling thread must execute work queued by other threads
  // by calling ExecuteQueuedTasks.
  bool Initialize(uint32 taskQueueSize = DefaultQueueSize, uint32 workerThreadCount = 1);

  // Initialize the command queue using a shared thread pool
  bool Initialize(ThreadPool* pThreadPool, uint32 taskQueueSize = DefaultQueueSize, bool yieldToOtherJobs = false);

  // Wait until all workers are finished, and then end the threads.
  void ExitWorkers();

  // Worker thread info
  const Thread* GetWorkerThread(uint32 idx) const { return m_workerThreads[idx]; }
  Thread::ThreadIdType GetWorkerThreadID(uint32 idx) const { return m_workerThreads[idx]->GetThreadId(); }
  uint32 GetWorkerThreadCount() const { return m_workerThreads.GetSize(); }

  // Pausing/resuming of thread
  void PauseWorkers();
  void ResumeWorkers();

  // Queuing of commands
  void QueueTask(Task* pCommand, uint32 taskSize);

  // Wrapper to assist with queueing lambda callbacks
  template<class T>
  void QueueLambdaTask(const T& lambda)
  {
    if (m_taskQueueBuffer.GetBufferSize() == 0)
    {
      lambda();
      return;
    }

    LockQueueForNewTask();

    LamdaTask<T>* trampoline = (LamdaTask<T>*)FifoAllocateTask(sizeof(LamdaTask<T>), nullptr);
    new (trampoline) LamdaTask<T>(lambda);

    UnlockQueueForNewTask();
  }

  // Queue lambda call using move semantics
  template<class T>
  void QueueLambdaTask(T&& lambda)
  {
    if (m_taskQueueBuffer.GetBufferSize() == 0)
    {
      lambda();
      return;
    }

    LockQueueForNewTask();

    LamdaTask<T>* trampoline = (LamdaTask<T>*)FifoAllocateTask(sizeof(LamdaTask<T>), nullptr);
    new (trampoline) LamdaTask<T>(std::move(lambda));

    UnlockQueueForNewTask();
  }

  // blocking variants
  void QueueBlockingTask(Task* pTask, uint32 taskSize);
  template<class T>
  void QueueBlockingLambdaTask(const T& lambda)
  {
    // We can't queue blocking tasks if we're on the same thread, so execute it immediately.
    // This could cause issues if a non-blocking task is queued first, so beware.
    if (m_taskQueueBuffer.GetBufferSize() == 0 || IsOnWorkerThread())
    {
      lambda();
      return;
    }

    LockQueueForNewTask();

    Barrier* pBarrier;
    LamdaTask<T>* trampoline = (LamdaTask<T>*)FifoAllocateTask(sizeof(LamdaTask<T>), &pBarrier);
    new (trampoline) LamdaTask<T>(lambda);

    UnlockQueueForNewTask();

    // block
    pBarrier->Wait();
  }

  // executes any pending tasks on the current thread, and blocks until the queue is empty
  // returns true if a task was executed, false if the queue was empty
  bool ExecuteQueuedTasks();

  // helper method for determining if the caller is on a worker thread.
  bool IsOnWorkerThread() const;

private:
  // worker thread class
  class WorkerThread : public Thread
  {
  public:
    WorkerThread(TaskQueue* pParent);

  protected:
    virtual int ThreadEntryPoint() override;
    TaskQueue* m_this;
  };

  // thread pool task class
  class ThreadPoolTask : public ThreadPoolWorkItem
  {
  public:
    ThreadPoolTask(TaskQueue* pParent);

    bool IsActive() const { return m_active; }
    void SetActive() { m_active = true; }

  protected:
    virtual int32 ProcessWork() override;
    TaskQueue* m_this;
    bool m_active;
  };

  // so the tasks can call our internal methods
  friend WorkerThread;
  friend ThreadPoolTask;

  // fifo queue entry
  struct FifoQueueEntryHeader
  {
    uint32 Size;
    Barrier* pBarrier;
    bool AcceptedFlag;
    bool CompletedFlag;
  };

  // allocate the queue
  void AllocateQueue(uint32 size);

  // lock the queue
  void LockQueueForNewTask();

  // unlock the queue, call this variant if no additional tasks were added
  void UnlockQueueForNewTask();

  // allocate bytes in the queue, assumes that the queue lock is held
  void* FifoAllocateTask(uint32 size, Barrier** ppBarrier);

  // fifo is empty?
  bool FifoIsEmpty() const;

  // retreives the first task off the fifo, but does not destroy it yet
  FifoQueueEntryHeader* FifoGetNextTask();

  // release a fifo task
  void FifoReleaseTask(FifoQueueEntryHeader* taskHdr);

  // barrier allocator
  Barrier* AllocateBarrier();
  void ReleaseBarrier(Barrier* pBarrier);

  // worker thread
  PODArray<WorkerThread*> m_workerThreads;
  bool m_workerThreadExitFlag;

  // thread pool
  PODArray<ThreadPoolTask*> m_threadPoolTasks;
  ThreadPool* m_pThreadPool;
  volatile uint32 m_activeThreadPoolTasks;
  bool m_threadPoolYieldToOtherJobs;

  // fifo members
  CircularBuffer m_taskQueueBuffer;
  RecursiveMutex m_queueLock;
  volatile uint32 m_activeWorkerThreads;

  // condition variable for worker wakeup
  ConditionVariable m_conditionVariable;

  // barrier for sync event pool
  PODArray<Barrier*> m_barrierPool;
  size_t m_allocatedBarrierCount;
};
