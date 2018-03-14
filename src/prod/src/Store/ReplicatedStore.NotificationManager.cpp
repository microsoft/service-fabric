// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

using namespace Store;

StringLiteral const TraceComponent("NotificationManager");

// *************
// Inner Classes
// *************

class ReplicatedStore::NotificationManager::StoreItemMetadata 
    : public ComponentRoot
    , public Api::IStoreItemMetadata
{
public:
    StoreItemMetadata(
        _int64 lsn,
        wstring && type,
        wstring && key,
        DateTime fileTime,
        DateTime fileTimeOnPrimary,
        size_t valueSize)
        : lsn_(lsn)
        , type_(forward<wstring>(type))
        , key_(forward<wstring>(key))
        , fileTime_(fileTime)
        , fileTimeOnPrimary_(fileTimeOnPrimary)
        , valueSize_(valueSize)
    {
    }

    _int64 GetOperationLSN() const { return lsn_; }
    wstring const & GetType() const { return type_; }
    wstring const & GetKey() const { return key_; }
    DateTime GetLastModifiedUtc() const { return fileTime_; }
    DateTime GetLastModifiedOnPrimaryUtc() const { return fileTimeOnPrimary_; }
    size_t GetValueSize() const { return valueSize_; }

    wstring TakeType() { return move(type_); }
    wstring TakeKey() { return move(key_); }

    Api::IKeyValueStoreItemMetadataResultPtr ToKeyValueItemMetadata()
    {
        auto sPtr = make_shared<KeyValueStoreItemMetadataResult>(
            key_,
            static_cast<LONG>(valueSize_),
            lsn_,
            fileTime_.AsFileTime,
            fileTimeOnPrimary_.AsFileTime);
        return Api::IKeyValueStoreItemMetadataResultPtr(sPtr.get(), sPtr->CreateComponentRoot());
    }

private:
    _int64 lsn_;
    wstring type_;
    wstring key_;
    DateTime fileTime_;
    DateTime fileTimeOnPrimary_;
    size_t valueSize_;
};

class ReplicatedStore::NotificationManager::StoreItem 
    : public ComponentRoot
    , public Api::IStoreItem
{
public:
    StoreItem(
        _int64 lsn,
        wstring && type,
        wstring && key,
        DateTime fileTime,
        DateTime fileTimeOnPrimary,
        vector<byte> && value,
        bool isDelete)
        : metadata_(make_shared<StoreItemMetadata>(
            lsn,
            forward<wstring>(type),
            forward<wstring>(key),
            fileTime,
            fileTimeOnPrimary,
            value.size()))
        , value_(forward<vector<byte>>(value))
        , isDelete_(isDelete)
    {
    }

    StoreItem(
        shared_ptr<StoreItemMetadata> const & metadata,
        vector<byte> && value,
        bool isDelete)
        : metadata_(metadata)
        , value_(forward<vector<byte>>(value))
        , isDelete_(isDelete)
    {
    }

    shared_ptr<StoreItemMetadata> const & GetMetadata() const { return metadata_; }

    _int64 GetOperationLSN() const { return metadata_->GetOperationLSN(); }
    wstring const & GetType() const { return metadata_->GetType(); }
    wstring const & GetKey() const { return metadata_->GetKey(); }
    DateTime GetLastModifiedUtc() const { return metadata_->GetLastModifiedUtc(); }
    DateTime GetLastModifiedOnPrimaryUtc() const { return metadata_->GetLastModifiedOnPrimaryUtc(); }
    size_t GetValueSize() const { return metadata_->GetValueSize(); }
    vector<byte> const & GetValue() const { return value_; }

    wstring TakeType() { return metadata_->TakeType(); }
    wstring TakeKey() { return metadata_->TakeKey(); }
    vector<byte> TakeValue() { return move(value_); }

    bool IsDelete() const { return isDelete_; }

    Api::IKeyValueStoreItemMetadataResultPtr ToKeyValueItemMetadata()
    {
        return metadata_->ToKeyValueItemMetadata();
    }

    Api::IKeyValueStoreItemResultPtr ToKeyValueItem()
    {
        auto sPtr = make_shared<KeyValueStoreItemResult>(
            this->GetKey(),
            move(value_),
            this->GetOperationLSN(),
            this->GetLastModifiedUtc().AsFileTime,
            this->GetLastModifiedOnPrimaryUtc().AsFileTime);
        return Api::IKeyValueStoreItemResultPtr(sPtr.get(), sPtr->CreateComponentRoot());
    }

private:
    shared_ptr<StoreItemMetadata> metadata_;
    vector<byte> value_;
    bool isDelete_;
};

