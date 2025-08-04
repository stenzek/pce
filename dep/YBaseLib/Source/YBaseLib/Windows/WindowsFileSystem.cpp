#include "YBaseLib/FileSystem.h"
#include "YBaseLib/String.h"

// Windows FileSystem implementation.
#ifdef Y_PLATFORM_WINDOWS

#include "YBaseLib/Log.h"
#include <shlobj.h>
Log_SetChannel(FileSystem);

static uint32 TranslateWin32Attributes(uint32 Win32Attributes)
{
  uint32 r = 0;

  if (Win32Attributes & FILE_ATTRIBUTE_DIRECTORY)
    r |= FILESYSTEM_FILE_ATTRIBUTE_DIRECTORY;
  if (Win32Attributes & FILE_ATTRIBUTE_READONLY)
    r |= FILESYSTEM_FILE_ATTRIBUTE_READ_ONLY;
  if (Win32Attributes & FILE_ATTRIBUTE_COMPRESSED)
    r |= FILESYSTEM_FILE_ATTRIBUTE_COMPRESSED;

  return r;
}

static const uint32 READ_DIRECTORY_CHANGES_NOTIFY_FILTER = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME |
                                                           FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE |
                                                           FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION;

class ChangeNotifierWin32 : public FileSystem::ChangeNotifier
{
public:
  ChangeNotifierWin32(HANDLE hDirectory, const String& directoryPath, bool recursiveWatch)
    : FileSystem::ChangeNotifier(directoryPath, recursiveWatch), m_hDirectory(hDirectory),
      m_directoryChangeQueued(false)
  {
    m_bufferSize = 16384;
    m_pBuffer = new byte[m_bufferSize];
  }

  virtual ~ChangeNotifierWin32()
  {
    // if there is outstanding io, cancel it
    if (m_directoryChangeQueued)
    {
      CancelIo(m_hDirectory);

      DWORD bytesTransferred;
      GetOverlappedResult(m_hDirectory, &m_overlapped, &bytesTransferred, TRUE);
    }

    CloseHandle(m_hDirectory);
    delete[] m_pBuffer;
  }

  virtual void EnumerateChanges(EnumerateChangesCallback callback, void* pUserData) override
  {
    DWORD bytesRead;
    if (!GetOverlappedResult(m_hDirectory, &m_overlapped, &bytesRead, FALSE))
    {
      if (GetLastError() == ERROR_IO_INCOMPLETE)
        return;

      CancelIo(m_hDirectory);
      m_directoryChangeQueued = false;

      QueueReadDirectoryChanges();
      return;
    }

    // not queued any more
    m_directoryChangeQueued = false;

    // has any bytes?
    if (bytesRead > 0)
    {
      const byte* pCurrentPointer = m_pBuffer;
      PathString fileName;
      for (;;)
      {
        const FILE_NOTIFY_INFORMATION* pFileNotifyInformation =
          reinterpret_cast<const FILE_NOTIFY_INFORMATION*>(pCurrentPointer);

        // translate the event
        uint32 changeEvent = 0;
        if (pFileNotifyInformation->Action == FILE_ACTION_ADDED)
          changeEvent = ChangeEvent_FileAdded;
        else if (pFileNotifyInformation->Action == FILE_ACTION_REMOVED)
          changeEvent = ChangeEvent_FileRemoved;
        else if (pFileNotifyInformation->Action == FILE_ACTION_MODIFIED)
          changeEvent = ChangeEvent_FileModified;
        else if (pFileNotifyInformation->Action == FILE_ACTION_RENAMED_OLD_NAME)
          changeEvent = ChangeEvent_RenamedOldName;
        else if (pFileNotifyInformation->Action == FILE_ACTION_RENAMED_NEW_NAME)
          changeEvent = ChangeEvent_RenamedNewName;

        // translate the filename
        int fileNameLength =
          WideCharToMultiByte(CP_UTF8, 0, pFileNotifyInformation->FileName,
                              pFileNotifyInformation->FileNameLength / sizeof(WCHAR), nullptr, 0, nullptr, nullptr);
        DebugAssert(fileNameLength >= 0);
        fileName.Resize(fileNameLength);
        fileNameLength = WideCharToMultiByte(CP_UTF8, 0, pFileNotifyInformation->FileName,
                                             pFileNotifyInformation->FileNameLength / sizeof(WCHAR),
                                             fileName.GetWriteableCharArray(), fileName.GetLength(), nullptr, nullptr);
        if (fileNameLength != (int)fileName.GetLength())
          fileName.Resize(fileNameLength);

        // prepend the base path
        fileName.PrependFormattedString("%s\\", m_directoryPath.GetCharArray());

        // construct change info
        ChangeInfo changeInfo;
        changeInfo.Path = fileName;
        changeInfo.Event = changeEvent;

        // invoke callback
        callback(&changeInfo, pUserData);

        // has a next entry?
        if (pFileNotifyInformation->NextEntryOffset == 0)
          break;

        pCurrentPointer += pFileNotifyInformation->NextEntryOffset;
        DebugAssert(pCurrentPointer < (m_pBuffer + m_bufferSize));
      }
    }

    // re-queue the operation
    QueueReadDirectoryChanges();
  }

