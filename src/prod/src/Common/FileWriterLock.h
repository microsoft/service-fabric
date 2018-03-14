// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
#ifdef PLATFORM_UNIX

    class FileWriterLock : public FileLock<false>
    {
    public:
        FileWriterLock(std::wstring const & path);
        ~FileWriterLock() override;

        ErrorCode Acquire(TimeSpan timeout = TimeSpan::Zero) override;
        bool Release() override;

        static std::wstring Test_GetWriteInProgressMarkerPath(std::wstring const & path);

    private:
        ErrorCode DeleteMarkerIfNeeded();

        bool shouldDeleteMarker_;
    };

#else

    using FileWriterLock =  FileLock<false>;

#endif
}