class ReplicatedStore::NotificationManager::IScopedEnumerator : public ComponentRoot
{
public:
    virtual bool IsStoreActive() const = 0;
    virtual bool IsEnumeratorClosed() const = 0;
};

class ReplicatedStore::NotificationManager::StoreItemMetadataEnumerator 
    : public ReplicatedStoreEnumeratorBase
    , public Api::IStoreItemMetadataEnumerator
{
public:
    StoreItemMetadataEnumerator(
        ReplicatedStore::NotificationManager::IScopedEnumerator const & owner,
        wstring const & keyPrefix,
        bool strictPrefix,
        EnumerationSPtr const & enumSPtr)
        : ReplicatedStoreEnumeratorBase()
        , owner_(owner)
        , keyPrefix_(keyPrefix)
        , strictPrefix_(strictPrefix)
        , enumSPtr_(enumSPtr)
        , root_(owner.CreateComponentRoot())
        , metadata_()
        , metadataLock_()
        , currentProgress_(0)
    {
    }

    virtual ErrorCode MoveNext() override
    {
        return this->OnMoveNextBase();
    }

    virtual ErrorCode OnMoveNextBase() override
    {
        // Unblock replicated store close if the notification is taking a long
        // time to process.
        //
        if (!owner_.IsStoreActive())
        {
            WriteInfo(
                TraceComponent,
                "{0} replicated store closed - aborting copy notification",
                this->TraceId);

            return ErrorCodeValue::ObjectClosed;
        }

        AcquireWriteLock lock(metadataLock_);

        if (owner_.IsEnumeratorClosed())
        {
            WriteError(
                TraceComponent,
                "{0} IStoreEnumerator cannot be used outside notification context",
                this->TraceId);

            metadata_.reset();
            return ErrorCodeValue::InvalidState;
        }

        auto error = enumSPtr_->MoveNext();
        if (!error.IsSuccess()) 
        { 
            metadata_.reset();
            return error; 
        }

        _int64 lsn = 0;
        error = enumSPtr_->CurrentOperationLSN(lsn);
        if (!error.IsSuccess()) 
        { 
            metadata_.reset();
            return error; 
        }

        wstring type;
        error = enumSPtr_->CurrentType(type);
        if (!error.IsSuccess()) 
        { 
            metadata_.reset();
            return error; 
        }

        wstring key;
        error = enumSPtr_->CurrentKey(key);
        if (!error.IsSuccess()) 
        { 
            metadata_.reset();
            return error; 
        }

        if (strictPrefix_ && !StringUtility::StartsWith(key, keyPrefix_))
        {
            metadata_.reset();
            return ErrorCodeValue::EnumerationCompleted;
        }

        FILETIME fileTime;
        error = enumSPtr_->CurrentLastModifiedFILETIME(fileTime);
        if (!error.IsSuccess()) 
        { 
            metadata_.reset();
            return error; 
        }

        FILETIME fileTimeOnPrimary;
        error = enumSPtr_->CurrentLastModifiedOnPrimaryFILETIME(fileTimeOnPrimary);
        if (!error.IsSuccess())
        {
            metadata_.reset();
            return error;
        }
        
        size_t valueSize;
        error = enumSPtr_->CurrentValueSize(valueSize);
        if (!error.IsSuccess()) 
        { 
            metadata_.reset();
            return error; 
        }

        metadata_ = make_shared<StoreItemMetadata>(
            lsn,
            move(type),
            move(key),
            DateTime(fileTime),
            DateTime(fileTimeOnPrimary),
            valueSize);

        ++currentProgress_;

        return error;
    }

    virtual ErrorCode TryMoveNext(bool & success) override
    {
        return this->TryMoveNextBase(success);
    }

    virtual Api::IStoreItemMetadataPtr GetCurrentItemMetadata() override
    {
        AcquireReadLock lock(metadataLock_);

        return Api::IStoreItemMetadataPtr(metadata_.get(), metadata_->CreateComponentRoot());
    }

    void GetQueryStatus(
        __out wstring & prefix,
        __out size_t & progress)
    {
        AcquireReadLock lock(metadataLock_);

        prefix = keyPrefix_;
        progress = currentProgress_;
    }

private:
    IScopedEnumerator const & owner_;
    wstring keyPrefix_;
    bool strictPrefix_;
    ComponentRootSPtr root_;
    RwLock metadataLock_;
    size_t currentProgress_;

protected:
    // destruct enumeration before root, which holds the associated transaction
    EnumerationSPtr enumSPtr_;
    shared_ptr<StoreItemMetadata> metadata_;
};

