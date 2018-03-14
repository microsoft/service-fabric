// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Utilities
    {
        //
        // Provides an async write lock implementation with awaitable semantics that also supports timeouts
        //
        class AsyncLock
            : public KShared<AsyncLock>
            , public KObject<AsyncLock>
        {
            K_FORCE_SHARED(AsyncLock)

        public:

            static NTSTATUS Create(
                __in KAllocator& allocator,
                __in ULONG tag,
                __out AsyncLock::SPtr & result) noexcept;

            //
            // Returns true if the lock was acquired within the timeout
            //
            ktl::Awaitable<bool> AcquireAsync(__in Common::TimeSpan const & timeout) noexcept; 
            
            //
            // Releases the lock
            // The name is explicitly verbose because the KShared object that this class derives from also has a "Release" method for the SPtr ref counting
            //
            void ReleaseLock() noexcept;

        private:

            template <typename _AwrT>
            ktl::Task ToTaskAndRelease(_AwrT&& A)
            {
                KCoShared$ApiEntry();

                auto awaitable = Ktl::Move(A);
                co_await awaitable;
                lock_.Release(false);
            }

            ktl::KCoLock lock_;
        };
    }
}
