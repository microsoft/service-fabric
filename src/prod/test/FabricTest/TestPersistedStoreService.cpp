// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace FabricTest;
using namespace std;
using namespace Transport;
using namespace TestCommon;

StringLiteral const TraceSource("TestPersistedStoreService");

wstring const TestPersistedStoreService::StoreDataType = L"PersistedStoreData";
int const TestPersistedStoreService::DestructorWaitDelayInSeconds = 70;

wstring const TestPersistedStoreService::DefaultServiceType = L"TestPersistedStoreServiceType";
wstring const TestPersistedStoreService::TestPersistedStoreDirectory = L"TPS";
wstring const TestPersistedStoreService::SharedLogFilename = L"PersistStore.log";
wstring const TestPersistedStoreService::EseDatabaseFilename = L"PersistStore.edb";

class TestPersistedStoreService::ComProxyDataLossAsyncOperation : public ComProxyAsyncOperation
{
    DENY_COPY(ComProxyDataLossAsyncOperation);

public:
    ComProxyDataLossAsyncOperation(
        ComPointer<IFabricStateProvider> const & stateProvider,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent)
        , stateProvider_(stateProvider)
        , stateChangedResult_(FALSE)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, __out bool & isStateChanged)
    {
        auto casted = AsyncOperation::End<ComProxyDataLossAsyncOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            isStateChanged = (casted->stateChangedResult_ == TRUE);
        }

        return casted->Error;
    }

protected:

    HRESULT BeginComAsyncOperation(__in IFabricAsyncOperationCallback * callback, __out IFabricAsyncOperationContext ** context)
    {
        return stateProvider_->BeginOnDataLoss(callback, context);
    }

    HRESULT EndComAsyncOperation(__in IFabricAsyncOperationContext * context)
    {
        return stateProvider_->EndOnDataLoss(context, &stateChangedResult_);
    }

private:
    ComPointer<IFabricStateProvider> stateProvider_;
    BOOLEAN stateChangedResult_;
};

//
// TestPersistedStoreService
//

TestPersistedStoreService::TestPersistedStoreService(
    Guid const & partitionId,
    FABRIC_REPLICA_ID replicaId,
    wstring const& serviceName,
    Federation::NodeId nodeId,
    bool changeServiceLocationOnChangeRole,
    TestCommon::TestSession & testSession,        
    wstring const& initDataStr,
    wstring const& serviceType,
    wstring const& appId)
    : KeyValueStoreReplica(partitionId, replicaId),
    serviceName_(serviceName),
    testServiceLocation_(),
    partition_(),
    partitionId_(this->PartitionId.ToString()),
    scheme_(::FABRIC_SERVICE_PARTITION_KIND_INVALID),
    highKey_(-1),
    lowKey_(-1),
    partitionName_(),
    nodeId_(nodeId),
    state_(State::Initialized),
    lockMapLock_(),
    lockMap_(),
    changeRoleCalled_(false),
    serviceType_(serviceType),
    appId_(appId),
    testSession_(testSession),
    changeServiceLocationOnChangeRole_(changeServiceLocationOnChangeRole),
    initDataStr_(initDataStr),
    onCopyCompletionDelay_(TimeSpan::Zero),
    faultCopyCompletion_(false),
    disableReplicatorCatchupSpecificReplica_(false)
{
    TestSession::WriteNoise(
        TraceSource, 
        this->PartitionId.ToString(),
        "{0} ctor: name='{1}' initialization-data='{2}'", 
        this->TraceId,
        serviceName, 
        initDataStr_);

    int copyCompletionDelaySeconds = 0;
    ServiceInitDataParser parser(initDataStr_);
    if (parser.GetValue<int>(L"Test_OnCopyCompletionDelaySeconds", copyCompletionDelaySeconds))
    {
        onCopyCompletionDelay_ = TimeSpan::FromSeconds(copyCompletionDelaySeconds);
    }

    bool faultCopyCompletion = false;
    if (parser.GetValue<bool>(L"Test_FaultOnCopyCompletion", faultCopyCompletion))
    {
        faultCopyCompletion_ = faultCopyCompletion;
    }

    bool disableReplicatorCatchupSpecificReplica = false;
    if (parser.GetValue<bool>(L"Test_DisableReplicatorCatchupSpecificReplica", disableReplicatorCatchupSpecificReplica))
    {
        disableReplicatorCatchupSpecificReplica_ = disableReplicatorCatchupSpecificReplica;
    }
}

TestPersistedStoreService::~TestPersistedStoreService()
{
    TestSession::WriteNoise(
        TraceSource, 
        this->PartitionId.ToString(),
        "{0} ~dtor called for Service at {1}: state={2}", 
        this->TraceId,
        nodeId_,
        ToString(state_));

    if (state_ != State::Initialized)
    {
        TestSession::FailTestIfNot(state_ == State::Closed, "Invalid state during destruction");

        testSession_.RemovePendingItem(
            this->GetPendingItemId(),
            true); // skipExistenceCheck (won't exist if open was aborted by RA)
    }
}

