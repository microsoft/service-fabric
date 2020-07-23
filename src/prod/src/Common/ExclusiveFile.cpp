// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Common
{
    static Common::StringLiteral const ExclusiveFileSource = "ExclusiveFile";

    ExclusiveFile::ExclusiveFile(std::wstring const & path)
        : id_(wformatString("{0}", TextTraceThis)), path_(path), file_(nullptr)
    {
    }

    ExclusiveFile::~ExclusiveFile()
    {
        Release();
    }

    ErrorCode ExclusiveFile::Acquire(TimeSpan timeout)
    {
        std::wstring machineName;
        Environment::GetMachineName(machineName);

        std::wstring ownerInfo;
        StringWriter(ownerInfo).Write("machine={0},pid={1}", machineName, ProcessUtility::GetProcessId());

        TimeoutHelper timeoutHelper(timeout);

        std::unique_ptr<File> file = Common::make_unique<File>();
        ErrorCode error;

        do
        {
            error = file->TryOpen(
                path_,
                FileMode::OpenOrCreate,
                FileAccess::ReadWrite,
                FileShare::Read,
                FileAttributes::None);

            if (!timeoutHelper.IsExpired && !error.IsSuccess())
            {
                Sleep(100);
            }
        }
        while (!timeoutHelper.IsExpired && !error.IsSuccess());

        if (!error.IsSuccess())
        {
            WriteWarning(ExclusiveFileSource, id_, "Failed to create exclusive file {0}, error = {1}.", path_, error);
            return ErrorCodeValue::Timeout;
        }

        // At this point we have the lock.  Any subsequent failures do not affect correctness of the lock
        // and therefore should not be reported to the caller.

        try
        {
            file->Write(ownerInfo.data(), static_cast<int>(ownerInfo.size() * sizeof(wchar_t)));
        }
        catch (std::system_error const &)
        {
            WriteWarning(ExclusiveFileSource, id_, "Failed to write machine info to lock file {0}", path_);
        }

        WriteInfo(ExclusiveFileSource, id_, "Obtained exclusive file {0}", path_);
        file_ = std::move(file);
        return ErrorCodeValue::Success;
    }

    bool ExclusiveFile::Release()
    {
        if (!file_)
        {
            return false;
        }

        file_.reset();
        File::Delete(path_, NOTHROW());
        WriteInfo(ExclusiveFileSource, id_, "Released exclusive file {0}", path_);

        return true;
    }
}

