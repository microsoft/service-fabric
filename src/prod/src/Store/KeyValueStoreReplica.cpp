// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Naming/MockOperationContext.h"
#include "Naming/MockReplicator.h"
#include "Naming/MockStatefulServicePartition.h"

using namespace std;
using namespace Common;
using namespace Api;
using namespace Reliability::ReplicationComponent;
using namespace Store;

StringLiteral const TraceComponent("KeyValueStoreReplica");    

//
// Lifecycle async operations
//

class KeyValueStoreReplica::OpenAsyncOperation : public AsyncOperation
{
public:
    OpenAsyncOperation(
        __in KeyValueStoreReplica & owner,
        FABRIC_REPLICA_OPEN_MODE openMode,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent, true) // skipCompleteOnCancel_
        , owner_(owner)
        , openMode_(openMode)
        , completionEvent_(false)
        , isCanceled_(false)
    {
    }

    static ErrorCode End(
        AsyncOperationSPtr const & operation, 
        __out ComPointer<IFabricReplicator> & replicator)
    {
        auto casted = AsyncOperation::End<OpenAsyncOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            replicator = casted->replicator_;
        }

        return casted->Error;
    }

protected:

    void OnCancel() override
    {
        // Workaround for RA not calling abort when cancelling a pending open
        //
        isCanceled_.store(true);

        // RA can potentially call Open->Cancel->Open->Close when deleting a replica.
        // The local store may still be open (or opening) when the cancel call aborts
        // the first replica open, causing the subsequent re-open to fail.
        //
        // Although the Open->Cancel->Open sequence should be optimized by RA, protect
        // against it at the store level by blocking cancel.
        //
        // We can block cancel in this case because skipCompleteOnCancel_ is being set 
        // to true and cancel will be called with forceComplete = false. Otherwise,
        // OnCancel() would deadlock waiting for OnCompleted().
        //
        // See also TSReplicatedStore.cpp
        //
        owner_.WriteInfo(TraceComponent, "{0} blocking cancel for completion", owner_.TraceId);

        completionEvent_.WaitOne();

        owner_.WriteInfo(TraceComponent, "{0} cancel unblocked", owner_.TraceId);
    }

    void OnCompleted() override
    {
        completionEvent_.Set();
    }

    void OnStart(AsyncOperationSPtr const & thisSPtr) override
    {
        auto operation = owner_.iReplicatedStore_->BeginOpen(
            openMode_,
            owner_.partition_,
            [this](AsyncOperationSPtr const & operation) { this->OnOpenCompleted(operation, false); },
            thisSPtr);
        this->OnOpenCompleted(operation, true);
    }

private:
    void OnOpenCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto const & thisSPtr = operation->Parent;

        auto error = owner_.iReplicatedStore_->EndOpen(operation, replicator_);

        if (error.IsSuccess())
        {
            error = owner_.OnOpen(owner_.partition_);
        }

        // RAP will not call Abort on the replica if open fails
        // so we must do it internally to release any circular references
        //
        if (!error.IsSuccess() || isCanceled_.load())
        {
            owner_.WriteInfo(TraceComponent, "{0} aborting open: error={1} canceled={2} ", owner_.TraceId, error, isCanceled_.load());
        
            // Ensure that both IReplicatedStore and derived service get aborted
            //
            owner_.Abort();
        }

        this->TryComplete(thisSPtr, error);
    }

    KeyValueStoreReplica & owner_;
    FABRIC_REPLICA_OPEN_MODE openMode_;
    ComPointer<IFabricReplicator> replicator_;
    ManualResetEvent completionEvent_;
    Common::atomic_bool isCanceled_;
};

class KeyValueStoreReplica::ChangeRoleAsyncOperation : public AsyncOperation
{
public:
    ChangeRoleAsyncOperation(
        __in KeyValueStoreReplica & owner,
        FABRIC_REPLICA_ROLE const & newRole,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , newRole_(newRole)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, __out wstring & serviceLocation)
    {
        auto casted = AsyncOperation::End<ChangeRoleAsyncOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            serviceLocation = casted->serviceLocation_;
        }

        return casted->Error;
    }

protected:

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = owner_.iReplicatedStore_->BeginChangeRole(
            newRole_,
            [this](AsyncOperationSPtr const & operation) { this->OnChangeRoleCompleted(operation, false); },
            thisSPtr);
        this->OnChangeRoleCompleted(operation, true);
    }

private:
    void OnChangeRoleCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto const & thisSPtr = operation->Parent;

        auto error = owner_.iReplicatedStore_->EndChangeRole(operation, serviceLocation_);

        if (error.IsSuccess())
        {
            if (newRole_ == FABRIC_REPLICA_ROLE_PRIMARY)
            {
                owner_.TryStartMigration();
            }
            else
            {
                owner_.TryCancelMigration();
            }

            error = owner_.OnChangeRole(newRole_, serviceLocation_);
        }
        
        this->TryComplete(thisSPtr, error);
    }

    KeyValueStoreReplica & owner_;
    FABRIC_REPLICA_ROLE newRole_;
    wstring serviceLocation_;
};

class KeyValueStoreReplica::CloseAsyncOperation : public AsyncOperation
{
public:
    CloseAsyncOperation(
        __in KeyValueStoreReplica & owner,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<CloseAsyncOperation>(operation)->Error;
    }

protected:

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        owner_.TryCancelMigration();

        auto error = owner_.OnClose();

        // Best effort service close. Do not leave underlying store
        // open even if service close fails.
        //
        if (!error.IsSuccess())
        {
            owner_.WriteInfo(
                TraceComponent,
                "{0} OnClose failed: {1}",
                owner_.TraceId,
                error);
        }