ErrorCode TestPersistedStoreService::OnOpen(Common::ComPointer<IFabricStatefulServicePartition> const & partition)
{
    FABRIC_SERVICE_PARTITION_INFORMATION const *partitionInfo;
    HRESULT result = partition->GetPartitionInfo(&partitionInfo);

    if (!SUCCEEDED(result))
    {
        return ErrorCode::FromHResult(result);
    }

    oldPartition_ = partition;
    partition_ = Common::ComPointer<IFabricStatefulServicePartition3>(partition, IID_IFabricStatefulServicePartition3);
    partitionWrapper_.SetPartition(partition_);

    TestSession::FailTestIf(partitionInfo->Value == nullptr, "KeyInfo not found.");
    
    switch (partitionInfo->Kind)
    {
    case ::FABRIC_SERVICE_PARTITION_KIND_INT64_RANGE:
        highKey_ = ((FABRIC_INT64_RANGE_PARTITION_INFORMATION *)partitionInfo->Value)->HighKey;
        lowKey_ = ((FABRIC_INT64_RANGE_PARTITION_INFORMATION *)partitionInfo->Value)->LowKey;
        break;
    case ::FABRIC_SERVICE_PARTITION_KIND_NAMED:
        {
        auto hr = StringUtility::LpcwstrToWstring(((FABRIC_NAMED_PARTITION_INFORMATION *)partitionInfo->Value)->Name, false /*acceptNull*/, partitionName_);
        TestSession::FailTestIf(FAILED(hr), "PartitionName validation failed with {0}", hr);
        break;
        }
    default:
        TestSession::FailTest("KeyType is not of expected scheme, instead {0}.", static_cast<int>(partitionInfo->Kind));
        break;
    }
    
    scheme_ = partitionInfo->Kind;
        
    testServiceLocation_ = Common::Guid::NewGuid().ToString();

    TestSession::WriteNoise(
        TraceSource, 
        this->PartitionId.ToString(),
        "{0} Open called for Service {1} at {2}. ServiceLocation {3}", 
        this->TraceId,
        serviceName_, 
        nodeId_, 
        testServiceLocation_);

    wstring name = this->GetPendingItemId();

    // Wait for a maximum of 30 seconds for previous destructor to get invoked
    // This timeout has to be longer than the naming resolve retry timeout
    TimeoutHelper timeout(TimeSpan::FromSeconds(DestructorWaitDelayInSeconds));
    while(testSession_.ItemExists(name) && !timeout.IsExpired)
    {
        Sleep(2000);
    }

    ComponentRootWPtr weakRoot = this->CreateComponentRoot();
    testSession_.AddPendingItem(name, weakRoot);

    state_ = State::Opened;

    TestServiceInfo testServiceInfo(
        appId_,
        serviceType_, 
        serviceName_, 
        this->PartitionId.ToString(), 
        testServiceLocation_, 
        true, 
        FABRIC_REPLICA_ROLE_NONE);

    // TestServiceFactory will cast ComponentRoot argument to derived service class
    //
    if (OnOpenCallback)
    {
        OnOpenCallback(testServiceInfo, shared_from_this(), true);
    }
    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode TestPersistedStoreService::OnChangeRole(::FABRIC_REPLICA_ROLE newRole, wstring & serviceLocation)
{
    wstring newServiceLocation;
    if(changeServiceLocationOnChangeRole_)
    {
        newServiceLocation = Common::Guid::NewGuid().ToString();
    }

    if (OnChangeRoleCallback)
    {
        OnChangeRoleCallback(testServiceLocation_, newServiceLocation, newRole);
    }

    if(changeServiceLocationOnChangeRole_)
    {
        testServiceLocation_ = newServiceLocation;
    }

    serviceLocation = testServiceLocation_;
    changeRoleCalled_ = true;

    TestSession::WriteNoise(
        TraceSource, 
        this->PartitionId.ToString(),
        "{0} ChangeRole called for Service {1} at {2} to {3}. ServiceLocation {4}",
        this->TraceId,
        serviceName_, 
        nodeId_, 
        static_cast<int>(newRole), 
        serviceLocation);

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode TestPersistedStoreService::OnClose()
{
    TestSession::WriteNoise(
        TraceSource, 
        this->PartitionId.ToString(),
        "{0} Close called for Service {1} at {2}: location={3}",
        this->TraceId,
        serviceName_, 
        nodeId_,
        testServiceLocation_);

    testSession_.ClosePendingItem(this->GetPendingItemId());

    if (OnCloseCallback && !testServiceLocation_.empty())
    {
        OnCloseCallback(testServiceLocation_);
    }

    Cleanup();

    return ErrorCode(ErrorCodeValue::Success);
}

void TestPersistedStoreService::OnAbort()
{
    TestSession::WriteNoise(
        TraceSource,
        this->PartitionId.ToString(),
        "{0} Abort called for Service {1} at {2}",
        this->TraceId,
        serviceName_, 
        nodeId_);

    this->OnClose().ReadValue();
}

void TestPersistedStoreService::Cleanup()
{
    state_ = State::Closed;

    partition_.Release();
    OnOpenCallback = nullptr;
    OnCloseCallback = nullptr;
    OnChangeRoleCallback = nullptr;

    {
        AcquireWriteLock lock(stateProviderLock_);

        stateProvider_.Release();
    }

    this->TryStartLeakDetectionTimer(TimeSpan::FromSeconds(DestructorWaitDelayInSeconds - 10));
}

ErrorCode TestPersistedStoreService::OnCopyComplete(Api::IStoreEnumeratorPtr const &)
{
    if (faultCopyCompletion_)
    {
        Random rand;
        if (rand.Next() % 2 == 0)
        {
            TestSession::WriteInfo(
                TraceSource, 
                this->PartitionId.ToString(),
                "{0} OnCopyComplete(): injecting OperationFailed fault",
                this->TraceId);

            return ErrorCodeValue::OperationFailed;
        }
        else
        {
            TestSession::WriteInfo(
                TraceSource, 
                this->PartitionId.ToString(),
                "{0} OnCopyComplete(): skipping fault injection",
                this->TraceId);
        }
    }

    if (onCopyCompletionDelay_ > TimeSpan::Zero)
    {
        TestSession::WriteInfo(
            TraceSource, 
            this->PartitionId.ToString(),
            "{0} OnCopyComplete(): delaying for {1}",
            this->TraceId,
            onCopyCompletionDelay_);

        Sleep(static_cast<DWORD>(onCopyCompletionDelay_.TotalMilliseconds()));
    }

    return ErrorCodeValue::Success;
}

ErrorCode TestPersistedStoreService::OnReplicationOperation(Api::IStoreNotificationEnumeratorPtr const &)
{
    if (IsSignalSet(PumpReplicationBlock))
    {
        WaitForSignalReset(PumpReplicationBlock);
    }

    if (ShouldFailOn(ApiFaultHelper::ComponentName::ServiceOperationPump, L"replication", ApiFaultHelper::FaultType::ReportFaultTransient))
    {
        ReportStreamFault(FABRIC_FAULT_TYPE_TRANSIENT);
    }

    if (ShouldFailOn(ApiFaultHelper::ComponentName::ServiceOperationPump, L"replication", ApiFaultHelper::FaultType::ReportFaultPermanent))
    {
        ReportStreamFault(FABRIC_FAULT_TYPE_PERMANENT);
    }

    if (ShouldFailOn(ApiFaultHelper::ComponentName::ServiceOperationPump, L"replication", ApiFaultHelper::FaultType::Delay))
    {
        ::Sleep(static_cast<DWORD>(ApiFaultHelper::Get().GetApiDelayInterval().TotalMilliseconds()));
    }

    return ErrorCode();
}

ErrorCode TestPersistedStoreService::Put(
    /* [in] */ __int64 key,
    /* [in] */ std::wstring const& value,
    /* [in] */ FABRIC_SEQUENCE_NUMBER currentVersion,
    /* [in] */ std::wstring serviceName,
    /* [retval][out] */ FABRIC_SEQUENCE_NUMBER & newVersion,
    /* [retval][out] */ ULONGLONG & dataLossVersion)
{
    TestSession::WriteNoise(
        TraceSource, 
        this->PartitionId.ToString(),
        "{0} Put called for Service. key {1}, value {2}, current version {3}", 
        this->TraceId,
        key, 
        value, 
        currentVersion);

    TestSession::FailTestIf(scheme_ != FABRIC_SERVICE_PARTITION_KIND_INT64_RANGE, "Put: Only INT64 scheme is supported");
    
    if(!VerifyServiceLocation(key, serviceName))
    {
        return ErrorCode(ErrorCodeValue::FabricTestIncorrectServiceLocation);
    }

    FABRIC_EPOCH currentEpoch = {0};
    auto error = this->GetCurrentEpoch(currentEpoch);
    if (!error.IsSuccess()) { return error; }

    dataLossVersion = currentEpoch.DataLossNumber;

    auto keyString = wformatString("{0}", key);

    vector<byte> buffer;
    VectorStream writeStream(buffer);
    writeStream << value;

    AcquireWriteLock grab(*GetLock(key));
    Store::IStoreBase::TransactionSPtr txSPtr;
    Store::IStoreBase::EnumerationSPtr enumSPtr(nullptr);

    error = static_cast<Store::IStoreBase *>(ReplicatedStore)->CreateTransaction(txSPtr);
    _int64 currentSequenceNumber = 0;

    if (!error.IsSuccess())
    {
        return error;
    }

    error = GetCurrentOperationLSN(txSPtr, keyString, currentSequenceNumber, enumSPtr);

    if (error.IsSuccess())
    {
        if(currentVersion != currentSequenceNumber)
        {
            TestSession::WriteNoise(
                TraceSource, 
                this->PartitionId.ToString(),
                "{0} Version does not match for key {1}. expected {2}, actual {3}", 
                this->TraceId,
                key, 
                currentVersion, 
                currentSequenceNumber);

            newVersion = currentSequenceNumber;
            return ErrorCode(ErrorCodeValue::FabricTestVersionDoesNotMatch);
        }

        error = ReplicatedStore->Update(
            txSPtr, 
            StoreDataType,
            keyString,
            currentSequenceNumber,
            keyString,
            buffer.data(),
            buffer.size());
        
        TestSession::WriteNoise(
            TraceSource, 
            this->PartitionId.ToString(),
            "{0} Updated data item {1}:{2} with error {3}",
            this->TraceId,
            key,
            value,
            error);
    }
    else if (error.ReadValue() == ErrorCodeValue::FabricTestKeyDoesNotExist)
    {
        if(currentVersion != 0)
        {
            TestSession::WriteNoise(
                TraceSource, 
                this->PartitionId.ToString(),
                "{0} Version does not match for key {1}. expected {2}, actual 0", 
                this->TraceId,
                key, 
                currentVersion);
            newVersion = 0;
            return ErrorCode(ErrorCodeValue::FabricTestVersionDoesNotMatch);
        }

        error = ReplicatedStore->Insert(
            txSPtr, 
            StoreDataType,
            keyString,
            buffer.data(),
            buffer.size());
        
        TestSession::WriteNoise(
            TraceSource, 
            this->PartitionId.ToString(),
            "{0} inserted data item {1}:{2} with error {3}",
            this->TraceId,
            key,
            value,
            error);
    }

    if(error.IsSuccess())
    {
        // Wait for 5 minutes for replication to complete else the FailoverUnit must be stuck
        // in quorum loss so timeout
        TimeSpan replicationTimeout(FabricTestCommonConfig::GetConfig().StoreReplicationTimeout);

        FABRIC_SEQUENCE_NUMBER commitSequenceNumber;
        error = txSPtr->Commit(commitSequenceNumber, replicationTimeout);

        if(error.IsSuccess())
        {
            newVersion = commitSequenceNumber;

            TestSession::WriteNoise(
                TraceSource, 
                this->PartitionId.ToString(),
                "{0} committed data item {1}:{2} lsn={3} with error {4}",
                this->TraceId,
                key,
                value,
                commitSequenceNumber,
                error);
        }
        else
        {
            TestSession::WriteWarning(TraceSource, this->PartitionId.ToString(), "Failed to commit txn in Put due to {0}", error);
        }
    }
    else
    {
        txSPtr->Rollback();
    }

    return error;
}

ErrorCode TestPersistedStoreService::Get(
    /* [in] */ __int64 key,
    /* [in] */ std::wstring serviceName,
    /* [retval][out] */ std::wstring & value,
    /* [retval][out] */ FABRIC_SEQUENCE_NUMBER & version,
    /* [retval][out] */ ULONGLONG & dataLossVersion)
{
    TestSession::WriteNoise(TraceSource, this->PartitionId.ToString(), "{0} Get called for Service. key {1}", this->TraceId, key);
    TestSession::FailTestIf(scheme_ != FABRIC_SERVICE_PARTITION_KIND_INT64_RANGE, "Get: Only INT64 scheme is supported");

    if(!VerifyServiceLocation(key, serviceName))
    {
        return ErrorCode(ErrorCodeValue::FabricTestIncorrectServiceLocation);
    }

    auto keyString = wformatString("{0}", key);

    FABRIC_EPOCH currentEpoch = {0};
    auto error = this->GetCurrentEpoch(currentEpoch);
    if (!error.IsSuccess()) { return error; }

    dataLossVersion = currentEpoch.DataLossNumber;

    AcquireReadLock grab(*GetLock(key));
    Store::IStoreBase::TransactionSPtr txSPtr;
    Store::IStoreBase::EnumerationSPtr enumSPtr(nullptr);

    error = static_cast<Store::IStoreBase *>(ReplicatedStore)->CreateTransaction(txSPtr);

    _int64 currentSequenceNumber = 0;

    if (error.IsSuccess())
    {
        error = GetCurrentOperationLSN(txSPtr, keyString, currentSequenceNumber, enumSPtr);

        if (error.IsSuccess())
        {
            if ((error = ReadCurrentData(enumSPtr, value)).IsSuccess())
            {
                version = currentSequenceNumber;
                error = txSPtr->Commit();
                if(error.IsSuccess())
                {
                    TestSession::WriteNoise(
                        TraceSource, 
                        this->PartitionId.ToString(),
                        "{0} Get operation for key {1} succeeded. value {2}, version {3}, dataLoss {4}", 
                        this->TraceId,
                        key, 
                        value, 
                        version, 
                        currentEpoch.DataLossNumber);
                }
            }
        }

        if(!error.IsSuccess())
        {
            txSPtr->Rollback();
        }
    }

    return error;
}

ErrorCode TestPersistedStoreService::Delete(
    /* [in] */ __int64 key,
    /* [in] */ FABRIC_SEQUENCE_NUMBER currentVersion,
    /* [in] */ std::wstring serviceName,
    /* [retval][out] */ FABRIC_SEQUENCE_NUMBER & newVersion,
    /* [retval][out] */ ULONGLONG & dataLossVersion)
{
    TestSession::WriteNoise(TraceSource, this->PartitionId.ToString(), "{0} Delete called for Service. key {1}, current version {2}", this->TraceId, key, currentVersion);
    TestSession::FailTestIf(scheme_ != FABRIC_SERVICE_PARTITION_KIND_INT64_RANGE, "Delete: Only INT64 scheme is supported");
    
    if(!VerifyServiceLocation(key, serviceName))
    {
        return ErrorCode(ErrorCodeValue::FabricTestIncorrectServiceLocation);
    }

    FABRIC_EPOCH currentEpoch = {0};
    auto error = this->GetCurrentEpoch(currentEpoch);
    if (!error.IsSuccess()) { return error; }

    dataLossVersion = currentEpoch.DataLossNumber;

    auto keyString = wformatString("{0}", key);

    AcquireWriteLock grab(*GetLock(key));
    Store::IStoreBase::TransactionSPtr txSPtr;
    Store::IStoreBase::EnumerationSPtr enumSPtr(nullptr);

    error = static_cast<Store::IStoreBase *>(ReplicatedStore)->CreateTransaction(txSPtr);
    _int64 currentSequenceNumber = 0;

    if (!error.IsSuccess())
    {
        return error;
    }

    error = GetCurrentOperationLSN(txSPtr, keyString, currentSequenceNumber, enumSPtr);

    if (error.IsSuccess())
    {
        if(currentVersion != currentSequenceNumber)
        {
            TestSession::WriteNoise(
                TraceSource, 
                this->PartitionId.ToString(),
                "{0} Version does not match for key {1}. expected {2}, actual {3}", 
                this->TraceId,
                key, 
                currentVersion, 
                currentSequenceNumber);
            newVersion = currentSequenceNumber;
            return ErrorCode(ErrorCodeValue::FabricTestVersionDoesNotMatch);
        }

        error = ReplicatedStore->Delete(
            txSPtr, 
            StoreDataType,
            keyString,
            currentSequenceNumber);
        
        TestSession::WriteNoise(
            TraceSource, 
            this->PartitionId.ToString(),
            "{0} Deleted data item {1} with error {2}",
            this->TraceId,
            key,
            error);
    }

    if(error.IsSuccess())
    {
        // Wait for 5 minutes for replication to complete else the FailoverUnit must be stuck
        // in quorum loss so timeout
        TimeSpan replicationTimeout(FabricTestCommonConfig::GetConfig().StoreReplicationTimeout);

        FABRIC_SEQUENCE_NUMBER commitSequenceNumber;
        error = txSPtr->Commit(commitSequenceNumber, replicationTimeout);

        if(error.IsSuccess())
        {
            newVersion = commitSequenceNumber;
        }
        else
        {
            TestSession::WriteWarning(TraceSource, this->PartitionId.ToString(), "Failed to commit txn in Delete due to {0}", error);
        }
    }
    else
    {
        txSPtr->Rollback();
    }

    return error;
}

Store::ReplicatedStore * TestPersistedStoreService::get_ReplicatedStorePrivate() const
{
    auto casted = this->TryGetReplicatedStorePrivate();
    TestSession::FailTestIf(casted == nullptr, "ReplicatedStore cast failed");

    return casted;
}

Store::ReplicatedStore * TestPersistedStoreService::TryGetReplicatedStorePrivate() const
{
    return dynamic_cast<Store::ReplicatedStore*>(this->ReplicatedStore);
}

void TestPersistedStoreService::SetSecondaryPumpEnabled(bool value) 
{ 
    UNREFERENCED_PARAMETER(value);

    TestSession::FailTest("SetSecondaryPumpEnabled not supported - use 'setsignal <node> <service> provider.secondarypump.block' instead");
}

void TestPersistedStoreService::ReportStreamFault(FABRIC_FAULT_TYPE faultType)
{
    auto casted = this->TryGetReplicatedStorePrivate();

    if (casted == nullptr)
    {
        TestSession::WriteError(
            TraceSource, 
            this->PartitionId.ToString(), 
            "failed to cast Store::ReplicatedStore");
    }
    else
    {
        casted->Test_TryFaultStreamAndStopSecondaryPump(faultType);
    }
}

ErrorCode TestPersistedStoreService::Backup(std::wstring const & dir)
{
    return ReplicatedStorePrivate->BackupLocal(dir);
}

ErrorCode TestPersistedStoreService::Restore(std::wstring const & dir)
{
    return ReplicatedStorePrivate->RestoreLocal(dir);
}

Common::AsyncOperationSPtr TestPersistedStoreService::BeginBackup(
	__in std::wstring const & backupDir,
	__in StoreBackupOption::Enum backupOption,
	__in Api::IStorePostBackupHandlerPtr const & postBackupHandler,
	__in Common::AsyncCallback const & callback,
	__in Common::AsyncOperationSPtr const & parent)
{
	return ReplicatedStorePrivate->BeginBackupLocal(backupDir, backupOption, postBackupHandler, callback, parent);
}

ErrorCode TestPersistedStoreService::EndBackup(
	__in Common::AsyncOperationSPtr const & operation)
{
	return ReplicatedStorePrivate->EndBackupLocal(operation);	
}

Common::AsyncOperationSPtr TestPersistedStoreService::BeginRestore(
    __in std::wstring const & backupDir,
    __in Common::AsyncCallback const & callback,
    __in Common::AsyncOperationSPtr const & parent)
{
    return ReplicatedStorePrivate->BeginRestoreLocal(backupDir, Store::RestoreSettings(), callback, parent);
}

ErrorCode TestPersistedStoreService::EndRestore(
    __in Common::AsyncOperationSPtr const & operation)
{
    return ReplicatedStorePrivate->EndRestoreLocal(operation);
}

ErrorCode TestPersistedStoreService::CompressionTest(std::vector<std::wstring> const & params)
{
    size_t minParamCount = 1;
    if (params.size() < minParamCount)
    {
        TestSession::WriteWarning(
            TraceSource, 
            this->PartitionId.ToString(), 
            "token count must be at least {0}: actual = {1} ({2})",
            minParamCount,
            params,
            params.size());

        return ErrorCodeValue::InvalidArgument;
    }

    wstring mode = params[0];

    if (mode == L"r")
    {
        return CompressionReadTest(params);
    }
    else if (mode == L"w")
    {
        return CompressionWriteTest(params);
    }
    else
    {
        TestSession::WriteWarning(
            TraceSource, 
            this->PartitionId.ToString(), 
            "mode must be one of [r, w] (read/write): actual = {0}",
            mode);

        return ErrorCodeValue::InvalidArgument;
    }
}

ErrorCode TestPersistedStoreService::CompressionWriteTest(std::vector<std::wstring> const & params)
{
    size_t minParamCount = 8;
    if (params.size() < minParamCount)
    {
        TestSession::WriteWarning(
            TraceSource, 
            this->PartitionId.ToString(), 
            "token count must be at least {0}: actual = {1} ({2})",
            minParamCount,
            params,
            params.size());

        return ErrorCodeValue::InvalidArgument;
    }

    wstring typeString(L"CompressionTestType");

    bool useRandomData = (params[1] == L"r");
    bool useCompression = (params[2] == L"c");
    int64 blockSizeMB = Int64_Parse(params[3]);
    wstring keyString = params[4];
    int64 bufferSize = Int64_Parse(params[5]);
    int64 operationCount = Int64_Parse(params[6]);
    int64 iterationCount = Int64_Parse(params[7]);

    Random rand;

    vector<byte> buffer;
    buffer.reserve(bufferSize);
    for (auto ix=0; ix<bufferSize; ++ix)
    {
        if (useRandomData)
        {
            buffer.push_back(rand.NextByte());
        }
        else
        {
            buffer.push_back(static_cast<byte>(ix));
        }
    }

    TestSession::WriteInfo(
        TraceSource, 
        this->PartitionId.ToString(),
        "{0} Testing ({1}, {2}, {3}MB): buffer size = {4} operations = {5} iterations = {6}",
        this->TraceId,
        useRandomData ? "random" : "sequential",
        useCompression ? "compress" : "no-compress",
        blockSizeMB,
        bufferSize,
        operationCount,
        iterationCount);

    Stopwatch overallStopWatch;
    Stopwatch commitStopWatch;

    overallStopWatch.Start();

    for (auto iteration=0; iteration<iterationCount; ++iteration)
    {
        Store::IStoreBase::TransactionSPtr txSPtr;
        ErrorCode error;

        // TODO: Apply useCompression flag to settings once available

        Store::TransactionSettings settings(static_cast<ULONG>(blockSizeMB * 1024 * 1024));

        error = ReplicatedStore->CreateTransaction(
            ActivityId(),
            settings,
            txSPtr); 

        if (!error.IsSuccess()) { return error; }

        for (auto ix=0; ix<operationCount; ++ix)
        {
            wstring key = wformatString("{0}_{1}", keyString, ix);

            if (iteration == 0)
            {
                error = ReplicatedStore->Insert(
                    txSPtr, 
                    typeString,
                    key,
                    buffer.data(),
                    buffer.size());
            }
            else
            {
                error = ReplicatedStore->Update(
                    txSPtr, 
                    typeString,
                    key,
                    0,
                    key,
                    buffer.data(),
                    buffer.size());
            }

            if (!error.IsSuccess())
            {
                TestSession::WriteWarning(
                    TraceSource, 
                    this->PartitionId.ToString(),
                    "Insert/Update failed: error = {0}",
                    error);
                return error;
            }
        }

        commitStopWatch.Start();

        FABRIC_SEQUENCE_NUMBER commitSequenceNumber;
        error = txSPtr->Commit(commitSequenceNumber);

        commitStopWatch.Stop();

        if (!error.IsSuccess())
        {
            TestSession::WriteWarning(
                TraceSource, 
                this->PartitionId.ToString(),
                "Commit failed: error = {0}",
                error);
            return error;
        }
    }

    overallStopWatch.Stop();

    TestSession::WriteInfo(
        TraceSource, 
        this->PartitionId.ToString(),
        "{0} Write Results: buffer size = {1} operations = {2} iterations = {3} overall = {4} per-tx = {5}ms commit = {6} per-txt = {7}ms",
        this->TraceId,
        bufferSize,
        operationCount,
        iterationCount,
        overallStopWatch.Elapsed,
        overallStopWatch.ElapsedMilliseconds / iterationCount,
        commitStopWatch.Elapsed,
        commitStopWatch.ElapsedMilliseconds / iterationCount);

    return ErrorCodeValue::Success;
}

ErrorCode TestPersistedStoreService::CompressionReadTest(vector<wstring> const & params)
{
    size_t minParamCount = 5;
    if (params.size() < minParamCount)
    {
        TestSession::WriteWarning(
            TraceSource, 
            this->PartitionId.ToString(), 
            "token count must be at least {0}: actual = {1} ({2})",
            minParamCount,
            params,
            params.size());

        return ErrorCodeValue::InvalidArgument;
    }

    wstring typeString(L"CompressionTestType");

    bool useSequential = (params[1] == L"s");
    wstring keyString = params[2];
    int64 bufferSize = Int64_Parse(params[3]);
    int64 operationCount = Int64_Parse(params[4]);

    Store::IStoreBase::TransactionSPtr txSPtr;
    Store::IStoreBase::EnumerationSPtr enumSPtr;

    auto error = static_cast<Store::IStoreBase *>(ReplicatedStore)->CreateTransaction(txSPtr);
    if (!error.IsSuccess()) { return error; }

    error = ReplicatedStore->CreateEnumerationByTypeAndKey(txSPtr, typeString, keyString, enumSPtr);
    if (!error.IsSuccess()) { return error; }

    int readCount = 0;

    Stopwatch stopwatch;
    stopwatch.Start();

    while ((error = enumSPtr->MoveNext()).IsSuccess())
    {
        vector<byte> buffer;
        error = enumSPtr->CurrentValue(buffer);

        if (!error.IsSuccess())
        {
            TestSession::WriteWarning(
                TraceSource, 
                this->PartitionId.ToString(),
                "CurrentValue failed: error = {0}",
                error);
            return error;
        }
        else if (buffer.size() != static_cast<size_t>(bufferSize))
        {
            TestSession::WriteWarning(
                TraceSource, 
                this->PartitionId.ToString(),
                "Data size mismatch: actual {0} != expected {1}",
                buffer.size(),
                bufferSize);
            return ErrorCodeValue::OperationFailed;
        }

        if (useSequential)
        {
            byte expectedByte = 0;

            for (auto it=buffer.begin(); it!=buffer.end(); ++it)
            {
                if (*it != expectedByte++)
                {
                    TestSession::WriteWarning(
                        TraceSource, 
                        this->PartitionId.ToString(),
                        "Data byte mismatch: actual {0} != expected {1}",
                        *it,
                        expectedByte);
                    return ErrorCodeValue::OperationFailed;
                }
            }
        }

        ++readCount;
    }

    stopwatch.Stop();

    if (readCount != operationCount)
    {
        TestSession::WriteWarning(
            TraceSource, 
            this->PartitionId.ToString(),
            "Operation count mismatch: actual {0} != expected {1}",
            readCount,
            operationCount);
        return ErrorCodeValue::OperationFailed;
    }

    TestSession::WriteInfo(
        TraceSource, 
        this->PartitionId.ToString(),
        "{0} Read Results: buffer size = {1} operations = {2} elapsed = {3} per-op = {4}ms",
        this->TraceId,
        bufferSize,
        readCount,
        stopwatch.Elapsed,
        stopwatch.ElapsedMilliseconds / readCount);

    return ErrorCodeValue::Success;
}

bool TestPersistedStoreService::VerifyServiceLocation(__int64 key, wstring const& serviceName)
{
    __int64 partitionKey = static_cast<__int64>(key);
    if((serviceName_.compare(serviceName) == 0) && partitionKey >= lowKey_ && partitionKey <= highKey_)
    {
        return true;
    }

    TestSession::WriteInfo(
        TraceSource, 
        this->PartitionId.ToString(),
        "{0} Actual Service Name {1}, Expected {2}. key {3}, Range.High {4} and Range.Low {5}",
        this->TraceId,
        serviceName_, 
        serviceName, 
        key, 
        highKey_, 
        lowKey_);

    return false;
}

ErrorCode TestPersistedStoreService::ReadCurrentData(Store::IStoreBase::EnumerationSPtr const & enumSPtr, wstring & value)
{
    WriteNoise(
        TraceSource,
        this->PartitionId.ToString(),
        "{0} Entering TestPersistedStoreService::ReadCurrentData", 
        this->TraceId);

    std::vector<byte> buffer;
    ErrorCode error = enumSPtr->CurrentValue(buffer);

    if (error.IsSuccess())
    {
        VectorStream readStream(buffer);
        readStream >> value;

        WriteNoise(
            TraceSource,
            this->PartitionId.ToString(),
            "{0} Leaving TestPersistedStoreService::ReadCurrentData. Value: {1}", 
            this->TraceId, 
            value);
    }
    else
    {
        WriteNoise(
            TraceSource,
            this->PartitionId.ToString(),
            "{0} Leaving TestPersistedStoreService::ReadCurrentData. Error: {1}", 
            this->TraceId, 
            error);
    }

    return error;
}

ErrorCode TestPersistedStoreService::GetCurrentOperationLSN(
    Store::IStoreBase::TransactionSPtr const & transactionSPtr, 
    wstring const & key, 
    _int64 & currentSequenceNumber, 
    Store::IStoreBase::EnumerationSPtr & enumSPtr)
{
    ErrorCode error = ReplicatedStore->CreateEnumerationByTypeAndKey(transactionSPtr, StoreDataType, key, enumSPtr);

    if (error.IsSuccess())
    {
        error = SeekToDataItem(key, enumSPtr);
    }

    if(error.IsSuccess())
    {
        error = enumSPtr->CurrentOperationLSN(currentSequenceNumber);
    }

    return error;
}

ErrorCode TestPersistedStoreService::SeekToDataItem(wstring const & key, Store::IStoreBase::EnumerationSPtr const & enumSPtr)
{
    ErrorCode error = enumSPtr->MoveNext();
    if (!error.IsSuccess())
    {
        TestSession::WriteNoise(
            TraceSource, 
            this->PartitionId.ToString(),
            "{0} enumerating data item {1} with error {2}",
            this->TraceId,
            key,
            error);
        if(error.ReadValue() == ErrorCodeValue::EnumerationCompleted)
        {
            return ErrorCode(ErrorCodeValue::FabricTestKeyDoesNotExist);
        }
        else
        {
            return error;
        }
    }

    wstring currentKey;
    if ((error = enumSPtr->CurrentKey(currentKey)).IsSuccess())
    {
        if (currentKey != key)
        {
            TestSession::WriteNoise(
                TraceSource, 
                this->PartitionId.ToString(),
                "{0} data item {1} not found",
                this->TraceId,
                key,
                error);
            return ErrorCode(ErrorCodeValue::FabricTestKeyDoesNotExist);
        }
    }

    return error;
}

shared_ptr<RwLock> TestPersistedStoreService::GetLock(__int64 key)
{
    AcquireExclusiveLock lock(lockMapLock_);
    auto iterator = lockMap_.find(key);
    if(iterator == lockMap_.end())
    {
        shared_ptr<RwLock> rwLock = make_shared<RwLock>();
        lockMap_[key] = rwLock;
    }

    return lockMap_[key];
}

Common::ErrorCode TestPersistedStoreService::GetBackupOption(
	__in FABRIC_STORE_BACKUP_OPTION backupOption,
	__out StoreBackupOption::Enum & storeBackupOption)
{
	switch (backupOption)
	{
	case FABRIC_STORE_BACKUP_OPTION_FULL:
		storeBackupOption = StoreBackupOption::Enum::Full;
		return ErrorCode::Success();

	case FABRIC_STORE_BACKUP_OPTION_INCREMENTAL:
		storeBackupOption = StoreBackupOption::Enum::Incremental;
		return ErrorCode::Success();

	case FABRIC_STORE_BACKUP_OPTION_TRUNCATE_LOGS_ONLY:
		storeBackupOption = StoreBackupOption::Enum::TruncateLogsOnly;
		return ErrorCode::Success();

	default:
		TestSession::WriteError(
			TraceSource,
			this->PartitionId.ToString(),
			"Invalid FABRIC_STORE_BACKUP_OPTION {0} specified",
			static_cast<int64>(backupOption));
		storeBackupOption = StoreBackupOption::Enum::Full;
		return Common::ErrorCode(Common::ErrorCodeValue::InvalidArgument);
	}
}

wstring TestPersistedStoreService::GetPendingItemId() const
{
    return wformatString("PersistedService.{0}-{1}-{2}", this->PartitionId, this->ReplicaId, nodeId_);
}

void TestPersistedStoreService::SetStateProvider(__in IFabricStateProvider * provider)
{
    AcquireWriteLock lock(stateProviderLock_);

    if (provider == nullptr)
    {
        stateProvider_.Release();
    }
    else
    {
        auto hr = provider->QueryInterface(IID_IFabricStateProvider, stateProvider_.VoidInitializationAddress());
        ASSERT_IF(FAILED(hr), "QueryInterface(IID_IFabricStateProvider) failed: hr={0}", hr);
    }
}

ComPointer<IFabricStateProvider> TestPersistedStoreService::GetStateProvider()
{
    AcquireReadLock lock(stateProviderLock_);

    return stateProvider_;
}

AsyncOperationSPtr TestPersistedStoreService::BeginOnDataLoss(
    __in AsyncCallback const & callback,
    __in AsyncOperationSPtr const & parent)
{
    auto stateProvider = this->GetStateProvider();

    if (stateProvider.GetRawPointer() == nullptr)
    {
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(callback, parent);
    }
    else
    {
        return AsyncOperation::CreateAndStart<ComProxyDataLossAsyncOperation>(
            stateProvider,
            callback,
            parent);
    }
}

ErrorCode TestPersistedStoreService::EndOnDataLoss(
    __in AsyncOperationSPtr const & operation,
    __out bool & isStateChanged)
{
    auto casted = dynamic_pointer_cast<ComProxyDataLossAsyncOperation>(operation);

    if (casted.get() == nullptr)
    {
        isStateChanged = false;

        return CompletedAsyncOperation::End(operation);
    }
    else
    {
        return ComProxyDataLossAsyncOperation::End(operation, isStateChanged);
    }
}

ErrorCode TestPersistedStoreService::GetReplicationQueueCounters(
    __out FABRIC_INTERNAL_REPLICATION_QUEUE_COUNTERS & counters)
{
    return ErrorCode::FromHResult(this->ReplicatedStorePrivate->InnerInternalReplicator->GetReplicationQueueCounters(&counters));
}

wstring TestPersistedStoreService::ToString(State const & e)
{
    switch (e)
    {
    case State::Initialized: return L"Initialized";
    case State::Opened: return L"Opened";
    case State::Closed: return L"Closed";
    }

    return wformatString("State({0})", static_cast<int>(e));
}