class ReplicatedStore::NotificationManager::StoreItemEnumerator 
    : public StoreItemMetadataEnumerator
    , public Api::IStoreItemEnumerator
{
public:
    StoreItemEnumerator(
        __in ReplicatedStore::NotificationManager::IScopedEnumerator const & owner,
        wstring const & keyPrefix,
        bool strictPrefix,
        EnumerationSPtr const & enumSPtr)
        : StoreItemMetadataEnumerator(
            owner,
            keyPrefix,
            strictPrefix,
            enumSPtr)
        , item_()
        , itemLock_()
    {
    }

    ErrorCode MoveNext() override
    {
        return this->OnMoveNextBase();
    }

    ErrorCode OnMoveNextBase() override
    {
        AcquireWriteLock lock(itemLock_);

        auto error = StoreItemMetadataEnumerator::OnMoveNextBase();
        if (!error.IsSuccess())
        {
            metadata_.reset();
            item_.reset();
            return error;
        }

        vector<byte> value;
        error = enumSPtr_->CurrentValue(value);
        if (!error.IsSuccess()) 
        { 
            metadata_.reset();
            item_.reset();
            return error; 
        }

        item_ = make_shared<StoreItem>(
            metadata_,  // GetCurrentItemMetadata() should still work
            move(value),
            false);     // isDelete

        return error;
    }

    ErrorCode TryMoveNext(bool & success) override
    {
        return this->TryMoveNextBase(success);
    }

    Api::IStoreItemMetadataPtr GetCurrentItemMetadata() override
    {
        AcquireReadLock lock(itemLock_);

        return StoreItemMetadataEnumerator::GetCurrentItemMetadata();
    }

    Api::IStoreItemPtr GetCurrentItem() override
    {
        AcquireReadLock lock(itemLock_);

        return Api::IStoreItemPtr(item_.get(), item_->CreateComponentRoot());
    }

private:
    shared_ptr<StoreItem> item_;
    RwLock itemLock_;
};

class ReplicatedStore::NotificationManager::StoreItemListEnumerator 
    : public ReplicatedStoreEnumeratorBase
    , public Api::IStoreNotificationEnumerator
{
public:
    StoreItemListEnumerator(
        vector<shared_ptr<StoreItem>> && items)
        : ReplicatedStoreEnumeratorBase()
        , items_(move(items))
        , itemLock_()
        , currentIndex_(0)
    {
    }

    size_t GetItemsSize() const
    {
        return items_.size();
    }

    _int64 GetOperationLSN() const
    {
        if (!items_.empty())
        {
            return items_.front()->GetOperationLSN();
        }

        return 0;
    }

    ErrorCode MoveNext() override
    {
        return this->OnMoveNextBase();
    }

    ErrorCode OnMoveNextBase() override
    {
        AcquireWriteLock lock(itemLock_);

        if (currentIndex_ < numeric_limits<size_t>::max() &&
            ++currentIndex_ <= items_.size())
        {
            return ErrorCodeValue::Success;
        }
        else
        {
            return ErrorCodeValue::EnumerationCompleted;
        }
    }

    ErrorCode TryMoveNext(bool & success) override
    {
        return this->TryMoveNextBase(success);
    }

    void Reset()
    {
        AcquireWriteLock lock(itemLock_);

        currentIndex_ = 0;
    }

    Api::IStoreItemMetadataPtr GetCurrentItemMetadata()
    {
        AcquireReadLock lock(itemLock_);

        if (currentIndex_ > 0 && currentIndex_ <= items_.size())
        {
            shared_ptr<StoreItemMetadata> const & sPtr = items_[currentIndex_-1]->GetMetadata();
            return Api::IStoreItemMetadataPtr(sPtr.get(), sPtr->CreateComponentRoot());
        }

        return Api::IStoreItemMetadataPtr();
    }

    Api::IStoreItemPtr GetCurrentItem()
    {
        AcquireReadLock lock(itemLock_);

        if (currentIndex_ > 0 && currentIndex_ <= items_.size())
        {
            shared_ptr<StoreItem> const & sPtr = items_[currentIndex_-1];
            return Api::IStoreItemPtr(sPtr.get(), sPtr->CreateComponentRoot());
        }

        return Api::IStoreItemPtr();
    }

private:
    vector<shared_ptr<StoreItem>> items_;
    RwLock itemLock_;
    size_t currentIndex_;
};