  bool QueueReadDirectoryChanges()
  {
    DebugAssert(!m_directoryChangeQueued);

    Y_memzero(&m_overlapped, sizeof(m_overlapped));
    if (ReadDirectoryChangesW(m_hDirectory, m_pBuffer, m_bufferSize, m_recursiveWatch,
                              READ_DIRECTORY_CHANGES_NOTIFY_FILTER, nullptr, &m_overlapped, nullptr) == FALSE)
      return false;

    m_directoryChangeQueued = true;
    return true;
  }

private:
  HANDLE m_hDirectory;
  OVERLAPPED m_overlapped;
  bool m_directoryChangeQueued;
  byte* m_pBuffer;
  uint32 m_bufferSize;
};

FileSystem::ChangeNotifier* FileSystem::CreateChangeNotifier(const char* path, bool recursiveWatch)
{
  // open the directory up
  HANDLE hDirectory = CreateFileA(path, FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                  nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);
  if (hDirectory == nullptr)
    return nullptr;

  // queue up the overlapped io
  ChangeNotifierWin32* pChangeNotifier = new ChangeNotifierWin32(hDirectory, path, recursiveWatch);
  if (!pChangeNotifier->QueueReadDirectoryChanges())
  {
    delete pChangeNotifier;
    return nullptr;
  }

  return pChangeNotifier;
}

void FileSystem::BuildOSPath(char* Destination, uint32 cbDestination, const char* Path)
{
  uint32 i;
  uint32 pathLength = Y_strlen(Path);

  if (Destination == Path)
  {
    // fast path
    for (i = 0; i < pathLength; i++)
    {
      if (Destination[i] == '/')
        Destination[i] = '\\';
    }
  }
  else
  {
    // slow path
    pathLength = Max(pathLength, cbDestination - 1);
    for (i = 0; i < pathLength; i++)
    {
      Destination[i] = (Path[i] == '/') ? '\\' : Path[i];
    }

    Destination[pathLength] = '\0';
  }
}

void FileSystem::BuildOSPath(String& Destination, const char* Path)
{
  uint32 i;
  uint32 pathLength;

  if (Destination.GetWriteableCharArray() == Path)
  {
    // fast path
    pathLength = Destination.GetLength();
    ;
    for (i = 0; i < pathLength; i++)
    {
      if (Destination[i] == '/')
        Destination[i] = '\\';
    }
  }
  else
  {
    // slow path
    pathLength = Y_strlen(Path);
    Destination.Resize(pathLength);
    for (i = 0; i < pathLength; i++)
    {
      Destination[i] = (Path[i] == '/') ? '\\' : Path[i];
    }
  }
}

void FileSystem::BuildOSPath(String& Destination)
{
  BuildOSPath(Destination, Destination);
}

