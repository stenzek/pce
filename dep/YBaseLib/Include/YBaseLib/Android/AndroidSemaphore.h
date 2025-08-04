#pragma once
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
