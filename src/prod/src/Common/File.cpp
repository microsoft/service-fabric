// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Common/File.h"

#if defined(PLATFORM_UNIX)
#include <libgen.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include <dirent.h>
#include <sys/file.h>
#include <wait.h>
#include <syscall.h>
#include <semaphore.h>
#include <libssh2.h>
#include <libssh2_sftp.h>
#include <arpa/inet.h>
#include <sys/fsuid.h>

Common::StringLiteral const FileApiTraceSource("FileApi");

Common::Global<std::map<int, unsigned int>> ErrnoToWindowsError(new std::map<int, unsigned int> {
        {0, ERROR_SUCCESS},
        {ENOENT, ERROR_PATH_NOT_FOUND},
        {ENAMETOOLONG, ERROR_FILENAME_EXCED_RANGE},
        {ENOTDIR, ERROR_PATH_NOT_FOUND},
        {EACCES, ERROR_ACCESS_DENIED},
        {EPERM, ERROR_ACCESS_DENIED},
        {EROFS, ERROR_ACCESS_DENIED},
        {EISDIR, ERROR_ACCESS_DENIED},
        {EEXIST, ERROR_ALREADY_EXISTS},
        {ENOTEMPTY, ERROR_DIR_NOT_EMPTY},
        {EBADF, ERROR_INVALID_HANDLE},
        {ENOMEM, ERROR_NOT_ENOUGH_MEMORY},
        {EBUSY, ERROR_BUSY},
        {ENOSPC, ERROR_DISK_FULL},
        {EDQUOT, ERROR_DISK_FULL},
        {ELOOP, ERROR_BAD_PATHNAME},
        {EIO, ERROR_WRITE_FAULT},
        {ERANGE, ERROR_BAD_PATHNAME}
});

bool GlobMatch(const char *str, const char *pat)
{
    if (!str || !pat) return false;

    if (*pat == 0) return (*str == 0);
    if (*str == 0)
    {
        //If your pattern is *foo* and str is foo then this case will handle this.
        while (*pat && *pat == '*')
        {
            pat++;
        }

        return (*pat == 0);
    }

    switch (*pat)
    {
        case '?':
            return (*str) ? GlobMatch(str + 1, pat + 1) : false;

        case '*':
            for(int i = 0; i <= strlen(str); i++)
            {
                if (GlobMatch(str + i, pat + 1)) return true;
            }
            return false;

        default:
            return (*str == *pat && GlobMatch(str + 1, pat + 1));
    }
    return false;
}

DWORD FileGetLastErrorFromErrno(void)
{
    uint32_t ret = ERROR_GEN_FAILURE;
    if (ErrnoToWindowsError->find(errno) != ErrnoToWindowsError->end())
    {
        ret = (*ErrnoToWindowsError)[errno];
    }
    return ret;
}

DWORD DirGetLastErrorFromErrno()
{
    if (errno == ENOENT)
        return ERROR_PATH_NOT_FOUND;
    else
        return FileGetLastErrorFromErrno();
}

static string FileNormalizePath(LPCWSTR pathW)
{
    string pathA;
    if (!pathW)
    {
        SetLastError(ERROR_INVALID_NAME);
        return pathA;
    }

    pathA = Common::StringUtility::Utf16ToUtf8(pathW);
    std::replace(pathA.begin(), pathA.end(), '\\', '/');
    while (pathA.back() == '/')
    {
        pathA.pop_back();
    }
    return pathA;
}

static int mkdir_r(string const & path)
{
    char tmp[PATH_MAX];

    snprintf(tmp, sizeof(tmp), "%s", path.c_str());

    char* dir = dirname((char*)tmp);
    size_t len = strlen(tmp);

    if(tmp[len - 1] != '/')
    {
        tmp[len] = '/';
        tmp[len + 1] = 0;
    }

    for(char* p = tmp + 1; *p; p++)
    {
        if (*p == '/')
        {
            *p = 0;
            DIR* dir = opendir(tmp);
            if (dir)
            {
                closedir(dir);
            }
            else if (ENOENT == errno)
            {
                mkdir(tmp, S_IRWXU);
            }
            else
            {
                return -1;
            }
            *p = '/';
        }
    }
    return 0;
}

DWORD GetFileAttributesW(LPCWSTR lpFileName)
{
    DWORD attr = 0;
    if (lpFileName == NULL)
    {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return INVALID_FILE_ATTRIBUTES;
    }
    struct stat stat_data, lstat_data;
    string pathA = FileNormalizePath(lpFileName);

    if (stat(pathA.c_str(), &stat_data) != 0)
    {
        SetLastError(FileGetLastErrorFromErrno());
        return INVALID_FILE_ATTRIBUTES;
    }

    if (lstat(pathA.c_str(), &lstat_data) == 0)
    {
        attr |= FILE_ATTRIBUTE_REPARSE_POINT;
    }

    if ((stat_data.st_mode & S_IFMT) == S_IFDIR)
    {
        attr |= FILE_ATTRIBUTE_DIRECTORY;
    }
    else if ((stat_data.st_mode & S_IFMT) != S_IFREG)
    {
        SetLastError(ERROR_ACCESS_DENIED);
        return INVALID_FILE_ATTRIBUTES;
    }

    if (attr == 0)
    {
        attr = FILE_ATTRIBUTE_NORMAL;
    }

    if ((stat_data.st_mode & S_IRUSR) && !(stat_data.st_mode & S_IWUSR))
    {
        attr |= FILE_ATTRIBUTE_READONLY;
    }

    return attr;
}