        auto operation = owner_.iReplicatedStore_->BeginClose(
            [this](AsyncOperationSPtr const & operation) { this->OnCloseCompleted(operation, false); },
            thisSPtr);
        this->OnCloseCompleted(operation, true);
    }

private:
    void OnCloseCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto const & thisSPtr = operation->Parent;

        auto error = owner_.iReplicatedStore_->EndClose(operation);

        this->TryComplete(thisSPtr, error);
    }

    KeyValueStoreReplica & owner_;
};

//
// KeyValueStoreReplica
//

KeyValueStoreReplica::KeyValueStoreReplica(
    Guid const & partitionId, 
    FABRIC_REPLICA_ID replicaId)
    : ComponentRoot(StoreConfig::GetConfig().EnableReferenceTracking)
    , IKeyValueStoreReplica()
    , ISecondaryEventHandler()
    , PartitionedReplicaTraceComponent<TraceTaskCodes::ReplicatedStore>(partitionId, replicaId)
    , iReplicatedStore_()
    , partition_()
    , deadlockDetector_()
    , migrator_()
    , migratorLock_()
{
    WriteInfo(
        TraceComponent,
        "{0} ctor: ref={1}",
        this->TraceId,
        this->IsReferenceTrackingEnabled);

    ComponentRoot::SetTraceId(this->TraceId);

    deadlockDetector_ = make_shared<DeadlockDetector>(*this);
}

KeyValueStoreReplica::~KeyValueStoreReplica()
{
    WriteInfo(
        TraceComponent,
        "{0} ~dtor",
        this->TraceId);
}

//
// Initializations
//

IReplicatedStoreUPtr KeyValueStoreReplica::CreateForUnitTests(
    Guid const & partitionId,
    FABRIC_REPLICA_ID replicaId,
    EseLocalStoreSettings const & eseSettings,
    ComponentRoot const & root)
{
#ifdef PLATFORM_UNIX
    UNREFERENCED_PARAMETER(partitionId);
    UNREFERENCED_PARAMETER(replicaId);
    UNREFERENCED_PARAMETER(root);

    return TSUnitTestStore::Create(eseSettings.DbFolderPath);
#else
    if (StoreConfig::GetConfig().EnableTStore)
    {
        return TSUnitTestStore::Create(eseSettings.DbFolderPath);
    }
    else
    {
        // EseReplicatedStore is not compiled for Linux
        //
        return EseReplicatedStore::Create(
            partitionId,
            replicaId,
            make_com<Naming::MockReplicator, IFabricReplicator>(),
            ReplicatedStoreSettings(),
            eseSettings,
            IStoreEventHandlerPtr(),
            ISecondaryEventHandlerPtr(),
            root);
    }
#endif
}

ErrorCode KeyValueStoreReplica::CreateForPublicStack_V1(
    Guid const & partitionId,
    FABRIC_REPLICA_ID replicaId,
    ReplicatorSettingsUPtr && replicatorSettings,
    ReplicatedStoreSettings const & replicatedStoreSettings,
    EseLocalStoreSettings const & eseSettings,
    IStoreEventHandlerPtr const & storeEventHandler,
    ISecondaryEventHandlerPtr const & secondaryEventHandler,
    IClientFactoryPtr const & clientFactory,
    wstring const & serviceName,
    __out IKeyValueStoreReplicaPtr & kvs)
{
    auto kvsImpl = make_shared<KeyValueStoreReplica>(
        partitionId,
        replicaId);

    auto error = kvsImpl->InitializeReplicatedStore_V1(
        move(replicatorSettings),
        replicatedStoreSettings,
        eseSettings,
        storeEventHandler,
        secondaryEventHandler,
        clientFactory,
        serviceName,
        false); // allowRepairUpToQuorum

    if (error.IsSuccess())
    {
        kvs = IKeyValueStoreReplicaPtr(
            kvsImpl.get(), 
            kvsImpl->CreateComponentRoot());
    }

    return error;
}

ErrorCode KeyValueStoreReplica::CreateForPublicStack_V2(
    Guid const & partitionId, 
    FABRIC_REPLICA_ID replicaId,
    ReplicatorSettingsUPtr && replicatorSettings,
    TSReplicatedStoreSettingsUPtr && storeSettings,
    Api::IStoreEventHandlerPtr const & storeEventHandler,
    Api::ISecondaryEventHandlerPtr const & secondaryEventHandler,
    __out Api::IKeyValueStoreReplicaPtr & kvs)
{
    auto kvsImpl = make_shared<KeyValueStoreReplica>(
        partitionId,
        replicaId);

    auto error = kvsImpl->InitializeReplicatedStore_V2(
        move(replicatorSettings),
        move(storeSettings),
        storeEventHandler,
        secondaryEventHandler);

    if (error.IsSuccess())
    {
        kvs = IKeyValueStoreReplicaPtr(
            kvsImpl.get(), 
            kvsImpl->CreateComponentRoot());
    }

    return error;
}

