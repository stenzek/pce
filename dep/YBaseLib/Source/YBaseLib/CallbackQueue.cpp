#include "YBaseLib/CallbackQueue.h"

CallbackQueue::CallbackQueue() {}

CallbackQueue::~CallbackQueue()
{
  uint32 i;
  for (i = 0; i < m_CallbackQueue.GetSize(); i++)
    delete m_CallbackQueue[i];
}

void CallbackQueue::AddCallback(Functor* pCallback)
{
  m_CallbackQueueLock.Lock();
  m_CallbackQueue.Add(pCallback);
  m_CallbackQueueLock.Unlock();
}

void CallbackQueue::RunCallbacks()
{
  uint32 i;

  for (;;)
  {
    m_CallbackQueueLock.Lock();

    if (m_CallbackQueue.GetSize() > 0)
    {
      Functor** pCallbacks;
      uint32 nCallbacks;

      // array has to be detached for recursive mutex safety, fix me later, replace with 'locked queue'
      m_CallbackQueue.DetachArray(&pCallbacks, &nCallbacks);
      m_CallbackQueueLock.Unlock();

      for (i = 0; i < nCallbacks; i++)
      {
        pCallbacks[i]->Invoke();
        delete pCallbacks[i];
      }

      std::free(pCallbacks);
    }
    else
    {
      m_CallbackQueueLock.Unlock();
      break;
    }
  }
}
