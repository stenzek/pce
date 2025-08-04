#pragma once
#include "YBaseLib/Common.h"
#include "YBaseLib/Windows/WindowsHeaders.h"

class Semaphore
{
public:
  Semaphore(int initial_count, int max_count)
  {
    m_semaphore = CreateSemaphoreA(nullptr, initial_count, max_count, nullptr);
  }

  ~Semaphore() { CloseHandle(m_semaphore); }

  void Wait() { WaitForSingleObject(m_semaphore, INFINITE); }

  void Post() { ReleaseSemaphore(m_semaphore, 1, nullptr); }

private:
  HANDLE m_semaphore;
};