ErrorCode KeyValueStoreReplica::InitializeForSystemServices(
    bool enableTStore,
    wstring const & workingDirectory,
    wstring const & databaseDirectory,
    wstring const & sharedLogFileName,
    TxnReplicator::SLConfigBase const & sharedLogConfig,
    Guid const & sharedLogGuid,
    ReplicatorSettingsUPtr && replicatorSettings,
    ReplicatedStoreSettings && replicatedStoreSettings_V1,
    EseLocalStoreSettings const & eseSettings,
    IClientFactoryPtr const & clientFactory,
    NamingUri const & serviceName,
    vector<byte> const & initData,
    bool allowRepairUpToQuorum)
{
    if (sharedLogGuid == Guid::Empty())
    {
        TRACE_ERROR_AND_TESTASSERT(
            TraceComponent,
            "{0} sharedLogGuid cannot be empty for system or FabricTest services", 
            this->TraceId);

        return ErrorCodeValue::InvalidArgument;
    }

    auto notificationMode = replicatedStoreSettings_V1.NotificationMode;
    auto storeEventHandler = IStoreEventHandlerPtr(this, this->CreateComponentRoot());
    auto secondaryEventHandler = (notificationMode == SecondaryNotificationMode::None)
        ? Api::ISecondaryEventHandlerPtr()
        : Api::ISecondaryEventHandlerPtr(this, this->CreateComponentRoot());

    bool isTStoreEnabled = (enableTStore || Store::StoreConfig::GetConfig().EnableTStore);

    auto migrationData = this->TryDeserializeMigrationInitData(initData);

    if (migrationData.get() != nullptr)
    {
        switch (migrationData->GetPhase(this->PartitionId))
        {
        case MigrationPhase::TargetDatabaseSwap:
        case MigrationPhase::SourceDatabaseCleanup:
        case MigrationPhase::TargetDatabaseActive:
            //
            // Only ESE->TStore migration is 
            // currently supported
            //
            isTStoreEnabled = true;
            break;
        }
    }

    // Create and save V2 settings to support dynamically starting migration
    // without restarting the replica. The KeyValueStoreMigrator will
    // be created dynamically and passed the saved V2 settings.
    //
    {
        TxnReplicator::KtlLoggerSharedLogSettingsUPtr sharedLogSettings;
        auto error = TxnReplicator::KtlLoggerSharedLogSettings::CreateForSystemService(
            sharedLogConfig,
            workingDirectory,
            databaseDirectory,
            sharedLogFileName,
            sharedLogGuid,
            sharedLogSettings);

        if (!error.IsSuccess()) 
        { 
            WriteWarning(
                TraceComponent, 
                "{0} failed to created shared log settings: {1}",
                this->TraceId,
                error);

            return error; 
        }

        replicatedStoreSettings_V2_ = make_unique<TSReplicatedStoreSettings>(
            Path::Combine(workingDirectory, databaseDirectory),
            move(sharedLogSettings),
            notificationMode);
    }

    if (isTStoreEnabled)
    {
        auto error = this->InitializeReplicatedStore_V2(
            move(replicatorSettings),
            make_unique<TSReplicatedStoreSettings>(*replicatedStoreSettings_V2_),
            storeEventHandler,
            secondaryEventHandler);

        if (!error.IsSuccess()) 
        { 
            WriteWarning(
                TraceComponent, 
                "{0} failed to initialize V2 store: {1}",
                this->TraceId,
                error);

            return error; 
        }
    }
    else
    {
        auto error = this->InitializeReplicatedStore_V1(
            move(replicatorSettings),
            replicatedStoreSettings_V1,
            eseSettings,
            storeEventHandler,
            secondaryEventHandler,
            clientFactory,
            serviceName.ToString(),
            allowRepairUpToQuorum);

        if (!error.IsSuccess()) 
        { 
            WriteWarning(
                TraceComponent, 
                "{0} failed to initialize V1 store: {1}",
                this->TraceId,
                error);

            return error; 
        }
    }

    // Migrator depends on initialization of source iReplicatedStore_ first
    //
    return this->TryInitializeMigrator(move(migrationData));
}

ErrorCode KeyValueStoreReplica::InitializeForTestServices(
    bool enableTStore,
    wstring const & workingDirectory,
    wstring const & databaseDirectory,
    wstring const & sharedLogFileName,
    TxnReplicator::SLConfigBase const & sharedLogConfig,
    Guid const & sharedLogGuid,
    ReplicatorSettingsUPtr && replicatorSettings,
    ReplicatedStoreSettings && replicatedStoreSettings_V1,
    EseLocalStoreSettings const & eseSettings,
    IClientFactoryPtr const & clientFactory,
    NamingUri const & serviceName,
    bool allowRepairUpToQuorum)
{
    // Multiple code packages in the same application will share the same working directory,
    // which can lead to multiple KtlSystem instances pointing to the same shared log file.
    // This is okay in Windows, but causes problems in Linux where the log driver is in-proc.
    //
    // In production, using the default application shared log ID will automatically cause
    // staging logs to be used instead (on Linux). Staging log file paths are scoped by partition 
    // and replica ID. However, this is not an option in FabricTest since it would cause
    // multiple concurrent running tests to use the same log ID to point to different
    // shared log files (on Windows).
    //
    // The only option is to scope shared log locations by log ID manually. 
    // For the test services in FabricTest, this will be the partition ID and
    // will be safe since two replicas running on the same node (regardless of whether
    // they're in the same or different code packages) must have different partition IDs.
    // 
    // Note: Although the various file paths used in the native store stack are supposed
    // to support long paths, that doesn't seem to be working at the moment. Instead
    // of using the log GUID directly, hash it to reduce the path length as a workaround
    // for now.
    //
    // Example long path in test:
    //
    // \??\C:\MCRoot\BinCache\bins\RunTests\log\Func_UpgradeNData\test\TC\UpgradeNDataRandom.Data\
    // ScaleMin\100\Applications\FabTRandomApp_App0\work\19537f6f-42c8-47f0-bf07-629f65d09031\PersistStore\
    // 19537f6f42c847f0bf07629f65d09031_131618249702007714\131618250230602343

    auto logIdHash = StringUtility::GetHash(sharedLogGuid.ToString());
    auto scopedWorkingDirectory = Path::Combine(workingDirectory, wformatString("{0:x}", logIdHash));

    return this->InitializeForSystemServices(
        enableTStore,
        scopedWorkingDirectory,
        databaseDirectory,
        sharedLogFileName,
        sharedLogConfig,
        sharedLogGuid,
        move(replicatorSettings),
        move(replicatedStoreSettings_V1),
        eseSettings,
        clientFactory,
        serviceName,
        vector<byte>(), // initialization data
        allowRepairUpToQuorum);
}

