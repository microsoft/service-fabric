// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Api;
using namespace Common;
using namespace Data::TStore;
using namespace Data::Utilities;
using namespace ktl;
using namespace ServiceModel;
using namespace std;
using namespace Store;

StringLiteral const TraceComponent("TSChangeHandler");

namespace CopyDispatchState
{
    enum Enum
    {
        Pending = 0,
        Dispatching,
        Dispatched
    };

    LONG ToAtomic(Enum value)
    {
        return static_cast<LONG>(value);
    }

    Enum ToEnum(LONG value)
    {
        return static_cast<Enum>(value);
    }
};

class TSChangeHandler::StoreItemMetadata 
    : public IStoreItemMetadata
    , public ComponentRoot
{
private:
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

public:

    virtual ~StoreItemMetadata() { }

    static IStoreItemMetadataPtr Create(
        _int64 lsn,
        wstring && type,
        wstring && key,
        DateTime fileTime,
        DateTime fileTimeOnPrimary,
        size_t valueSize)
    {
        auto item = shared_ptr<StoreItemMetadata>(new StoreItemMetadata(
            lsn, 
            move(type), 
            move(key), 
            fileTime, 
            fileTimeOnPrimary,
            valueSize));

        return IStoreItemMetadataPtr(item.get(), item->CreateComponentRoot());
    }

public:

    //
    // IStoreItemMetadata
    //

    _int64 GetOperationLSN() const override { return lsn_; }
    std::wstring const & GetType() const override { return type_; }
    std::wstring const & GetKey() const override { return key_; }
    Common::DateTime GetLastModifiedUtc() const override { return fileTime_; }
    Common::DateTime GetLastModifiedOnPrimaryUtc() const override { return fileTimeOnPrimary_; }
    size_t GetValueSize() const override { return valueSize_; }
    std::wstring TakeType() override { return move(type_); }
    std::wstring TakeKey() override { return move(key_); }
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

class TSChangeHandler::StoreItem 
    : public IStoreItem
    , public ComponentRoot
{
private:
    StoreItem(
        IStoreItemMetadataPtr const & metadata,
        vector<byte> && value,
        bool isDelete)
        : metadata_(metadata)
        , value_(move(value))
        , isDelete_(isDelete)
    {
    }

public:

    virtual ~StoreItem() { }

    static IStoreItemPtr Create(
        IStoreItemMetadataPtr const & metadata,
        vector<byte> && value,
        bool isDelete)
    {
        auto item = shared_ptr<StoreItem>(new StoreItem(
            metadata, 
            move(value),
            isDelete));

        return IStoreItemPtr(item.get(), item->CreateComponentRoot());
    }

public:

    //
    // IStoreItemMetadata
    //

    _int64 GetOperationLSN() const override { return metadata_->GetOperationLSN(); }
    std::wstring const & GetType() const override { return metadata_->GetType(); }
    std::wstring const & GetKey() const override { return metadata_->GetKey(); }
    Common::DateTime GetLastModifiedUtc() const override { return metadata_->GetLastModifiedUtc(); }
    Common::DateTime GetLastModifiedOnPrimaryUtc() const override { return metadata_->GetLastModifiedOnPrimaryUtc(); }
    size_t GetValueSize() const override { return metadata_->GetValueSize(); }
    std::wstring TakeType() override { return metadata_->TakeType(); }
    std::wstring TakeKey() override { return metadata_->TakeKey(); }
    Api::IKeyValueStoreItemMetadataResultPtr ToKeyValueItemMetadata() { return metadata_->ToKeyValueItemMetadata(); }

public:

    //
    // IStoreItem
    //

    virtual std::vector<byte> const & GetValue() const override { return value_; }
    virtual std::vector<byte> TakeValue() override { return move(value_); }
    virtual bool IsDelete() const override { return isDelete_; }
    virtual Api::IKeyValueStoreItemResultPtr ToKeyValueItem()
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
    IStoreItemMetadataPtr metadata_;
    vector<byte> value_;
    bool isDelete_;
};

class TSChangeHandler::StoreItemMetadataEnumerator
    : public IStoreItemMetadataEnumerator
    , public TSEnumerationBase
{
public:
    StoreItemMetadataEnumerator(
        ::Store::PartitionedReplicaId const & partitionedReplicaId,
        wstring const & type,
        wstring const & keyPrefix,
        bool strictPrefix,
        IAsyncEnumerator<IKvsRebuildEntry> & innerEnumerator)
        : TSEnumerationBase(partitionedReplicaId, type, keyPrefix, strictPrefix) 
        , innerEnumerator_(innerEnumerator)
    {
    }

    virtual ~StoreItemMetadataEnumerator() {};

public:

    static IStoreItemMetadataEnumeratorPtr Create(
        ::Store::PartitionedReplicaId const & partitionedReplicaId,
        wstring const & type,
        wstring const & keyPrefix,
        bool strictPrefix,
        IAsyncEnumerator<IKvsRebuildEntry> & innerEnumerator)
    {
        auto enumerator = make_shared<StoreItemMetadataEnumerator>(
            partitionedReplicaId,
            type,
            keyPrefix,
            strictPrefix,
            innerEnumerator);

        return IStoreItemMetadataEnumeratorPtr(enumerator.get(), enumerator->CreateComponentRoot());
    }

public:

    //
    // IStoreItemMetadataEnumerator
    //
    
    virtual Common::ErrorCode MoveNext() override { return this->OnMoveNextBase(); }
    virtual Common::ErrorCode TryMoveNext(bool & success) override { return this->TryMoveNextBase(success); }
    virtual IStoreItemMetadataPtr GetCurrentItemMetadata() override
    {
        KBuffer::SPtr unused;
        return this->InternalGetCurrentItemMetadata(unused);
    }

public:

    //
    // TSEnumerationBase
    //

    bool OnInnerMoveNext() override
    {
        return SyncAwait(innerEnumerator_.MoveNextAsync(CancellationToken::None));
    }

    KString::SPtr OnGetCurrentKey() override
    {
        auto item = innerEnumerator_.GetCurrent();

        return item.Key;
    }

protected:

    //
    // TSComponent
    //

    StringLiteral const & GetTraceComponent() const
    {
        return TraceComponent;
    }

    wstring const & GetTraceId() const
    {
        return this->TraceId;
    }

protected:

    IStoreItemMetadataPtr InternalGetCurrentItemMetadata(__out KBuffer::SPtr & buffer)
    {
        auto item = innerEnumerator_.GetCurrent();

        wstring type;
        wstring key;

        this->SplitKey(item.Key, type, key);

        auto metadata = StoreItemMetadata::Create(
            item.Value.Key, // lsn
            move(type),
            move(key),
            DateTime::Zero, // TODO: last modified
            DateTime::Zero, // last modified on primary
            item.Value.Value->QuerySize());

        buffer = item.Value.Value;

        return metadata;
    }

private:
    IAsyncEnumerator<IKvsRebuildEntry> & innerEnumerator_;
};

class TSChangeHandler::StoreItemEnumerator
    : public IStoreItemEnumerator
    , public StoreItemMetadataEnumerator
{
public:
    StoreItemEnumerator(
        ::Store::PartitionedReplicaId const & partitionedReplicaId,
        wstring const & type,
        wstring const & keyPrefix,
        bool strictPrefix,
        IAsyncEnumerator<IKvsRebuildEntry> & innerEnumerator)
        : StoreItemMetadataEnumerator(partitionedReplicaId, type, keyPrefix, strictPrefix, innerEnumerator)
    {
    }

    virtual ~StoreItemEnumerator() {}

public:

    static IStoreItemEnumeratorPtr Create(
        ::Store::PartitionedReplicaId const & partitionedReplicaId,
        wstring const & type,
        wstring const & keyPrefix,
        bool strictPrefix,
        __in IAsyncEnumerator<IKvsRebuildEntry> & innerEnumerator)
    {
        auto enumerator = make_shared<StoreItemEnumerator>(
            partitionedReplicaId,
            type,
            keyPrefix,
            strictPrefix,
            innerEnumerator);

        return IStoreItemEnumeratorPtr(enumerator.get(), enumerator->CreateComponentRoot());
    }

public:

    //
    // IStoreItemMetadataEnumerator
    //
    
    Common::ErrorCode MoveNext() override { return StoreItemMetadataEnumerator::MoveNext(); }
    Common::ErrorCode TryMoveNext(bool & success) override { return StoreItemMetadataEnumerator::TryMoveNextBase(success); }
    IStoreItemMetadataPtr GetCurrentItemMetadata() override { return StoreItemMetadataEnumerator::GetCurrentItemMetadata(); }

public:

    //
    // IStoreItemEnumerator
    //

    virtual IStoreItemPtr GetCurrentItem()
    {
        KBuffer::SPtr kBuffer;
        auto metadata = this->InternalGetCurrentItemMetadata(kBuffer);

        return StoreItem::Create(
            metadata,
            this->ToByteVector(kBuffer),
            false); // isDelete
    }
};

class TSChangeHandler::StoreEnumerator
    : public IStoreEnumerator
    , public PartitionedReplicaTraceComponent<Common::TraceTaskCodes::ReplicatedStore>
    , public TSComponent
    , public ComponentRoot
{
    using PartitionedReplicaTraceComponent<Common::TraceTaskCodes::ReplicatedStore>::TraceId;
    using PartitionedReplicaTraceComponent<Common::TraceTaskCodes::ReplicatedStore>::get_TraceId;

public:

    StoreEnumerator(
        ::Store::PartitionedReplicaId const & partitionedReplicaId,
        __in IAsyncEnumerator<IKvsRebuildEntry> & innerEnumerator)
        : PartitionedReplicaTraceComponent(partitionedReplicaId)
        , innerEnumerator_(innerEnumerator)
        , isInitialized_(false)
    {
    }

    virtual ~StoreEnumerator() {};

public:

    //
    // IStoreEnumerator
    //

    Common::ErrorCode Enumerate(
        std::wstring const & type,
        std::wstring const & keyPrefix,
        bool strictPrefix,
        __out IStoreItemEnumeratorPtr & itemEnumerator) override
    {
        // TODO: TStore does not support Reset()
        //       Also, even better would be a Seek(prefix) API
        //       instead of Reset()
        //
        //innerEnumerator_.Reset();

        if (isInitialized_)
        {
            WriteInfo(
                TraceComponent,
                "{0} only one call to Enumerate is currently supported",
                this->TraceId);

            return ErrorCodeValue::InvalidOperation;
        }
        else
        {
            isInitialized_ = true;
        }

        itemEnumerator = StoreItemEnumerator::Create(
            this->PartitionedReplicaId,
            type,
            keyPrefix,
            strictPrefix,
            innerEnumerator_);

        return ErrorCodeValue::Success;
    }

    Common::ErrorCode EnumerateMetadata(
        std::wstring const & type,
        std::wstring const & keyPrefix,
        bool strictPrefix,
        __out IStoreItemMetadataEnumeratorPtr & itemMetadataEnumerator) override
    {
        // TODO: TStore does not support Reset()
        //       Also, even better would be a Seek(prefix) API
        //       instead of Reset()
        //
        //innerEnumerator_.Reset();

        if (isInitialized_)
        {
            WriteInfo(
                TraceComponent,
                "{0} only one call to Enumerate is currently supported",
                this->TraceId);

            return ErrorCodeValue::InvalidOperation;
        }
        else
        {
            isInitialized_ = true;
        }

        itemMetadataEnumerator = StoreItemMetadataEnumerator::Create(
            this->PartitionedReplicaId,
            type,
            keyPrefix,
            strictPrefix,
            innerEnumerator_);

        return ErrorCodeValue::Success;
    }

    Common::ErrorCode EnumerateKeyValueStore(
        std::wstring const & keyPrefix,
        bool strictPrefix,
        __out IStoreItemEnumeratorPtr & itemEnumerator) override
    {
        return this->Enumerate(*Constants::KeyValueStoreItemType, keyPrefix, strictPrefix, itemEnumerator);
    }

    Common::ErrorCode EnumerateKeyValueStoreMetadata(
        std::wstring const & keyPrefix,
        bool strictPrefix,
        __out IStoreItemMetadataEnumeratorPtr & itemMetadataEnumerator) override
    {
        return this->EnumerateMetadata(*Constants::KeyValueStoreItemType, keyPrefix, strictPrefix, itemMetadataEnumerator);
    }

protected:

    //
    // TSComponent
    //

    StringLiteral const & GetTraceComponent() const
    {
        return TraceComponent;
    }

    wstring const & GetTraceId() const
    {
        return this->TraceId;
    }

private:
    IAsyncEnumerator<IKvsRebuildEntry> & innerEnumerator_;
    bool isInitialized_;
};

class TSChangeHandler::NotificationEntry
{
public:
    NotificationEntry(
        wstring && type,
        wstring && key,
        KBuffer::SPtr const & value,
        _int64 lsn,
        bool isDelete)
        : type_(move(type))
        , key_(move(key))
        , value_(value)
        , lsn_(lsn)
        , isDelete_(isDelete)
    {
    }

    __declspec(property(get=get_Type)) wstring const & Type;
    __declspec(property(get=get_Key)) wstring const & Key;
    __declspec(property(get=get_Value)) KBuffer::SPtr const & Value;
    __declspec(property(get=get_Lsn)) _int64 Lsn;
    __declspec(property(get=get_IsDelete)) bool IsDelete;
    
    wstring const & get_Type() const { return type_; }
    wstring const & get_Key() const { return key_; }
    KBuffer::SPtr const & get_Value() const { return value_; }
    _int64 get_Lsn() const { return lsn_; }
    bool get_IsDelete() const { return isDelete_; }

private:
    wstring type_;
    wstring key_;
    KBuffer::SPtr value_;
    _int64 lsn_;
    bool isDelete_;
};

class TSChangeHandler::StoreNotificationEnumerator
    : public IStoreNotificationEnumerator
    , public TSComponent
    , public ReplicatedStoreEnumeratorBase
{
public:
    StoreNotificationEnumerator(
        wstring const & traceId,
        shared_ptr<NotificationEntry> && notification)
        : traceId_(traceId)
        , items_()
        , isInitialized_(false)
        , currentIndex_(0)
    {
        items_.push_back(move(notification));
    }

    virtual ~StoreNotificationEnumerator() {};

public:
    //
    // IStoreItemMetadataEnumerator
    //

    ErrorCode MoveNext() override
    {
        return this->OnMoveNextBase();
    }

    ErrorCode TryMoveNext(bool & success) override
    {
        return this->TryMoveNextBase(success);
    }

    IStoreItemMetadataPtr GetCurrentItemMetadata() override
    {
        auto const & item = items_[currentIndex_];

        return StoreItemMetadata::Create(
            item->Lsn,
            wstring(item->Type),
            wstring(item->Key),
            DateTime::Zero, // TODO: last modified
            DateTime::Zero, // last modified on primary
            item->IsDelete ? 0 : item->Value->QuerySize());
    }

public:
    //
    // IStoreItemEnumerator
    //
    
    IStoreItemPtr GetCurrentItem() override
    {
        auto const & item = items_[currentIndex_];

        return StoreItem::Create(
            this->GetCurrentItemMetadata(),
            item->IsDelete ? vector<byte>() : this->ToByteVector(item->Value),
            item->IsDelete);
    }

public:
    //
    // IStoreNotificationEnumerator
    //

    virtual void Reset()
    {
        isInitialized_ = false;
        currentIndex_ = 0;
    }

protected:
    //
    // ReplicatedStoreEnumeratorBase
    //
    ErrorCode OnMoveNextBase() override
    {
        if (items_.empty()) 
        { 
            return ErrorCodeValue::EnumerationCompleted; 
        }

        if (isInitialized_)
        {
            if (currentIndex_ < items_.size() - 1)
            {
                ++currentIndex_;
                return ErrorCodeValue::Success;
            }
            else
            {
                return ErrorCodeValue::EnumerationCompleted;
            }
        }
        else
        {
            isInitialized_ = true;
            currentIndex_ = 0;

            return ErrorCodeValue::Success;
        }
    }

protected:

    //
    // TSComponent
    //

    StringLiteral const & GetTraceComponent() const
    {
        return TraceComponent;
    }

    wstring const & GetTraceId() const 
    {
        return traceId_;
    }

private:
    std::wstring traceId_;
    std::vector<shared_ptr<NotificationEntry>> items_;
    bool isInitialized_;
    size_t currentIndex_;
};

#define REBUILD_BUFFER_ENUMERATOR_TAG 'brST'

class TSChangeHandler::RebuildBufferEnumerator 
    : public KObject<RebuildBufferEnumerator>
    , public KShared<RebuildBufferEnumerator>
    , public Data::Utilities::IAsyncEnumerator<IKvsRebuildEntry>
{
    K_SHARED_INTERFACE_IMP(IDisposable)
    K_SHARED_INTERFACE_IMP(IAsyncEnumerator)

private:
    RebuildBufferEnumerator(RebuildBuffer && rebuildBuffer)
        : rebuildBuffer_(move(rebuildBuffer))
    {
    }

public:

    static NTSTATUS Create(
        RebuildBuffer && rebuildBuffer,
        __in KAllocator & allocator,
        __out KSharedPtr<IAsyncEnumerator<IKvsRebuildEntry>> & result)
    {
        auto * pointer = _new(REBUILD_BUFFER_ENUMERATOR_TAG, allocator) RebuildBufferEnumerator(move(rebuildBuffer));

        if (pointer == nullptr) { return SF_STATUS_INVALID_OPERATION; }
        if (NT_ERROR(pointer->Status())) { return pointer->Status(); }

        result = KSharedPtr<IAsyncEnumerator<IKvsRebuildEntry>>(pointer);

        return S_OK;
    }

    //
    // IDisposable
    //

    void Dispose() override
    {
        this->Reset();
        rebuildBuffer_.clear();
    }

public:
    //
    // IAsyncEnumerator
    //

    IKvsRebuildEntry GetCurrent() override
    {
        return (*keyIterator_)->second;
    }

    ktl::Awaitable<bool> MoveNextAsync(__in CancellationToken const &) override
    {
        if (typeIterator_.get() == nullptr)
        {
            typeIterator_ = make_shared<RebuildBuffer::iterator>(rebuildBuffer_.begin());

            if (*typeIterator_ != rebuildBuffer_.end())
            {
                keyIterator_ = make_shared<RebuildBufferKeyEntries::iterator>((*typeIterator_)->second.begin());
            }
            else
            {
                co_return false;
            }
        }
        else
        {
            if (++(*keyIterator_) == (*typeIterator_)->second.end() && ++(*typeIterator_) != rebuildBuffer_.end())
            {
                keyIterator_ = make_shared<RebuildBufferKeyEntries::iterator>((*typeIterator_)->second.begin());
            }
        }

        co_return (*typeIterator_ != rebuildBuffer_.end() && *keyIterator_ != (*typeIterator_)->second.end());
    }


    void Reset() override
    {
        typeIterator_.reset();
        keyIterator_.reset();
    }

private:
    RebuildBuffer rebuildBuffer_;
    shared_ptr<RebuildBuffer::iterator> typeIterator_;
    shared_ptr<RebuildBufferKeyEntries::iterator> keyIterator_;
};

// 
// TSChangeHandler
//

TSChangeHandler::TSChangeHandler(
    __in TSReplicatedStore & replicatedStore,
    Api::ISecondaryEventHandlerPtr && secondaryEventHandler)
    : PartitionedReplicaTraceComponent(replicatedStore.PartitionedReplicaId)
    , replicatedStore_(replicatedStore)
    , innerStore_()
    , secondaryEventHandler_(move(secondaryEventHandler))
    , rebuildBuffer_()
    , rebuildBufferLock_()
    , copyDispatchState_(CopyDispatchState::Pending)
    , copyDispatchEvent_(false)
{
}

TSChangeHandler::~TSChangeHandler()
{
}

#define TS_CHANGE_HANDLER_TAG 'hcST'

NTSTATUS TSChangeHandler::Create(
    __in TSReplicatedStore & replicatedStore,
    Api::ISecondaryEventHandlerPtr && secondaryEventHandler,
    __out KSharedPtr<TSChangeHandler> & result)
{
    auto * pointer = _new(TS_CHANGE_HANDLER_TAG, replicatedStore.GetAllocator()) TSChangeHandler(
        replicatedStore,
        move(secondaryEventHandler));

    if (pointer == nullptr) { return SF_STATUS_INVALID_OPERATION; }
    if (NT_ERROR(pointer->Status())) { return pointer->Status(); }

    result = KSharedPtr<TSChangeHandler>(pointer);

    return S_OK;
}

void TSChangeHandler::TryRegister(__in TxnReplicator::IStateProvider2 & stateProvider)
{
    if (innerStore_.RawPtr() == nullptr)
    {
        innerStore_ = IKvs::SPtr(dynamic_cast<IKvs*>(&stateProvider));

        innerStore_->DictionaryChangeHandlerSPtr = KSharedPtr<IKvsChangeHandler>(this);

        WriteInfo(
            TraceComponent,
            "{0} registered change handler",
            this->TraceId);
    }
}

void TSChangeHandler::TryUnregister()
{
    if (innerStore_.RawPtr() != nullptr)
    {
        innerStore_->DictionaryChangeHandlerSPtr = nullptr;
        
        innerStore_ = nullptr;

        WriteInfo(
            TraceComponent,
            "{0} unregistered change handler",
            this->TraceId);
    }
}

void TSChangeHandler::OnRebuilt(
    __in TxnReplicator::ITransactionalReplicator & source,
    __in Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, KSharedPtr<TxnReplicator::IStateProvider2>>> & stateProviders)
{
    // We only use a single transactional replicator
    //
    UNREFERENCED_PARAMETER(source);

    while (stateProviders.MoveNext())
    {
        auto current = stateProviders.Current();

        WriteInfo(
            TraceComponent,
            "{0} IStateManagerChangeHandler::OnRebuilt: {1}",
            this->TraceId,
            ToWString(*current.Key));

        if (TSReplicatedStore::TStoreProviderName->Compare(*current.Key) == TRUE)
        {
            this->TryRegister(*current.Value);

            break;
        }
    }
}

void TSChangeHandler::OnAdded(
    __in TxnReplicator::ITransactionalReplicator &,
    __in TxnReplicator::ITransaction const &,
    __in KUri const & stateProviderName,
    __in TxnReplicator::IStateProvider2 & stateProvider)
{
    WriteInfo(
        TraceComponent,
        "{0} IStateManagerChangeHandler::OnAdded: {1}",
        this->TraceId,
        ToWString(stateProviderName));

    if (TSReplicatedStore::TStoreProviderName->Compare(stateProviderName) == TRUE)
    {
        this->TryRegister(stateProvider);
    }
}

void TSChangeHandler::OnRemoved(
    __in TxnReplicator::ITransactionalReplicator &,
    __in TxnReplicator::ITransaction const &,
    __in KUri const & stateProviderName,
    __in TxnReplicator::IStateProvider2 &)
{
    WriteInfo(
        TraceComponent,
        "{0} IStateManagerChangeHandler::OnRemoved: {1}",
        this->TraceId,
        ToWString(stateProviderName));

    if (TSReplicatedStore::TStoreProviderName->Compare(stateProviderName) == TRUE)
    {
        this->TryUnregister();
    }
}

ktl::Awaitable<void> TSChangeHandler::OnAddedAsync(
    __in TxnReplicator::TransactionBase const& replicatorTransaction,
    __in KString::SPtr kKey,
    __in KBuffer::SPtr kValue,
    __in LONG64 sequenceNumber,
    __in bool isPrimary) noexcept
{
    UNREFERENCED_PARAMETER(replicatorTransaction);
    UNREFERENCED_PARAMETER(isPrimary);

    if (this->ShouldIgnore()) { co_return; }

    wstring type, key;
    this->SplitKey(kKey, type, key);

    bool shouldBuffer = this->ShouldBuffer();

    // TODO: Make Noise trace
    //
    WriteInfo(
        TraceComponent,
        "{0} IDictionaryChangeHandler::OnAddedAsync: type={1} key={2} lsn={3} buffer={4}",
        this->TraceId,
        type,
        key,
        sequenceNumber,
        shouldBuffer);
        
    if (shouldBuffer)
    {
        AcquireWriteLock lock(rebuildBufferLock_);

        rebuildBuffer_[type][key] = IKvsRebuildEntry(kKey, IKvsGetEntry(sequenceNumber, kValue));
    }
    else
    {
        this->DispatchRebuildBufferIfNeeded(type, key);

        auto notification = make_shared<NotificationEntry>(
            move(type),
            move(key),
            kValue,
            sequenceNumber,
            false); // isDelete

        this->DispatchReplication(move(notification));
    }

    co_return;
}

ktl::Awaitable<void> TSChangeHandler::OnUpdatedAsync(
    __in TxnReplicator::TransactionBase const& replicatorTransaction,        
    __in KString::SPtr kKey,
    __in KBuffer::SPtr kValue,
    __in LONG64 sequenceNumber,
    __in bool isPrimary) noexcept
{
    UNREFERENCED_PARAMETER(replicatorTransaction);
    UNREFERENCED_PARAMETER(isPrimary);

    if (this->ShouldIgnore()) { co_return; }

    wstring type, key;
    this->SplitKey(kKey, type, key);

    bool shouldBuffer = this->ShouldBuffer();

    WriteInfo(
        TraceComponent,
        "{0} IDictionaryChangeHandler::OnUpdatedAsync: type={1} key={2} lsn={3} buffer={4}",
        this->TraceId,
        type,
        key,
        sequenceNumber,
        shouldBuffer);
        
    if (shouldBuffer)
    {
        AcquireWriteLock lock(rebuildBufferLock_);

        rebuildBuffer_[type][key] = IKvsRebuildEntry(kKey, IKvsGetEntry(sequenceNumber, kValue));
    }
    else
    {
        this->DispatchRebuildBufferIfNeeded(type, key);

        auto notification = make_shared<NotificationEntry>(
            move(type),
            move(key),
            kValue,
            sequenceNumber,
            false); // isDelete

        this->DispatchReplication(move(notification));
    }

    co_return;
}

ktl::Awaitable<void> TSChangeHandler::OnRemovedAsync(
    __in TxnReplicator::TransactionBase const& replicatorTransaction,        
    __in KString::SPtr kKey,
    __in LONG64 sequenceNumber,
    __in bool isPrimary) noexcept
{
    UNREFERENCED_PARAMETER(replicatorTransaction);
    UNREFERENCED_PARAMETER(isPrimary);

    if (this->ShouldIgnore()) { co_return; }

    wstring type, key;
    this->SplitKey(kKey, type, key);

    bool shouldBuffer = this->ShouldBuffer();

    WriteInfo(
        TraceComponent,
        "{0} IDictionaryChangeHandler::OnRemovedAsync: type={1} key={2} lsn={3} buffer={4}",
        this->TraceId,
        type,
        key,
        sequenceNumber,
        shouldBuffer);
        
    if (shouldBuffer)
    {
        AcquireWriteLock lock(rebuildBufferLock_);

        rebuildBuffer_[type].erase(key);

        if (rebuildBuffer_[type].empty())
        {
            rebuildBuffer_.erase(type);
        }
    }
    else
    {
        this->DispatchRebuildBufferIfNeeded(type, key);

        auto notification = make_shared<NotificationEntry>(
            move(type),
            move(key),
            KBuffer::SPtr(),
            sequenceNumber,
            true); // isDelete

        this->DispatchReplication(move(notification));
    }

    co_return;
}

ktl::Awaitable<void> TSChangeHandler::OnRebuiltAsync(
    __in IAsyncEnumerator<IKvsRebuildEntry> & enumerator) noexcept
{
    bool shouldBuffer = this->ShouldBuffer();

    WriteInfo(
        TraceComponent,
        "{0} IDictionaryChangeHandler::OnRebuiltAsync: buffer={1}",
        this->TraceId,
        shouldBuffer);

    if (shouldBuffer)
    {
        RebuildBuffer tempBuffer;

        // Recovery
        //
        while (co_await enumerator.MoveNextAsync(CancellationToken::None))
        {
            auto rebuildEntry = enumerator.GetCurrent();

            wstring type, key;
            this->SplitKey(rebuildEntry.Key, type, key);

            tempBuffer[type][key] = enumerator.GetCurrent();
        }

        AcquireWriteLock lock(rebuildBufferLock_);

        for (auto const & temp : tempBuffer)
        {
            for (auto const & entry : temp.second)
            {
                rebuildBuffer_[temp.first][entry.first] = entry.second;
            }
        }
    }
    else
    {
        AcquireWriteLock lock(rebuildBufferLock_);

        // Full build - It's okay to dispatch the copy completion notification
        // inline since replicator only waits for recovery notifications
        // to drain before completing change role to Idle. This will not block
        // change role to Idle.
        //
        this->DispatchCopyCompletion(enumerator);
    }

    co_return;
}

void TSChangeHandler::DispatchRebuildBufferIfNeeded(wstring const & type, wstring const & key)
{
    if (CopyDispatchState::ToEnum(copyDispatchState_.load()) == CopyDispatchState::Dispatched)
    {
        return;
    }

    // If no full build occurred, then the first undo or replication operation will trigger
    // dispatching of all buffered notifications.
    //
    auto expected = CopyDispatchState::ToAtomic(CopyDispatchState::Pending);
    if (copyDispatchState_.compare_exchange_weak(expected, CopyDispatchState::ToAtomic(CopyDispatchState::Dispatching)))
    {
        this->DispatchRebuildBuffer();
    }
    else
    {
        WriteInfo(
            TraceComponent,
            "{0} waiting for rebuild buffer dispatch to complete: type={1} key={2}",
            this->TraceId,
            type,
            key);

        // TStore notifications can occur in parallel as long as there are
        // no key conflicts, so the first batch of notifications after recovery 
        // must wait for the rebuild buffer dispatch to complete. Otherwise, the
        // higher layer may observe replication notifications before the
        // copy completion notification is finished processing.
        //
        copyDispatchEvent_.WaitOne();
    }
}

void TSChangeHandler::DispatchRebuildBuffer()
{
    AcquireWriteLock lock(rebuildBufferLock_);

    WriteInfo(
        TraceComponent,
        "{0} dispatching rebuild buffer: count={1}",
        this->TraceId,
        rebuildBuffer_.size());
        
    KSharedPtr<IAsyncEnumerator<IKvsRebuildEntry>> innerEnumerator;
    auto status = RebuildBufferEnumerator::Create(move(rebuildBuffer_), replicatedStore_.GetAllocator(), innerEnumerator);
    auto error = this->FromNtStatus("RebuildBufferEnumerator::Create", status);

    if (error.IsSuccess())
    {
        this->DispatchCopyCompletion(*innerEnumerator);

        innerEnumerator->Dispose();
    }
    else
    {
        replicatedStore_.TransientFault(L"RebuildBufferEnumerator::Create failed", error);

        throw ktl::Exception(SF_STATUS_OBJECT_CLOSED);
    }
}

void TSChangeHandler::DispatchCopyCompletion(__in IAsyncEnumerator<IKvsRebuildEntry> & innerEnumerator)
{
    rebuildBuffer_.clear();

    WriteInfo(
        TraceComponent,
        "{0} posting copy completion",
        this->TraceId);
        
    auto enumerator = make_shared<StoreEnumerator>(this->PartitionedReplicaId, innerEnumerator);

    ErrorCode error;
    ManualResetEvent callbackEvent(false);

    // The OnCopyComplete processing thread will call MoveNext, which results
    // in SyncAwait on MoveNextAsync. Move OnCopyComplete processing off the
    // KTL thread.
    //
    Threadpool::Post([this, &enumerator, &error, &callbackEvent]
    {
        WriteInfo(
            TraceComponent,
            "{0} dispatching copy completion",
            this->TraceId);

        error = secondaryEventHandler_->OnCopyComplete(IStoreEnumeratorPtr(
            enumerator.get(),
            enumerator->CreateComponentRoot()));

        callbackEvent.Set();
    });

    callbackEvent.WaitOne();

    if (error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0} copy completion dispatched",
            this->TraceId);

        copyDispatchState_.store(CopyDispatchState::ToAtomic(CopyDispatchState::Dispatched));

        copyDispatchEvent_.Set();
    }
    else
    {
        replicatedStore_.TransientFault(L"OnCopyComplete failed", error);

        throw ktl::Exception(SF_STATUS_OBJECT_CLOSED);
    }
}

