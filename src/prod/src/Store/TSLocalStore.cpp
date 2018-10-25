// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Api;
using namespace Common;
using namespace Data::TStore;
using namespace ktl;
using namespace Reliability::ReplicationComponent;
using namespace TxnReplicator;
using namespace ServiceModel;
using namespace std;
using namespace Store;

#define TRY_GET_STORE_ROOT() \
    auto storeRoot = this->GetStoreRoot(); \
    if (storeRoot.get() == nullptr) \
    { \
        return ErrorCodeValue::ObjectClosed; \
    } \

StringLiteral const TraceComponent("TSLocalStore");

GlobalWString TSLocalStore::TraceNodeId = make_global<std::wstring>(L"RA");

class TSLocalStore::ComAsyncOperationCallbackHelper
    : public IFabricAsyncOperationCallback
    , public ComUnknownBase
{
    COM_INTERFACE_LIST1(ComAsyncOperationCallbackHelper, IID_IFabricAsyncOperationCallback, IFabricAsyncOperationCallback);

public:
    ComAsyncOperationCallbackHelper(__in AutoResetEvent & event)
        : event_(event)
    {
    }

public:

    //
    // IFabricAsyncOperationCallback
    //

    void STDMETHODCALLTYPE Invoke(IFabricAsyncOperationContext *)
    {
        event_.Set();
    }

private:
    AutoResetEvent & event_;
};