ErrorCode KeyValueStoreReplica::InitializeReplicatedStore_V1(
    ReplicatorSettingsUPtr && replicatorSettings,
    ReplicatedStoreSettings const & replicatedStoreSettings,
    EseLocalStoreSettings const & eseSettings,
    IStoreEventHandlerPtr const & storeEventHandler,
    ISecondaryEventHandlerPtr const & secondaryEventHandler,
    IClientFactoryPtr const & clientFactory,
    wstring const & serviceName,
    bool allowRepairUpToQuorum)
{
#ifdef PLATFORM_UNIX
    UNREFERENCED_PARAMETER(replicatorSettings);
    UNREFERENCED_PARAMETER(replicatedStoreSettings);
    UNREFERENCED_PARAMETER(eseSettings);
    UNREFERENCED_PARAMETER(storeEventHandler);
    UNREFERENCED_PARAMETER(secondaryEventHandler);
    UNREFERENCED_PARAMETER(clientFactory);
    UNREFERENCED_PARAMETER(serviceName);
    UNREFERENCED_PARAMETER(allowRepairUpToQuorum);

    CODING_ASSERT("{0} KVS/ESE not supported in Linux", this->TraceId);
#else
    //
    // EseReplicatedStore is not compiled for Linux
    //
    iReplicatedStore_ = EseReplicatedStore::Create(
        this->PartitionId,
        this->ReplicaId,
        move(replicatorSettings),
        replicatedStoreSettings,
        eseSettings,
        storeEventHandler,
        secondaryEventHandler,
        *this);

    if (clientFactory.get() != nullptr && !serviceName.empty())
    {
        return iReplicatedStore_->InitializeRepairPolicy(
            clientFactory,
            serviceName,
            allowRepairUpToQuorum);
    }
    else
    {
        return ErrorCodeValue::Success;
    }
#endif
}

ErrorCode KeyValueStoreReplica::InitializeReplicatedStore_V2(
    ReplicatorSettingsUPtr && replicatorSettings,
    TSReplicatedStoreSettingsUPtr && storeSettings,
    IStoreEventHandlerPtr const & storeEventHandler,
    ISecondaryEventHandlerPtr const & secondaryEventHandler)
{
    iReplicatedStore_ = make_unique<TSReplicatedStore>(
        this->PartitionId,
        this->ReplicaId,
        move(replicatorSettings),
        move(storeSettings),
        storeEventHandler,
        secondaryEventHandler,
        *this);

    return ErrorCodeValue::Success;
}

//
// For subclass
//

ErrorCode KeyValueStoreReplica::OnOpen(ComPointer<IFabricStatefulServicePartition> const &) 
{ 
    return ErrorCodeValue::Success; 
}

ErrorCode KeyValueStoreReplica::OnChangeRole(::FABRIC_REPLICA_ROLE, __out std::wstring &)
{ 
    return ErrorCodeValue::Success; 
}

ErrorCode KeyValueStoreReplica::OnClose() 
{ 
    return ErrorCodeValue::Success; 
}

void KeyValueStoreReplica::OnAbort() 
{ 
}

//
// IStoreEventHandler
//

void KeyValueStoreReplica::OnDataLoss()
{
}

AsyncOperationSPtr KeyValueStoreReplica::BeginOnDataLoss(
    __in AsyncCallback const & callback,
    __in AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(callback, parent);
}

ErrorCode KeyValueStoreReplica::EndOnDataLoss(
    __in AsyncOperationSPtr const & asyncOperation,
    __out bool & isStateChanged)
{
    isStateChanged = false;

    return CompletedAsyncOperation::End(asyncOperation);
}

//
// ISecondaryEventHandler
//

ErrorCode KeyValueStoreReplica::OnCopyComplete(IStoreEnumeratorPtr const &)
{
    WriteWarning(
        TraceComponent,
        "{0} KeyValueStoreReplica::OnCopyComplete not implemented",
        this->TraceId);

    return ErrorCodeValue::NotImplemented;
}

ErrorCode KeyValueStoreReplica::OnReplicationOperation(IStoreNotificationEnumeratorPtr const &)
{
    WriteWarning(
        TraceComponent,
        "{0} KeyValueStoreReplica::OnReplicationOperation not implemented",
        this->TraceId);

    return ErrorCodeValue::NotImplemented;
}

//
// IStatefulServiceReplica methods
//
AsyncOperationSPtr KeyValueStoreReplica::BeginOpen(
    ::FABRIC_REPLICA_OPEN_MODE openMode,
    ComPointer<::IFabricStatefulServicePartition> const & partition,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    this->TrackLifeCycleAssertEvent(*Constants::LifecycleOpen, StoreConfig::GetConfig().LifecycleOpenAssertTimeout);

    partition_ = partition;

    return AsyncOperation::CreateAndStart<OpenAsyncOperation>(*this, openMode, callback, parent);
}

ErrorCode KeyValueStoreReplica::EndOpen(
    AsyncOperationSPtr const & operation, 
    __out ComPointer<::IFabricReplicator> & replicator)
{
    this->UntrackLifeCycleAssertEvent(*Constants::LifecycleOpen);

    return OpenAsyncOperation::End(operation, replicator);
}

