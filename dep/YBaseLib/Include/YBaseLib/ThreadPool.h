#pragma once
#include "YBaseLib/Common.h"
#include "YBaseLib/ConditionVariable.h"
#include "YBaseLib/Event.h"
#include "YBaseLib/Mutex.h"
#include "YBaseLib/PODArray.h"
#include "YBaseLib/Thread.h"

class ThreadPool;
class ThreadPoolWorkItem;
class ThreadPoolWorkerThread;

class ThreadPool
{
  friend class ThreadPoolWorkerThread;

public:
  ThreadPool(uint32 workerThreadCount = GetDefaultWorkerThreadCount());
  ~ThreadPool();

  const uint32 GetWorkerThreadCount() const { return m_nWorkerThreads; }

  // queues a work item
  void EnqueueWorkItem(ThreadPoolWorkItem* pWorkItem);

  // allows a work item to yield, ie checks if there are any pending tasks of
  // higher priority, allowing them to preempt this task
  bool ShouldYieldToOtherTask();

private:
  void StartWorkerThreads();
  void StopWorkerThreads();

  // callback from worker thread method. returns NULL if the thread is to exit.
  ThreadPoolWorkItem* ThreadGetNextWorkItem();

  // vars
  PODArray<ThreadPoolWorkerThread*> m_WorkerThreads;
  uint32 m_nWorkerThreads;
  bool m_bExitThreads;
  PODArray<ThreadPoolWorkItem*> m_WorkItemQueue;
  Mutex m_WorkQueueLock;
  ConditionVariable m_WorkQueueConditionVariable;

  // todo: lock-free queue

public:
  // default number of worker threads is max(1, ncpus - 1)
  static uint32 GetDefaultWorkerThreadCount();
};

class ThreadPoolWorkerThread : public Thread
{
public:
  ThreadPoolWorkerThread(ThreadPool* pThreadPool);
  ~ThreadPoolWorkerThread();

protected:
  virtual int32 ThreadEntryPoint();

private:
  ThreadPool* m_pThreadPool;
};

class ThreadPoolWorkItem : public ReferenceCounted
{
  friend class ThreadPool;
  friend class ThreadPoolWorkerThread;

  enum STATE
  {
    STATE_QUEUED,
    STATE_STARTED,
    STATE_COMPLETED,
  };

public:
  ThreadPoolWorkItem();
  virtual ~ThreadPoolWorkItem();

  const bool IsStarted() const { return (m_iState != STATE_QUEUED); }
  const bool IsCompleted() const { return (m_iState == STATE_COMPLETED); }
  const int32 GetReturnValue() const { return m_iReturnValue; }

protected:
  // callback methods
  virtual int32 ProcessWork();
  virtual void OnCompleted();

private:
  uint32 m_iState;
  int32 m_iReturnValue;
};

class ThreadPoolWorkItemSignaled : public ThreadPoolWorkItem
{
public:
  ThreadPoolWorkItemSignaled();
  virtual ~ThreadPoolWorkItemSignaled();

  void WaitForCompletion();

protected:
  virtual void OnCompleted();

private:
  Event m_CompletionEvent;
};
