#include "YBaseLib/ProgressCallbacks.h"
#include "YBaseLib/ByteStream.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/Math.h"
#include "YBaseLib/NumericLimits.h"
#include "YBaseLib/String.h"
#include <cstdio>
Log_SetChannel(ProgressCallbacks);

ProgressCallbacks::~ProgressCallbacks() {}

void ProgressCallbacks::SetFormattedStatusText(const char* Format, ...)
{
  SmallString str;
  va_list ap;

  va_start(ap, Format);
  str.FormatVA(Format, ap);
  va_end(ap);

  SetStatusText(str);
}

void ProgressCallbacks::DisplayFormattedError(const char* format, ...)
{
  SmallString str;
  va_list ap;

  va_start(ap, format);
  str.FormatVA(format, ap);
  va_end(ap);

  DisplayError(str);
}

void ProgressCallbacks::DisplayFormattedWarning(const char* format, ...)
{
  SmallString str;
  va_list ap;

  va_start(ap, format);
  str.FormatVA(format, ap);
  va_end(ap);

  DisplayWarning(str);
}

void ProgressCallbacks::DisplayFormattedInformation(const char* format, ...)
{
  SmallString str;
  va_list ap;

  va_start(ap, format);
  str.FormatVA(format, ap);
  va_end(ap);

  DisplayInformation(str);
}

void ProgressCallbacks::DisplayFormattedDebugMessage(const char* format, ...)
{
  SmallString str;
  va_list ap;

  va_start(ap, format);
  str.FormatVA(format, ap);
  va_end(ap);

  DisplayDebugMessage(str);
}

void ProgressCallbacks::DisplayFormattedModalError(const char* format, ...)
{
  SmallString str;
  va_list ap;

  va_start(ap, format);
  str.FormatVA(format, ap);
  va_end(ap);

  ModalError(str);
}

bool ProgressCallbacks::DisplayFormattedModalConfirmation(const char* format, ...)
{
  SmallString str;
  va_list ap;

  va_start(ap, format);
  str.FormatVA(format, ap);
  va_end(ap);

  return ModalConfirmation(str);
}

void ProgressCallbacks::UpdateProgressFromStream(ByteStream* pStream)
{
  uint32 streamSize = (uint32)pStream->GetSize();
  uint32 streamPosition = (uint32)pStream->GetPosition();

  SetProgressRange(streamSize);
  SetProgressValue(streamPosition);
}

class NullProgressCallbacks : public ProgressCallbacks
{
public:
  virtual void PushState() {}
  virtual void PopState() {}

  virtual const bool IsCancelled() const { return false; }
  virtual const bool IsCancellable() const { return false; }

  virtual void SetCancellable(bool cancellable) {}
  virtual void SetStatusText(const char* statusText) {}
  virtual void SetProgressRange(uint32 range) {}
  virtual void SetProgressValue(uint32 value) {}
  virtual void IncrementProgressValue() {}

  virtual void DisplayError(const char* message) { Log_ErrorPrint(message); }
  virtual void DisplayWarning(const char* message) { Log_WarningPrint(message); }
  virtual void DisplayInformation(const char* message) { Log_InfoPrint(message); }
  virtual void DisplayDebugMessage(const char* message) { Log_DevPrint(message); }

  virtual void ModalError(const char* message) { Log_ErrorPrint(message); }
  virtual bool ModalConfirmation(const char* message)
  {
    Log_InfoPrint(message);
    return false;
  }
  virtual uint32 ModalPrompt(const char* message, uint32 nOptions, ...)
  {
    DebugAssert(nOptions > 0);
    Log_InfoPrint(message);
    return 0;
  }
};

static NullProgressCallbacks s_nullProgressCallbacks;
ProgressCallbacks* ProgressCallbacks::NullProgressCallback = &s_nullProgressCallbacks;

BaseProgressCallbacks::BaseProgressCallbacks()
  : m_cancellable(false), m_cancelled(false), m_progressRange(1), m_progressValue(0), m_baseProgressValue(0),
    m_pSavedState(NULL)
{
}

BaseProgressCallbacks::~BaseProgressCallbacks()
{
  State* pNextState = m_pSavedState;
  while (pNextState != NULL)
  {
    State* pCurrentState = pNextState;
    pNextState = pCurrentState->pNextSavedState;
    delete pCurrentState;
  }
}

void BaseProgressCallbacks::PushState()
{
  State* pNewState = new State;
  pNewState->Cancellable = m_cancellable;
  pNewState->StatusText = m_statusText;
  pNewState->ProgressRange = m_progressRange;
  pNewState->ProgressValue = m_progressValue;
  pNewState->BaseProgressValue = m_baseProgressValue;
  pNewState->pNextSavedState = m_pSavedState;
  m_pSavedState = pNewState;
}

void BaseProgressCallbacks::PopState()
{
  DebugAssert(m_pSavedState != NULL);
  State* pState = m_pSavedState;
  m_pSavedState = NULL;

  // impose the current position into the previous range
  uint32 newProgressValue =
    (m_progressRange != 0) ?
      (uint32)Math::Truncate(((float)m_progressValue / (float)m_progressRange) * (float)pState->ProgressRange) :
      pState->ProgressValue;

  SetCancellable(pState->Cancellable);
  SetStatusText(pState->StatusText);
  SetProgressRange(pState->ProgressRange);
  SetProgressValue(newProgressValue);

  m_baseProgressValue = pState->BaseProgressValue;
  m_pSavedState = pState->pNextSavedState;
  delete pState;
}