class TSLocalStore::ComMockPartition
    : public IFabricStatefulServicePartition
    , public IFabricInternalStatefulServicePartition
    , public ComUnknownBase
    , public TextTraceComponent<Common::TraceTaskCodes::ReplicatedStore>
{
    DENY_COPY(ComMockPartition)

    BEGIN_COM_INTERFACE_LIST(ComMockPartition)
        COM_INTERFACE_ITEM(IID_IUnknown,IFabricStatefulServicePartition)
        COM_INTERFACE_ITEM(IID_IFabricStatefulServicePartition,IFabricStatefulServicePartition)
        COM_INTERFACE_ITEM(IID_IFabricInternalStatefulServicePartition,IFabricInternalStatefulServicePartition)
    END_COM_INTERFACE_LIST()

public:

    ComMockPartition(
        __in RootedObjectPointer<TSLocalStore> && localStore,
        __in KtlSystem & ktlSystem,
        Guid const & partitionId,
        FABRIC_REPLICA_ID replicaId)
        : localStore_(move(localStore))
        , ktlSystem_(ktlSystem)
        , replicaId_(replicaId)
        , singletonInfo_()
        , partitionInfo_()
        , replicatorFactory_()
        , txReplicatorFactory_()
        , readWriteStatus_(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED)
    {
        localStore_->WriteNoise(
            TraceComponent,
            "{0} ComMockPartition::ctor: {1}",
            localStore_->TraceId,
            TraceThis);

        singletonInfo_.Id = partitionId.AsGUID();
        singletonInfo_.Reserved = NULL;

        partitionInfo_.Kind = FABRIC_SERVICE_PARTITION_KIND_SINGLETON;
        partitionInfo_.Value = &singletonInfo_;
    }

    virtual ~ComMockPartition()
    {
        localStore_->WriteNoise(
            TraceComponent,
            "{0} ComMockPartition::dtor: {1}",
            localStore_->TraceId,
            TraceThis);
    }

    // V1 replicator has a test assert against granting read/write status
    // when the replicator is not primary status. This must be called
    // before closing the replicator.
    //
    void RevokeReadWriteAccess()
    {
        readWriteStatus_ = FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY;
    }

public:

    //
    // IFabricStatefulServicePartition
    //

    HRESULT GetPartitionInfo(
        /*[out, retval]*/ const FABRIC_SERVICE_PARTITION_INFORMATION ** partitionInfo)
    {
        *partitionInfo = &partitionInfo_;

        return S_OK;
    }

    HRESULT GetReadStatus(
        /*[out, retval]*/ FABRIC_SERVICE_PARTITION_ACCESS_STATUS * readStatus)
    {
        *readStatus = readWriteStatus_;
        return S_OK;
    }

    HRESULT GetWriteStatus(
        /*[out, retval]*/ FABRIC_SERVICE_PARTITION_ACCESS_STATUS * writeStatus)
    {
        *writeStatus = readWriteStatus_;
        return S_OK;
    }

    HRESULT CreateReplicator(
        /*[in]*/ IFabricStateProvider *,
        /*[in]*/ FABRIC_REPLICATOR_SETTINGS const *,
        /*[out]*/ IFabricReplicator **,
        /*[out, retval]*/ IFabricStateReplicator **)
    {
        return E_NOTIMPL;
    }

    HRESULT ReportLoad(
        /*[in]*/ ULONG,
        /*[in, size_is(metricCount)]*/ const FABRIC_LOAD_METRIC *)
    {
        return E_NOTIMPL;
    }

    HRESULT ReportFault(
        /*[in]*/ FABRIC_FAULT_TYPE)
    {
        return E_NOTIMPL;
    }

public:

    //
    // IFabricInternalStatefulServicePartition
    //

    HRESULT CreateTransactionalReplicator(
        /*[in]*/ IFabricStateProvider2Factory *,
        /*[in]*/ IFabricDataLossHandler *,
        /*[in]*/ FABRIC_REPLICATOR_SETTINGS const *,
        /*[in]*/ TRANSACTIONAL_REPLICATOR_SETTINGS const *,
        /*[in]*/ KTLLOGGER_SHARED_LOG_SETTINGS const * ,                                          
        /*[out]*/ IFabricPrimaryReplicator **,
        /*[out, retval]*/ void **)
    {
        return E_NOTIMPL;
    }
    
    // Taken from data\collections\ReliableDictionary.Test.cpp
    //
    HRESULT CreateTransactionalReplicatorInternal(
        /*[in]*/ IFabricTransactionalReplicatorRuntimeConfigurations * runtimeConfigurations,
        /*[in]*/ IFabricStateProvider2Factory * stateProviderFactory,
        /*[in]*/ IFabricDataLossHandler * dataLossHandler,
        /*[in]*/ FABRIC_REPLICATOR_SETTINGS const * replicatorSettings,
        /*[in]*/ TRANSACTIONAL_REPLICATOR_SETTINGS const * transactionalReplicatorSettings,
        /*[in]*/ KTLLOGGER_SHARED_LOG_SETTINGS const * ktlloggerSharedSettings,
        /*[out]*/ IFabricPrimaryReplicator ** primaryReplicator,
        /*[out, retval]*/ void ** transactionalReplicator)
    {
        auto error = this->TryCreateLogManager(ktlloggerSharedSettings);
        if (!error.IsSuccess()) { return error.ToHResult(); }

        error = this->TryCreateReplicatorFactory();
        if (!error.IsSuccess()) { return error.ToHResult(); }

        error = this->TryCreateTransactionalReplicatorFactory();
        if (!error.IsSuccess()) { return error.ToHResult(); }

        return txReplicatorFactory_->CreateReplicator(
            replicaId_,
            *replicatorFactory_,
            this, // IFabricStatefulServicePartition
            replicatorSettings,
            transactionalReplicatorSettings,
            ktlloggerSharedSettings,
            runtimeConfigurations,
            TRUE, // hasPersistedState
            nullptr, // TODO: healthClient
            stateProviderFactory,
            dataLossHandler,
            primaryReplicator,
            transactionalReplicator);
    }

    HRESULT GetKtlSystem(
        /*[out, retval]*/ void ** ktlSystem)
    {
        *ktlSystem = &ktlSystem_;

        return S_OK;
    }

private:

    ErrorCode TryCreateLogManager(KTLLOGGER_SHARED_LOG_SETTINGS const * publicSettings)
    {
        if (logManager_.RawPtr() != nullptr) { return ErrorCodeValue::Success; }

        auto status = Data::Log::LogManager::Create(ktlSystem_.PagedAllocator(), logManager_);
        if (NT_ERROR(status))
        {
            return localStore_->FromNtStatus("LogManager->Create", status); 
        }

        shared_ptr<KtlLogger::SharedLogSettings> sharedLogSettings;

        if (localStore_->ktlLogger_.get() != nullptr)
        {
            // Shared log settings are created when KtlLoggerNode is opened
            //
            sharedLogSettings = localStore_->ktlLogger_->SystemServicesSharedLogSettings;
        }

        if (sharedLogSettings.get() == nullptr && publicSettings != nullptr)
        {
            unique_ptr<KtlLoggerSharedLogSettings> internalSettingsUPtr;
            auto error = KtlLoggerSharedLogSettings::FromPublicApi(*publicSettings, internalSettingsUPtr);
            if (!error.IsSuccess())
            {
                localStore_->WriteWarning(
                    TraceComponent,
                    "{0} KtlLoggerSharedLogSettings::FromPublicApi failed: {1}",
                    localStore_->TraceId,
                    error);

                return error;
            }

            sharedLogSettings = make_shared<KtlLogger::SharedLogSettings>();
            error = sharedLogSettings->FromServiceModel(*internalSettingsUPtr);
            if (!error.IsSuccess())
            {
                localStore_->WriteWarning(
                    TraceComponent,
                    "{0} KtlLogger::SharedLogSettings::FromServiceModel failed: {1}",
                    localStore_->TraceId,
                    error);

                return error;
            }
        }

        if (sharedLogSettings.get() == nullptr)
        {
            localStore_->WriteWarning(
                TraceComponent,
                "{0} shared log settings from RA is null - generating default settings",
                localStore_->TraceId);

            auto error = CreateDefaultSharedLogSettings(sharedLogSettings);

            if (!error.IsSuccess()) { return error; }
        }

        localStore_->WriteInfo(
            TraceComponent,
            "{0} opening log manager: {1}",
            localStore_->TraceId,
            *sharedLogSettings);

        status = SyncAwait(logManager_->OpenAsync(CancellationToken::None, sharedLogSettings));
        if (NT_ERROR(status))
        {
            return localStore_->FromNtStatus("LogManager->Open", status); 
        }

        return ErrorCodeValue::Success;
    }

    ErrorCode TryCreateReplicatorFactory()
    {
        if (replicatorFactory_.get() != nullptr) { return ErrorCodeValue::Success; }

        ReplicatorFactoryConstructorParameters replicatorFactoryConstructorParameters; 
        replicatorFactoryConstructorParameters.Root = localStore_.get();
        replicatorFactory_ = ReplicatorFactoryFactory(replicatorFactoryConstructorParameters);

        auto error = replicatorFactory_->Open(TraceNodeId);
        if (!error.IsSuccess()) 
        { 
            localStore_->WriteWarning(
                TraceComponent,
                "{0} ReplicatorFactoryFactory->Open failed: {1}",
                localStore_->TraceId,
                error);
        }

        return error;
    }

    ErrorCode TryCreateTransactionalReplicatorFactory()
    {
        if (txReplicatorFactory_.get() != nullptr) { return ErrorCodeValue::Success; }

        TransactionalReplicatorFactoryConstructorParameters params; 
        params.Root = localStore_.get();
        txReplicatorFactory_ = TransactionalReplicatorFactoryFactory(params);

        auto error = txReplicatorFactory_->Open(ktlSystem_, *logManager_);
        if (!error.IsSuccess()) 
        { 
            localStore_->WriteWarning(
                TraceComponent,
                "{0} TransactionalReplicatorFactoryFactory->Open failed: {1}",
                localStore_->TraceId,
                error);
        }

        return error;
    }

public:

    // LogManager close will block if the transactional replicator is not
    // closed first so this must be called after closing the replicator.
    //
    void TryClose()
    {
        this->TryCloseLogManager();
        this->TryCloseReplicatorFactory();
        this->TryCloseTransactionalReplicatorFactory();
    }

private:

    void TryCloseLogManager()
    {
        if (logManager_.RawPtr() != nullptr)
        {
            localStore_->WriteInfo(
                TraceComponent,
                "{0} closing log manager",
                localStore_->TraceId);

            auto status = SyncAwait(logManager_->CloseAsync(CancellationToken::None));
            if (NT_ERROR(status))
            {
                localStore_->WriteWarning(
                    TraceComponent,
                    "{0} LogManager->Close failed: {1}",
                    localStore_->TraceId,
                    status);

                logManager_->Abort();
            }
            else
            {
                localStore_->WriteInfo(
                    TraceComponent,
                    "{0} log manager closed",
                    localStore_->TraceId);
            }
        }
    }

    void TryCloseReplicatorFactory()
    {
        if (replicatorFactory_.get() != nullptr)
        {
            localStore_->WriteInfo(
                TraceComponent,
                "{0} closing replicator factory",
                localStore_->TraceId);

            auto error = replicatorFactory_->Close();
            if (!error.IsSuccess())
            {
                localStore_->WriteWarning(
                    TraceComponent,
                    "{0} ReplicatorFactoryFactory->Close failed: {1}",
                    localStore_->TraceId,
                    error);

                replicatorFactory_->Abort();
            }
        }
    }

    void TryCloseTransactionalReplicatorFactory()
    {
        if (txReplicatorFactory_.get() != NULL)
        {
            localStore_->WriteInfo(
                TraceComponent,
                "{0} closing transactional replicator factory",
                localStore_->TraceId);

            auto error = txReplicatorFactory_->Close();
            if (!error.IsSuccess())
            {
                localStore_->WriteWarning(
                    TraceComponent,
                    "{0} TransactionalReplicatorFactoryFactory->Close failed: {1}",
                    localStore_->TraceId,
                    error);

                txReplicatorFactory_->Abort();
            }
        }
    }

    ErrorCode CreateDefaultSharedLogSettings(__out shared_ptr<KtlLogger::SharedLogSettings> & result)
    {
        auto settings = make_unique<KtlLogManager::SharedLogContainerSettings>();

        wstring dataRoot;
        auto error = FabricEnvironment::GetFabricDataRoot(dataRoot);
        if (!error.IsSuccess())
        {
            dataRoot = Directory::GetCurrentDirectory();

            localStore_->WriteWarning(
                TraceComponent,
                "{0} FabricEnvironment::GetFabricDataRoot failed: {1} - using {2}",
                localStore_->TraceId,
                error,
                dataRoot);

            error = ErrorCodeValue::Success;
        }

        // Defaults currently copied from KtlLoggerNode.cpp
        //
        auto logFileName = Path::Combine(
            KtlLogger::Constants::DefaultApplicationSharedLogSubdirectory,
            KtlLogger::Constants::DefaultApplicationSharedLogName);
        auto logFilePath = Path::Combine(dataRoot, logFileName);

#if !defined(PLATFORM_UNIX) 
        logFilePath = wformatString("{0}{1}", *TxnReplicator::KtlLoggerSharedLogSettings::WindowsPathPrefix, logFilePath);
#endif

        auto hr = StringCchCopyW(settings->Path, sizeof(settings->Path) / 2, logFilePath.c_str());
        if (FAILED(hr))
        {
            localStore_->WriteWarning(
                TraceComponent,
                "{0} StringCchCopyW({1}) failed: {2}",
                localStore_->TraceId,
                dataRoot,
                hr);

            return ErrorCode::FromHResult(hr);
        }

        settings->DiskId = KGuid(Guid(KtlLogger::Constants::NullGuidString).AsGUID());
        settings->LogContainerId = KGuid(Guid(KtlLogger::Constants::DefaultSystemServicesSharedLogIdString).AsGUID());
        settings->LogSize = (LONGLONG)KtlLogger::KtlLoggerConfig::GetConfig().SystemSharedLogSizeInMB * 1024 * 1024;
        settings->MaximumNumberStreams = KtlLogger::KtlLoggerConfig::GetConfig().SystemSharedLogNumberStreams;
        settings->MaximumRecordSize = KtlLogger::KtlLoggerConfig::GetConfig().SystemSharedLogMaximumRecordSizeInKB * 1024;
        settings->Flags = KtlLogger::KtlLoggerConfig::GetConfig().SystemSharedLogCreateFlags;

        result = make_shared<KtlLogger::SharedLogSettings>(move(settings));

        return ErrorCodeValue::Success;
    }

    RootedObjectPointer<TSLocalStore> localStore_;
    KtlSystem & ktlSystem_;
    FABRIC_REPLICA_ID replicaId_;

    FABRIC_SINGLETON_PARTITION_INFORMATION singletonInfo_;
    FABRIC_SERVICE_PARTITION_INFORMATION partitionInfo_;

    Data::Log::LogManager::SPtr logManager_;
    IReplicatorFactoryUPtr replicatorFactory_;
    ITransactionalReplicatorFactoryUPtr txReplicatorFactory_;

    FABRIC_SERVICE_PARTITION_ACCESS_STATUS readWriteStatus_;
};