BOOL SetFileAttributesW(LPCWSTR lpFileName, IN DWORD dwFileAttributes)
{
    if (lpFileName == NULL)
    {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
    }
    struct stat stat_data;
    string pathA = FileNormalizePath(lpFileName);

    if (stat(pathA.c_str(), &stat_data) != 0)
    {
        SetLastError(FileGetLastErrorFromErrno());
        return FALSE;
    }

    int mode = stat_data.st_mode;
    if (!(mode & S_IFREG) && !(mode & S_IFDIR))
    {
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    if (dwFileAttributes & FILE_ATTRIBUTE_READONLY)
    {
        mode &= ~(S_IWUSR | S_IWGRP | S_IWOTH);
    }
    else if (mode & S_IRUSR)
    {
        mode |= S_IWUSR;
    }

    if (mode != stat_data.st_mode)
    {
        if (chmod(pathA.c_str(), mode) != 0)
        {
            SetLastError(FileGetLastErrorFromErrno());
            return FALSE;
        }
    }
    return TRUE;
}

BOOL CopyFileW(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, BOOL bFailIfExists)
{
    string srcA = FileNormalizePath(lpExistingFileName);
    string destA = FileNormalizePath(lpNewFileName);

    int oflags = O_CREAT | O_WRONLY | O_TRUNC | O_CLOEXEC;
    int srcfd = open(srcA.c_str(), O_RDONLY | O_CLOEXEC);
    if (srcfd == -1)
    {
        SetLastError(FileGetLastErrorFromErrno());
        return FALSE;
    }
    int destfd = open(destA.c_str(), oflags);
    if (destfd == -1)
    {
        close(srcfd);
        SetLastError(FileGetLastErrorFromErrno());
        return FALSE;
    }

    if (flock(destfd, LOCK_EX | LOCK_NB) != 0 || fcntl(destfd, F_SETFD, 1) != 0)
    {
        close(srcfd);
        close(destfd);
        SetLastError(FileGetLastErrorFromErrno());
        return FALSE;
    }

    // read and write
    char sbuf[4096], dbuf[4096];
    int bytesRead;
    while((bytesRead = read(srcfd, sbuf, sizeof(sbuf))) > 0) {
        int offset = 0, left = bytesRead;
        do
        {
            int written = write(destfd, sbuf + offset, left);
            if (written == -1) {
                close(srcfd);
                close(destfd);
                SetLastError(FileGetLastErrorFromErrno());
                return FALSE;
            }
            else
            {
                left -= written;
                offset += written;
            }
        } while(left != 0);
    }
    close(srcfd);
    close(destfd);

    // set attributes
    struct stat src_stat;
    int permissions = (S_IRWXU | S_IRWXG | S_IRWXO);
    if (stat(srcA.c_str(), &src_stat) == -1)
    {
        SetLastError(FileGetLastErrorFromErrno());
        return FALSE;
    }
    if ((src_stat.st_mode & S_IRUSR) && !(src_stat.st_mode & S_IWUSR))
    {
        permissions &= ~(S_IWUSR | S_IWGRP | S_IWOTH);
    }
    // ignore if chmod fails
    if (0 != chmod(destA.c_str(), src_stat.st_mode & permissions)) 
    {
        Trace.WriteInfo(FileApiTraceSource, L"File.CopyFileW","Failed to chmod {0}. errno {1}", destA, errno);
    }

    return TRUE;
}

BOOL MoveFileExW(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, DWORD dwFlags)
{
    string srcA = FileNormalizePath(lpExistingFileName);
    string destA = FileNormalizePath(lpNewFileName);

    struct stat lstat_data;
    if (lstat(srcA.c_str(), &lstat_data) == 0 && S_ISLNK(lstat_data.st_mode))
    {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
    }

    if (!(dwFlags & MOVEFILE_REPLACE_EXISTING))
    {
        if (srcA.compare(destA) != 0 && access(destA.c_str(), F_OK) == 0)
        {
            SetLastError(ERROR_ALREADY_EXISTS);
            return FALSE;
        }
    }
    int result = rename(srcA.c_str(), destA.c_str());
    if ((result != 0) && (dwFlags & MOVEFILE_REPLACE_EXISTING) && ((errno == ENOTDIR) || (errno == EEXIST)))
    {
        if (unlink(destA.c_str()) == 0)
        {
            result = rename(srcA.c_str(), destA.c_str());
        }
        else
        {
            SetLastError(FileGetLastErrorFromErrno());
            return FALSE;
        }
    }

    if (result != 0)
    {
        SetLastError(FileGetLastErrorFromErrno());
        return FALSE;
    }
    return TRUE;
}

BOOL DeleteFileW(LPCWSTR lpFileName)
{
    string fileA = FileNormalizePath(lpFileName);
    int result = unlink(fileA.c_str());
    if (result != 0)
    {
        SetLastError(FileGetLastErrorFromErrno());
    }
    return (result == 0);
}

BOOL GetFileAttributesExW(LPCWSTR lpFileName, GET_FILEEX_INFO_LEVELS fInfoLevelId, LPVOID lpFileInformation)
{
    if (lpFileName == NULL)
    {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
    }
    LPWIN32_FILE_ATTRIBUTE_DATA attr_data = (LPWIN32_FILE_ATTRIBUTE_DATA)lpFileInformation;;
    struct stat stat_data;
    string pathA = FileNormalizePath(lpFileName);
    attr_data->dwFileAttributes = GetFileAttributesW(lpFileName);
    if (attr_data->dwFileAttributes == (DWORD)-1)
    {
        return FALSE;
    }
    if (stat(pathA.c_str(), &stat_data) != 0)
    {
        SetLastError(FileGetLastErrorFromErrno());
        return FALSE;
    }
    attr_data->ftCreationTime = UnixTimeToFILETIME(stat_data.st_ctime, stat_data.st_ctim.tv_nsec);
    attr_data->ftLastAccessTime = UnixTimeToFILETIME(stat_data.st_atime, stat_data.st_atim.tv_nsec);
    attr_data->ftLastWriteTime = UnixTimeToFILETIME(stat_data.st_mtime, stat_data.st_mtim.tv_nsec);
    attr_data->nFileSizeLow = (DWORD) stat_data.st_size;
    attr_data->nFileSizeHigh = (DWORD)(stat_data.st_size >> 32);
    return TRUE;
}

HANDLE CreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
                          LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
                          IN DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
    if (lpFileName == NULL)
    {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return INVALID_HANDLE_VALUE;
    }
    string pathA = FileNormalizePath(lpFileName);

    map<DWORD, int> FlagMap = {
            {0, 0},
            {GENERIC_READ, O_RDONLY},
            {GENERIC_WRITE, O_WRONLY},
            {GENERIC_READ | GENERIC_WRITE, O_RDWR}
    };
    map<DWORD, int> FlagMapEx = {
            {CREATE_ALWAYS, O_CREAT | O_TRUNC},
            {CREATE_NEW, O_CREAT | O_EXCL},
            {OPEN_EXISTING, 0},
            {OPEN_ALWAYS, O_CREAT},
            {TRUNCATE_EXISTING, O_TRUNC}
    };
    if (FlagMap.find(dwDesiredAccess) == FlagMap.end() || FlagMapEx.find(dwCreationDisposition) == FlagMapEx.end())
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }
    int oflags = FlagMap[dwDesiredAccess] | FlagMapEx[dwCreationDisposition] | O_CLOEXEC;

    if (dwFlagsAndAttributes & FILE_FLAG_NO_BUFFERING )
    {
        oflags |= O_DIRECT;
    }

    struct stat stat_data;
    if (stat(pathA.c_str(), &stat_data) == 0 && (stat_data.st_mode & S_IFDIR))
    {
        SetLastError(ERROR_ACCESS_DENIED);
        return INVALID_HANDLE_VALUE;
    }

    int cflags = (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    int fd = open(pathA.c_str(), oflags, cflags);

    if (fd < 0)
    {
        SetLastError(FileGetLastErrorFromErrno());
        return INVALID_HANDLE_VALUE;
    }

    int lock_mode = (dwShareMode == 0) ? LOCK_EX : LOCK_SH;
    if (flock(fd, lock_mode | LOCK_NB) != 0)
    {
        if (errno == EWOULDBLOCK)
        {
            SetLastError(ERROR_SHARING_VIOLATION);
        }
        else
        {
            SetLastError(FileGetLastErrorFromErrno());
        }
        close(fd);
        return INVALID_HANDLE_VALUE;
    }
    if (fcntl(fd, F_SETFD, 1) != 0)
    {
        SetLastError(FileGetLastErrorFromErrno());
        close(fd);
        return INVALID_HANDLE_VALUE;
    }
    return (HANDLE)fd;
}