AsyncOperationSPtr KeyValueStoreReplica::BeginChangeRole(
    ::FABRIC_REPLICA_ROLE newRole,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    this->TrackLifeCycleAssertEvent(*Constants::LifecycleChangeRole, StoreConfig::GetConfig().LifecycleChangeRoleAssertTimeout);

    return AsyncOperation::CreateAndStart<ChangeRoleAsyncOperation>(*this, newRole, callback, parent);
}

ErrorCode KeyValueStoreReplica::EndChangeRole(
    AsyncOperationSPtr const & operation, 
    __out wstring & serviceLocation)
{
    this->UntrackLifeCycleAssertEvent(*Constants::LifecycleChangeRole);

    return ChangeRoleAsyncOperation::End(operation, serviceLocation);
}

AsyncOperationSPtr KeyValueStoreReplica::BeginClose(
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    this->TrackLifeCycleAssertEvent(*Constants::LifecycleClose, StoreConfig::GetConfig().LifecycleCloseAssertTimeout);

    return AsyncOperation::CreateAndStart<CloseAsyncOperation>(*this, callback, parent);
}

ErrorCode KeyValueStoreReplica::EndClose(
    AsyncOperationSPtr const & operation)
{
    this->UntrackLifeCycleAssertEvent(*Constants::LifecycleClose);

    return CloseAsyncOperation::End(operation);
}

void KeyValueStoreReplica::Abort()
{
    reinterpret_cast<IStatefulServiceReplica*>(iReplicatedStore_.get())->Abort();

    this->OnAbort();
}

ErrorCode KeyValueStoreReplica::UpdateInitializationData(vector<byte> && initData)
{
    auto migrationData = this->TryDeserializeMigrationInitData(initData);
    
    bool startNewMigration = false;
    KeyValueStoreMigratorSPtr migrator;
    {
        AcquireExclusiveLock lock(migratorLock_);

        if (migrationData.get() == nullptr)
        {
            if (migrator_.get() != nullptr)
            {
                migrator_->Cancel();

                migrator_.reset();

                this->TransientFault(L"Clearing migration");
            }
        }
        else if (migrator_.get() == nullptr)
        {
            startNewMigration = true;

            auto error = this->TryInitializeMigrator_Unlocked(move(migrationData));
            if (!error.IsSuccess()) { return error; }

            if (iReplicatedStore_->GetIsActivePrimary())
            {
                migrator = migrator_;
            }
        }
        else
        {
            startNewMigration = false;

            migrator = migrator_;
        }
    }

    if (migrator.get() != nullptr)
    {
        if (startNewMigration)
        {
            this->TryStartMigration_Unlocked(migrator);
        }
        else if (migrator->TryUpdateInitializationData(move(migrationData)))
        {
            this->TransientFault(L"Updating migration phase");
        }
    }

    return ErrorCodeValue::Success;
}

//
// IKeyValueStoreMethods methods
//
ErrorCode KeyValueStoreReplica::GetCurrentEpoch(
    __out ::FABRIC_EPOCH & currentEpoch)
{
    return iReplicatedStore_->GetCurrentEpoch(currentEpoch);
}

ErrorCode KeyValueStoreReplica::UpdateReplicatorSettings(
    ::FABRIC_REPLICATOR_SETTINGS const & replicatorSettings)
{
    return this->iReplicatedStore_->UpdateReplicatorSettings(replicatorSettings);
}

ErrorCode KeyValueStoreReplica::UpdateReplicatorSettings(
    ReplicatorSettingsUPtr const & replicatorSettings)
{
    return this->iReplicatedStore_->UpdateReplicatorSettings(replicatorSettings);
}

void KeyValueStoreReplica::TryStartMigration()
{
    KeyValueStoreMigratorSPtr migrator;
    {
        AcquireExclusiveLock lock(migratorLock_);

        migrator = migrator_;
    }

    this->TryStartMigration_Unlocked(migrator);
}

void KeyValueStoreReplica::TryStartMigration_Unlocked(KeyValueStoreMigratorSPtr const & migrator)
{
    if (migrator)
    {
        auto state = migrator->GetMigrationState();

        if (state == MigrationState::Inactive)
        {
            auto operation = migrator->BeginMigration(
                [this](AsyncOperationSPtr const & operation) { this->OnMigrationComplete(operation, false); },
                this->CreateAsyncOperationRoot());
            this->OnMigrationComplete(operation, true);
        }
        else
        {
            WriteInfo(
                TraceComponent,
                "{0} migrator already active: {1}",
                this->TraceId,
                state);
        }
    }
    else
    {
        WriteInfo(
            TraceComponent,
            "{0} migrator not initialized",
            this->TraceId);
    }
}

void KeyValueStoreReplica::TryCancelMigration()
{
    AcquireExclusiveLock lock(migratorLock_);

    if (migrator_.get() != nullptr)
    {
        migrator_->Cancel();
    }
    else
    {
        WriteInfo(
            TraceComponent,
            "{0} migrator not initialized",
            this->TraceId);
    }
}


void KeyValueStoreReplica::OnMigrationComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    AcquireExclusiveLock lock(migratorLock_);

    auto error = migrator_->EndMigration(operation);

    if (error.IsSuccess())
    {
        WriteInfo(
            TraceComponent, 
            "{0} migration successful",
            this->TraceId);
    }
    else
    {
        WriteWarning(
            TraceComponent, 
            "{0} migration failed: {1}",
            this->TraceId,
            error);
    }
}