class TSLocalStore::InnerStoreRoot : public ComponentRoot
{
public:

    InnerStoreRoot(
        Common::Guid const & partitionId,
        ::FABRIC_REPLICA_ID replicaId,
        Reliability::ReplicationComponent::ReplicatorSettingsUPtr && replicatorSettings,
        TSReplicatedStoreSettingsUPtr && storeSettings)
    {
        replicatedStore_ = make_unique<TSReplicatedStore>(
            partitionId,
            replicaId,
            move(replicatorSettings),
            move(storeSettings),
            IStoreEventHandlerPtr(),
            ISecondaryEventHandlerPtr(),
            *this);
    }

    static shared_ptr<InnerStoreRoot> Create(
        Common::Guid const & partitionId,
        ::FABRIC_REPLICA_ID replicaId,
        Reliability::ReplicationComponent::ReplicatorSettingsUPtr && replicatorSettings,
        TSReplicatedStoreSettingsUPtr && storeSettings)
    {
        return shared_ptr<InnerStoreRoot>(new InnerStoreRoot(
            partitionId, 
            replicaId, 
            move(replicatorSettings),
            move(storeSettings)));
    }

    TSReplicatedStore * GetReplicatedStore() const { return replicatedStore_.get(); }

private:
    unique_ptr<TSReplicatedStore> replicatedStore_;
};

