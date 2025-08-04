#include "YBaseLib/FileSystem.h"

// Posix FileSystem implementation.
#if defined(Y_PLATFORM_POSIX)

#include "YBaseLib/Log.h"
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

Log_SetChannel(FileSystem);

FileSystem::ChangeNotifier* FileSystem::CreateChangeNotifier(const char* path, bool recursiveWatch)
{
  Log_ErrorPrintf("FileSystem::CreateChangeNotifier(%s) not implemented", path);
  return nullptr;
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
      if (Destination[i] == '\\')
        Destination[i] = '/';
    }
  }
  else
  {
    // slow path
    pathLength = Max(pathLength, cbDestination - 1);
    for (i = 0; i < pathLength; i++)
    {
      Destination[i] = (Path[i] == '\\') ? '/' : Path[i];
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
      if (Destination[i] == '\\')
        Destination[i] = '/';
    }
  }
  else
  {
    // slow path
    pathLength = Y_strlen(Path);
    Destination.Resize(pathLength);
    for (i = 0; i < pathLength; i++)
    {
      Destination[i] = (Path[i] == '\\') ? '/' : Path[i];
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
      Y_snprintf(tempStr, tempPathLength + 1, "%s/%s/%s", OriginPath, ParentPath, Path);
    else
      Y_snprintf(tempStr, tempPathLength + 1, "%s/%s", OriginPath, Path);
  }
  else
  {
    Y_snprintf(tempStr, tempPathLength + 1, "%s", OriginPath);
  }

  DIR* pDir = opendir(tempStr);
  if (pDir == NULL)
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
  struct dirent* pDirEnt;
  while ((pDirEnt = readdir(pDir)) != NULL)
  {
    FILESYSTEM_FIND_DATA outData;
    outData.Attributes = 0;

    //        if (wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN && !(Flags & FILESYSTEM_FIND_HIDDEN_FILES))
    //            continue;
    //
    if (pDirEnt->d_name[0] == '.')
    {
      if (pDirEnt->d_name[1] == '\0' || (pDirEnt->d_name[1] == '.' && pDirEnt->d_name[2] == '\0'))
        continue;

      if (!(Flags & FILESYSTEM_FIND_HIDDEN_FILES))
        continue;
    }

    if (pDirEnt->d_type == DT_DIR)
    {
      if (Flags & FILESYSTEM_FIND_RECURSIVE)
      {
        // recurse into this directory
        if (ParentPath != NULL)
        {
          Y_snprintf(tempStr, tempPathLength + 1, "%s/%s", ParentPath, Path);
          nFiles += RecursiveFindFiles(OriginPath, tempStr, pDirEnt->d_name, Pattern, Flags, pResults);
        }
        else
        {
          nFiles += RecursiveFindFiles(OriginPath, Path, pDirEnt->d_name, Pattern, Flags, pResults);
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

    //        if (wfd.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
    //            outData.Attributes |= FILESYSTEM_FILE_ATTRIBUTE_READ_ONLY;

    // match the filename
    if (hasWildCards)
    {
      if (!wildCardMatchAll && !Y_strwildcmp(pDirEnt->d_name, Pattern))
        continue;
    }
    else
    {
      if (Y_strcmp(pDirEnt->d_name, Pattern) != 0)
        continue;
    }

    // add file to list
    // TODO string formatter, clean this mess..
    if (!(Flags & FILESYSTEM_FIND_RELATIVE_PATHS))
    {
      if (ParentPath != NULL)
        Y_snprintf(outData.FileName, countof(outData.FileName), "%s/%s/%s/%s", OriginPath, ParentPath, Path,
                   pDirEnt->d_name);
      else if (Path != NULL)
        Y_snprintf(outData.FileName, countof(outData.FileName), "%s/%s/%s", OriginPath, Path, pDirEnt->d_name);
      else
        Y_snprintf(outData.FileName, countof(outData.FileName), "%s/%s", OriginPath, pDirEnt->d_name);
    }
    else
    {
      if (ParentPath != NULL)
        Y_snprintf(outData.FileName, countof(outData.FileName), "%s\\%s\\%s", ParentPath, Path, pDirEnt->d_name);
      else if (Path != NULL)
        Y_snprintf(outData.FileName, countof(outData.FileName), "%s\\%s", Path, pDirEnt->d_name);
      else
        Y_strncpy(outData.FileName, countof(outData.FileName), pDirEnt->d_name);
    }

    nFiles++;
    pResults->Add(outData);
  }

  closedir(pDir);
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

  // stat file
  struct stat64 sysStatData;
  if (stat64(Path, &sysStatData) < 0)
    return false;

  // parse attributes
  pStatData->Attributes = 0;
  if (S_ISDIR(sysStatData.st_mode))
    pStatData->Attributes |= FILESYSTEM_FILE_ATTRIBUTE_DIRECTORY;

  // parse times
  pStatData->ModificationTime.SetUnixTimestamp((Timestamp::UnixTimestampValue)sysStatData.st_mtime);

  // parse size
  if (S_ISREG(sysStatData.st_mode))
    pStatData->Size = static_cast<uint64>(sysStatData.st_size);
  else
    pStatData->Size = 0;

  // ok
  return true;
}

bool FileSystem::FileExists(const char* Path)
{
  // has a path
  if (Path[0] == '\0')
    return false;

  // stat file
  struct stat64 sysStatData;
  if (stat64(Path, &sysStatData) < 0)
    return false;

  if (S_ISDIR(sysStatData.st_mode))
    return false;
  else
    return true;
}

bool FileSystem::DirectoryExists(const char* Path)
{
  // has a path
  if (Path[0] == '\0')
    return false;

  // stat file
  struct stat64 sysStatData;
  if (stat64(Path, &sysStatData) < 0)
    return false;

  if (S_ISDIR(sysStatData.st_mode))
    return true;
  else
    return false;
}

bool FileSystem::GetFileName(String& Destination, const char* FileName)
{
  // fastpath for non-existant files
  struct stat sysStatData;
  if (stat(FileName, &sysStatData) < 0)
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
  int lastError;

  // has a path
  if (Path[0] == '\0')
    return false;

  // try just flat-out, might work if there's no other segments that have to be made
  if (mkdir(Path, 0777) == 0)
    return true;

  // check error
  lastError = errno;
  if (lastError == EEXIST)
  {
    // check the attributes
    struct stat sysStatData;
    if (stat(Path, &sysStatData) == 0 && S_ISDIR(sysStatData.st_mode))
      return true;
    else
      return false;
  }
  else if (lastError == ENOENT)
  {
    // part of the path does not exist, so we'll create the parent folders, then
    // the full path again. allocate another buffer with the same length
    uint32 pathLength = Y_strlen(Path);
    char* tempStr = (char*)alloca(pathLength + 1);

    // create directories along the path
    for (i = 0; i < pathLength; i++)
    {
      if (Path[i] == '/')
      {
        tempStr[i] = '\0';
        if (mkdir(tempStr, 0777) < 0)
        {
          lastError = errno;
          if (lastError == EEXIST) // fine, continue to next path segment
            continue;
          else // anything else is a fail
            return false;
        }
      }

      tempStr[i] = Path[i];
    }

    // re-create the end if it's not a separator, check / as well because windows can interpret them
    if (Path[pathLength - 1] != '/')
    {
      if (mkdir(Path, 0777) < 0)
      {
        lastError = errno;
        if (lastError != EEXIST)
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

  struct stat sysStatData;
  if (stat(Path, &sysStatData) != 0 || S_ISDIR(sysStatData.st_mode))
    return false;

  return (unlink(Path) == 0);
}

bool FileSystem::DeleteDirectory(const char* Path, bool Recursive)
{
  Log_ErrorPrintf("FileSystem::DeleteDirectory(%s) not implemented", Path);
  return false;
}

#endif // Y_PLATFORM_POSIX