unique_ptr<MigrationInitData> KeyValueStoreReplica::TryDeserializeMigrationInitData(vector<byte> const & initData)
{
    unique_ptr<MigrationInitData> migrationData;

    if (!initData.empty())
    {
        migrationData = make_unique<MigrationInitData>();

        auto error = migrationData->FromBytes(initData);

        if (error.IsSuccess())
        {
            WriteInfo(
                TraceComponent, 
                "{0} deserialized migration data: {1}",
                this->TraceId,
                *migrationData);
        }
        else
        {
            TRACE_ERROR_AND_TESTASSERT(
                TraceComponent, 
                "{0} failed to deserialize MigrationInitData: {1}", 
                this->TraceId, 
                error);

            //
            // Ignore initialization data and continue to initialize 
            // replica without migration support
            //
        }
    }

    return migrationData;
}

ErrorCode KeyValueStoreReplica::TryInitializeMigrator(unique_ptr<MigrationInitData> && migrationData)
{
    AcquireExclusiveLock lock(migratorLock_);

    return this->TryInitializeMigrator_Unlocked(move(migrationData));
}

ErrorCode KeyValueStoreReplica::TryInitializeMigrator_Unlocked(unique_ptr<MigrationInitData> && )
{
    WriteInfo(
        TraceComponent, 
        "{0} did not create migrator",
        this->TraceId);

    
    return ErrorCodeValue::Success;
}

ErrorCode KeyValueStoreReplica::CreateTransaction(
    ::FABRIC_TRANSACTION_ISOLATION_LEVEL isolationLevel,
    __out ITransactionPtr & transaction)
{
    Guid txId = Guid::NewGuid();

    if (isolationLevel == FABRIC_TRANSACTION_ISOLATION_LEVEL_DEFAULT)
    {
        isolationLevel = iReplicatedStore_->GetDefaultIsolationLevel();
    }
    else if (isolationLevel != iReplicatedStore_->GetDefaultIsolationLevel())
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument);
    }

    IStoreBase::TransactionSPtr replicatedStoreTx;
    auto error = iReplicatedStore_->CreateTransaction(ActivityId(txId), replicatedStoreTx);
    if (!error.IsSuccess()) { return error; }

    auto keyValueStoreTx = make_shared<KeyValueStoreTransaction>(txId, isolationLevel, replicatedStoreTx, deadlockDetector_);
    transaction = ITransactionPtr((ITransaction*)keyValueStoreTx.get(), keyValueStoreTx->CreateComponentRoot());

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode KeyValueStoreReplica::CreateTransaction(
    ::FABRIC_TRANSACTION_ISOLATION_LEVEL isolationLevel,
    ::FABRIC_KEY_VALUE_STORE_TRANSACTION_SETTINGS const & publicSettings,
    __out ITransactionPtr & transaction)
{
    Guid txId = Guid::NewGuid();

    if (isolationLevel == FABRIC_TRANSACTION_ISOLATION_LEVEL_DEFAULT)
    {
        isolationLevel = iReplicatedStore_->GetDefaultIsolationLevel();
    }
    else if (isolationLevel != iReplicatedStore_->GetDefaultIsolationLevel())
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument);
    }

    TransactionSettings settings;
    auto error = settings.FromPublicApi(publicSettings);
    if (!error.IsSuccess()) { return error; }

    IStoreBase::TransactionSPtr replicatedStoreTx;
    error = iReplicatedStore_->CreateTransaction(ActivityId(txId), settings, replicatedStoreTx);
    if (!error.IsSuccess()) { return error; }

    auto keyValueStoreTx = make_shared<KeyValueStoreTransaction>(txId, isolationLevel, replicatedStoreTx, deadlockDetector_);
    transaction = ITransactionPtr((ITransaction*)keyValueStoreTx.get(), keyValueStoreTx->CreateComponentRoot());

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode KeyValueStoreReplica::Add(
    ITransactionBasePtr const & transaction,
    wstring const & key,
    LONG valueSizeInBytes,
    const BYTE * value)
{
    auto keyValueStoreTx = this->GetCastedTransaction(transaction);

    return iReplicatedStore_->Insert(
        keyValueStoreTx->get_ReplicatedStoreTransaction(),
        *Constants::KeyValueStoreItemType,
        key,
        value,
        (size_t)valueSizeInBytes);
}

ErrorCode KeyValueStoreReplica::Remove(
    ITransactionBasePtr const & transaction,
    wstring const & key,
    ::FABRIC_SEQUENCE_NUMBER checkSequenceNumber)
{
    auto keyValueStoreTx = this->GetCastedTransaction(transaction);

    return iReplicatedStore_->Delete(
        keyValueStoreTx->get_ReplicatedStoreTransaction(),
        *Constants::KeyValueStoreItemType,
        key,
        checkSequenceNumber);
}

ErrorCode KeyValueStoreReplica::Update(
    ITransactionBasePtr const & transaction,
    wstring const & key,
    LONG valueSizeInBytes,
    const BYTE * value,
    ::FABRIC_SEQUENCE_NUMBER checkSequenceNumber)
{
    auto keyValueStoreTx = this->GetCastedTransaction(transaction);

    return iReplicatedStore_->Update(
        keyValueStoreTx->get_ReplicatedStoreTransaction(),
        *Constants::KeyValueStoreItemType,
        key,
        checkSequenceNumber,
        key,
        value,
        (size_t)valueSizeInBytes);
}

ErrorCode KeyValueStoreReplica::Get(
    ITransactionBasePtr const & transaction,
    wstring const & key,
    __out IKeyValueStoreItemResultPtr & itemResult)
{
    auto error = this->TryGet(transaction, key, itemResult);

    if (error.IsSuccess() && itemResult.get() == nullptr)
    {
        error = ErrorCodeValue::StoreRecordNotFound;
    }

    return error;
}

