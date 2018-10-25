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
        // Wrapper over a SPtr that provides thread safe access to the pointer + debug info that is protected inside the SPtr
        //
        // When is this needed?
        //      1. A class has a SPtr member variable AND
        //      2. The SPtr is NOT a const AND
        //      3. The SPtr is re-assigned more than once after being constructed AND
        //      4. There can be racing reader threads that try to get a copy of the SPtr
        //
        // Usage:
        //      Put(__in_opt T *) -> Re-assigns the SPtr. nullptr is valid to reset the underlying SPtr
        //      T::SPtr Get() -> Provides a copy of the current SPtr
        //
        template<typename T>
        class ThreadSafeSPtrCache
            : KObject<ThreadSafeSPtrCache<T>>
        {
            K_DENY_COPY(ThreadSafeSPtrCache)

        public:
            
            //
            // Initialized with an optional value for the current pointer
            //
            ThreadSafeSPtrCache(__in_opt T * const current)
                : KObject<ThreadSafeSPtrCache<T>>()
                , current_(current)
                , lock_()
            {
            }

            ThreadSafeSPtrCache()
                : KObject<ThreadSafeSPtrCache<T>>()
                , current_(nullptr)
                , lock_()
            {
            }

            ~ThreadSafeSPtrCache()
            {
                current_ = nullptr;
            }
            
            //
            // Thread safe method to re-assign the shared pointer
            //
            void Put(__in_opt T * const newCurrent)
            {
                lock_.AcquireExclusive();
                current_ = newCurrent;
                lock_.ReleaseExclusive();
            }

            //
            // Moves the SPtr into the the current value
            //
            void Put(__in KSharedPtr<T> && newCurrent)
            {
                lock_.AcquireExclusive();
                current_ = Ktl::Move(newCurrent);
                lock_.ReleaseExclusive();
            }

            //
            // Moves the SPtr into the the current value if matches
            //
            bool PutIf(__in KSharedPtr<T> && newCurrent, __in KSharedPtr<T> &current)
            {
                bool changed = false;

                lock_.AcquireExclusive();

                if (current_ == current)
                {
                    current_ = Ktl::Move(newCurrent);
                    changed = true;
                }

                lock_.ReleaseExclusive();

                return changed;
            }

            //
            // Moves the SPtr into the the current value if matches
            //
            bool PutIf(__in KSharedPtr<T> && newCurrent, __in T *current)
            {
                bool changed = false;

                lock_.AcquireExclusive();

                if (current_ == current)
                {
                    current_ = Ktl::Move(newCurrent);
                    changed = true;
                }

                lock_.ReleaseExclusive();

                return changed;
            }

            //
            // Thread safe method to read the shared pointer
            //
            KSharedPtr<T> Get() const
            {
                KSharedPtr<T> result = nullptr;

                {
                    lock_.AcquireShared();
                    result = current_;
                    lock_.ReleaseShared();
                }

                return result;
            }

        private:
            KSharedPtr<T> current_;
            mutable KReaderWriterSpinLock lock_;
        };

        //
        // Same as above, except this is for protecting access to CSPtr rather than just a SPtr
        //
        template<typename T>
        class ThreadSafeCSPtrCache
            : KObject<ThreadSafeCSPtrCache<T>>
        {
            K_DENY_COPY(ThreadSafeCSPtrCache)

        public:
            
            ThreadSafeCSPtrCache(__in_opt T const * const current)
                : KObject<ThreadSafeCSPtrCache<T>>()
                , current_(current)
                , lock_()
            {
            }

            ~ThreadSafeCSPtrCache()
            {
                current_ = nullptr;
            }
            
            void Put(__in_opt T const * const newCurrent)
            {
                lock_.AcquireExclusive();
                current_ = newCurrent;
                lock_.ReleaseExclusive();
            }

            void Put(__in KSharedPtr<const T> && newCurrent)
            {
                lock_.AcquireExclusive();
                current_ = Ktl::Move(newCurrent);
                lock_.ReleaseExclusive();
            }

            KSharedPtr<const T> Get() const
            {
                KSharedPtr<const T> result = nullptr;
                
                {
                    lock_.AcquireShared();
                    result = current_;
                    lock_.ReleaseShared();
                }

                return result;
            }

        private:
            KSharedPtr<const T> current_;
            mutable KReaderWriterSpinLock lock_;
        };
    }
}
