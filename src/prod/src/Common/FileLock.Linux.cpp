// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <sys/file.h>

using namespace Common;

static StringLiteral const TraceType = "FileLock";

template <bool IsReaderLock>
FileLock<IsReaderLock>::FileLock(std::wstring const & path)
    : id_(wformatString("{0}", TextTraceThis)), file_(-1)
{
    WriteNoise(TraceType, id_, "input path = {0}", path);
    auto folder = Path::GetDirectoryName(path);
    if (!Directory::Exists(folder))
    {
        Directory::Create2(folder);
    }

    StringUtility::Utf16ToUtf8(path, path_);
    file_ = open(
        path_.c_str(),
        O_CREAT|O_CLOEXEC | (IsReaderLock? O_RDONLY : O_WRONLY),
        S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);

    if (file_ < 0)
    {
        openStatus_ = ErrorCode::FromErrno();
        WriteError(TraceType, id_, "open({0}) failed: {1}. user id: {2}", path_, openStatus_, geteuid());
    }
}

template <bool IsReaderLock>
FileLock<IsReaderLock>::~FileLock()
{
    Release();
}

template <bool IsReaderLock>
ErrorCode FileLock<IsReaderLock>::Acquire(TimeSpan timeout)
{
    if(!openStatus_.IsSuccess())
    {
        return openStatus_;
    }

    TimeoutHelper timeoutHelper(timeout);
    auto retryCount = 0;
    auto retval = 0;
    do
    {
        retval = flock(file_, (IsReaderLock? LOCK_SH : LOCK_EX) | LOCK_NB);
        if (retval == 0)
        {
            WriteInfo(TraceType, id_, "acquired {0}, IsReaderLock = {1}", path_, IsReaderLock);
            return ErrorCode();
        }
        
         ++retryCount;
        //WriteNoise(TraceType, id_, "flock returned with error {0}", errno);
        if (errno == EINTR) continue;

        Invariant((errno == EWOULDBLOCK) || (errno == ENOLCK));
        Sleep(100);
    }while(!timeoutHelper.IsExpired);

    WriteNoise(TraceType, id_, "acquire lock failed {0}, IsReaderLock = {1} with errorCode={2}, retryCount={3}", path_, IsReaderLock, retval, retryCount);
    return ErrorCodeValue::Timeout;
}

template <bool IsReaderLock>
bool FileLock<IsReaderLock>::Release()
{
    if (file_ < 0)
    {
        return false;
    }

    close(file_);
    file_ = -1;
    WriteInfo(TraceType, id_, "closed {0}", path_);

    // LINUXTODO investigate how/when to delete the file
    // deleting file here will cause race conditions, e.g.
    // thread 1 and 2 call Acquire() on the same file at the same time,
    // both open the file, suppose thread 1 gets the lock, thread 2 will
    // hold the file descriptor and stay in the retry loop. when thread 1 
    // releases its lock, if it deletes the file, then thread 2's file 
    // descriptor keeps a ref count on the file and then acquires lock 
    // on the file already deleted from its parent dir. If a third thread
    // calls Acquire(), then a new file will be created as the old file is
    // already from the parent dir. Then third thread will call flock on 
    // the new file, thus conflicts with thread 2.
    return true;
}

namespace Common
{
    template class FileLock<true>;
    template class FileLock<false>;
}
