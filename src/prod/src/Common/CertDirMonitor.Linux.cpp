// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/mman.h>

using namespace Common;
using namespace std;

static StringLiteral const CertDirMonitorTrace("CertDirMonitor");

INIT_ONCE CertDirMonitor::initOnce_ = INIT_ONCE_STATIC_INIT;
Global<CertDirMonitor> CertDirMonitor::singleton_;

BOOL CertDirMonitor::InitFunction(PINIT_ONCE, PVOID, PVOID *)
{
    singleton_ = make_global<CertDirMonitor>();
    return TRUE;
}

CertDirMonitor & CertDirMonitor::GetSingleton()
{
    PVOID lpContext = NULL;
    Invariant(::InitOnceExecuteOnce(&initOnce_, InitFunction, NULL, &lpContext));
    return *singleton_;
}

const wchar_t  *CertDirMonitor::CertFileExtsW[] = { L"pem", L"PEM", L"crt", L"prv", L"pfx" };
const char     *CertDirMonitor::CertFileExtsA[] = {  "pem",  "PEM",  "crt",  "prv", "pfx" };

const int CertDirMonitor::NrCertFileExts = (sizeof(CertDirMonitor::CertFileExtsW) / sizeof((CertDirMonitor::CertFileExtsW)[0]));

CertDirMonitor::CertDirMonitor() : inotifyFd_(inotify_init1(IN_CLOEXEC)), wd_(-1)
{
    ASSERT_IF(inotifyFd_ < 0, "inotify_init failed: {0}", ErrorCode::FromErrno());
    WriteInfo(CertDirMonitorTrace, "inotify_init created descriptor {0}", inotifyFd_);

    bzero(&aiocb_, sizeof(aiocb_));
    aiocb_.aio_fildes = inotifyFd_;
    aiocb_.aio_sigevent.sigev_notify = SIGEV_THREAD;
    aiocb_.aio_sigevent.sigev_notify_function = OnAioComplete;
    aiocb_.aio_sigevent.sigev_value.sival_ptr = this;

    eventBuffer_.resize(1024 * (sizeof(inotify_event) + 16));
    aiocb_.aio_buf = eventBuffer_.data();
    aiocb_.aio_nbytes = eventBuffer_.size();

    WaitForNextChange();
}

bool CertDirMonitor::IsCertFile(const string &fileName)
{
    const char *ext = strrchr(fileName.c_str(), '.');
    if (!ext) return false;

    ext++;
    for (int i = 0; i < CertDirMonitor::NrCertFileExts; i++)
    {
        if (strcmp(ext, CertFileExtsA[i]) == 0) return true;
    }
    return false;
}

static ErrorCode SetDefaultMode(const char* filepath)
{
    ErrorCode error;
    struct stat filestat;

    if (0 != stat(filepath, &filestat))
    {
        error = ErrorCodeValue::FileNotFound;
    }
    else
    {
        const char *ext = strrchr(filepath, '.');
        if (!ext) return ErrorCodeValue::UnspecifiedError;
        ext++;

        bool IsCrtFile = (strcmp(ext, "crt") == 0);
        filestat.st_mode &= ~(S_IROTH | S_IWOTH);
        if (IsCrtFile)
        {
            filestat.st_mode |= S_IROTH;
        }

        if (0 != chmod(filepath, filestat.st_mode))
        {
            error = ErrorCodeValue::AccessDenied;
        }
    }
    return error;
}

static bool CompareFileContent(const string &fn1, const string &fn2)
{
    bool res = false;
    int fd1 = -1, fd2 = -1;
    char *p1 = 0, *p2 = 0;
    struct stat st1, st2;
    if (stat(fn1.c_str(), &st1) || stat(fn2.c_str(), &st2)
             || !S_ISREG (st1.st_mode) || !S_ISREG (st1.st_mode) || st1.st_size != st2.st_size )
    {
        return false;
    }
    int sz = (int)st1.st_size;
    do
    {
        if (-1 == (fd1 = open(fn1.c_str(), O_RDONLY))) break;
        if (-1 == (fd2 = open(fn2.c_str(), O_RDONLY))) break;
        if (MAP_FAILED == (p1 = (char*)mmap(0, sz, PROT_READ, MAP_SHARED, fd1, 0))) break;
        if (MAP_FAILED == (p2 = (char*)mmap(0, sz, PROT_READ, MAP_SHARED, fd2, 0))) break;
        if (memcmp(p1, p2, sz) == 0) res = true;
    } while(0);

    if (p1 != MAP_FAILED) munmap(p1, sz);
    if (p2 != MAP_FAILED) munmap(p2, sz);
    if (fd1 != -1) close(fd1);
    if (fd2 != -1) close(fd2);
    return res;
}