class ReplicatedStore::NotificationManager::StoreEnumerator 
    : public IScopedEnumerator
    , public Api::IStoreEnumerator
{
public:
    StoreEnumerator(
        __in ReplicatedStore::NotificationManager & owner, 
        TransactionSPtr && txSPtr,
        ComponentRoot const & root)
        : owner_(owner)
        , txSPtr_(move(txSPtr))
        , root_(root.CreateComponentRoot())
        , lock_()
        , isClosed_(false)
    {
    }

    bool IsStoreActive() const
    {
        return owner_.replicatedStore_.IsActive;
    }

    bool IsEnumeratorClosed() const
    {
        AcquireReadLock lock(lock_);
        return isClosed_;
    }

    void Close()
    {
        AcquireWriteLock lock(lock_);
        isClosed_ = true;
        txSPtr_.reset();
    }

    ErrorCode GetTransaction(__out TransactionSPtr & txSPtr)
    {
        AcquireReadLock lock(lock_);

        if (isClosed_)
        {
            // Do not return ObjectClosed, which indicates the replica is closed.
            // Here the enumerator is being used outside its intended lifetime.
            //
            WriteError(
                TraceComponent,
                "{0} IStoreEnumerator cannot be used outside notification context",
                this->TraceId);
            return ErrorCodeValue::InvalidState;
        }

        txSPtr = txSPtr_;

        return ErrorCodeValue::Success;
    }

    ErrorCode Enumerate(
        std::wstring const & type,
        std::wstring const & keyPrefix,
        bool strictPrefix,
        __out Api::IStoreItemEnumeratorPtr & itemEnumerator)
    {
        TransactionSPtr txSPtr;
        auto error = this->GetTransaction(txSPtr);
        if (!error.IsSuccess()) { return error; }

        EnumerationSPtr enumSPtr;
        error = owner_.CreateEnumeration(txSPtr, type, keyPrefix, enumSPtr);
        if (!error.IsSuccess()) { return error; }

        auto sPtr = make_shared<StoreItemEnumerator>(*this, keyPrefix, strictPrefix, enumSPtr);
        itemEnumerator = Api::IStoreItemEnumeratorPtr(sPtr.get(), sPtr->CreateComponentRoot());

        {
            AcquireWriteLock lock(lock_);

            currentEnumeratorPtr_ = sPtr;
        }

        return ErrorCodeValue::Success;
    }

    ErrorCode EnumerateMetadata(
        std::wstring const & type,
        std::wstring const & keyPrefix,
        bool strictPrefix,
        __out Api::IStoreItemMetadataEnumeratorPtr & itemMetadataEnumerator)
    {
        TransactionSPtr txSPtr;
        auto error = this->GetTransaction(txSPtr);
        if (!error.IsSuccess()) { return error; }

        EnumerationSPtr enumSPtr;
        error = owner_.CreateEnumeration(txSPtr, type, keyPrefix, enumSPtr);
        if (!error.IsSuccess()) { return error; }

        auto sPtr = make_shared<StoreItemMetadataEnumerator>(*this, keyPrefix, strictPrefix, enumSPtr);
        itemMetadataEnumerator = Api::IStoreItemMetadataEnumeratorPtr(sPtr.get(), sPtr->CreateComponentRoot());

        {
            AcquireWriteLock lock(lock_);

            currentEnumeratorPtr_ = sPtr;
        }

        return ErrorCodeValue::Success;
    }

    ErrorCode EnumerateKeyValueStore(
        std::wstring const & keyPrefix,
        bool strictPrefix,
        __out Api::IStoreItemEnumeratorPtr & itemEnumerator)
    {
        return this->Enumerate(*Constants::KeyValueStoreItemType, keyPrefix, strictPrefix, itemEnumerator);
    }

    ErrorCode EnumerateKeyValueStoreMetadata(
        std::wstring const & keyPrefix,
        bool strictPrefix,
        __out Api::IStoreItemMetadataEnumeratorPtr & itemMetadataEnumerator)
    {
        return this->EnumerateMetadata(*Constants::KeyValueStoreItemType, keyPrefix, strictPrefix, itemMetadataEnumerator);
    }

    void GetQueryStatus(
        __out shared_ptr<wstring> & copyNotificationPrefix,
        __out size_t & copyNotificationProgress)
    {
        shared_ptr<StoreItemMetadataEnumerator> enumeratorSPtr;
        {
            AcquireReadLock lock(lock_);

            enumeratorSPtr = currentEnumeratorPtr_;
        }

        if (enumeratorSPtr)
        {
            wstring prefix;
            enumeratorSPtr->GetQueryStatus(prefix, copyNotificationProgress);
            copyNotificationPrefix = make_shared<wstring>(prefix);
        }
    }

private:
    ReplicatedStore::NotificationManager & owner_;
    TransactionSPtr txSPtr_;
    ComponentRootSPtr root_;
    mutable RwLock lock_;
    bool isClosed_;
    shared_ptr<StoreItemMetadataEnumerator> currentEnumeratorPtr_;
};

