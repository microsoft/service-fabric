// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TracePoints
{
    class AcquireReadLock : private DenyCopy
    {
    public:
        AcquireReadLock(ReaderWriterLockSlim & lock)
            : lock_(lock)
        {
            lock_.AcquireShared();
        }

        ~AcquireReadLock()
        {
            lock_.ReleaseShared();
        }

    private:
        ReaderWriterLockSlim & lock_;
    };
}