TSLocalStore::TSLocalStore(
    TSReplicatedStoreSettingsUPtr && storeSettings,
    KtlLogger::KtlLoggerNodeSPtr const & ktlLogger)
    : PartitionedReplicaTraceComponent(Guid(), 0)
    , TSComponent()
    , storeSettings_(move(storeSettings))
    , ktlLogger_(ktlLogger)
    , mockPartitionId_(Guid::Empty())
    , mockReplicaId_(0)
    , innerStore_()
    , innerStoreLock_()
    , isActive_(false)
    , shouldCleanup_(false)
{
    ASSERT_IF(storeSettings_.get() == nullptr, "Null TSReplicatedStoreSettings");
}

TSLocalStore::~TSLocalStore()
{
    WriteInfo(
        TraceComponent,
        "{0} dtor: {1}",
        this->TraceId,
        TraceThis);

    this->Terminate();

    this->Drain();
}

TSReplicatedStore * TSLocalStore::Test_GetReplicatedStore() const
{
    auto storeRoot = this->GetStoreRoot();
    return (storeRoot.get() == nullptr ? nullptr : storeRoot->GetReplicatedStore());
}

::FABRIC_TRANSACTION_ISOLATION_LEVEL TSLocalStore::GetDefaultIsolationLevel()
{
    auto storeRoot = this->GetStoreRoot();
    if (storeRoot.get() == nullptr) 
    { 
        return FABRIC_TRANSACTION_ISOLATION_LEVEL_DEFAULT;
    }
    else
    {
        return storeRoot->GetReplicatedStore()->GetDefaultIsolationLevel();
    }
}