void TSChangeHandler::DispatchReplication(shared_ptr<NotificationEntry> && notification)
{
    auto enumerator = make_shared<StoreNotificationEnumerator>(this->TraceId, move(notification));

    IStoreNotificationEnumeratorPtr iEnumerator(enumerator.get(), enumerator->CreateComponentRoot());

    auto error = secondaryEventHandler_->OnReplicationOperation(iEnumerator);

    if (!error.IsSuccess())
    {
        replicatedStore_.TransientFault(L"OnReplicationOperation failed", error);

        // Must throw to prevent TStore from acking the replication operation.
        // TStore asserts on everything but TIMEOUT and OBJECT_CLOSED.
        //
        throw ktl::Exception(SF_STATUS_OBJECT_CLOSED);
    }
}

bool TSChangeHandler::ShouldIgnore()
{
    // V2 stack delivers notifications on all replicas including the primary.
    // V1 stack only delivered notifications to secondary replicas.
    //
    // Since the Image Store Service was designed against V1 stack notification
    // semantics, suppress notifications on the primary to avoid executing
    // unexpected logic in the service.
    //
    switch (replicatedStore_.GetReplicatorRole())
    {
    case FABRIC_REPLICA_ROLE_UNKNOWN:
    case FABRIC_REPLICA_ROLE_IDLE_SECONDARY:
    case FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY:
        return false;

    default:
        return true;
    }
}

bool TSChangeHandler::ShouldBuffer()
{
    // Image Store Service replicas must not block Open or ChangeRole(Idle)
    // since processing of the copy completion notification depends on
    // communicating with other replicas in the partition to
    // pull missing files.
    //
    // Therefore, all notifications must be buffered during Open (recovery).
    // Once Open (recovery) is complete, then we can dispatch all buffered
    // notifications as a copy completion notification and start
    // processing all subsequent change notifications normally
    // without buffering.
    //
    switch (replicatedStore_.GetReplicatorRole())
    {
    case FABRIC_REPLICA_ROLE_UNKNOWN:
        return true;

    default:
        return false;
    }
}

StringLiteral const & TSChangeHandler::GetTraceComponent() const
{
    return TraceComponent;
}

wstring const & TSChangeHandler::GetTraceId() const
{
    return this->TraceId;
}