BOOL ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped)
{
    if (NULL != lpNumberOfBytesRead)
    {
        *lpNumberOfBytesRead = 0;
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (INVALID_HANDLE_VALUE == hFile)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    else if (NULL == lpBuffer)
    {
        SetLastError(ERROR_NOACCESS);
        return FALSE;
    }

    BOOL readSucceeded = 0;
    while(true)
    {
        int res = read((int)hFile, lpBuffer, nNumberOfBytesToRead);
        if (res >= 0)
        {
            *lpNumberOfBytesRead = res;
            readSucceeded = 1;
            break;
        }
        else if (errno != EINTR)
        {
            SetLastError(FileGetLastErrorFromErrno());
            break;
        }
    }
    return readSucceeded;
}

BOOL WriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped)
{
    if (NULL != lpNumberOfBytesWritten) {
        *lpNumberOfBytesWritten = 0;
    }
    else {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (hFile == INVALID_HANDLE_VALUE || hFile == 0) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    BOOL writeSucceeded = 0;
    while (true) {
        int res = write((int) hFile, lpBuffer, nNumberOfBytesToWrite);
        if (res >= 0) {
            *lpNumberOfBytesWritten = res;
            writeSucceeded = 1;
            break;
        }
        else if (errno != EINTR) {
            SetLastError(FileGetLastErrorFromErrno());
            break;
        }
    }
    return writeSucceeded;
}

BOOL SetFilePointerEx(HANDLE hFile, LARGE_INTEGER liDistanceToMove, PLARGE_INTEGER lpNewFilePointer, DWORD dwMoveMethod)
{
    LONG dist_low = (LONG)liDistanceToMove.u.LowPart;
    LONG dist_high = liDistanceToMove.u.HighPart;

    map<DWORD, int> WhenceMap = {
            {FILE_BEGIN, SEEK_SET},
            {FILE_CURRENT, SEEK_CUR},
            {FILE_END, SEEK_END}
    };
    if (WhenceMap.find(dwMoveMethod) == WhenceMap.end())
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    int seek_whence = WhenceMap[dwMoveMethod];
    int64_t seek_offset = 0L;
    seek_offset = ((int64_t)dist_high << 32);
    seek_offset |= (ULONG) dist_low;

    off_t old_offset = lseek((int)hFile, 0, SEEK_CUR);
    if (old_offset == -1)
    {
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }
    if ((seek_whence == SEEK_SET && seek_offset < 0) || (seek_whence == SEEK_CUR && seek_offset + old_offset < 0))
    {
        SetLastError(ERROR_NEGATIVE_SEEK);
        return FALSE;
    }
    else if (seek_whence == SEEK_END && seek_offset < 0)
    {
        struct stat fileData;
        if (fstat((int)hFile, &fileData) == -1)
        {
            SetLastError(ERROR_ACCESS_DENIED);
            return FALSE;
        }
        if (fileData.st_size < -seek_offset)
        {
            SetLastError(ERROR_NEGATIVE_SEEK);
            return FALSE;
        }
    }
    int64_t seek_res = (int64_t)lseek((int)hFile, seek_offset, seek_whence);
    if ( seek_res < 0 )
    {
        lseek((int)hFile, old_offset, SEEK_SET);
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }
    else
    {
        lpNewFilePointer->HighPart = (DWORD)(seek_res >> 32);
        lpNewFilePointer->LowPart = (DWORD)seek_res;
        return TRUE;
    }
}

BOOL GetFileSizeEx(HANDLE hFile,PLARGE_INTEGER lpFileSize)
{
    if (INVALID_HANDLE_VALUE == hFile)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (NULL == lpFileSize)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    struct stat stat_data;
    if (fstat((int)hFile, &stat_data) != 0)
    {
        SetLastError(FileGetLastErrorFromErrno());
        return FALSE;
    }
    lpFileSize->HighPart = (DWORD) (stat_data.st_size >> 32);
    lpFileSize->LowPart = (DWORD)stat_data.st_size;
    errno = 0;
    return TRUE;
}

BOOL SetEndOfFile(HANDLE hFile)
{
    if (INVALID_HANDLE_VALUE == hFile)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    off_t curr = lseek((int)hFile, 0, SEEK_CUR);
    if (curr < 0)
    {
        SetLastError(FileGetLastErrorFromErrno());
        return FALSE;
    }
    if (ftruncate((int)hFile, curr) != 0)
    {
        SetLastError(FileGetLastErrorFromErrno());
        return FALSE;
    }
    return TRUE;
}

BOOL GetFileTime(HANDLE hFile, LPFILETIME lpCreationTime, LPFILETIME lpLastAccessTime, LPFILETIME lpLastWriteTime)
{
    if (INVALID_HANDLE_VALUE == hFile)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    struct stat stat_data;
    if ( fstat((int)hFile, &stat_data) != 0 )
    {
        SetLastError(FileGetLastErrorFromErrno());
        return FALSE;
    }

    if (lpCreationTime)
    {
        *lpCreationTime = UnixTimeToFILETIME(stat_data.st_ctime, stat_data.st_ctim.tv_nsec);
    }
    if (lpLastWriteTime)
    {
        *lpLastWriteTime = UnixTimeToFILETIME(stat_data.st_mtime, stat_data.st_mtim.tv_nsec);
    }
    if (lpLastAccessTime)
    {
        *lpLastAccessTime = UnixTimeToFILETIME(stat_data.st_atime, stat_data.st_atim.tv_nsec);
    }
    return TRUE;
}

BOOL FlushFileBuffers(HANDLE hFile)
{
    if (INVALID_HANDLE_VALUE == hFile)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    BOOL succ = FALSE;
    while(true)
    {
        if (fsync((int)hFile) == 0)
        {
            succ = TRUE; break;
        }
        if (errno != EINTR)
        {
            SetLastError(FileGetLastErrorFromErrno());
        }
    };
    return succ;
}

#endif

using namespace std;

namespace Common
{
    StringLiteral const TraceSource("File");

    //
    // *** FileFind (search helper)
    //

#if !defined(PLATFORM_UNIX)
    class File::FileFind : public File::IFileEnumerator
    {
        DENY_COPY(FileFind);

    public:

        FileFind(std::wstring const & pattern)
            : pattern_(pattern), handle_(INVALID_HANDLE_VALUE)
        { }

        FileFind(std::wstring && pattern)
            : pattern_(pattern), handle_(INVALID_HANDLE_VALUE)
        { }

        ~FileFind()
        {
            if (handle_ != INVALID_HANDLE_VALUE) { FindClose(handle_); }
        }

    public: // IFileEnumerator

        ErrorCode MoveNext() override
        {
            if (handle_ == INVALID_HANDLE_VALUE)
            {
                auto ntPath = Path::ConvertToNtPath(pattern_);
                handle_ = ::FindFirstFile(ntPath.c_str(), &data_);
                if (handle_ != INVALID_HANDLE_VALUE)
                {
                    return ErrorCodeValue::Success;
                }
            }
            else
            {
                if (::FindNextFile(handle_, &data_))
                {
                    return ErrorCodeValue::Success;
                }
            }

            return ErrorCode::FromWin32Error(::GetLastError());
        }

        WIN32_FIND_DATA const & GetCurrent() override { return data_; }
        DWORD GetCurrentAttributes() override { return data_.dwFileAttributes; }
        std::wstring GetCurrentPath() override { return std::wstring(data_.cFileName); }

    private:
        std::wstring pattern_;
        HANDLE handle_;
        WIN32_FIND_DATA data_;
    };

#else

    class File::FileFind : public File::IFileEnumerator
    {
    DENY_COPY(FileFind);

    public:

        FileFind(std::wstring const & pattern)
                : pattern_(pattern), inited_(false), index_(0)
        { }

        FileFind(std::wstring && pattern)
                : pattern_(pattern), inited_(false), index_(0)
        { }

        ~FileFind()
        {
        }

    public: // IFileEnumerator

        ErrorCode MoveNext() override
        {
            if (!inited_)
            {
                inited_ = true;
                string pathA = FileNormalizePath(pattern_.c_str());
                char dirA[MAX_PATH], patA[MAX_PATH];
                memcpy(dirA, pathA.c_str(), pathA.length() + 1);
                memcpy(patA, pathA.c_str(), pathA.length() + 1);
                char *dir = dirname(dirA);
                char *pat = basename(patA);

                DIR *dp;
                if (0 ==(dp = opendir(dir))) {
                    return ErrorCode::FromWin32Error(ERROR_FILE_NOT_FOUND);
                }

                struct dirent entry;
                struct dirent *dptr = NULL;
                while (true) {
                    if (readdir_r(dp, &entry, &dptr) || !dptr) break;

                    string currentName(dptr->d_name);
                    string currentPath = string(dir) + "/" + currentName;
                    if (GlobMatch(currentName.c_str(), pat))
                    {
                        wstring currentNameW;
                        StringUtility::Utf8ToUtf16(currentName, currentNameW);

                        WIN32_FIND_DATA findData;
                        memset(&findData, 0, sizeof(findData));
                        memcpy(findData.cFileName, currentNameW.c_str(), (currentNameW.length() + 1) * sizeof(wchar_t));
                        struct stat stat_data;
                        if (stat(currentPath.c_str(), &stat_data) == 0) {
                            findData.dwFileAttributes = S_ISDIR(stat_data.st_mode) ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
                            findData.ftCreationTime = UnixTimeToFILETIME(stat_data.st_ctime, stat_data.st_ctim.tv_nsec);
                            findData.ftLastAccessTime = UnixTimeToFILETIME(stat_data.st_atime, stat_data.st_atim.tv_nsec);
                            findData.ftLastWriteTime = UnixTimeToFILETIME(stat_data.st_mtime, stat_data.st_mtim.tv_nsec);
                            findData.nFileSizeLow = (DWORD) stat_data.st_size;
                            findData.nFileSizeHigh = (DWORD) (stat_data.st_size >> 32);
                            fileList_.push_back(findData);
                        }
                    }
                }
                closedir(dp);
                if (fileList_.size() > 0) {
                    memcpy(&data_, fileList_.data(), sizeof(data_));
                    return ErrorCodeValue::Success;
                }
                else
                {
                    return ErrorCode::FromWin32Error(ERROR_FILE_NOT_FOUND);
                }
            }
            else
            {
                index_++;
                if (index_ < fileList_.size())
                {
                    memcpy(&data_, &(fileList_[index_]), sizeof(data_));
                    return ErrorCodeValue::Success;
                }
                else
                {
                    return ErrorCode::FromWin32Error(ERROR_NO_MORE_FILES);
                }
            }
        }

        WIN32_FIND_DATA const & GetCurrent() override { return data_; }
        DWORD GetCurrentAttributes() override { return data_.dwFileAttributes; }
        std::wstring GetCurrentPath() override { return std::wstring(data_.cFileName); }

    private:
        std::wstring pattern_;
        WIN32_FIND_DATA data_;

        bool inited_;
        int index_;
        vector<WIN32_FIND_DATA> fileList_;
    };

#endif

    //
    // ** File
    //

    File::File()
        : fileName_()
        , handle_(INVALID_HANDLE_VALUE)
        , isHandleOwned_(false)
    {
    }

    File::File(File && other)
        : fileName_(move(other.fileName_))
        , handle_(move(other.handle_))
        , isHandleOwned_(move(other.isHandleOwned_))
    {
        other.handle_ = INVALID_HANDLE_VALUE;
        other.isHandleOwned_ = false;
    }

    File::File(::HANDLE handle)
        : fileName_()
        , handle_(CHK_HANDLE_VALID(handle))
        // handle resource is owned by caller
        , isHandleOwned_(false)
    {
    }

    File & File::operator=(File && other)
    {
        if (this != &other)
        {
            fileName_ = move(other.fileName_);
            handle_ = CHK_HANDLE_VALID(other.handle_);
            isHandleOwned_ = other.isHandleOwned_;

            other.handle_ = INVALID_HANDLE_VALUE;
            other.isHandleOwned_ = false;
        }

        return *this;
    }


    ErrorCode File::Open(std::wstring const & fname, FABRIC_FILE_MODE fileMode, FABRIC_FILE_ACCESS fileAccess, FABRIC_FILE_SHARE fileShare, ::HANDLE * fileHandle)
    {
        return Open(fname, fileMode, fileAccess, fileShare, FABRIC_FILE_ATTRIBUTES_NORMAL, fileHandle);
    }

    ErrorCode File::Open(std::wstring const & fname, FABRIC_FILE_MODE fileMode, FABRIC_FILE_ACCESS fileAccess, FABRIC_FILE_SHARE fileShare, FABRIC_FILE_ATTRIBUTES fileAttributes, ::HANDLE * fileHandle)
    {
        auto error = TryOpen(fname, FileMode::Enum(fileMode), FileAccess::Enum(fileAccess), FileShare::Enum(fileShare), FileAttributes::Enum(fileAttributes));
        if (!error.IsSuccess())
        {
            return error;
        }

#if !defined(PLATFORM_UNIX)
        ::DuplicateHandle(::GetCurrentProcess(), handle_, ::GetCurrentProcess(), fileHandle, 0, false, DUPLICATE_SAME_ACCESS);
#else
        *fileHandle = handle_;
#endif
        if (*fileHandle == INVALID_HANDLE_VALUE)
        {
            return ErrorCode::FromWin32Error(::GetLastError());
        }
        else
        {
            return error;
        }
    }

    //
    // !!! Don't do tracing in TryOpen, because the file may be a trace file, and it may lead to deadlock.
    // !!! [Windows 8 Bugs:207669] Microsoft.Fabric.Test hangs if text file tracing is enabled
    //
    ErrorCode File::TryOpen(std::wstring const & fname, FileMode::Enum mode, FileAccess::Enum access, FileShare::Enum share, FileAttributes::Enum attributes )
    {
        Close();
        fileName_ = fname;
        auto ntPath = Path::ConvertToNtPath(fname);
        handle_ = ::CreateFileW(ntPath.c_str(), access, share, nullptr, mode, attributes, nullptr);
        isHandleOwned_ = true;

        DWORD win32Error = ERROR_SUCCESS;
        if (handle_ == INVALID_HANDLE_VALUE)
        {
            win32Error = GetLastError();
        }

        return ErrorCode::FromWin32Error(win32Error);
    }

    void File::Close()
    {
        if (INVALID_HANDLE_VALUE != handle_)
        {
#if !defined(PLATFORM_UNIX)
            CHK_WBOOL( ::CloseHandle( handle_ ));
#else
            int res = close((int)handle_);
#endif

            handle_ = INVALID_HANDLE_VALUE;
        }
    }

    ErrorCode File::Close2()
    {
        if (INVALID_HANDLE_VALUE != handle_)
        {
#if !defined(PLATFORM_UNIX)
            if (!::CloseHandle(handle_))
            {
                return ErrorCode::FromWin32Error(::GetLastError());
            }
#else
            close((int)handle_);
#endif

            handle_ = INVALID_HANDLE_VALUE;
        }

        return ErrorCodeValue::Success;
    }

    void File::GetWriteTime(FILETIME & fileWriteTime)
    {
        CHK_HANDLE_VALID(handle_);

        CHK_WBOOL( ::GetFileTime( handle_, NULL, NULL, &fileWriteTime ));

        return;
    }

    ErrorCode File::GetLastWriteTime(std::wstring const & path, __out DateTime & lastWriteTime)
    {
        WIN32_FILE_ATTRIBUTE_DATA fileAttributes;
        auto ntPath = Path::ConvertToNtPath(path);
        if (!::GetFileAttributesEx(ntPath.c_str(), GET_FILEEX_INFO_LEVELS::GetFileExInfoStandard, &fileAttributes))
        {
            Trace.WriteError(
                TraceSource,
                L"File.GetLastWriteTime",
                "GetFileAttributesEx failed with the following error {0}",
                ::GetLastError());
            return ErrorCode::FromWin32Error(::GetLastError());
        }
        lastWriteTime = DateTime(fileAttributes.ftLastWriteTime);
        Trace.WriteInfo(
            TraceSource,
            L"File.GetLastWriteTime",
            "GetFileAttributesEx got file time {0} and set last writetime to {1}",
            fileAttributes.ftLastWriteTime.dwLowDateTime,
            lastWriteTime);
        return ErrorCodeValue::Success;
    }

    int File::TryRead(__out_bcount_part(count, return) void* buffer, int count)
    {
        ::DWORD bytesRead = 0;
        if (!::ReadFile(handle_, buffer, count, &bytesRead, nullptr))
        {
            return 0;
        }

        return bytesRead;
    };

    ErrorCode File::TryRead2(__out_bcount_part(count, bytesRead) void* buffer, int count, DWORD &bytesRead)
    {
        if (count == 0)
        {
            bytesRead = 0;
            return ErrorCodeValue::Success;
        }

        BOOL result = ::ReadFile(handle_, buffer, count, &bytesRead, nullptr);
        if (result)
        {
            return ErrorCodeValue::Success;
        }
        else
        {
            return ErrorCode::FromWin32Error(::GetLastError());
        }
    };

    int File::TryWrite(__in_bcount(count) const void* buffer, int count)
    {
        ::DWORD bytesWritten = 0;
        ::WriteFile(handle_, buffer, (DWORD)count, &bytesWritten, nullptr);

        return bytesWritten;
    }

    ErrorCode File::TryWrite2(__in_bcount(count) const void* buffer, int count, DWORD & bytesWritten)
    {
        if(!WriteFile(handle_, buffer, (DWORD)count, &bytesWritten, nullptr))
        {
            return ErrorCode::FromWin32Error();
        }

        return ErrorCodeValue::Success;
    }

    ErrorCode File::GetAttributes(const std::wstring& path, FileAttributes::Enum & attribute)
    {
        ErrorCode error;
        auto ntPath = Path::ConvertToNtPath(path);
        ::DWORD result = ::GetFileAttributesW(ntPath.c_str());
        if (result == INVALID_FILE_ATTRIBUTES)
        {
            error = ErrorCode::FromWin32Error(::GetLastError());
            Trace.WriteWarning(
                TraceSource,
                L"File.GetAttributes",
                "GetAttributes failed with the following error {0} for the path:{1}",
                error,
                path);
        }

        attribute = FileAttributes::Enum(result);
        return error;
    }

    /*
     On Linux if you want to grant 777 permission to file/directory use method AllowAccessToAll.
    */
    ErrorCode File::SetAttributes(const std::wstring& path, FileAttributes::Enum fileAttributes)
    {
        ErrorCode error;
        auto ntPath = Path::ConvertToNtPath(path);
        if(!::SetFileAttributesW(ntPath.c_str(), fileAttributes))
        {
            error = ErrorCode::FromWin32Error(::GetLastError());
        }

        return error;
    }

#if defined(PLATFORM_UNIX)
    /*
     Grants 777 permission to a file/directory.
    */
    ErrorCode File::AllowAccessToAll(const std::wstring& path)
    {
        auto ntPath = Path::ConvertToNtPath(path);
        LPCWSTR lpFileName = ntPath.c_str();
        if (lpFileName == NULL)
        {
            Trace.WriteError(
                TraceSource,
                L"File.AllowAccessToAll",
                "AllowAccessToAll Failed. Error:FileNotFound, path:{0}, ntPath:{1}",
                path,
                ntPath);
            SetLastError(ERROR_PATH_NOT_FOUND);
            return ErrorCode::FromWin32Error(::GetLastError());
        }

        string pathA = FileNormalizePath(lpFileName);
        int result = chmod(pathA.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
        if (result != 0)
        {
            Trace.WriteError(
                TraceSource,
                L"File.AllowAccessToAll",
                "AllowAccessToAll Failed. Error:{0}, path:{1}, lpFileName:{2}",
                result,
                path,
                lpFileName);
            SetLastError(FileGetLastErrorFromErrno());
            return ErrorCode::FromWin32Error(::GetLastError());
        }

        return ErrorCodeValue::Success;
    }
#endif

    ErrorCode File::RemoveReadOnlyAttribute(const std::wstring& path)
    {
        FileAttributes::Enum attributes;
        ErrorCode attribError = File::GetAttributes(path, attributes);

        if (attribError.IsSuccess())
        {
            bool isReadOnlySet = (attributes & FileAttributes::ReadOnly) != 0;
            if (isReadOnlySet)
            {
                attributes = FileAttributes::Enum(attributes & ~FileAttributes::ReadOnly);
                attribError = File::SetAttributes(path, attributes);
            }
        }

        return attribError;
    }

    ErrorCode File::Delete()
    {
        return File::Delete2(fileName_);
    }

    void File::Delete(
        const std::wstring& path,
        bool throwIfNotFound)
    {
        bool success = File::Delete(path, NOTHROW());
        if (!success)
        {
            ::DWORD status = ::GetLastError();

            if ((status != ERROR_FILE_NOT_FOUND &&
                status != ERROR_PATH_NOT_FOUND &&
                status != ERROR_INVALID_NAME)||
                throwIfNotFound )
            {
                THROW_FMT(WinError(status), "Delete failed for file at '{0}'", path);
            }
        }
    }

    bool File::Delete(std::wstring const & path, NOTHROW, bool const deleteReadonly)
    {
        if (deleteReadonly)
        {
            FileAttributes::Enum attributes;
            ErrorCode attribError = File::GetAttributes(path, attributes);
            bool isReadOnlySet = (attributes & FileAttributes::ReadOnly) != 0;
            if (attribError.IsSuccess() && isReadOnlySet)
            {
                attributes = FileAttributes::Enum(attributes & ~FileAttributes::ReadOnly);
                attribError = File::SetAttributes(path, attributes);
                if (!attribError.IsSuccess())
                {
                    Trace.WriteError(
                        TraceSource,
                        L"File.Delete",
                        "SetFileAttributes failed with the following error {0}",
                        attribError);
                }
            }
        }
        auto ntPath = Path::ConvertToNtPath(path);
        return (::DeleteFileW(ntPath.c_str()) != 0);
    }

    ErrorCode File::Delete2( std::wstring const & path, bool const deleteReadonly)
    {
        bool success = File::Delete(path, NOTHROW(), deleteReadonly);
        if (!success)
        {
            return ErrorCode::FromWin32Error(::GetLastError());
        }

        return ErrorCode::Success();
    }

    ErrorCode File::Replace(std::wstring const & replacedFileName, std::wstring const & replacementFileName, std::wstring const & backupFileName, bool ignoreMergeErrors)
    {
#ifdef PLATFORM_UNIX
        ignoreMergeErrors;

        wstring backupFileToUse = backupFileName.empty()? GetTempFileName() : backupFileName;
        auto err = Move(replacedFileName, backupFileToUse, false);
        if (!err.IsSuccess())
        {
            Trace.WriteError(
                TraceSource,
                "{0}: failed to move orignal file {1} to {2}",
                __func__,
                replacedFileName,
                backupFileToUse,
                err);

            return err;
        }
       
        err = Move(replacementFileName, replacedFileName, false);
        if (!err.IsSuccess())
        {
            Trace.WriteError(
                TraceSource,
                "{0}: failed to rename replacement file {1} to replaced file {2}", 
                __func__,
                replacementFileName,
                replacedFileName,
                err);

            return err;
        }

        if (backupFileName.empty())
        {
            err = Delete2(backupFileToUse);
            if (!err.IsSuccess())
            {
                Trace.WriteWarning(
                    TraceSource,
                    "{0}: failed to delete temporary backup file {1}", 
                    __func__,
                    backupFileToUse,
                    err);
            }
        }
#else
        auto ntPathReplacedFileName = Path::ConvertToNtPath(replacedFileName);
        auto ntPathReplacementFileName = Path::ConvertToNtPath(replacementFileName);
        wstring ntPathBackupFileName;
        if (backupFileName.length() != 0)
        {
            ntPathBackupFileName = Path::ConvertToNtPath(backupFileName);
        }

        BOOL success = ::ReplaceFileW(
            ntPathReplacedFileName.c_str(),
            ntPathReplacementFileName.c_str(),
            (backupFileName.length() == 0) ? NULL : ntPathBackupFileName.c_str(),
            ignoreMergeErrors ? REPLACEFILE_IGNORE_MERGE_ERRORS : 0,
            0,
            0);

        if (!success)
        {
            return ErrorCode::FromWin32Error(GetLastError());
        }
#endif

        return ErrorCode::Success();
    }

    bool File::CreateHardLink(std::wstring const & fileName, std::wstring const & existingFileName)
    {
#ifdef PLATFORM_UNIX
        string fileNameUtf8;
        StringUtility::Utf16ToUtf8(fileName, fileNameUtf8);
        string existingFileNameUtf8;
        StringUtility::Utf16ToUtf8(existingFileName, existingFileNameUtf8);
        auto retval = link(existingFileNameUtf8.c_str(), fileNameUtf8.c_str());
        if (retval < 0)
        {
            auto err = errno;
            Trace.WriteWarning(
                TraceSource,
                "{0}: link({1}, {2}) failed: {3}",
                __func__,
                existingFileNameUtf8,
                fileNameUtf8,
                err);
        }

        return retval == 0;
#else
        auto ntPathFileName = Path::ConvertToNtPath(fileName);
        auto ntPathExistingFileName = Path::ConvertToNtPath(existingFileName);
        BOOL result = ::CreateHardLinkW(
            ntPathFileName.c_str(),
            ntPathExistingFileName.c_str(),
            0);

        return (result != 0);
#endif
    }

    shared_ptr<File::IFileEnumerator> File::Search(wstring && pattern)
    {
        return make_shared<FileFind>(move(pattern));
    }

    bool File::TryGetSize(__out int64 & size) const
    {
        return GetSize(size).IsSuccess();
    }

    _Use_decl_annotations_
    ErrorCode File::GetSize(int64 & size) const
    {
        ::LARGE_INTEGER length;
        if (GetFileSizeEx(handle_, &length) == 0)
        {
            size = 0;
            return ErrorCode::FromWin32Error();
        }

        size = length.QuadPart;
        return ErrorCode();
    }

    ErrorCode File::GetSize(wstring const & fname, _Out_ int64 & size)
    {
        if (!File::Exists(fname))
        {
            Trace.WriteInfo(
                TraceSource, 
                "GetSize: file not found {0}",
                fname);

            size = 0;
            return ErrorCodeValue::Success;
        }

        File file;
        auto error = file.TryOpen(fname, FileMode::Open, FileAccess::Read, FileShare::Read);
        if (!error.IsSuccess()) 
        { 
            Trace.WriteWarning(
                TraceSource, 
                "failed to open {0}: {1}",
                fname,
                error);

            return error;
        }

        int64 fileSize = 0;
        error = file.GetSize(fileSize);
        if (!error.IsSuccess()) 
        { 
            Trace.WriteWarning(
                TraceSource, 
                "failed to get file size {0}: {1}",
                fname,
                error);

            return error;
        }

        error = file.Close2();
        if (!error.IsSuccess()) 
        { 
            Trace.WriteWarning(
                TraceSource, 
                "failed to close {0}: {1}",
                fname,
                error);

            return error;
        }

        size = fileSize;

        return error;
    }

    int64 File::size() const
    {
        int64 size;
        CHK_WBOOL(TryGetSize(size));
        return size;
    }

#if !defined(PLATFORM_UNIX)
    void File::resize(int64 position)
    {
        ::LARGE_INTEGER oldPosition, delta;
        delta.QuadPart = position;
        CHK_WBOOL(SetFilePointerEx(handle_, delta, &oldPosition, FILE_BEGIN));
        CHK_WBOOL(SetEndOfFile(handle_));
    }
#endif

    int64 File::Seek(int64 offset, Common::SeekOrigin::Enum origin)
    {
        ::LARGE_INTEGER newPosition, delta;
        delta.QuadPart = offset;

        CHK_WBOOL( SetFilePointerEx(handle_, delta, &newPosition, (int)origin) );
        return newPosition.QuadPart;
    }

    void File::Flush()
    {
        CHK_WBOOL( ::FlushFileBuffers(handle_) );
    }

#if !defined(PLATFORM_UNIX)
    ErrorCode File::FlushVolume(const WCHAR driveLetter)
    {
        if (driveLetter == NULL)
        {
            return ErrorCodeValue::ArgumentNull;
        }

        ErrorCode error(ErrorCodeValue::Success);
        wstring volumePath = L"\\\\.\\";
        volumePath.push_back(driveLetter);
        volumePath.push_back(L':');
        ::HANDLE volumeHandle = ::CreateFile(volumePath.c_str(), FILE_GENERIC_WRITE, Common::FileShare::ReadWrite, nullptr, Common::FileMode::Open, FILE_FLAG_OVERLAPPED, nullptr);

        if (volumeHandle == INVALID_HANDLE_VALUE)
        {
            error = ErrorCode::FromWin32Error(GetLastError());
            if (error.IsError(ErrorCodeValue::AccessDenied))
            {
                Trace.WriteError(
                    TraceSource,
                    "failed to obtain volume handle '{0}' due to permissions issue. Error {1}.",
                    volumePath.c_str(),
                    error);
            }
            else
            {
                Trace.WriteError(
                    TraceSource,
                    "failed to obtain volume handle '{0}' error {1}.",
                    volumePath.c_str(),
                    error);
            }
            
            return error;
        }

        BOOL result = ::FlushFileBuffers(volumeHandle);
        if (!result)
        {
            error = ErrorCode::FromWin32Error(GetLastError());
            Trace.WriteError(
                TraceSource,
                "FlushVolume failed to execute FlushFileBuffers on volume '{0}' handle '{1}' error {2}. This may be the result of a permissions issue.",
                volumePath.c_str(),
                volumeHandle,
                error);
        }

        if (!::CloseHandle(volumeHandle)) 
        {
            ErrorCode handleCloseError = ErrorCode::FromWin32Error(GetLastError());
            Trace.WriteError(
                TraceSource,
                "FlushVolume failed to CloseHandle on volume '{0}' handle '{1}' error {2}.",
                volumePath.c_str(),
                volumeHandle,
                handleCloseError);

            if (error.IsSuccess()) // Don't overwrite initial errorcode if previous call failed)
            {
                error = handleCloseError;
            }
        }

        return error;
    }
#endif // !defined(PLATFORM_UNIX)


    ErrorCode File::Touch(wstring const & fileName)
    {
        File file;
        auto error = file.TryOpen(
            fileName, 
            FileMode::OpenOrCreate, 
            FileAccess::Write, 
            FileShare::None);

        if (!error.IsSuccess())
        {
            Trace.WriteWarning(
                TraceSource,
                "Failed to touch file '{0}': {1}",
                fileName,
                error);

            return error;
        }

        error = file.Close2();

        if (!error.IsSuccess())
        {
            Trace.WriteWarning(
                TraceSource,
                "Failed to close file after touch '{0}': {1}",
                fileName,
                error);
        }

        return error;
    }

    bool File::Exists(const std::wstring& path)
    {
        auto ntPath = Path::ConvertToNtPath(path);
        ::DWORD result = ::GetFileAttributesW(ntPath.c_str());

        if (result != INVALID_FILE_ATTRIBUTES) {
            return (result & FILE_ATTRIBUTE_DIRECTORY) == 0; // not a directory
        }
        if (result == ERROR_FILE_NOT_FOUND)
            return false;
        if (result == ERROR_ACCESS_DENIED)
            return true;

        // CONSIDER: throwing exception on unknown failures
        return false;
    }

    ErrorCode File::Move( const std::wstring& SourceFile, const std::wstring& DestFile, bool throwIfFail)
    {
        //Log.Info.WriteLine("[FILE] Moving from <{0}> to <{1}>", SourceFile, DestFile);
#if defined(PLATFORM_UNIX)
        auto flag = 0;
#else
        auto flag = MOVEFILE_WRITE_THROUGH;
#endif
        auto ntPathSourceFile = Path::ConvertToNtPath(SourceFile);
        auto ntPathDestFile = Path::ConvertToNtPath(DestFile);
        auto retval =  ::MoveFileExW(
            ntPathSourceFile.c_str(),
            ntPathDestFile.c_str(),
            MOVEFILE_REPLACE_EXISTING | flag);

        if (throwIfFail) CHK_WBOOL(retval);

        ErrorCode err;
        if (retval == FALSE)
        {
            err = ErrorCode::FromWin32Error();
        }

        return err;
    }

    temporary_file::temporary_file(std::wstring const & fileName)
        : file_handle_(INVALID_HANDLE_VALUE),
        file_name_(fileName)
    {
        //
        // Open the file handle, marking the file to be deleted upon close.
        //
        auto ntPath = Path::ConvertToNtPath(fileName);
        file_handle_
            = CreateFile(ntPath.c_str(), GENERIC_ALL, Common::FileShare::ReadWriteDelete, NULL, Common::FileMode::OpenOrCreate, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, NULL);
        std::error_code err = microsoft::GetLastErrorCode();
        system_error_if(file_handle_ == INVALID_HANDLE_VALUE, err, "Cannot create temporary file at {0}", fileName);
    }

    temporary_file::~temporary_file()
    {
        if (file_handle_ != INVALID_HANDLE_VALUE)
        {
#if !defined(PLATFORM_UNIX)
            if (!CloseHandle(file_handle_))
            {
                std::error_code err = microsoft::GetLastErrorCode();
                FAIL_CODING_ERROR_FMT("Cannot close file handle of temp file at {0} because of {1}", file_name_, err);
            }
#else
            if (0 != close((int)file_handle_))
            {
                FAIL_CODING_ERROR_FMT("Cannot close file handle of temp file at {0} because of {1}", file_name_, errno);
            }
#endif
        }
    }

#if defined(PLATFORM_UNIX)
    class ScpSessionManagerSingleton
    {
        DENY_COPY(ScpSessionManagerSingleton)

    public:
        class ScpRequest
        {
            friend class ScpSessionManagerSingleton;

        private:
            string peer_;
            string src_;
            string dest_;
            string acct_;
            string passwd_;

            bool write_;

            sem_t sema_;
            ErrorCode error_;
            size_t szCopied_;
            size_t szSrc_;

        public:
            ScpRequest(const string &peer, const string &acct, const string &passwd, const string &src, const string &dest, bool write)
            : peer_(peer), acct_(acct), passwd_(passwd), src_(src), dest_(dest), write_(write), error_(ErrorCodeValue::Success), szSrc_(0), szCopied_(0)
            {
                sem_init(&sema_, 0, 0);
            }

            ~ScpRequest()
            {
                sem_destroy(&sema_);
            }

            void WaitForCompletion()
            {
                int ret = 0;
                do
                {
                    ret = sem_wait(&sema_);
                } while (ret != 0 && errno == EINTR);
            }

            void SetCompleted() { sem_post(&sema_); }

            ErrorCode GetError() { return error_; }

            size_t GetSrcSize() { return szSrc_; }

            size_t GetCopiedSize() { return szCopied_; }
        };

        class ScpWorker
        {
        public:
            ScpWorker(const string & addr, const string & acct, const string & passwd)
            : peerAddr_(addr), account_(acct), password_(passwd), closed_(false), errno_(0)
            {
                pthread_mutex_init(&queueLock_, NULL);
                sem_init(&queueSema_, 0, 0);
            }

            ~ScpWorker()
            {
                pthread_mutex_destroy(&queueLock_);
                sem_destroy(&queueSema_);
            }

            void Start()
            {
                pthread_t pthread;
                pthread_attr_t attr;
                int err = pthread_attr_init(&attr);
                _ASSERTE(err == 0);
                err = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
                _ASSERTE(err == 0);
                err = pthread_create(&pthread, &attr, ScpWorkerThreadStart, this);
                _ASSERTE(err == 0);
                pthread_attr_destroy(&attr);
            }

            ErrorCode QueueWork(ScpRequest *request)
            {
                ErrorCode err = ErrorCodeValue::Success;
                int closed = false;

                pthread_mutex_lock(&queueLock_);
                if (closed_)
                {
                    closed = true;
                }
                else
                {
                    queue_.push(request);
                    sem_post(&this->queueSema_);
                }
                pthread_mutex_unlock(&queueLock_);

                if (closed)
                {
                    err = ErrorCodeValue::ObjectClosed;
                }
                return err;
            }

        private:
            string peerAddr_;
            string account_;
            string password_;

            int errno_;
            bool closed_;

            sem_t queueSema_;
            pthread_mutex_t queueLock_;
            queue<ScpRequest *> queue_;

            const int MaxIdleTimeInMs = 10000;
            const short ScpHostPort = 22;

            static void ScpWorkerThreadCleanup(ScpWorker* worker, LIBSSH2_SESSION *session, LIBSSH2_SFTP *sftp_session)
            {
                Trace.WriteInfo(TraceSource, L"SCPCopy", "ScpCopy worker to {0} is cleaning up with errno {1}.", worker->peerAddr_, worker->errno_);

                if (sftp_session)
                {
                    libssh2_sftp_shutdown(sftp_session);
                }

                if (session)
                {
                    worker->errno_ = libssh2_session_last_errno(session);
                    libssh2_session_disconnect(session, "Shutdown");
                    libssh2_session_free(session);
                }

                // clean up the queue
                pthread_mutex_lock(&worker->queueLock_);
                worker->closed_ = true;
                while (!worker->queue_.empty())
                {
                    ScpRequest* pRequest = worker->queue_.front();
                    pRequest->error_ = ErrorCodeValue::ObjectClosed;
                    pRequest->SetCompleted();
                    worker->queue_.pop();
                }
                pthread_mutex_unlock(&worker->queueLock_);
            }

            static void* ScpWorkerThreadStart(void* param)
            {
                const char* ScpTmpFileSuffix = "._sftmp_";

                ScpWorker* worker = (ScpWorker*)param;

                Trace.WriteInfo(TraceSource, L"SCPCopy", "ScpCopy worker to {0} Created.", worker->peerAddr_);

                struct addrinfo hints;
                struct addrinfo *peer, *peeriter;
                memset(&hints, 0, sizeof(struct addrinfo));
                hints.ai_family = AF_UNSPEC;
                hints.ai_socktype = SOCK_STREAM;
                int ret = getaddrinfo(worker->peerAddr_.c_str(), "ssh", &hints, &peer);
                if(0 != ret)
                {
                    worker->errno_ = errno;
                    ScpWorkerThreadCleanup(worker, 0, 0);
                    Trace.WriteInfo(TraceSource, L"SCPCopy", "ScpCopy worker to {0}: getaddrinfo failed with error: {1}.", worker->peerAddr_, gai_strerror(ret));
                    return 0;
                }
                int sock = socket(AF_INET, SOCK_STREAM, 0);
                if(-1 == sock)
                {
                    worker->errno_ = errno;
                    ScpWorkerThreadCleanup(worker, 0, 0);
                    Trace.WriteInfo(TraceSource, L"SCPCopy", "ScpCopy worker to {0}: socket failed with errno {1}.", worker->peerAddr_, errno);
                    return 0;
                }

                bool connected = false;
                struct sockaddr_in *sin;
                peeriter = peer;
                while (peeriter)
                {
                    sin = (struct sockaddr_in *) peeriter->ai_addr;
                    if (connect(sock, (struct sockaddr*)(sin), sizeof(struct sockaddr_in)) == 0)
                    {
                        connected = true;
                        break;
                    }
                    peeriter = peeriter->ai_next;
                }

                if (!connected)
                {
                    Trace.WriteInfo(TraceSource, L"SCPCopy", "ScpCopy worker to {0}: connect failed with errno {1}.", worker->peerAddr_, errno);
                    freeaddrinfo(peer);
                    worker->errno_ = errno;
                    ScpWorkerThreadCleanup(worker, 0, 0);
                    return 0;
                }
                freeaddrinfo(peer);

                LIBSSH2_SESSION *session = libssh2_session_init();

                if (!session)
                {
                    Trace.WriteInfo(TraceSource, L"SCPCopy", "ssh session initialization failed.");
                    worker->errno_ = ENOMEM;
                    ScpWorkerThreadCleanup(worker, 0, 0);
                    return 0;
                }

                int rc = libssh2_session_handshake(session, sock);

                if (rc) {
                    Trace.WriteInfo(TraceSource, L"SCPCopy", "ScpCopy worker to {0}: ssh session handshake failed with {1}.", worker->peerAddr_, rc);
                    ScpWorkerThreadCleanup(worker, session, 0);
                    return 0;
                }

                if (libssh2_userauth_password(session, worker->account_.c_str(), worker->password_.c_str()))
                {
                    Trace.WriteInfo(TraceSource, L"SCPCopy", "ScpCopy worker to {0}: ssh authentication failed.", worker->peerAddr_);
                    ScpWorkerThreadCleanup(worker, session, 0);
                    return 0;
                }

                LIBSSH2_SFTP *sftp_session = libssh2_sftp_init(session);
                if (!sftp_session)
                {
                    Trace.WriteInfo(TraceSource, L"SCPCopy", "ScpCopy worker to {0}: sftp session initialization failed.", worker->peerAddr_);
                    ScpWorkerThreadCleanup(worker, session, sftp_session);
                    return 0;
                }

                // process the queue items
                while (true)
                {
                    int ret = 0;
                    timespec ts;
                    clock_gettime(CLOCK_REALTIME, &ts);
                    ts.tv_sec += worker->MaxIdleTimeInMs / 1000;
                    ts.tv_nsec += (worker->MaxIdleTimeInMs % 1000) * 1000000;
                    if (ts.tv_nsec > 1000000000)
                    {
                        ts.tv_nsec -= 1000000000;
                        ts.tv_sec += 1;
                    }

                    do
                    {
                        ret = sem_timedwait(&worker->queueSema_, &ts);
                    } while (ret != 0 && errno == EINTR);

                    if (ret != 0 && errno == ETIMEDOUT)
                    {
                        break;
                    }

                    ScpRequest *pRequest = 0;

                    pthread_mutex_lock(&worker->queueLock_);
                    if (!worker->queue_.empty())
                    {
                        pRequest = worker->queue_.front();
                        worker->queue_.pop();
                    }
                    pthread_mutex_unlock(&worker->queueLock_);

                    if (pRequest)
                    {
                        Trace.WriteInfo(TraceSource, L"SCPCopy", "ScpCopy worker to {0} is processing request: Src: {1}, Dest: {2}, Op: {3}",
                                        pRequest->peer_, pRequest->src_, pRequest->dest_,
                                        pRequest->write_ ? "Upload" : "Download");
                        if (pRequest->write_)
                        {
                            struct stat srcinfo;
                            if (stat(pRequest->src_.c_str(), &srcinfo) != 0)
                            {
                                Trace.WriteInfo(TraceSource, L"SCPCopy", "Failed to stat source file {0}", pRequest->src_);
                                pRequest->error_.Overwrite(ErrorCodeValue::FileNotFound);
                                pRequest->SetCompleted();
                                continue;
                            }

                            pRequest->szSrc_ = srcinfo.st_size;

                            int srcfile = open(pRequest->src_.c_str(), O_RDONLY | O_CLOEXEC);
                            if (srcfile < 0) {
                                Trace.WriteInfo(TraceSource, L"SCPCopy", "Failed to open source file {0}", pRequest->src_);
                                pRequest->error_.Overwrite(ErrorCodeValue::FileNotFound);
                                pRequest->SetCompleted();
                                continue;
                            }

                            string tmpdest = pRequest->dest_ + ScpTmpFileSuffix;
                            LIBSSH2_CHANNEL *channel = libssh2_scp_send(session, tmpdest.c_str(), srcinfo.st_mode & 0777, (unsigned long)srcinfo.st_size);
                            if (!channel)
                            {
                                close(srcfile);
                                Trace.WriteInfo(TraceSource, L"SCPCopy", "Failed to open scp channel for {0}. {1}", tmpdest, libssh2_session_last_errno(session));
                                pRequest->error_.Overwrite(static_cast<ErrorCodeValue::Enum>(HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND)));
                                pRequest->SetCompleted();
                                continue;
                            }

                            char buf[4096];
                            bool internal_failure = false;
                            size_t totalsize = 0;
                            do
                            {
                                ssize_t nread = read(srcfile, buf, sizeof(buf));
                                if (nread <= 0) {
                                    if (nread < 0 && errno == EINTR) continue;
                                    break;
                                }
                                char* ptr = buf;
                                do
                                {
                                    rc = libssh2_channel_write(channel, ptr, nread);
                                    if (rc < 0)
                                    {
                                        Trace.WriteInfo(TraceSource, L"SCPCopy", "Failed to write scp channel. {0}", libssh2_session_last_errno(session));
                                        pRequest->error_.Overwrite(ErrorCodeValue::SendFailed);
                                        internal_failure = true;
                                        break;
                                    }
                                    else
                                    {
                                        ptr += rc;
                                        nread -= rc;
                                        totalsize += rc;
                                        pRequest->szCopied_ += rc;
                                    }
                                } while (nread);
                            } while (!internal_failure);

                            if (srcfile >= 0)
                            {
                                close(srcfile);
                            }

                            libssh2_channel_send_eof(channel);
                            libssh2_channel_wait_eof(channel);
                            libssh2_channel_wait_closed(channel);
                            libssh2_channel_free(channel);

                            if (libssh2_sftp_rename(sftp_session, tmpdest.c_str(), pRequest->dest_.c_str()))
                            {
                                pRequest->error_.Overwrite(ErrorCode::FromErrno(libssh2_sftp_last_error(sftp_session)));
                                pRequest->SetCompleted();
                                continue;
                            }
                            pRequest->SetCompleted();
                        }

                        else // if (pRequest->write_)
                        {
                            string tmpdest = pRequest->dest_ + ScpTmpFileSuffix;
                            int destfile = open(tmpdest.c_str(), O_CREAT | O_WRONLY | O_TRUNC | O_CLOEXEC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                            if (destfile < 0) {
                                Trace.WriteInfo(TraceSource, L"SCPCopy", "Failed to create tmp target file {0}. {1}", tmpdest, errno);
                                pRequest->error_.Overwrite(ErrorCode::FromErrno(errno));
                                pRequest->SetCompleted();
                                continue;
                            }

                            struct stat srcinfo;
                            LIBSSH2_CHANNEL *channel = libssh2_scp_recv(session, pRequest->src_.c_str(), &srcinfo);

                            if (!channel)
                            {
                                close(destfile);
                                Trace.WriteInfo(TraceSource, L"SCPCopy", "Failed to open scp channel for {0}. {1}", pRequest->src_, libssh2_session_last_errno(session));
                                pRequest->error_.Overwrite(static_cast<ErrorCodeValue::Enum>(HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND)));
                                pRequest->SetCompleted();
                                continue;
                            }
                            pRequest->szSrc_ = srcinfo.st_size;

                            size_t got = 0;
                            char buf[4096];
                            while (got < srcinfo.st_size)
                            {
                                int amount = sizeof(buf);

                                if((srcinfo.st_size - got) < amount)
                                {
                                    amount = (int)(srcinfo.st_size - got);
                                }

                                rc = libssh2_channel_read(channel, buf, amount);

                                if(rc > 0)
                                {
                                    int remaining = rc;
                                    char* ptr = buf;
                                    do
                                    {
                                        ssize_t nwrite = write(destfile, ptr, remaining);
                                        if (nwrite < 0)
                                        {
                                            if (errno == EINTR) continue;
                                            Trace.WriteInfo(TraceSource, L"SCPCopy", "Failed to write file content");
                                            pRequest->error_.Overwrite(ErrorCode::FromErrno(errno));
                                            break;
                                        }
                                        else
                                        {
                                            remaining -= nwrite;
                                            ptr += nwrite;
                                            pRequest->szCopied_ += nwrite;
                                        }
                                    } while (remaining != 0);
                                }
                                else if(rc < 0)
                                {
                                    Trace.WriteInfo(TraceSource, L"SCPCopy", "Failed to read from scp channel {0}", libssh2_session_last_errno(session));
                                    pRequest->error_.Overwrite(ErrorCode::FromErrno(errno));
                                    break;
                                }
                                got += rc;
                            }

                            close(destfile);
                            if (rename(tmpdest.c_str(), pRequest->dest_.c_str()) != 0)
                            {
                                pRequest->error_.Overwrite(ErrorCode::FromErrno(errno));
                            }

                            libssh2_channel_send_eof(channel);
                            libssh2_channel_wait_eof(channel);
                            libssh2_channel_wait_closed(channel);
                            pRequest->SetCompleted();

                            libssh2_channel_free(channel);
                        }
                    }
                }

                ScpWorkerThreadCleanup(worker, session, sftp_session);
                close(sock);

                Trace.WriteInfo(TraceSource, L"SCPCopy", "ScpCopy worker to {0} Exited.", worker->peerAddr_);

                return 0;
            }
        };

    private:
        ScpSessionManagerSingleton()
        {
            pthread_mutex_init(&apiLock_, NULL);
        }

    public:
        static ScpSessionManagerSingleton* GetSingleton()
        {
            static ScpSessionManagerSingleton* pSingleton = new ScpSessionManagerSingleton();
            static int rc = libssh2_init(0);
            _ASSERTE(rc == 0);

            return pSingleton;
        }

        ScpRequest* QueueWork(const string & peer, const string & acct, const string &passwd, const string & src, const string & dest, bool write)
        {
            ErrorCode err = ErrorCodeValue::Success;
            int closed = false;

            ScpRequest *pRequest = new ScpRequest(peer, acct, passwd, src, dest, write);

            pthread_mutex_lock(&apiLock_);

            bool queued = false;
            string key = pRequest->peer_ + ":" + pRequest->acct_ + ":" + pRequest->passwd_;
            if (this->workers_.find(key) != this->workers_.end())
            {
                ScpWorker *pWorker = this->workers_[key];
                if ((pWorker->QueueWork(pRequest)).IsSuccess())
                {
                    queued = true;
                }
                else
                {
                    this->workers_.erase(key);
                    delete pWorker;
                }
            }

            if (!queued)
            {
                ScpWorker *pWorker = new ScpWorker(pRequest->peer_, pRequest->acct_, pRequest->passwd_);
                workers_[key] = pWorker;
                pWorker->QueueWork(pRequest);
                pWorker->Start();
            }

            pthread_mutex_unlock(&apiLock_);

            return pRequest;
        }

    private:
        pthread_mutex_t apiLock_;
        map<string, ScpWorker*> workers_;
    };

    ErrorCode File::Copy(const std::wstring& src, const std::wstring& dest, const std::wstring& acct, const std::wstring& pwd, bool overwrite)
    {
        bool isWrite = false;
        string peerA, acctA, pwdA;
        StringUtility::Utf16ToUtf8(acct, acctA);
        StringUtility::Utf16ToUtf8(pwd, pwdA);

        // for scp copy
        if(src.find(L":/")!=wstring::npos || dest.find(L":/")!=wstring::npos)
        {
            string srcA, destA;
            StringUtility::Utf16ToUtf8(src, srcA);
            StringUtility::Utf16ToUtf8(dest, destA);
            std::replace(srcA.begin(), srcA.end(), '\\', '/');
            std::replace(destA.begin(), destA.end(), '\\', '/');

            auto scpSeparatorPos = destA.find(":/");
            if(scpSeparatorPos == string::npos)
            {
                mkdir_r(destA);
            }
            else
            {
                peerA = destA.substr(0, scpSeparatorPos);
                destA = destA.substr(scpSeparatorPos + 1);
                isWrite = true;
            }

            if (!isWrite)
            {
                scpSeparatorPos = srcA.find(":/");
                if(scpSeparatorPos != string::npos)
                {
                    peerA = srcA.substr(0, scpSeparatorPos);
                    srcA = srcA.substr(scpSeparatorPos + 1);
                }
            }

            ScpSessionManagerSingleton *pScpSessionManager = ScpSessionManagerSingleton::GetSingleton();

            ScpSessionManagerSingleton::ScpRequest * pRequest = pScpSessionManager->QueueWork(peerA, acctA, pwdA, srcA, destA, isWrite);
            pRequest->WaitForCompletion();
            ErrorCode err = pRequest->GetError();

            Trace.WriteInfo(TraceSource, L"SCPCopy", "Finish ScpCopy: src: {0}, dest: {1}, account: {2}, Ret: {3}, SrcSize: {4}, CopiedSize: {5}, Session Id: {6}. ",
                            src, dest, acct, pRequest->GetError(), pRequest->GetSrcSize(), pRequest->GetCopiedSize(), (int) syscall(__NR_gettid));

            delete pRequest;

            return err;
        }
        else
        {
            return ErrorCodeValue::OperationFailed;
        }
    }
#endif

    ErrorCode File::Copy(const std::wstring & src, const std::wstring & dest, bool overwrite)
    {
        BOOL result = ::CopyFileW(Path::ConvertToNtPath(src).c_str(), Path::ConvertToNtPath(dest).c_str(), !overwrite);
        if (result)
        {
            return ErrorCode::Success();
        }
        else
        {
            return ErrorCode::FromWin32Error(::GetLastError());
        }
    }

    ErrorCode File::MoveTransacted(const std::wstring& source, const std::wstring& destination, bool owerwrite)
    {
        bool isReadOnly = false;
#if defined(PLATFORM_UNIX)
        DWORD flag = 0;
#else
        DWORD flag = MOVEFILE_WRITE_THROUGH;
#endif
        DWORD fileAttributes = INVALID_FILE_ATTRIBUTES;
        std::wstring srcUncPath = Path::ConvertToNtPath(source);
        std::wstring destUncPath = Path::ConvertToNtPath(destination);
        if (owerwrite)
        {
            if (File::Exists(destination))
            {
                fileAttributes = ::GetFileAttributes(destUncPath.c_str());
                if (fileAttributes == INVALID_FILE_ATTRIBUTES)
                {
                    return ErrorCode::FromWin32Error(fileAttributes);
                }
                if (fileAttributes & FILE_ATTRIBUTE_READONLY)
                {
                    isReadOnly = true;
                    if (!::SetFileAttributes(destUncPath.c_str(), fileAttributes & !FILE_ATTRIBUTE_READONLY))
                    {
                        return ErrorCode::FromWin32Error(::GetLastError());
                    }
                }
                if (!File::Delete(destination, NOTHROW()))
                {
                    return ErrorCode::FromWin32Error(::GetLastError());
                }
            }

            flag = flag | MOVEFILE_REPLACE_EXISTING;
        }

        if (!::MoveFileExW(srcUncPath.c_str(), destUncPath.c_str(), flag))
        {
            return ErrorCode::FromWin32Error(::GetLastError());
        }

        if (isReadOnly)
        {
            if (!::SetFileAttributes(destUncPath.c_str(), fileAttributes | FILE_ATTRIBUTE_READONLY))
            {
                return ErrorCode::FromWin32Error(::GetLastError());
            }
        }
        return ErrorCodeValue::Success;
    }


    std::wstring File::GetTempFileName(std::wstring const& path)
    {
        std::wstring tempFileName;
        StringWriter writer(tempFileName);
        writer.Write(rand());
        writer.Write("-");
        writer.Write(GetCurrentThreadId());

        return Path::Combine(path, tempFileName);
    }

    ErrorCode File::SafeCopy(
        const std::wstring & src,
        const std::wstring & dest,
        bool overwrite,
        bool shouldAcquireLock,
        Common::TimeSpan const timeout)
    {
        std::wstring path = Path::GetDirectoryName(dest);
        path = path.empty() ? Directory::GetCurrentDirectory() : path;
        if (!path.empty())
        {
            ErrorCode err = Directory::Create2(path);
            if (!err.IsSuccess())
            {
                Trace.WriteWarning(
                    TraceSource,
                    L"SafeCopy",
                    "Failed to create directory. Path:{0}, Error:{1}",
                    path,
                    err);
                return err;
            }
        }
        else
        {
            path = L".";
        }

        std::wstring tempFilePath = GetTempFileName(path);

        Trace.WriteNoise(
            TraceSource,
            L"SafeCopy",
            "Doing Safecopy from {0} to temp file {1} {2} overwrite. shouldAcquireLock: {3}",
            src,
            tempFilePath,
            overwrite ? L"with" : L"without",
            shouldAcquireLock);

        FileReaderLock srcLock(src);
        FileWriterLock cacheLock(tempFilePath), destLock(dest);
        if(shouldAcquireLock)
        {
            ErrorCode error = srcLock.Acquire(timeout);
            if(!error.IsSuccess())
            {
                return error;
            }

            error = cacheLock.Acquire(timeout);
            if(!error.IsSuccess())
            {
                return error;
            }

            error = destLock.Acquire(timeout);
            if(!error.IsSuccess())
            {
                return error;
            }
        }

        ErrorCode error = File::Copy(src, tempFilePath, overwrite);
        if (!error.IsSuccess())
        {
            auto err = File::Delete2(tempFilePath);
            if (!err.IsSuccess())
            {
                Trace.WriteInfo(
                    TraceSource,
                    L"SafeCopy",
                    "SafeCopy failed to clean up '{0}' after copy from '{1}' failed. Error: {2}",
                    tempFilePath,
                    src,
                    err);
            }

            return error;
        }

        error = File::MoveTransacted(tempFilePath, dest, overwrite);
        if (!error.IsSuccess())
        {
            auto err = File::Delete2(tempFilePath);
            if (!err.IsSuccess())
            {
                Trace.WriteInfo(
                    TraceSource,
                    L"SafeCopy",
                    "SafeCopy failed to clean up '{0}' after move to '{1}' failed. Error: {2}",
                    tempFilePath,
                    dest,
                    err);
            }
        }

        return error;
    }

    ErrorCode File::Echo(const std::wstring & src, const std::wstring & dest, Common::TimeSpan const timeout)
    {
        if (!File::Exists(dest))
        {
            Trace.WriteInfo(TraceSource, L"Echo", "Destination not found SafeCopy Started file {0} to {1}", src, dest);
            return File::SafeCopy(src, dest, true, true, timeout);
        }
        return ErrorCodeValue::Success;
    }   

    string File::GetFileAccessDiagnostics(string const & path)
    {
#ifdef PLATFORM_UNIX
        string str;
        StringWriterA sw(str);
        auto fsuid = setfsuid(-1);
        sw.Write("fsuid={0}", fsuid);

        passwd pwd = {};
        if (SecurityUtility::GetPwByUid(fsuid, pwd) == 0)
        {
            sw.Write("/{0}", pwd.pw_name);
        }

        sw.Write(",");

        auto fsgid = setfsgid(-1);
        sw.Write("fsgid={0}", fsgid);

        group grp = {};
        if (SecurityUtility::GetGrByGid(fsgid, grp) == 0)
        {
            sw.Write("/{0}", grp.gr_name);
        }

        gid_t* groups = nullptr; 
        auto groupCount = getgroups(0, groups);
        if (groupCount > 0)
        {
            auto sgrBuf = (gid_t*)malloc(groupCount*sizeof(gid_t));
            if (sgrBuf)
            {
                if (getgroups(groupCount, sgrBuf) > 0)
                {
                    sw.Write(",groups:{{{0}", sgrBuf[0]);
                    if (SecurityUtility::GetGrByGid(sgrBuf[0], grp) == 0)
                    {
                        sw.Write("/{0}", grp.gr_name);
                    }

                    for (int i = 1; i < groupCount; ++i)
                    {
                        sw.Write(",{0}", sgrBuf[i]);
                        if (SecurityUtility::GetGrByGid(sgrBuf[i], grp) == 0)
                        {
                            sw.Write("/{0}", grp.gr_name);
                        }
                    }

                    sw.Write("}");
                }
            }

        }

        vector<string> facl;
        if (ProcessUtility::GetStatusOutput(formatString("getfacl {0}", path), facl).IsSuccess() && !facl.empty())
        {
            sw.Write(",facl={{{0}", facl[0]);
            for (int i = 1; i < facl.size(); ++i)
            {
                if (!facl[i].empty())
                {
                    sw.Write(", {0}", facl[i]);
                }
            }
            sw.Write("}");
        }

        return str;

#else

        path;
        return string();

#endif
    }
} // end namespace Common