// *******************
// NotificationManager
// *******************

ReplicatedStore::NotificationManager::NotificationManager(
    __in ReplicatedStore & owner,
    Api::ISecondaryEventHandlerPtr const & secondaryEventHandler,
    SecondaryNotificationMode::Enum notificationMode)
    : PartitionedReplicaTraceComponent(owner) 
    , replicatedStore_(owner)
    , secondaryEventHandler_(secondaryEventHandler)
    , notificationMode_(notificationMode)
    , notificationBuffer_()
    , notificationBufferLock_()
    , dispatchQueueSPtr_()
    , dispatchDrainedEvent_(false)
    , dispatchError_()
{
    if (SecondaryNotificationMode::ToPublicApi(notificationMode_) == FABRIC_KEY_VALUE_STORE_NOTIFICATION_MODE_INVALID)
    {
        WriteInfo(
            TraceComponent,
            "{0} invalid notification mode {1}",
            this->TraceId,
            notificationMode_);

        notificationMode_ = SecondaryNotificationMode::None;
    }

    if (this->IsEnabled())
    {
        WriteInfo(
            TraceComponent,
            "{0} secondary notifications enabled: {1}",
            this->TraceId,
            notificationMode_);
    }
    else
    {
        WriteInfo(
            TraceComponent,
            "{0} secondary notifications disabled",
            this->TraceId);
    }
}

bool ReplicatedStore::NotificationManager::IsEnabled() const
{
    return (notificationMode_ != SecondaryNotificationMode::None && secondaryEventHandler_.get() != NULL);
}

bool ReplicatedStore::NotificationManager::IsNonBlockingQuorumAcked() const
{
    return (notificationMode_ == SecondaryNotificationMode::NonBlockingQuorumAcked && secondaryEventHandler_.get() != NULL);
}

ErrorCode ReplicatedStore::NotificationManager::CreateEnumeration(
    TransactionSPtr const & txSPtr,
    wstring const & type,
    wstring const & keyPrefix,
    __out EnumerationSPtr & enumSPtr)
{
    return replicatedStore_.LocalStore->CreateEnumerationByTypeAndKey(
        txSPtr,
        type,
        keyPrefix,
        enumSPtr);
}

ErrorCode ReplicatedStore::NotificationManager::NotifyCopyComplete()
{
    if (!this->IsEnabled())
    {
        return ErrorCodeValue::Success;
    }

    TransactionSPtr txSPtr;
    auto error = replicatedStore_.CreateLocalStoreTransaction(txSPtr);

    if (!error.IsSuccess()) { return error; }

    Api::IStoreEnumeratorPtr ptr;
    {
        AcquireWriteLock lock(currentStoreEnumeratorLock_);

        auto sPtr = make_shared<StoreEnumerator>(*this, move(txSPtr), replicatedStore_.Root);
        ptr = Api::IStoreEnumeratorPtr(sPtr.get(), sPtr->CreateComponentRoot()); 
        currentStoreEnumeratorSPtr_ = move(sPtr);
    }

    WriteInfo(
        TraceComponent,
        "{0} notifying copy completion",
        this->TraceId);

    dispatchError_ = secondaryEventHandler_->OnCopyComplete(ptr);

    {
        AcquireWriteLock lock(currentStoreEnumeratorLock_);

        currentStoreEnumeratorSPtr_->Close();
        currentStoreEnumeratorSPtr_.reset();
    }

    if (dispatchError_.IsSuccess())
    {
        WriteInfo(
            TraceComponent, 
            "{0} OnCopyComplete() succeeded",
            this->TraceId);
    }
    else
    {
        WriteWarning(
            TraceComponent, 
            "{0} OnCopyComplete() returned error {1}: details='{2}'", 
            this->TraceId, 
            dispatchError_,
            dispatchError_.Message);
    }

    error = replicatedStore_.PostCopyNotification();
    
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent, 
            "{0} Best-effort PostCopyNotification() failed: error {1}",
            this->TraceId, 
            error);
    }

    return dispatchError_;
}

