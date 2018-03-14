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
            template<typename T>
            class LockedEntityPtrBase
            {
                DENY_COPY(LockedEntityPtrBase);

            public:
                typedef typename EntityTraits<T>::DataType DataType;
                typedef std::shared_ptr<DataType> DataSPtrType;
                typedef EntityState<T> EntityStateType;
                typedef EntityStateSnapshot<T> EntityStateSnapshotType;

            protected:
                LockedEntityPtrBase() : 
                    state_(nullptr)
                {}

                LockedEntityPtrBase(
                    EntityStateType & state,
                    LockType::Enum lockType) : 
                    state_(&state),
                    lockType_(lockType)
                {
                    stateSnapshot_ = state.AcquireLock(lockType);
                }

                LockedEntityPtrBase(LockedEntityPtrBase && other) :
                    stateSnapshot_(std::move(other.stateSnapshot_)),
                    lockType_(std::move(other.lockType_)),
                    state_(std::move(other.state_))
                {
                    other.state_ = nullptr;
                }

            public:
                virtual ~LockedEntityPtrBase()
                {
                    if (!IsValid)
                    {
                        return;
                    }

                    state_->ReleaseLock(lockType_);
                }

                __declspec(property(get = get_IsValid)) bool IsValid;
                bool get_IsValid() const { return state_ != nullptr; }

                __declspec(property(get = get_IsEntityDeleted)) bool IsEntityDeleted;
                bool get_IsEntityDeleted() const
                {
                    AssertIsValid();
                    return stateSnapshot_.IsDeleted;
                }

                __declspec(property(get = get_Current)) DataType * Current;
                virtual DataType const * get_Current() const
                {
                    return get();
                }

            protected:
                void AssertIsValid() const { ASSERT_IF(!IsValid, "LockedPtr is invalid"); }
                void AssertIsNotValid() const { ASSERT_IF(IsValid, "LockedPtr is valid"); }

                DataType const * get() const
                {
                    AssertIsValid();
                    return stateSnapshot_.Data.get();
                }

                DataType * get()
                {
                    AssertIsValid();
                    return stateSnapshot_.Data.get();
                }

                EntityStateSnapshotType stateSnapshot_;
                EntityStateType * state_;
                LockType::Enum lockType_;
            };
        }
    }
}
