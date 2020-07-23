// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TracePoints
{
    class AcquireWriteLock : private DenyCopy
    {
    public:
        AcquireWriteLock(ReaderWriterLockSlim & lock)
            : lock_(lock)
        {
            lock_.AcquireExclusive();
        }

        ~AcquireWriteLock()
        {
            lock_.ReleaseExclusive();
        }

    private:
        ReaderWriterLockSlim & lock_;
    };
}