static uint32 RecursiveFindFiles(const char* OriginPath, const char* ParentPath, const char* Path, const char* Pattern,
                                 uint32 Flags, FileSystem::FindResultsArray* pResults)
{
  uint32 tempPathLength = Y_strlen(OriginPath) + 2;
  if (ParentPath != NULL)
    tempPathLength += 1 + Y_strlen(ParentPath);
  if (Path != NULL)
    tempPathLength += 1 + Y_strlen(Path);

  char* tempStr = (char*)alloca(tempPathLength + 1);
  if (Path != NULL)
  {
    if (ParentPath != NULL)
      Y_snprintf(tempStr, tempPathLength + 1, "%s\\%s\\%s\\*", OriginPath, ParentPath, Path);
    else
      Y_snprintf(tempStr, tempPathLength + 1, "%s\\%s\\*", OriginPath, Path);
  }
  else
  {
    Y_snprintf(tempStr, tempPathLength + 1, "%s\\*", OriginPath);
  }

  WIN32_FIND_DATA wfd;
  HANDLE hFind = FindFirstFileA(tempStr, &wfd);
  if (hFind == INVALID_HANDLE_VALUE)
    return 0;

  // small speed optimization for '*' case
  bool hasWildCards = false;
  bool wildCardMatchAll = false;
  uint32 nFiles = 0;
  if (Y_strpbrk(Pattern, "*?") != NULL)
  {
    hasWildCards = true;
    wildCardMatchAll = !(Y_strcmp(Pattern, "*"));
  }

  // iterate results
  do
  {
    FILESYSTEM_FIND_DATA outData;
    outData.Attributes = 0;

    if (wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN && !(Flags & FILESYSTEM_FIND_HIDDEN_FILES))
      continue;

    if (wfd.cFileName[0] == '.')
    {
      if (wfd.cFileName[1] == '\0' || (wfd.cFileName[1] == '.' && wfd.cFileName[2] == '\0'))
        continue;

      if (!(Flags & FILESYSTEM_FIND_HIDDEN_FILES))
        continue;
    }

    if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
      if (Flags & FILESYSTEM_FIND_RECURSIVE)
      {
        // recurse into this directory
        if (ParentPath != NULL)
        {
          Y_snprintf(tempStr, tempPathLength + 1, "%s\\%s", ParentPath, Path);
          nFiles += RecursiveFindFiles(OriginPath, tempStr, wfd.cFileName, Pattern, Flags, pResults);
        }
        else
        {
          nFiles += RecursiveFindFiles(OriginPath, Path, wfd.cFileName, Pattern, Flags, pResults);
        }
      }

      if (!(Flags & FILESYSTEM_FIND_FOLDERS))
        continue;

      outData.Attributes |= FILESYSTEM_FILE_ATTRIBUTE_DIRECTORY;
    }
    else
    {
      if (!(Flags & FILESYSTEM_FIND_FILES))
        continue;
    }

    if (wfd.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
      outData.Attributes |= FILESYSTEM_FILE_ATTRIBUTE_READ_ONLY;

    // match the filename
    if (hasWildCards)
    {
      if (!wildCardMatchAll && !Y_strwildcmp(wfd.cFileName, Pattern))
        continue;
    }
    else
    {
      if (Y_strcmp(wfd.cFileName, Pattern) != 0)
        continue;
    }

    // add file to list
    // TODO string formatter, clean this mess..
    if (!(Flags & FILESYSTEM_FIND_RELATIVE_PATHS))
    {
      if (ParentPath != NULL)
        Y_snprintf(outData.FileName, countof(outData.FileName), "%s\\%s\\%s\\%s", OriginPath, ParentPath, Path,
                   wfd.cFileName);
      else if (Path != NULL)
        Y_snprintf(outData.FileName, countof(outData.FileName), "%s\\%s\\%s", OriginPath, Path, wfd.cFileName);
      else
        Y_snprintf(outData.FileName, countof(outData.FileName), "%s\\%s", OriginPath, wfd.cFileName);
    }
    else
    {
      if (ParentPath != NULL)
        Y_snprintf(outData.FileName, countof(outData.FileName), "%s\\%s\\%s", ParentPath, Path, wfd.cFileName);
      else if (Path != NULL)
        Y_snprintf(outData.FileName, countof(outData.FileName), "%s\\%s", Path, wfd.cFileName);
      else
        Y_strncpy(outData.FileName, countof(outData.FileName), wfd.cFileName);
    }

    outData.ModificationTime.SetWindowsFileTime(&wfd.ftLastWriteTime);
    outData.Size = (uint64)wfd.nFileSizeHigh << 32 | (uint64)wfd.nFileSizeLow;

    nFiles++;
    pResults->Add(outData);
  } while (FindNextFileA(hFind, &wfd) == TRUE);
  FindClose(hFind);

  return nFiles;
}