ErrorCode ReplicatedStore::NotificationManager::NotifyReplication(
    vector<ReplicationOperation> const & replicationOperation,
    ::FABRIC_SEQUENCE_NUMBER operationLsn,
    ::FABRIC_SEQUENCE_NUMBER lastQuorumAcked)
{
    if (!this->IsEnabled())
    {
        return ErrorCodeValue::Success;
    }

    if (!dispatchError_.IsSuccess())
    {
        return dispatchError_;
    }

    vector<shared_ptr<StoreItem>> items;
    for (auto it = replicationOperation.begin(); it != replicationOperation.end(); ++it)
    {
        bool isDelete = (it->Operation == ReplicationOperationType::Delete);

        auto sPtr = make_shared<StoreItem>(
            operationLsn,
            wstring(it->Type),
            wstring(it->Key),
            DateTime::Now(),
            it->GetLastModifiedOnPrimaryUtcAsDateTime(),
            vector<byte>(it->Bytes),
            isDelete);

        items.push_back(move(sPtr));
    }

    auto sPtr = make_shared<StoreItemListEnumerator>(move(items));

    if (notificationMode_ == SecondaryNotificationMode::BlockSecondaryAck)
    {
        this->Dispatch(sPtr);
    }
    else if (notificationMode_ == SecondaryNotificationMode::NonBlockingQuorumAcked)
    {
        if (!dispatchQueueSPtr_)
        {
            TRACE_ERROR_AND_TESTASSERT(TraceComponent, "{0} dispatching not started", this->TraceId);

            return ErrorCodeValue::InvalidState;
        }

        bool scheduleDispatch = false;
        {
            AcquireExclusiveLock lock(notificationBufferLock_);

            while (!notificationBuffer_.empty() && notificationBuffer_.front()->GetOperationLSN() <= lastQuorumAcked)
            {
                auto item = notificationBuffer_.front();

                scheduleDispatch = dispatchQueueSPtr_->EnqueueWithoutDispatch(make_unique<StoreItemListEnumeratorSPtr>(move(item)));

                notificationBuffer_.pop();
            }

            notificationBuffer_.push(move(sPtr));
        }

        if (scheduleDispatch)
        {
            auto root = replicatedStore_.Root.CreateComponentRoot();
            Threadpool::Post([this, root]() { dispatchQueueSPtr_->Dispatch(); });
        }
    }
    else
    {
        TRACE_ERROR_AND_TESTASSERT(TraceComponent, "{0} invalid notification mode {1}", this->TraceId, notificationMode_);

        return ErrorCodeValue::InvalidState;
    }

    return dispatchError_;
}

void ReplicatedStore::NotificationManager::ScheduleDispatchPump()
{
    auto root = replicatedStore_.Root.CreateComponentRoot();
    Threadpool::Post([this, root]() { this->DispatchPump(); });
}

void ReplicatedStore::NotificationManager::DispatchPump()
{
    bool shouldPump = false;
    do
    {
        auto operation = dispatchQueueSPtr_->BeginDequeue(
            TimeSpan::MaxValue,
            [this](AsyncOperationSPtr const & operation) { this->OnDequeueComplete(operation, false); },
            replicatedStore_.Root.CreateAsyncOperationRoot());
        shouldPump = this->OnDequeueComplete(operation, true);

    } while (shouldPump);
}

