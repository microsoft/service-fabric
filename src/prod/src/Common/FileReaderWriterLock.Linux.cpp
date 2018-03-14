// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

namespace
{
    const std::wstring LockExtension = L".RWLock";
    const std::wstring WriteOngoingMarkerExtension = L".WriteInProgress";

    const StringLiteral RTrace = "FileReaderLock";
    const StringLiteral WTrace = "FileWriterLock";

    wstring LockPath(wstring const & path)
    {
        auto inputPath = path;
        if (Path::IsPathRooted(inputPath))
        {
            inputPath.erase(inputPath.cbegin());
        }

        auto lockPath = Path::Combine(Environment::GetObjectFolder(), L"file.lock");
        Path::CombineInPlace(lockPath, inputPath); 
        return lockPath + LockExtension;
    }

    wstring WriteOngoingMarkerPath(wstring const & path)
    {
        return path + WriteOngoingMarkerExtension;
    }

    wstring WriteOngoingMarkerPath(string const & path)
    {
        wstring pathw;
        StringUtility::Utf8ToUtf16(path, pathw);
        return WriteOngoingMarkerPath(pathw); 
    }
}

FileReaderLock::FileReaderLock(wstring const & path) : FileLock<true>(LockPath(path))
{
}

ErrorCode FileReaderLock::Acquire(TimeSpan timeout)
{
    auto err = FileLock<true>::Acquire(timeout);
    if (!err.IsSuccess()) return err;

    if (File::Exists(WriteOngoingMarkerPath(path_)))
    {
        WriteWarning(RTrace, "CorruptedImageStoreObjectFound found for path {0}", path_);
        return ErrorCodeValue::CorruptedImageStoreObjectFound;
    }

    return err;
}

FileWriterLock::FileWriterLock(wstring const & path) : FileLock<false>(LockPath(path)), shouldDeleteMarker_(false)
{
}

FileWriterLock::~FileWriterLock()
{
    DeleteMarkerIfNeeded();
}

ErrorCode FileWriterLock::Acquire(TimeSpan timeout)
{
    auto err = FileLock<false>::Acquire(timeout);
    if (!err.IsSuccess()) return err;

    File writerFile;
    err = writerFile.TryOpen(
        WriteOngoingMarkerPath(path_),
        FileMode::OpenOrCreate,
        FileAccess::Write,
        FileShare::None,
        FileAttributes::None);

    if (!err.IsSuccess())
    {
        WriteWarning(WTrace, "Failed to create write-ongoing marker file {0}", WriteOngoingMarkerPath(path_));
    }

    shouldDeleteMarker_ = true;
    return err;
}

ErrorCode FileWriterLock::DeleteMarkerIfNeeded()
{
    ErrorCode markerDeleted;
    if (shouldDeleteMarker_)
    {
        shouldDeleteMarker_ = false;
        markerDeleted = File::Delete2(WriteOngoingMarkerPath(path_));
        WriteTrace(
                markerDeleted.ToLogLevel(),
                WTrace,
                "Delete writer file {0}: {1}",
                WriteOngoingMarkerPath(path_),
                markerDeleted);
    }

    return markerDeleted;
}

bool FileWriterLock::Release()
{
    auto deletedMarker = DeleteMarkerIfNeeded();
    auto released = FileLock<false>::Release();
    return deletedMarker.IsSuccess() && released;
}

wstring FileWriterLock::Test_GetWriteInProgressMarkerPath(wstring const & path)
{
    return WriteOngoingMarkerPath(LockPath(path));
}
