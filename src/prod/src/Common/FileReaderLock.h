// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
#ifdef PLATFORM_UNIX

    class FileReaderLock : public FileLock<true>
    {
    public:
        FileReaderLock(std::wstring const & path);

        ErrorCode Acquire(TimeSpan timeout = TimeSpan::Zero) override;
    };

#else

    using FileReaderLock =  FileLock<true>;

#endif
}