ErrorCode TSLocalStore::CreateTransaction(__out TransactionSPtr & tx)
{
    TRY_GET_STORE_ROOT()

    return storeRoot->GetReplicatedStore()->CreateTransaction(tx);
}

FILETIME TSLocalStore::GetStoreUtcFILETIME()
{
    auto storeRoot = this->GetStoreRoot();
    if (storeRoot.get() == nullptr) 
    { 
        FILETIME result = { 0 };
        return result;
    }
    else
    {
        return storeRoot->GetReplicatedStore()->GetStoreUtcFILETIME();
    }
}

ErrorCode TSLocalStore::CreateEnumerationByTypeAndKey(
    TransactionSPtr const & transaction,
    std::wstring const & type,
    std::wstring const & keyStart,
    __out EnumerationSPtr & enumSPtr)
{
    TRY_GET_STORE_ROOT()

    return storeRoot->GetReplicatedStore()->CreateEnumerationByTypeAndKey(transaction, type, keyStart, enumSPtr);
}

ErrorCode TSLocalStore::Initialize(wstring const &)
{
    WriteWarning(
        TraceComponent,
        "{0} Initialize not implemented",
        this->TraceId);

    return ErrorCodeValue::NotImplemented;
}

// Used by RA
//
ErrorCode TSLocalStore::Initialize(wstring const & instanceName, Federation::NodeId const & nodeId)
{
    return this->InnerInitialize(instanceName, ToGuid(nodeId), 0, nullptr);
}

// Used by KeyValueStoreMigrator
//
ErrorCode TSLocalStore::Initialize(
    wstring const & instanceName, 
    Guid const & partitionId, 
    FABRIC_REPLICA_ID replicaId,
    FABRIC_EPOCH const & epoch)
{
    auto epochPtr = make_unique<FABRIC_EPOCH>();
    epochPtr->DataLossNumber = epoch.DataLossNumber;
    epochPtr->ConfigurationNumber = epoch.ConfigurationNumber;

    return this->InnerInitialize(instanceName, partitionId, replicaId, epochPtr);
}

ErrorCode TSLocalStore::InnerInitialize(
    wstring const & instanceName, 
    Guid const & partitionId, 
    FABRIC_REPLICA_ID replicaId,
    unique_ptr<FABRIC_EPOCH> const & epoch)
{
    mockPartitionId_ = partitionId;
    mockReplicaId_ = replicaId;

    this->UpdateTraceId(mockPartitionId_, mockReplicaId_);

    WriteInfo(
        TraceComponent,
        "{0} ctor initializing: this={1} instance={2} dir={3} partition={4} replica={5}",
        this->TraceId,
        TraceThis,
        instanceName,
        storeSettings_->WorkingDirectory,
        mockPartitionId_,
        mockReplicaId_);

    ReplicatorSettingsUPtr replicatorSettings;
    FABRIC_REPLICATOR_SETTINGS replicatorSettingsStruct = {0};
    ReplicatorSettings::FromPublicApi(replicatorSettingsStruct, replicatorSettings);

    {
        AcquireWriteLock lock(innerStoreLock_);

        innerStore_ = InnerStoreRoot::Create(
            mockPartitionId_,
            mockReplicaId_,
            move(replicatorSettings),
            make_unique<TSReplicatedStoreSettings>(*storeSettings_));
    }

    auto error = this->InnerOpen();
    if (!error.IsSuccess()) { return error; }

    isActive_.store(true);

    error = this->InnerChangeRolePrimary(epoch);
    if (!error.IsSuccess()) { return error; }

    TRY_GET_STORE_ROOT()

    // RA expects the local store to be ready for use
    // after Initialize() returns
    //
    storeRoot->GetReplicatedStore()->WaitForInitialization();

    WriteInfo(
        TraceComponent,
        "{0} inner store and replicator initialized",
        this->TraceId);

    return error;
}

