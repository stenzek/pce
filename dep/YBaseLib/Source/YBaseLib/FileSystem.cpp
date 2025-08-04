#include "YBaseLib/FileSystem.h"

FileSystem::ChangeNotifier::ChangeNotifier(const String& directoryPath, bool recursiveWatch)
  : m_directoryPath(directoryPath), m_recursiveWatch(recursiveWatch)
{
}

FileSystem::ChangeNotifier::~ChangeNotifier() {}

void FileSystem::CanonicalizePath(char* Destination, uint32 cbDestination, const char* Path, bool OSPath /*= true*/)
{
  uint32 i, j;
  DebugAssert(Destination != NULL && cbDestination > 0 && Path != NULL);

  // get length
  uint32 pathLength = Y_strlen(Path);

  // clone to a local buffer if the same pointer
  if (Destination == Path)
  {
    char* pathClone = (char*)alloca(pathLength + 1);
    Y_strncpy(pathClone, pathLength + 1, Path);
    Path = pathClone;
  }

  // zero destination
  Y_memzero(Destination, cbDestination);

  // iterate path
  uint32 destinationLength = 0;
  for (i = 0; i < pathLength;)
  {
    char prevCh = (i > 0) ? Path[i - 1] : '\0';
    char currentCh = Path[i];
    char nextCh = (i < pathLength) ? Path[i + 1] : '\0';

    if (currentCh == '.')
    {
      if (prevCh == '\\' || prevCh == '/' || prevCh == '\0')
      {
        // handle '.'
        if (nextCh == '\\' || nextCh == '/' || nextCh == '\0')
        {
          // skip '.\'
          i++;

          // remove the previous \, if we have one trailing the dot it'll append it anyway
          if (destinationLength > 0)
            Destination[--destinationLength] = '\0';

          continue;
        }
        // handle '..'
        else if (nextCh == '.')
        {
          char afterNext = ((i + 1) < pathLength) ? Path[i + 2] : '\0';
          if (afterNext == '\\' || afterNext == '/' || afterNext == '\0')
          {
            // remove one directory of the path, including the /.
            if (destinationLength > 1)
            {
              for (j = destinationLength - 2; j > 0; j--)
              {
                if (Destination[j] == '\\' || Destination[j] == '/')
                  break;
              }

              destinationLength = j;
#if Y_BUILD_CONFIG_DEBUG
              Destination[destinationLength] = '\0';
#endif
            }

            // skip the dot segment
            i += 2;
            continue;
          }
        }
      }
    }

    // fix ospath
    if (OSPath && (currentCh == '\\' || currentCh == '/'))
      currentCh = FS_OSPATH_SEPERATOR_CHARACTER;

    // copy character
    if (destinationLength < cbDestination)
    {
      Destination[destinationLength++] = currentCh;
#if Y_BUILD_CONFIG_DEBUG
      Destination[destinationLength] = '\0';
#endif
    }
    else
      break;

    // increment position by one
    i++;
  }

  // ensure null termination
  if (destinationLength < cbDestination)
    Destination[destinationLength] = '\0';
  else
    Destination[destinationLength - 1] = '\0';
}

void FileSystem::CanonicalizePath(String& Destination, const char* Path, bool OSPath /* = true */)
{
  // the function won't actually write any more characters than are present to the buffer,
  // so we can get away with simply passing both pointers if they are the same.
  if (Destination.GetWriteableCharArray() != Path)
  {
    // otherwise, resize the destination to at least the source's size, and then pass as-is
    Destination.Reserve(Y_strlen(Path) + 1);
  }

  CanonicalizePath(Destination.GetWriteableCharArray(), Destination.GetBufferSize(), Path, OSPath);
  Destination.UpdateSize();
}

void FileSystem::CanonicalizePath(String& Destination, bool OSPath /* = true */)
{
  CanonicalizePath(Destination, Destination);
}

static inline bool FileSystemCharacterIsSane(char c, bool StripSlashes)
{
  if (!(c >= 'a' && c <= 'z') && !(c >= 'A' && c <= 'Z') && !(c >= '0' && c <= '9') && c != ' ' && c != ' ' &&
      c != '_' && c != '-')
  {
    if (!StripSlashes && (c == '/' || c == '\\'))
      return true;

    return false;
  }

  return true;
}

void FileSystem::SanitizeFileName(char* Destination, uint32 cbDestination, const char* FileName,
                                  bool StripSlashes /* = true */)
{
  uint32 i;
  uint32 fileNameLength = Y_strlen(FileName);

  if (FileName == Destination)
  {
    for (i = 0; i < fileNameLength; i++)
    {
      if (!FileSystemCharacterIsSane(FileName[i], StripSlashes))
        Destination[i] = '_';
    }
  }
  else
  {
    for (i = 0; i < fileNameLength && i < cbDestination; i++)
    {
      if (FileSystemCharacterIsSane(FileName[i], StripSlashes))
        Destination[i] = FileName[i];
      else
        Destination[i] = '_';
    }
  }
}

