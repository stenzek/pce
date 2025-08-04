#pragma once

class Semaphore
{
public:
  Semaphore(int initial_count, int maximum_count) {}

  ~Semaphore() = default;

  void Wait() {}

  void Post() {}
};