ErrorCode CertDirMonitor::StartWatch(const string &srcPath, const string &destPath)
{
    ErrorCode error;

    srcPath_ = StringUtility::Utf8ToUtf16(srcPath);
    destPath_ = StringUtility::Utf8ToUtf16(destPath);

    if (!Directory::Exists(srcPath))
    {
        return ErrorCodeValue::DirectoryNotFound;
    }
    if (!Directory::Exists(destPath))
    {
        error = Directory::Create2(destPath);
        if (!error.IsSuccess()) return error;
    }

    for (int i = 0; i < CertDirMonitor::NrCertFileExts; i++)
    {
        auto search = File::Search(srcPath_ + L"/*." + CertFileExtsW[i]);
        while ((search->MoveNext()).IsSuccess())
        {
            wstring filename((wchar_t *)(search->GetCurrent().cFileName));

            //skip copying when dest file exists with same content
            string srcFilePath = StringUtility::Utf16ToUtf8(Path::Combine(srcPath_, filename));
            string destFilePath = StringUtility::Utf16ToUtf8(Path::Combine(destPath_, filename));
            if (CompareFileContent(srcFilePath, destFilePath)) continue;

            error = File::Copy(Path::Combine(srcPath_, filename), Path::Combine(destPath_, filename), true);
            if (!error.IsSuccess())
            {
                WriteWarning(CertDirMonitorTrace, "InitialCopy: failed to copy {0} from {1} to {2}",
                             srcPath_, destPath_, filename);
            }
            else
            {
                string destPathA = StringUtility::Utf16ToUtf8(Path::Combine(destPath_, filename));
                SetDefaultMode(destPathA.c_str());
            }
        }
    }

    auto mask = IN_CLOSE_WRITE | IN_MOVED_TO | IN_DELETE | IN_MOVED_FROM;
    wd_ = inotify_add_watch(inotifyFd_, srcPath.c_str(), mask);
    if (wd_ < 0)
    {
        error = ErrorCode::FromErrno();
        WriteError(CertDirMonitorTrace, "inotify_add_watch failed: {0}", error);
        return error;
    }
    return error;
}

ErrorCode CertDirMonitor::StopWatch()
{
    if ((inotifyFd_ >= 0) && (wd_ >= 0))
    {
        inotify_rm_watch(inotifyFd_, wd_);
        wd_ = -1;
        srcPath_.clear();
        destPath_.clear();
    }
}

void CertDirMonitor::WaitForNextChange()
{
    if (aio_read(&aiocb_) == 0) return;

    auto error = ErrorCode::FromErrno();
    WriteError(CertDirMonitorTrace, "aio_read({0}) failed: {1}", inotifyFd_, error);
    RetryWithDelay();
}

void CertDirMonitor::RetryWithDelay()
{
    Threadpool::Post([this] { WaitForNextChange(); }, TimeSpan::FromMilliseconds(Random().Next(5000)));
}

void CertDirMonitor::OnAioComplete(sigval_t sigval)
{
    auto thisPtr = (CertDirMonitor*)(sigval.sival_ptr);
    thisPtr->ProcessNotifications();
}

void CertDirMonitor::ProcessNotifications()
{
    ErrorCode error;

    auto aioError = aio_error(&aiocb_);
    if (aioError != 0)
    {
        error = ErrorCode::FromErrno(aioError);
        WriteError(CertDirMonitorTrace, "ProcessNotifications: aio_error returned {0}", aioError);

        if (aioError == ECANCELED) return;

        RetryWithDelay();
        return;
    }

    auto resultSize = aio_return(&aiocb_);
    int processed = 0;
    int count = 0;

    while (processed < resultSize)
    {
        inotify_event const * event = (inotify_event const*)(eventBuffer_.data() + processed);
        processed += (sizeof(inotify_event) + event->len);
        ++count;
        WriteInfo(CertDirMonitorTrace,
                  "ProcessNotifications: event retrieved: wd = {0}, mask = {1:x}, name = '{2}', count = {3}",
                  event->wd, event->mask, (event->len > 0) ? string(event->name) : "", count);

        if ((event->len > 0) && IsCertFile(string(event->name)))
        {
            if (event->mask & (IN_CLOSE_WRITE | IN_MOVED_TO))
            {
                wstring srcFilePathW = Path::Combine(srcPath_, StringUtility::Utf8ToUtf16(event->name));
                wstring dstFilePathW = Path::Combine(destPath_, StringUtility::Utf8ToUtf16(event->name));
                if (CompareFileContent(StringUtility::Utf16ToUtf8(srcFilePathW), StringUtility::Utf16ToUtf8(dstFilePathW)))
                {
                    WriteInfo(CertDirMonitorTrace, "ProcessNotifications: Skip copying {0} to {1} since content is same.",
			      srcFilePathW, dstFilePathW);
                    continue;
                }
                error = File::Copy(srcFilePathW, dstFilePathW);
                if (!error.IsSuccess())
                {
                    WriteWarning(CertDirMonitorTrace, "ProcessNotifications: failed to copy {0} to {1}",
                                 srcFilePathW, dstFilePathW);
                }
                string destPathA = StringUtility::Utf16ToUtf8(dstFilePathW);
                SetDefaultMode(destPathA.c_str());
            }
            else if (event->mask & (IN_DELETE|IN_MOVED_FROM))
            {
                wstring dstFilePathW = Path::Combine(destPath_, StringUtility::Utf8ToUtf16(event->name));
                File::Delete(dstFilePathW, false);
            }
        }
    }

    WaitForNextChange();
}

