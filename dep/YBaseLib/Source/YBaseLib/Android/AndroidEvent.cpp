#include "YBaseLib/Assert.h"
#include "YBaseLib/Common.h"

#ifdef Y_PLATFORM_ANDROID

#include "YBaseLib/Android/AndroidEvent.h"
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

Event::Event(bool autoReset) : m_autoReset(autoReset)
{
  pipe(m_pipeFds);
  fcntl(m_pipeFds[0], F_SETFL, fcntl(m_pipeFds[0], F_GETFL) | O_NONBLOCK);
  fcntl(m_pipeFds[1], F_SETFL, fcntl(m_pipeFds[1], F_GETFL) | O_NONBLOCK);
}

Event::~Event()
{
  close(m_pipeFds[1]);
  close(m_pipeFds[0]);
}

void Event::Signal()
{
  char buf[1] = {0};
  write(m_pipeFds[1], buf, sizeof(buf));
}

void Event::Wait()
{
  pollfd pollDescriptors;
  pollDescriptors.fd = m_pipeFds[0];
  pollDescriptors.events = POLLRDNORM;
  poll(&pollDescriptors, 1, -1);

  if (m_autoReset)
    Reset();
}

bool Event::TryWait(uint32 timeout)
{
  pollfd pollDescriptors;
  pollDescriptors.fd = m_pipeFds[0];
  pollDescriptors.events = POLLRDNORM;
  if (poll(&pollDescriptors, 1, timeout) == 0)
    return false;

  if (m_autoReset)
    Reset();

  return true;
}

void Event::Reset()
{
  char buf[1];
  while (read(m_pipeFds[0], buf, sizeof(buf)) > 0)
    ;
}

void Event::WaitForMultipleEvents(Event** ppEvents, uint32 nEvents)
{
  DebugAssert(nEvents > 0);

  uint32 i;
  pollfd pollDescriptors;
  pollDescriptors.events = POLLRDNORM;
  for (i = 0; i < nEvents; i++)
  {
    pollDescriptors.fd = ppEvents[i]->m_pipeFds[0];
    poll(&pollDescriptors, 1, -1);
  }
}

#endif // Y_PLATFORM_ANDROID
