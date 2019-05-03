// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TracePoints
{
    class ReaderWriterLockSlim : private DenyCopy
    {
    public:
        ReaderWriterLockSlim()
            : lock_()
        {
            InitializeSRWLock(&lock_);
        }

        ~ReaderWriterLockSlim()
        {
        }

        void AcquireShared()
        {
            AcquireSRWLockShared(&lock_);
        }

        void ReleaseShared()
        {
            ReleaseSRWLockShared(&lock_);
        }

        void AcquireExclusive()
        {
            AcquireSRWLockExclusive(&lock_);
        }

        void ReleaseExclusive()
        {
            ReleaseSRWLockExclusive(&lock_);
        }

    private:
        SRWLOCK lock_;
    };
}