bool FileSystem::FindFiles(const char* Path, const char* Pattern, uint32 Flags, FindResultsArray* pResults)
{
  // has a path
  if (Path[0] == '\0')
    return false;

  // clear result array
  if (!(Flags & FILESYSTEM_FIND_KEEP_ARRAY))
    pResults->Clear();

  // enter the recursive function
  return (RecursiveFindFiles(Path, NULL, NULL, Pattern, Flags, pResults) > 0);
}

bool FileSystem::StatFile(const char* Path, FILESYSTEM_STAT_DATA* pStatData)
{
  // has a path
  if (Path[0] == '\0')
    return false;

  // determine attributes for the path. if it's a directory, things have to be handled differently..
  DWORD fileAttributes = GetFileAttributesA(Path);
  if (fileAttributes == INVALID_FILE_ATTRIBUTES)
    return false;

  // test if it is a directory
  HANDLE hFile;
  if (fileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    hFile = CreateFileA(Path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING,
                        FILE_FLAG_BACKUP_SEMANTICS, NULL);
  else
    hFile = CreateFileA(Path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING,
                        0, NULL);

  // createfile succeded?
  if (hFile == INVALID_HANDLE_VALUE)
    return false;

  // use GetFileInformationByHandle
  BY_HANDLE_FILE_INFORMATION bhfi;
  if (GetFileInformationByHandle(hFile, &bhfi) == FALSE)
  {
    CloseHandle(hFile);
    return false;
  }

  // close handle
  CloseHandle(hFile);

  // fill in the stat data
  pStatData->Attributes = TranslateWin32Attributes(bhfi.dwFileAttributes);
  pStatData->ModificationTime.SetWindowsFileTime(&bhfi.ftLastWriteTime);
  pStatData->Size = ((uint64)bhfi.nFileSizeHigh) << 32 | (uint64)bhfi.nFileSizeLow;
  return true;
}

bool FileSystem::FileExists(const char* Path)
{
  // has a path
  if (Path[0] == '\0')
    return false;

  // determine attributes for the path. if it's a directory, things have to be handled differently..
  DWORD fileAttributes = GetFileAttributesA(Path);
  if (fileAttributes == INVALID_FILE_ATTRIBUTES)
    return false;

  if (fileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    return false;
  else
    return true;
}

bool FileSystem::DirectoryExists(const char* Path)
{
  // has a path
  if (Path[0] == '\0')
    return false;

  // determine attributes for the path. if it's a directory, things have to be handled differently..
  DWORD fileAttributes = GetFileAttributesA(Path);
  if (fileAttributes == INVALID_FILE_ATTRIBUTES)
    return false;

  if (fileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    return true;
  else
    return false;
}

bool FileSystem::GetFileName(String& Destination, const char* FileName)
{
  // fastpath for non-existant files
  DWORD fileAttributes = GetFileAttributesA(FileName);
  if (fileAttributes == INVALID_FILE_ATTRIBUTES)
    return false;

  //     // temp buffer for storing string returned by windows
  //     char tempName[MAX_PATH];
  //     DWORD tempNameLength;
  //
  //     // query windows
  //     if ((tempNameLength = GetFullPathNameA(FileName, countof(tempName), tempName, NULL)) == 0 || tempNameLength >=
  //     countof(tempName))
  //     {
  //         // something went wrong, or buffer overflow
  //         return false;
  //     }
  //
  //     // move it into destination buffer, doesn't matter if it's the same as FileName, as
  //     // we aren't going to use it any more.
  //     DebugAssert(Destination[tempNameLength] == '\0');
  //     Destination = tempName;
  if (Destination.GetWriteableCharArray() != FileName)
    Destination = FileName;

  return true;
}

bool FileSystem::GetFileName(String& FileName)
{
  return GetFileName(FileName, FileName);
}

bool FileSystem::CreateDirectory(const char* Path, bool Recursive)
{
  uint32 i;
  DWORD lastError;

  // has a path
  if (Path[0] == '\0')
    return false;

  // try just flat-out, might work if there's no other segments that have to be made
  if (CreateDirectoryA(Path, NULL))
    return true;

  // check error
  lastError = GetLastError();
  if (lastError == ERROR_ALREADY_EXISTS)
  {
    // check the attributes
    uint32 Attributes = GetFileAttributesA(Path);
    if (Attributes != INVALID_FILE_ATTRIBUTES && Attributes & FILE_ATTRIBUTE_DIRECTORY)
      return true;
    else
      return false;
  }
  else if (lastError == ERROR_PATH_NOT_FOUND)
  {
    // part of the path does not exist, so we'll create the parent folders, then
    // the full path again. allocate another buffer with the same length
    uint32 pathLength = Y_strlen(Path);
    char* tempStr = (char*)alloca(pathLength + 1);

    // create directories along the path
    for (i = 0; i < pathLength; i++)
    {
      if (Path[i] == '\\')
      {
        tempStr[i] = '\0';
        if (!CreateDirectoryA(tempStr, NULL))
        {
          lastError = GetLastError();
          if (lastError == ERROR_ALREADY_EXISTS) // fine, continue to next path segment
            continue;
          else // anything else is a fail
            return false;
        }
      }

      tempStr[i] = Path[i];
    }

    // re-create the end if it's not a separator, check / as well because windows can interpret them
    if (Path[pathLength - 1] != '\\' && Path[pathLength - 1] != '\\')
    {
      if (!CreateDirectoryA(Path, NULL))
      {
        lastError = GetLastError();
        if (lastError != ERROR_ALREADY_EXISTS)
          return false;
      }
    }

    // ok
    return true;
  }
  else
  {
    // unhandled error
    return false;
  }
}

bool FileSystem::DeleteFile(const char* Path)
{
  if (Path[0] == '\0')
    return false;

  DWORD fileAttributes = GetFileAttributesA(Path);
  if (fileAttributes == INVALID_FILE_ATTRIBUTES)
    return false;

  if (!(fileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    return (DeleteFileA(Path) == TRUE);
  else
    return false;
}

bool FileSystem::DeleteDirectory(const char* Path, bool Recursive)
{
  // ensure it exists
  DWORD fileAttributes = GetFileAttributesA(Path);
  if (fileAttributes == INVALID_FILE_ATTRIBUTES || !(fileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    return false;

  // non-recursive case just try removing the directory
  if (!Recursive)
    return (RemoveDirectoryA(Path) == TRUE);

  // doing a recursive delete
  SmallString fileName;
  fileName.Format("%s\\*", Path);

  // is there any files?
  WIN32_FIND_DATA findData;
  HANDLE hFind = FindFirstFileA(fileName, &findData);
  if (hFind == INVALID_HANDLE_VALUE)
    return false;

  // search through files
  do
  {
    // skip . and ..
    if (findData.cFileName[0] == '.')
    {
      if ((findData.cFileName[1] == '\0') || (findData.cFileName[1] == '.' && findData.cFileName[2] == '\0'))
      {
        continue;
      }
    }

    // found a directory?
    fileName.Format("%s\\%s", Path, findData.cFileName);
    if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
      // recurse into that
      if (!DeleteDirectory(fileName, true))
        return false;
    }
    else
    {
      // found a file, so delete it
      if (!DeleteFileA(fileName))
        return false;
    }
  } while (FindNextFileA(hFind, &findData));
  FindClose(hFind);

  // nuke the directory itself
  if (!RemoveDirectoryA(Path))
    return false;

  // done
  return true;
}

#endif // Y_PLATFORM_WINDOWS