ErrorCode TSLocalStore::Cleanup()
{
    if (replicator_.GetRawPointer() != nullptr)
    {
        WriteInfo(
            TraceComponent,
            "{0} changing replicator role to none",
            this->TraceId);

        TRY_GET_STORE_ROOT()

        FABRIC_EPOCH epoch = {0};
        auto error = storeRoot->GetReplicatedStore()->GetCurrentEpoch(epoch);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                "{0} TSReplicatedStore->GetCurrentEpoch failed: {1}",
                this->TraceId,
                error);

            return error;
        }

        AutoResetEvent event(false);

        auto callback = make_com<ComAsyncOperationCallbackHelper, IFabricAsyncOperationCallback>(event);

        ComPointer<IFabricAsyncOperationContext> context;
        auto hr = replicator_->BeginChangeRole(&epoch, FABRIC_REPLICA_ROLE_NONE, callback.GetRawPointer(), context.InitializationAddress());
        if (FAILED(hr))
        {
            WriteWarning(
                TraceComponent,
                "{0} IFabricReplicator->BeginChangeRole failed: {1}",
                this->TraceId,
                hr);

            return ErrorCode::FromHResult(hr);
        }

        event.WaitOne();

        hr = replicator_->EndChangeRole(context.GetRawPointer());
        if (FAILED(hr))
        {
            WriteWarning(
                TraceComponent,
                "{0} IFabricReplicator->EndChangeRole failed: {1}",
                this->TraceId,
                hr);

            return ErrorCode::FromHResult(hr);
        }
    }

    return ErrorCodeValue::Success;
}

ErrorCode TSLocalStore::MarkCleanupInTerminate()
{
    shouldCleanup_ = true;

    return ErrorCodeValue::Success;
}

ErrorCode TSLocalStore::Terminate()
{
    if (!isActive_.exchange(false))
    {
        return ErrorCodeValue::Success;
    }

    if (partition_.GetRawPointer() != nullptr)
    {
        static_cast<ComMockPartition*>(partition_.GetRawPointer())->RevokeReadWriteAccess();
    }

    if (shouldCleanup_)
    {
        // Since this local store is built on top of a single replica,
        // cleanup of underlying database resources actually happens
        // during ChangeRole(none), which needs to be called before
        // close on the replica.
        //
        this->Cleanup().ReadValue();
    }

    auto error = this->InnerClose();

    WriteInfo(
        TraceComponent,
        "{0} terminated: {1}",
        this->TraceId,
        error);

    return error;
}

void TSLocalStore::Drain()
{
    if (replicator_.GetRawPointer() != nullptr)
    {
        replicator_.Release();
    }

    {
        AcquireWriteLock lock(innerStoreLock_);

        if (innerStore_.get() != nullptr)
        {
            innerStore_.reset();
        }
    }

    if (partition_.GetRawPointer() != nullptr)
    {
        partition_.Release();
    }

    WriteInfo(TraceComponent, "{0} drained", this->TraceId);
}

ErrorCode TSLocalStore::Insert(
    TransactionSPtr const & transaction,
    std::wstring const & type,
    std::wstring const & key,
    void const * value,
    size_t valueSizeInBytes,
    _int64 operationNumber,
    FILETIME const * lastModifiedOnPrimaryUtc)
{
    UNREFERENCED_PARAMETER(operationNumber);
    UNREFERENCED_PARAMETER(lastModifiedOnPrimaryUtc);

    TRY_GET_STORE_ROOT()

    return storeRoot->GetReplicatedStore()->Insert(transaction, type, key, value, valueSizeInBytes);
}

ErrorCode TSLocalStore::Update(
    __in TransactionSPtr const & transaction,
    __in std::wstring const & type,
    __in std::wstring const & key,
    __in _int64 checkOperationNumber,
    __in std::wstring const & newKey,
    __in_opt void const * newValue,
    __in size_t valueSizeInBytes,
    __in _int64 operationNumber,
    __in FILETIME const * lastModifiedOnPrimaryUtc)
{
    UNREFERENCED_PARAMETER(operationNumber);
    UNREFERENCED_PARAMETER(lastModifiedOnPrimaryUtc);

    TRY_GET_STORE_ROOT()

    return storeRoot->GetReplicatedStore()->Update(transaction, type, key, checkOperationNumber, newKey, newValue, valueSizeInBytes);
}

ErrorCode TSLocalStore::Delete(
    __in TransactionSPtr const & transaction,
    __in std::wstring const & type,
    __in std::wstring const & key,
    __in _int64 checkOperationNumber)
{
    TRY_GET_STORE_ROOT()

    return storeRoot->GetReplicatedStore()->Delete(transaction, type, key, checkOperationNumber);
}

ErrorCode TSLocalStore::UpdateOperationLSN(
    TransactionSPtr const &,
    std::wstring const &,
    std::wstring const &,
    ::FABRIC_SEQUENCE_NUMBER)
{
    WriteWarning(
        TraceComponent,
        "{0} UpdateOperationLSN not implemented",
        this->TraceId);

    return ErrorCodeValue::NotImplemented;
}

ErrorCode TSLocalStore::CreateEnumerationByOperationLSN(
    TransactionSPtr const &,
    _int64,
    __out EnumerationSPtr &)
{
    WriteWarning(
        TraceComponent,
        "{0} CreateEnumerationByOperationLSN not implemented",
        this->TraceId);

    return ErrorCodeValue::NotImplemented;
}