ErrorCode KeyValueStoreReplica::TryAdd(
    ITransactionBasePtr const & transaction,
    wstring const & key,
    LONG valueSizeInBytes,
    const BYTE * value,
    __out bool & added)
{
    auto keyValueStoreTx = this->GetCastedTransaction(transaction);

    auto error = iReplicatedStore_->Insert(
        keyValueStoreTx->get_ReplicatedStoreTransaction(),
        *Constants::KeyValueStoreItemType,
        key,
        value,
        (size_t)valueSizeInBytes);

    if (error.IsError(ErrorCodeValue::StoreRecordAlreadyExists))
    {
        error = ErrorCodeValue::Success;

        added = false;
    }
    else
    {
        added = error.IsSuccess();
    }

    return error;
}

ErrorCode KeyValueStoreReplica::TryRemove(
    ITransactionBasePtr const & transaction,
    wstring const & key,
    ::FABRIC_SEQUENCE_NUMBER checkSequenceNumber,
    __out bool & exists)
{
    auto keyValueStoreTx = this->GetCastedTransaction(transaction);

    auto error = iReplicatedStore_->Delete(
        keyValueStoreTx->get_ReplicatedStoreTransaction(),
        *Constants::KeyValueStoreItemType,
        key,
        checkSequenceNumber);

    if (error.IsError(ErrorCodeValue::StoreRecordNotFound))
    {
        error = ErrorCodeValue::Success;

        exists = false;
    }
    else
    {
        exists = error.IsSuccess();
    }

    return error;
}

ErrorCode KeyValueStoreReplica::TryUpdate(
    ITransactionBasePtr const & transaction,
    wstring const & key,
    LONG valueSizeInBytes,
    const BYTE * value,
    ::FABRIC_SEQUENCE_NUMBER checkSequenceNumber,
    __out bool & exists)
{
    auto keyValueStoreTx = this->GetCastedTransaction(transaction);

    auto error = iReplicatedStore_->Update(
        keyValueStoreTx->get_ReplicatedStoreTransaction(),
        *Constants::KeyValueStoreItemType,
        key,
        checkSequenceNumber,
        key,
        value,
        (size_t)valueSizeInBytes);

    if (error.IsError(ErrorCodeValue::StoreRecordNotFound))
    {
        error = ErrorCodeValue::Success;

        exists = false;
    }
    else
    {
        exists = error.IsSuccess();
    }

    return error;
}

ErrorCode KeyValueStoreReplica::TryGet(
    ITransactionBasePtr const & transaction,
    wstring const & key,
    __out IKeyValueStoreItemResultPtr & itemResult)
{
    itemResult = IKeyValueStoreItemResultPtr();

    IKeyValueStoreItemEnumeratorPtr itemEnumerator;
    auto error = EnumerateByKey(
        transaction,
        key,
        true, // strictPrefix
        itemEnumerator);

    if (!error.IsSuccess()) { return error; }

    bool hasNext = false;
    error = itemEnumerator->TryMoveNext(hasNext);

    while(error.IsSuccess() && hasNext)
    {
        auto current = itemEnumerator->get_Current();
        if (StringUtility::Compare(key.c_str(), current->get_Item()->Metadata->Key) == 0)
        {
            itemResult = current;
            return ErrorCodeValue::Success;
        }
        else
        {
            error = itemEnumerator->TryMoveNext(hasNext);
        }
    }

    return error;
}

ErrorCode KeyValueStoreReplica::GetMetadata(
    ITransactionBasePtr const & transaction,
    wstring const & key,
    __out IKeyValueStoreItemMetadataResultPtr & itemMetadataResult)
{
    auto error = this->TryGetMetadata(transaction, key, itemMetadataResult);

    if (error.IsSuccess() && itemMetadataResult.get() == nullptr)
    {
        error = ErrorCodeValue::StoreRecordNotFound;
    }

    return error;
}

ErrorCode KeyValueStoreReplica::TryGetMetadata(
    ITransactionBasePtr const & transaction,
    wstring const & key,
    __out IKeyValueStoreItemMetadataResultPtr & itemMetadataResult)
{
    itemMetadataResult = IKeyValueStoreItemMetadataResultPtr();

    IKeyValueStoreItemMetadataEnumeratorPtr itemMetadataEnumerator;
    auto error = EnumerateMetadataByKey(
        transaction,
        key,
        true, // strictPrefix
        itemMetadataEnumerator);

    if (!error.IsSuccess()) { return error; }

    bool hasNext = false;
    error = itemMetadataEnumerator->TryMoveNext(hasNext);

    while(error.IsSuccess() && hasNext)
    {
        auto current = itemMetadataEnumerator->get_Current();
        if (StringUtility::Compare(key.c_str(), current->get_Metadata()->Key) == 0)
        {
            itemMetadataResult = current;
            return ErrorCodeValue::Success;
        }
        else
        {
            error = itemMetadataEnumerator->TryMoveNext(hasNext);
        }
    }

    return error;
}

ErrorCode KeyValueStoreReplica::Contains(
    ITransactionBasePtr const & transaction,
    wstring const & key,
    __out BOOLEAN & result)
{
    IKeyValueStoreItemMetadataResultPtr itemMetadataResult;
    auto error = this->TryGetMetadata(transaction, key, itemMetadataResult);
    
    if (error.IsSuccess())
    {
        result = (itemMetadataResult.get() != nullptr ? TRUE : FALSE);
    } 

    return error;
}

ErrorCode KeyValueStoreReplica::Enumerate(
    ITransactionBasePtr const & transaction,
    __out IKeyValueStoreItemEnumeratorPtr & itemEnumerator)
{
   return EnumerateByKey(
       transaction, 
       wstring(), 
       false, // strictPrefix
       itemEnumerator); 
}

