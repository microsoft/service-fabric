// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Infrastructure
        {
            template <typename T>
            class EntityState
            {
            public:
                typedef typename EntityTraits<T>::DataType DataType;
                typedef std::shared_ptr<DataType> DataSPtrType;
                typedef EntityStateSnapshot<T> EntityStateSnapshotType;

                EntityState() : 
                    lockingThreadId_(0)
                {
                }

                explicit EntityState(DataSPtrType && data) : 
                    lockingThreadId_(0),
                    data_(std::move(data))
                {
                }

                __declspec(property(get = get_Test_Data)) EntityStateSnapshotType & Test_Data;
                EntityStateSnapshotType & get_Test_Data() { return data_; }

                __declspec(property(get = get_Test_IsWriteLockHeld)) bool Test_IsWriteLockHeld;
                bool get_Test_IsWriteLockHeld() const { return IsWriteLockHeld();}

                void SetData(DataSPtrType const & data)
                {
                    AssertWriteLockIsHeld();
                    
                    Common::AcquireWriteLock grab(lock_);
                    data_.SetData(data);
                }

                void Delete()
                {
                    AssertWriteLockIsHeld();

                    Common::AcquireWriteLock grab(lock_);
                    data_.Delete();
                }

                EntityStateSnapshotType AcquireLock(LockType::Enum lockType)
                {
                    switch (lockType)
                    {
                    case LockType::Read:
                        return AcquireReadLock();
                    case LockType::Write:
                        return AcquireWriteLock();
                    default:
                        Common::Assert::CodingError("unknown lock type {0}", static_cast<int>(lockType));
                    }
                }

                void ReleaseLock(LockType::Enum lockType)
                {
                    switch (lockType)
                    {
                    case LockType::Read:
                        return ReleaseReadLock();
                    case LockType::Write:
                        return ReleaseWriteLock();
                    default:
                        Common::Assert::CodingError("unknown lock type {0}", static_cast<int>(lockType));
                    }
                }
            private:
                void AssertWriteLockIsHeld() const { ASSERT_IF(!IsWriteLockHeld(), "Lock must be held"); }
                bool IsWriteLockHeld() const { return isLocked_.load(); }

                EntityStateSnapshotType AcquireWriteLock()
                {
                    /*
                        Guaranteed by the scheduler that you cannot acquire the lock twice
                    */
                    bool wasLocked = isLocked_.exchange(true);
                    DWORD currentThreadId = ::GetCurrentThreadId();
                    ASSERT_IF(wasLocked, "Entry is already locked. Lock was acquired by thread {0}. Current {1}", lockingThreadId_, currentThreadId);
                    lockingThreadId_ = currentThreadId;

                    EntityStateSnapshotType snap;
                    
                    /*
                        Acquiring a write lock clones the current state of the entity
                        This is the state that will be modified

                        There is no need to acquire a read or write lock here because the check above
                        guarantees that there is no other writer

                        Since there is no other writer then there can only be readers and no lock is needed
                    */
                    snap = data_;
                    return snap.Clone();
                }

                void ReleaseWriteLock()
                {
                    bool wasLocked = isLocked_.exchange(false);
                    ASSERT_IF(!wasLocked, "Releasing a write lock when entity was not locked");
                }

                EntityStateSnapshotType AcquireReadLock() const
                {
                    Common::AcquireReadLock grab(lock_);
                    return data_;
                }

                void ReleaseReadLock()
                {
                }

                mutable Common::RwLock lock_;
                EntityStateSnapshotType data_;
                DWORD lockingThreadId_;
                Common::atomic_bool isLocked_;
            };
        }
    }
}