ErrorCode TSLocalStore::GetLastChangeOperationLSN(
    TransactionSPtr const &,
    __out ::FABRIC_SEQUENCE_NUMBER &)
{
    WriteWarning(
        TraceComponent,
        "{0} GetLastChangeOperationLSN not implemented",
        this->TraceId);

    return ErrorCodeValue::NotImplemented;
}

ErrorCode TSLocalStore::DeleteDatabaseFiles(wstring const & sharedLogFilePath)
{
    return storeSettings_->DeleteDatabaseFiles(mockPartitionId_, mockReplicaId_, sharedLogFilePath);
}

#if defined(PLATFORM_UNIX)

ErrorCode TSLocalStore::Lock(
    TransactionSPtr const &,
    std::wstring const &,
    std::wstring const &)
{
    return ErrorCodeValue::Success;
}

ErrorCode TSLocalStore::Flush()
{
    return ErrorCodeValue::Success;
}

#endif

shared_ptr<TSLocalStore::InnerStoreRoot> TSLocalStore::GetStoreRoot() const
{
    AcquireReadLock lock(innerStoreLock_);

    if (innerStore_.get() == nullptr)
    {
        WriteInfo(
            TraceComponent,
            "{0} inner store is closed",
            this->TraceId);
    }

    return innerStore_;
}

ErrorCode TSLocalStore::InnerOpen()
{
    AutoResetEvent event(false);

    WriteInfo(
        TraceComponent,
        "{0} opening inner store",
        this->TraceId);

    TRY_GET_STORE_ROOT()

    partition_ = make_com<ComMockPartition, IFabricStatefulServicePartition>(
        RootedObjectPointer<TSLocalStore>(this, this->CreateComponentRoot()),
        ktlLogger_.get() == nullptr ? Common::GetSFDefaultKtlSystem() : ktlLogger_->KtlSystemObject,
        mockPartitionId_,
        mockReplicaId_);

    auto operation = storeRoot->GetReplicatedStore()->BeginOpen(
        FABRIC_REPLICA_OPEN_MODE_INVALID, // ignored by TSReplicatedStore
        partition_,
        [&event](AsyncOperationSPtr const &) { event.Set(); },
        this->CreateAsyncOperationRoot());

    event.WaitOne();

    auto error = storeRoot->GetReplicatedStore()->EndOpen(operation, replicator_);

    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "{0} TSReplicatedStore->Open failed: {1}",
            this->TraceId,
            error);

        return error;
    }

    WriteInfo(
        TraceComponent,
        "{0} opening replicator",
        this->TraceId);

    auto callback = make_com<ComAsyncOperationCallbackHelper, IFabricAsyncOperationCallback>(event);

    ComPointer<IFabricAsyncOperationContext> context;
    auto hr = replicator_->BeginOpen(callback.GetRawPointer(), context.InitializationAddress());
    if (FAILED(hr))
    {
        WriteWarning(
            TraceComponent,
            "{0} IFabricReplicator->BeginOpen failed: {1}",
            this->TraceId,
            hr);

        return ErrorCode::FromHResult(hr);
    }

    event.WaitOne();

    ComPointer<IFabricStringResult> unused;
    hr = replicator_->EndOpen(context.GetRawPointer(), unused.InitializationAddress());
    if (FAILED(hr))
    {
        WriteWarning(
            TraceComponent,
            "{0} IFabricReplicator->EndOpen failed: {1}",
            this->TraceId,
            hr);

        replicator_.Release();
    }

    return ErrorCode::FromHResult(hr);
}

