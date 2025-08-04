#pragma once
#include "YBaseLib/Common.h"
#include "YBaseLib/String.h"

class ByteStream;

class ProgressCallbacks
{
public:
  virtual ~ProgressCallbacks();

  virtual void PushState() = 0;
  virtual void PopState() = 0;

  virtual const bool IsCancelled() const = 0;
  virtual const bool IsCancellable() const = 0;

  virtual void SetCancellable(bool cancellable) = 0;

  virtual void SetStatusText(const char* statusText) = 0;
  virtual void SetProgressRange(uint32 range) = 0;
  virtual void SetProgressValue(uint32 value) = 0;
  virtual void IncrementProgressValue() = 0;

  void SetFormattedStatusText(const char* Format, ...);

  virtual void DisplayError(const char* message) = 0;
  virtual void DisplayWarning(const char* message) = 0;
  virtual void DisplayInformation(const char* message) = 0;
  virtual void DisplayDebugMessage(const char* message) = 0;

  virtual void ModalError(const char* message) = 0;
  virtual bool ModalConfirmation(const char* message) = 0;
  virtual uint32 ModalPrompt(const char* message, uint32 nOptions, ...) = 0;

  void DisplayFormattedError(const char* format, ...);
  void DisplayFormattedWarning(const char* format, ...);
  void DisplayFormattedInformation(const char* format, ...);
  void DisplayFormattedDebugMessage(const char* format, ...);
  void DisplayFormattedModalError(const char* format, ...);
  bool DisplayFormattedModalConfirmation(const char* format, ...);

  void UpdateProgressFromStream(ByteStream* pStream);

public:
  static ProgressCallbacks* NullProgressCallback;
};

class BaseProgressCallbacks : public ProgressCallbacks
{
public:
  BaseProgressCallbacks();
  virtual ~BaseProgressCallbacks();

  virtual void PushState();
  virtual void PopState();

  virtual const bool IsCancelled() const { return m_cancelled; }
  virtual const bool IsCancellable() const { return m_cancellable; }

  virtual void SetCancellable(bool cancellable) { m_cancellable = cancellable; }
  virtual void SetStatusText(const char* statusText) { m_statusText = statusText; }
  virtual void SetProgressRange(uint32 range);
  virtual void SetProgressValue(uint32 value);
  virtual void IncrementProgressValue();

protected:
  struct State
  {
    bool Cancellable;
    String StatusText;
    uint32 ProgressRange;
    uint32 ProgressValue;
    uint32 BaseProgressValue;
    State* pNextSavedState;
  };

  bool m_cancellable;
  bool m_cancelled;
  String m_statusText;
  uint32 m_progressRange;
  uint32 m_progressValue;

  uint32 m_baseProgressValue;

  State* m_pSavedState;
};

class ConsoleProgressCallbacks : public BaseProgressCallbacks
{
public:
  static const uint32 COLUMNS = 78;

public:
  ConsoleProgressCallbacks();
  ~ConsoleProgressCallbacks();

  virtual void PushState();
  virtual void PopState();

  virtual void SetCancellable(bool cancellable);
  virtual void SetStatusText(const char* statusText);
  virtual void SetProgressRange(uint32 range);
  virtual void SetProgressValue(uint32 value);

  virtual void DisplayError(const char* message);
  virtual void DisplayWarning(const char* message);
  virtual void DisplayInformation(const char* message);
  virtual void DisplayDebugMessage(const char* message);

  virtual void ModalError(const char* message);
  virtual bool ModalConfirmation(const char* message);
  virtual uint32 ModalPrompt(const char* message, uint32 nOptions, ...);

private:
  void Clear();
  void Redraw(bool updateValueOnly);

  float m_lastPercentComplete;
  uint32 m_lastBarLength;
};
