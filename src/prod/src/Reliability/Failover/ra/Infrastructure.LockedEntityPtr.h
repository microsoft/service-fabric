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
            class LockedEntityPtr : public LockedEntityPtrBase<T>
            {
                DENY_COPY(LockedEntityPtr);
        
                class UpdateContextImpl : public UpdateContext
                {
                    DENY_COPY(UpdateContextImpl);

                public:
                    UpdateContextImpl(LockedEntityPtr & ptr) : ptr_(ptr)
                    {
                    }

                    void EnableUpdate()
                    {
                        ptr_.EnableUpdate();
                    }

                    void EnableInMemoryUpdate()
                    {
                        ptr_.EnableInMemoryUpdate();
                    }

                private:
                    LockedEntityPtr & ptr_;
                };

            public:
                typedef typename EntityTraits<T>::DataType DataType;
                typedef EntityState<T> EntityStateType;
                typedef std::shared_ptr<DataType> DataSPtrType;
                typedef EntityStateSnapshot<T> EntityStateSnapshotType;
                typedef EntityMap<T> EntityMapType;
                typedef CommitDescription<T> CommitDescriptionType;

                LockedEntityPtr() : 
                    LockedEntityPtrBase<T>(), 
                    isUpdating_(false),
                    isInMemoryOperation_(false),
                    wasInitiallyNull_(false),
                    operationType_(Storage::Api::OperationType::Delete),
                    updateContext_(*this)
                {}

                explicit LockedEntityPtr(EntityStateType & state) : 
                    LockedEntityPtrBase<T>(state, LockType::Write),
                    isUpdating_(false),
                    isInMemoryOperation_(false),
                    operationType_(Storage::Api::OperationType::Delete),
                    updateContext_(*this)
                {
                    // state snapshot is only valid after base class constructor has run
                    wasInitiallyNull_ = this->stateSnapshot_.Data.get() == nullptr;
                }

                LockedEntityPtr(LockedEntityPtr && other) :
                    LockedEntityPtrBase<T>(std::move(other)),
                    isUpdating_(std::move(other.isUpdating_)),
                    isInMemoryOperation_(std::move(other.isInMemoryOperation_)),
                    wasInitiallyNull_(std::move(other.wasInitiallyNull_)),
                    operationType_(std::move(other.operationType_)),
                    updateContext_(*this)
                {
                }

                LockedEntityPtr & operator =(LockedEntityPtr && other)
                {
                    // Currently assigning to a locked ft ptr that is valid is not supported
                    this->AssertIsNotValid();

                    // Currently assigning from an invalid locked ft ptr is not supported
                    other.AssertIsValid();

                    if (this != &other)
                    {
                        this->state_ = std::move(other.state_);
                        isUpdating_ = std::move(other.isUpdating_);
                        operationType_ = std::move(other.operationType_);
                        this->stateSnapshot_ = std::move(other.stateSnapshot_);
                        this->lockType_ = std::move(other.lockType_);
                        wasInitiallyNull_ = std::move(other.wasInitiallyNull_);
                        isInMemoryOperation_ = std::move(other.isInMemoryOperation_);
                        other.state_ = nullptr;
                    }

                    return *this;
                }

                __declspec(property(get = get_UpdateContextObj)) UpdateContext & UpdateContextObj;
                UpdateContext & get_UpdateContextObj() 
                {
                    return updateContext_;
                }

                __declspec(property(get = get_Current)) DataType * Current;
                DataType const * get_Current() const
                {
                    return this->get();
                }

                DataType * get_Current() 
                { 
                    return this->get();
                }

                __declspec(property(get = get_IsUpdating)) bool IsUpdating;
                bool get_IsUpdating() const 
                {
                    this->AssertIsValid();
                    return isUpdating_; 
                }

                void EnableUpdate()
                {
                    Update(false);
                }

                void EnableInMemoryUpdate()
                {
                    Update(true);
                }
                    
                void MarkForDelete()
                {
                    this->AssertIsValid();
                    AssertEntryNotDeleted();
                    AssertIsNotDeleting();                    

                    isUpdating_ = true;
                    isInMemoryOperation_ = wasInitiallyNull_;
                    operationType_ = Storage::Api::OperationType::Delete;
                    this->stateSnapshot_.Delete();
                }

                void Insert(DataSPtrType && data)
                {
                    this->AssertIsValid();
                    AssertEntryNotDeleted();
                    AssertIsNotUpdating();
                    AssertEntryDoesNotHaveData();                    

                    isUpdating_ = true;
                    operationType_ = Storage::Api::OperationType::Insert;
                    this->stateSnapshot_.SetData(std::move(data));
                }

                DataType * operator->() { return this->get(); }
                DataType & operator*() { return *this->get(); }
                DataType const* operator->() const { return this->get(); }
                DataType const& operator*() const { return *this->get(); }

                explicit operator bool() const
                {
                    return (this->get() != nullptr);
                }

                CommitDescriptionType CreateCommitDescription() const
                {
                    AssertIsUpdating();
                    CommitDescriptionType commitDescription;
                    commitDescription.Type.StoreOperationType = operationType_;
                    commitDescription.Type.IsInMemoryOperation = isInMemoryOperation_;
                    commitDescription.Data = operationType_ == Storage::Api::OperationType::Delete ? nullptr : this->stateSnapshot_.Data;
                    return commitDescription;
                }

            private:
                void Update(bool inMemory)
                {
                    this->AssertIsValid();
                    AssertEntryNotDeleted();
                    AssertIsNotDeleting();

                    /*
                        The semantics of enable update allow for multiple callers to call this
                        EnableUpdate can also be called after Insert

                        UpdateInMemory must mark the entity as updating only in memory. 

                        Update after UpdateInMemory must clear the in memory only flag

                        UpdateInMemory after update must not clear the in memory flag as persistence has been previously requested
                    */
                    if (isUpdating_ && !isInMemoryOperation_)
                    {
                        return;
                    }

                    AssertEntryHasData();

                    isInMemoryOperation_ = inMemory;
                    isUpdating_ = true;
                    operationType_ = Storage::Api::OperationType::Update;
                }

                void AssertIsUpdating() const { ASSERT_IF(!isUpdating_, "Expected locked ft to be updating"); }
                void AssertIsNotUpdating() const { ASSERT_IF(isUpdating_, "Expected lockedFT to not be updating"); }
                void AssertEntryHasData() const { ASSERT_IF(this->get() == nullptr, "Entry must have data"); }
                void AssertEntryDoesNotHaveData() const { ASSERT_IF(this->get() != nullptr, "Entry must not have data"); }
                void AssertEntryNotDeleted() const { ASSERT_IF(this->stateSnapshot_.IsDeleted, "Entry cannot be deleted"); }
                void AssertIsNotDeleting() const { ASSERT_IF(isUpdating_ && operationType_ == Storage::Api::OperationType::Delete, "Entity is already deleting"); }

                bool isUpdating_;   
                bool isInMemoryOperation_;
                bool wasInitiallyNull_;
                    
                // operationType_ is valid only if isUpdating_ = true
                Storage::Api::OperationType::Enum operationType_;

                UpdateContextImpl updateContext_;
            };
        }
    }
}