bool ReplicatedStore::NotificationManager::OnDequeueComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return false; }

    unique_ptr<StoreItemListEnumeratorSPtr> itemUPtr;
    auto error = dispatchQueueSPtr_->EndDequeue(operation, itemUPtr);

    bool shouldPump = operation->CompletedSynchronously;
    bool scheduleDispatch = !shouldPump;

    if (error.IsSuccess())
    {
        if (itemUPtr)
        {
            replicatedStore_.PerfCounters.NotificationQueueSize.Value = dispatchQueueSPtr_->ItemCount;

            this->Dispatch(*itemUPtr);
        }
        else
        {
            replicatedStore_.PerfCounters.NotificationQueueSize.Value = 0;

            WriteInfo(
                TraceComponent,
                "{0} pumped null dispatch",
                this->TraceId);

            shouldPump = false;
            scheduleDispatch = false;

            dispatchDrainedEvent_.Set();
        }
    }
    else
    {
        WriteWarning(
            TraceComponent,
            "{0} dequeue failed: {1}",
            this->TraceId,
            error);
    }

    if (scheduleDispatch)
    {
        this->ScheduleDispatchPump();
    }

    return shouldPump;
}

void ReplicatedStore::NotificationManager::Dispatch(StoreItemListEnumeratorSPtr const & sPtr)
{
    if (dispatchError_.IsSuccess())
    {
        ReplicatedStoreEventSource::Trace->NotifyReplication(
            this->PartitionedReplicaId,
            sPtr->GetItemsSize(),
            sPtr->GetOperationLSN());

        auto ptr = Api::IStoreNotificationEnumeratorPtr(sPtr.get(), sPtr->CreateComponentRoot());

        dispatchError_ = secondaryEventHandler_->OnReplicationOperation(ptr);

        if (!dispatchError_.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                "{0} OnReplicationOperation() returned error {1}: details='{2}'",
                this->TraceId, 
                dispatchError_,
                dispatchError_.Message);

            if (notificationMode_ == SecondaryNotificationMode::NonBlockingQuorumAcked)
            {
                auto root = replicatedStore_.Root.CreateComponentRoot();
                
                // This will call into CancelInternal() in the secondary pump, which will in turn
                // call into DrainNotifications(). Post a thread to unblock the drain/dispatch.
                //
                Threadpool::Post([this, root]()
                { 
                    replicatedStore_.TryFaultStreamAndStopSecondaryPump(dispatchError_, ::FABRIC_FAULT_TYPE_TRANSIENT);
                });
            }
        }
    }
}

void ReplicatedStore::NotificationManager::StartDispatchingNotifications()
{
    if (this->IsNonBlockingQuorumAcked())
    {
        WriteInfo(
            TraceComponent,
            "{0} starting dispatch queue",
            this->TraceId);

        dispatchDrainedEvent_.Reset();

        dispatchQueueSPtr_ = ReaderQueue<StoreItemListEnumeratorSPtr>::Create();
        this->ScheduleDispatchPump();
    }
}

void ReplicatedStore::NotificationManager::DrainNotifications()
{
    if (!this->IsNonBlockingQuorumAcked() || !dispatchQueueSPtr_) { return; }

    {
        AcquireExclusiveLock lock(notificationBufferLock_);

        WriteInfo(
            TraceComponent,
            "{0} draining dispatch queue: buffer={1} queued={2}",
            this->TraceId,
            notificationBuffer_.size(),
            dispatchQueueSPtr_->ItemCount);

        while (!notificationBuffer_.empty())
        {
            auto item = notificationBuffer_.front();

            dispatchQueueSPtr_->EnqueueWithoutDispatch(make_unique<StoreItemListEnumeratorSPtr>(move(item)));

            notificationBuffer_.pop();
        }
    }

    dispatchQueueSPtr_->Dispatch();

    WriteInfo(
        TraceComponent,
        "{0} closing dispatch queue",
        this->TraceId);

    dispatchQueueSPtr_->Close();

    WriteInfo(
        TraceComponent,
        "{0} waiting for dispatch queue to drain",
        this->TraceId);

    dispatchDrainedEvent_.WaitOne();

    WriteInfo(
        TraceComponent,
        "{0} dispatch queued drained",
        this->TraceId);
}

void ReplicatedStore::NotificationManager::GetQueryStatus(
    __out std::shared_ptr<std::wstring> & copyNotificationPrefix,
    __out size_t & copyNotificationProgress)
{
    shared_ptr<StoreEnumerator> enumeratorSPtr;
    {
        AcquireReadLock lock(currentStoreEnumeratorLock_);

        enumeratorSPtr = currentStoreEnumeratorSPtr_;
    }

    if (enumeratorSPtr)
    {
        enumeratorSPtr->GetQueryStatus(
            copyNotificationPrefix,
            copyNotificationProgress);
    }
}

