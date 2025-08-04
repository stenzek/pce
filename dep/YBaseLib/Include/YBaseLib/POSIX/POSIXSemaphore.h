#pragma once

#if !defined(__APPLE__)

#include <semaphore.h>

class Semaphore
{
public:
  Semaphore(int initial_count, int maximum_count) { sem_init(&m_handle, 0, initial_count); }

  ~Semaphore() { sem_destroy(&m_handle); }

  void Wait() { sem_wait(&m_handle); }

  void Post() { sem_post(&m_handle); }

private:
  sem_t m_handle;
};

#else

#include <dispatch/dispatch.h>

class Semaphore
{
public:
  Semaphore(int initial_count, int maximum_count)
  {
    m_handle = dispatch_semaphore_create(0);
    for (int i = 0; i < initial_count; i++)
      dispatch_semaphore_signal(m_handle);
  }

  ~Semaphore() { dispatch_release(m_handle); }

  void Wait() { dispatch_semaphore_wait(m_handle, DISPATCH_TIME_FOREVER); }

  void Post() { dispatch_semaphore_signal(m_handle); }

private:
  dispatch_semaphore_t m_handle;
};

#endif