ErrorCode KeyValueStoreReplica::EnumerateByKey(
    ITransactionBasePtr const & transaction,
    wstring const & key,
    bool strictPrefix,
    __out IKeyValueStoreItemEnumeratorPtr & itemEnumerator)
{
    auto keyValueStoreTx = this->GetCastedTransaction(transaction);

    IStoreBase::EnumerationSPtr replicatedStoreEnumeration;
    auto error = iReplicatedStore_->CreateEnumerationByTypeAndKey(
        keyValueStoreTx->get_ReplicatedStoreTransaction(),
        *Constants::KeyValueStoreItemType,
        key,
        replicatedStoreEnumeration);
    if (!error.IsSuccess()) { return error; }

    auto keyValueStoreEnum = make_shared<KeyValueStoreItemEnumerator>(key, strictPrefix, replicatedStoreEnumeration);
    itemEnumerator = IKeyValueStoreItemEnumeratorPtr(
        (IKeyValueStoreItemEnumerator*)keyValueStoreEnum.get(), 
        keyValueStoreEnum->CreateComponentRoot());

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode KeyValueStoreReplica::EnumerateMetadata(
    ITransactionBasePtr const & transaction,
    __out IKeyValueStoreItemMetadataEnumeratorPtr & itemMetadataEnumerator)
{
    return EnumerateMetadataByKey(
        transaction,
        wstring(),
        false, // strictPrefix
        itemMetadataEnumerator);
}

ErrorCode KeyValueStoreReplica::EnumerateMetadataByKey(
    ITransactionBasePtr const & transaction,
    wstring const & key,
    bool strictPrefix,
    __out IKeyValueStoreItemMetadataEnumeratorPtr & itemMetadataEnumerator)
{
    auto keyValueStoreTx = this->GetCastedTransaction(transaction);

    IStoreBase::EnumerationSPtr replicatedStoreEnumeration;
    auto error = iReplicatedStore_->CreateEnumerationByTypeAndKey(
        keyValueStoreTx->get_ReplicatedStoreTransaction(),
        *Constants::KeyValueStoreItemType,
        key,
        replicatedStoreEnumeration);
    if (!error.IsSuccess()) { return error; }

    auto keyValueStoreEnum = make_shared<KeyValueStoreItemMetadataEnumerator>(key, strictPrefix, replicatedStoreEnumeration);
    itemMetadataEnumerator = IKeyValueStoreItemMetadataEnumeratorPtr(
        (IKeyValueStoreItemMetadataEnumerator*)keyValueStoreEnum.get(), 
        keyValueStoreEnum->CreateComponentRoot());

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode KeyValueStoreReplica::Backup(wstring const & dir)
{
    return iReplicatedStore_->BackupLocal(dir);
}

AsyncOperationSPtr KeyValueStoreReplica::BeginBackup(
    std::wstring const & backupDir,
    FABRIC_STORE_BACKUP_OPTION backupOption,
    IStorePostBackupHandlerPtr const & postBackupHandler,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    StoreBackupOption::Enum storeBackupOption;
    auto createError = StoreBackupOption::FromPublicApi(backupOption, storeBackupOption);
    
    return iReplicatedStore_->BeginBackupLocal(
        backupDir,
        storeBackupOption,
        postBackupHandler,
        callback, 
        parent,
        createError);
}

ErrorCode KeyValueStoreReplica::EndBackup(__in AsyncOperationSPtr const & operation)
{   
    return iReplicatedStore_->EndBackupLocal(operation);
}

ErrorCode KeyValueStoreReplica::Restore(wstring const & dir)
{
    return iReplicatedStore_->RestoreLocal(dir);
}

AsyncOperationSPtr KeyValueStoreReplica::BeginRestore(
    std::wstring const & backupDir,
    RestoreSettings const & restoreSettings,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{        
    return iReplicatedStore_->BeginRestoreLocal(
        backupDir,
        restoreSettings,
        callback,
        parent);
}

ErrorCode KeyValueStoreReplica::EndRestore(__in AsyncOperationSPtr const & operation)
{
    return iReplicatedStore_->EndRestoreLocal(operation);
}

ErrorCode KeyValueStoreReplica::GetQueryStatus(__out IStatefulServiceReplicaStatusResultPtr & result)
{
    return iReplicatedStore_->GetQueryStatus(result);
}

void KeyValueStoreReplica::TestAssertAndTransientFault(std::wstring const & reason) const
{
    TRACE_WARNING_AND_TESTASSERT(
        TraceComponent,
        "{0} TransientFault({1})", 
        this->TraceId,
        reason);

    partition_->ReportFault(FABRIC_FAULT_TYPE_TRANSIENT);
}

void KeyValueStoreReplica::TransientFault(wstring const & reason) const
{
    WriteWarning(
        TraceComponent,
        "{0} TransientFault({1})", 
        this->TraceId,
        reason);

    partition_->ReportFault(FABRIC_FAULT_TYPE_TRANSIENT);
}

KeyValueStoreTransaction * KeyValueStoreReplica::GetCastedTransaction(Api::ITransactionBasePtr const & tx)
{
#ifdef DBG
    return dynamic_cast<KeyValueStoreTransaction *>(tx.get());
#else
    return static_cast<KeyValueStoreTransaction *>(tx.get());
#endif
}

void KeyValueStoreReplica::TrackLifeCycleAssertEvent(wstring const & eventName, TimeSpan const timeout)
{
    deadlockDetector_->TrackAssertEvent(eventName, timeout);
}

void KeyValueStoreReplica::UntrackLifeCycleAssertEvent(wstring const & eventName)
{
    deadlockDetector_->UntrackAssertEvent(eventName);
}