ErrorCode TSLocalStore::InnerChangeRolePrimary(unique_ptr<FABRIC_EPOCH> const & epochPtr)
{
    AutoResetEvent event(false);

    TRY_GET_STORE_ROOT()

    FABRIC_EPOCH epoch = {0};

    if (epochPtr)
    {
        epoch.DataLossNumber = epochPtr->DataLossNumber;
        epoch.ConfigurationNumber = epochPtr->ConfigurationNumber;
    }
    else
    {
        auto error = storeRoot->GetReplicatedStore()->GetCurrentEpoch(epoch);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                "{0} TSReplicatedStore->GetCurrentEpoch failed: {1}",
                this->TraceId,
                error);

            return error;
        }

        if (epoch.DataLossNumber > 0 && epoch.ConfigurationNumber > 0)
        {
            epoch.ConfigurationNumber = epoch.ConfigurationNumber + 1;
        }
        else
        {
            epoch.DataLossNumber = DateTime::Now().Ticks;
            epoch.ConfigurationNumber = epoch.DataLossNumber;
        }
    }

    WriteInfo(
        TraceComponent,
        "{0} promoting replicator to primary: epoch=[config:0x{1:x}, dataloss:0x{2:x}]",
        this->TraceId,
        epoch.ConfigurationNumber,
        epoch.DataLossNumber);

    auto callback = make_com<ComAsyncOperationCallbackHelper, IFabricAsyncOperationCallback>(event);

    ComPointer<IFabricAsyncOperationContext> context;
    auto hr = replicator_->BeginChangeRole(&epoch, FABRIC_REPLICA_ROLE_PRIMARY, callback.GetRawPointer(), context.InitializationAddress());
    if (FAILED(hr))
    {
        WriteWarning(
            TraceComponent,
            "{0} IFabricReplicator->BeginChangeRole failed: {1}",
            this->TraceId,
            hr);

        return ErrorCode::FromHResult(hr);
    }

    event.WaitOne();

    hr = replicator_->EndChangeRole(context.GetRawPointer());
    if (FAILED(hr))
    {
        WriteWarning(
            TraceComponent,
            "{0} IFabricReplicator->EndChangeRole failed: {1}",
            this->TraceId,
            hr);

        return ErrorCode::FromHResult(hr);
    }

    WriteInfo(
        TraceComponent,
        "{0} promoting inner store to primary",
        this->TraceId);

    auto operation = storeRoot->GetReplicatedStore()->BeginChangeRole(
        FABRIC_REPLICA_ROLE_PRIMARY,
        [&event](AsyncOperationSPtr const &) { event.Set(); },
        this->CreateAsyncOperationRoot());

    event.WaitOne();

    wstring unused;
    auto error = storeRoot->GetReplicatedStore()->EndChangeRole(operation, unused);

    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "{0} TSReplicatedStore->ChangeRole failed: {1}",
            this->TraceId,
            error);
    }

    return error;
}

ErrorCode TSLocalStore::InnerClose()
{
    ErrorCode error;

    // Transactional replicator will AV if close is called on it after open fails
    //
    if (replicator_.GetRawPointer() != nullptr)
    {
        AutoResetEvent event(false);

        WriteInfo(
            TraceComponent,
            "{0} closing replicator",
            this->TraceId);

        auto callback = make_com<ComAsyncOperationCallbackHelper, IFabricAsyncOperationCallback>(event);

        ComPointer<IFabricAsyncOperationContext> context;
        auto hr = replicator_->BeginClose(callback.GetRawPointer(), context.InitializationAddress());
        if (FAILED(hr))
        {
            WriteWarning(
                TraceComponent,
                "{0} IFabricReplicator->BeginClose failed: {1}",
                this->TraceId,
                hr);

            // Don't return - continue closing other components
        }
        else
        {
            event.WaitOne();

            hr = replicator_->EndClose(context.GetRawPointer());
            if (FAILED(hr))
            {
                WriteWarning(
                    TraceComponent,
                    "{0} IFabricReplicator->EndOpen failed: {1}",
                    this->TraceId,
                    hr);

                // Don't return - continue closing other components
            }
        }

        error = ErrorCode::FirstError(error, ErrorCode::FromHResult(hr));
    }

    auto storeRoot = this->GetStoreRoot();
    if (storeRoot.get() != nullptr)
    {
        AutoResetEvent event(false);

        WriteInfo(
            TraceComponent,
            "{0} closing inner store",
            this->TraceId);

        auto operation = storeRoot->GetReplicatedStore()->BeginClose(
            [&event](AsyncOperationSPtr const &) { event.Set(); },
            this->CreateAsyncOperationRoot());

        event.WaitOne();

        auto innerError = storeRoot->GetReplicatedStore()->EndClose(operation);

        if (!innerError.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                "{0} TSReplicatedStore->Close failed: {1}",
                this->TraceId,
                innerError);

            error = ErrorCode::FirstError(error, innerError);
        }
    }

    if (partition_.GetRawPointer() != nullptr)
    {
        static_cast<ComMockPartition*>(partition_.GetRawPointer())->TryClose();
    }

    return error;
}

StringLiteral const & TSLocalStore::GetTraceComponent() const
{
    return TraceComponent;
}

wstring const & TSLocalStore::GetTraceId() const
{
    return this->TraceId;
}

Guid TSLocalStore::ToGuid(Federation::NodeId const & nodeId)
{
    auto value = nodeId.getIdValue();

    USHORT data3 = (value.High & 0xFFFF);
    USHORT data2 = (value.High >> (sizeof(data3) * 8)) & 0xFFFF;
    int data1 = (value.High >> ((sizeof(data2)+sizeof(data3)) * 8)) & 0xFFFFFFFF;

    byte data4[8];
    for (auto ix=0; ix<8; ++ix)
    {
        data4[ix] = (value.Low >> ((7-ix) * 8)) & 0xFF;
    }

    return Guid(data1, data2, data3, data4[0], data4[1], data4[2], data4[3], data4[4], data4[5], data4[6], data4[7]);
}
