// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
#ifdef PLATFORM_UNIX

    using ExclusiveFile = FileLock<false>;

#else

    class ExclusiveFile : TextTraceComponent<TraceTaskCodes::Common>
    {
        DENY_COPY(ExclusiveFile);

    public:
        ExclusiveFile(std::wstring const & path);
        ~ExclusiveFile();

        ErrorCode Acquire(TimeSpan timeout);
        bool Release();

    private:
        std::wstring const id_;
        std::wstring path_;
        std::unique_ptr<File> file_;
    };

#endif
}