void BaseProgressCallbacks::SetProgressRange(uint32 range)
{
  if (m_pSavedState != NULL)
  {
    // impose the previous range on this range
    m_progressRange = m_pSavedState->ProgressRange * range;
    m_baseProgressValue = m_progressValue = m_pSavedState->ProgressValue * range;
  }
  else
  {
    m_progressRange = range;
    m_progressValue = 0;
    m_baseProgressValue = 0;
  }
}

void BaseProgressCallbacks::SetProgressValue(uint32 value)
{
  m_progressValue = m_baseProgressValue + value;
}

void BaseProgressCallbacks::IncrementProgressValue()
{
  SetProgressValue((m_progressValue - m_baseProgressValue) + 1);
}

ConsoleProgressCallbacks::ConsoleProgressCallbacks()
  : BaseProgressCallbacks(), m_lastPercentComplete(Y_FLT_INFINITE), m_lastBarLength(0xFFFFFFFF)
{
}

ConsoleProgressCallbacks::~ConsoleProgressCallbacks()
{
  Clear();
}

void ConsoleProgressCallbacks::PushState()
{
  BaseProgressCallbacks::PushState();
}

void ConsoleProgressCallbacks::PopState()
{
  BaseProgressCallbacks::PopState();
  Redraw(false);
}

void ConsoleProgressCallbacks::SetCancellable(bool cancellable)
{
  BaseProgressCallbacks::SetCancellable(cancellable);
  Redraw(false);
}

void ConsoleProgressCallbacks::SetStatusText(const char* statusText)
{
  BaseProgressCallbacks::SetStatusText(statusText);
  Redraw(false);
}

void ConsoleProgressCallbacks::SetProgressRange(uint32 range)
{
  uint32 lastRange = m_progressRange;

  BaseProgressCallbacks::SetProgressRange(range);

  if (m_progressRange != lastRange)
    Redraw(false);
}

void ConsoleProgressCallbacks::SetProgressValue(uint32 value)
{
  uint32 lastValue = m_progressValue;

  BaseProgressCallbacks::SetProgressValue(value);

  if (m_progressValue != lastValue)
    Redraw(true);
}

void ConsoleProgressCallbacks::Clear()
{
  SmallString message;
  for (uint32 i = 0; i < COLUMNS; i++)
    message.AppendCharacter(' ');
  message.AppendCharacter('\r');

  fwrite(message.GetCharArray(), message.GetLength(), 1, stderr);
  fflush(stderr);
}

void ConsoleProgressCallbacks::Redraw(bool updateValueOnly)
{
  float percentComplete = (m_progressRange > 0) ? ((float)m_progressValue / (float)m_progressRange) * 100.0f : 0.0f;
  if (percentComplete > 100.0f)
    percentComplete = 100.0f;

  uint32 curLength = m_statusText.GetLength() + 14;
  uint32 maxBarLength = (curLength < COLUMNS) ? COLUMNS - curLength : 0;
  uint32 curBarLength = (maxBarLength > 0) ? ((uint32)Math::Floor(percentComplete / 100.0f * (float)maxBarLength)) : 0;

  if (updateValueOnly && (curBarLength == m_lastBarLength) &&
      Math::Abs(percentComplete - m_lastPercentComplete) < 0.01f)
    return;

  m_lastBarLength = curBarLength;
  m_lastPercentComplete = percentComplete;

  SmallString message;
  message.AppendString(m_statusText);
  message.AppendFormattedString(" [%.2f%%]", percentComplete);

  if (maxBarLength > 0)
  {
    message.AppendString(" |");

    uint32 i;
    for (i = 0; i < curBarLength; i++)
      message.AppendCharacter('=');
    for (; i < maxBarLength; i++)
      message.AppendCharacter(' ');

    message.AppendString("|");
  }

  message.AppendCharacter('\r');

  fwrite(message.GetCharArray(), message.GetLength(), 1, stderr);
  fflush(stderr);
}

void ConsoleProgressCallbacks::DisplayError(const char* message)
{
  Clear();
  Log_ErrorPrint(message);
  Redraw(false);
}

void ConsoleProgressCallbacks::DisplayWarning(const char* message)
{
  Clear();
  Log_WarningPrint(message);
  Redraw(false);
}

void ConsoleProgressCallbacks::DisplayInformation(const char* message)
{
  Clear();
  Log_InfoPrint(message);
  Redraw(false);
}

void ConsoleProgressCallbacks::DisplayDebugMessage(const char* message)
{
  Clear();
  Log_DevPrint(message);
  Redraw(false);
}

void ConsoleProgressCallbacks::ModalError(const char* message)
{
  Clear();
  Log_ErrorPrint(message);
  Redraw(false);
}

bool ConsoleProgressCallbacks::ModalConfirmation(const char* message)
{
  Clear();
  Log_InfoPrint(message);
  Redraw(false);
  return false;
}

uint32 ConsoleProgressCallbacks::ModalPrompt(const char* message, uint32 nOptions, ...)
{
  Clear();
  DebugAssert(nOptions > 0);
  Log_InfoPrint(message);
  Redraw(false);
  return 0;
}
