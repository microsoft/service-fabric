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
            class ReadOnlyLockedEntityPtr : public LockedEntityPtrBase<T>
            {
                DENY_COPY(ReadOnlyLockedEntityPtr);

            public:
                typedef EntityState<T> EntityStateType;
                typedef typename EntityTraits<T>::DataType DataType;

                explicit ReadOnlyLockedEntityPtr(EntityStateType & state)
                    : LockedEntityPtrBase<T>(state, LockType::Read)
                {
                }

                ReadOnlyLockedEntityPtr(ReadOnlyLockedEntityPtr && other) :
                    LockedEntityPtrBase<T>(std::move(other))
                {
                }

                ReadOnlyLockedEntityPtr & operator=(ReadOnlyLockedEntityPtr && other) 
                {
                    if (this != &other)
                    {
                        this->state_ = std::move(other.state_);
                        this->stateSnapshot_ = std::move(other.stateSnapshot_);
                        this->lockType_ = std::move(other.lockType_);
                        other.state_ = nullptr;
                    }

                    return *this;
                }

                DataType const* operator->() const { return this->get(); }
                DataType const& operator*() const { return *this->get(); }

                explicit operator bool() const
                {
                    return (this->get() != nullptr);
                }
            };
        }
    }
}