void FileSystem::SanitizeFileName(String& Destination, const char* FileName, bool StripSlashes /* = true */)
{
  uint32 i;
  uint32 fileNameLength;

  // if same buffer, use fastpath
  if (Destination.GetWriteableCharArray() == FileName)
  {
    fileNameLength = Destination.GetLength();
    for (i = 0; i < fileNameLength; i++)
    {
      if (!FileSystemCharacterIsSane(FileName[i], StripSlashes))
        Destination[i] = '_';
    }
  }
  else
  {
    fileNameLength = Y_strlen(FileName);
    Destination.Resize(fileNameLength);
    for (i = 0; i < fileNameLength; i++)
    {
      if (FileSystemCharacterIsSane(FileName[i], StripSlashes))
        Destination[i] = FileName[i];
      else
        Destination[i] = '_';
    }
  }
}

void FileSystem::SanitizeFileName(String& Destination, bool StripSlashes /* = true */)
{
  return SanitizeFileName(Destination, Destination, StripSlashes);
}

void FileSystem::BuildPathRelativeToFile(char* Destination, uint32 cbDestination, const char* CurrentFileName,
                                         const char* NewFileName, bool OSPath /* = true */,
                                         bool Canonicalize /* = true */)
{
  int32 i;
  uint32 currentPos = 0;
  DebugAssert(Destination != NULL && cbDestination > 0 && CurrentFileName != NULL && NewFileName != NULL);

  // clone to a local buffer if the same pointer
  char pathClone[FS_MAX_PATH];
  if (Destination == CurrentFileName)
  {
    Y_strncpy(pathClone, countof(pathClone), CurrentFileName);
    Y_memzero(Destination, cbDestination);
    CurrentFileName = pathClone;
  }

  // search for a / or \, copy everything up to and including it to the destination
  i = (int32)Y_strlen(CurrentFileName);
  for (; i >= 0; i--)
  {
    if (CurrentFileName[i] == '/' || CurrentFileName[i] == '\\')
    {
      // cap to destination length
      uint32 copyLen;
      if (NewFileName[0] != '\0')
        copyLen = Min((uint32)(i + 1), cbDestination);
      else
        copyLen = Min((uint32)i, cbDestination);

      if (copyLen > 0)
      {
        std::memcpy(Destination, CurrentFileName, copyLen);
        if (copyLen == cbDestination)
          Destination[cbDestination - 1] = '\0';

        currentPos = copyLen;
      }

      break;
    }
  }

  // copy the new parts in
  if (currentPos < cbDestination && NewFileName[0] != '\0')
    Y_strncpy(Destination + currentPos, cbDestination - currentPos, NewFileName);

  // canonicalize it
  if (Canonicalize)
    CanonicalizePath(Destination, cbDestination, Destination, OSPath);
  else if (OSPath)
    BuildOSPath(Destination, cbDestination, Destination);
}

void FileSystem::BuildPathRelativeToFile(String& Destination, const char* CurrentFileName, const char* NewFileName,
                                         bool OSPath /* = true */, bool Canonicalize /* = true */)
{
  int32 i;
  DebugAssert(CurrentFileName != NULL && NewFileName != NULL);

  // get curfile length
  uint32 curFileLength = Y_strlen(CurrentFileName);

  // clone to a local buffer if the same pointer
  if (Destination.GetWriteableCharArray() == CurrentFileName)
  {
    char* pathClone = (char*)alloca(curFileLength + 1);
    Y_strncpy(pathClone, curFileLength + 1, CurrentFileName);
    CurrentFileName = pathClone;
  }

  // search for a / or \\, copy everything up to and including it to the destination
  Destination.Clear();
  i = (int32)curFileLength;
  for (; i >= 0; i--)
  {
    if (CurrentFileName[i] == '/' || CurrentFileName[i] == '\\')
    {
      if (NewFileName[0] != '\0')
        Destination.AppendSubString(CurrentFileName, 0, i + 1);
      else
        Destination.AppendSubString(CurrentFileName, 0, i);

      break;
    }
  }

  // copy the new parts in
  if (NewFileName[0] != '\0')
    Destination.AppendString(NewFileName);

  // canonicalize it
  if (Canonicalize)
    CanonicalizePath(Destination, Destination.GetCharArray(), OSPath);
  else if (OSPath)
    BuildOSPath(Destination, Destination.GetCharArray());
}

ByteStream* FileSystem::OpenFile(const char* FileName, uint32 Flags)
{
  // has a path
  if (FileName[0] == '\0')
    return NULL;

  // forward to local file wrapper
  ByteStream* pStream;
  if (!ByteStream_OpenFileStream(FileName, Flags, &pStream))
    return NULL;

  return pStream;
}
