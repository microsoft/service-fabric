// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <SfStatus.h>

using namespace Common;
using namespace FabricTest;
using namespace std;
using namespace Transport;
using namespace TestCommon;
using namespace TxnReplicator::TestCommon;

StringLiteral const TraceSource("TXRService");

wstring const TXRService::DefaultServiceType = L"TXRServiceType"; 

TXRService::TXRService(
    FABRIC_PARTITION_ID partitionId,
    FABRIC_REPLICA_ID replicaId,
    wstring const& serviceName,
    Federation::NodeId nodeId,
    wstring const& workingDir,
    TestSession & testSession,
    wstring const& initDataStr,
    wstring const& serviceType,
    wstring const& appId)
    : ITestStoreService()
    , partitionId_(Guid(partitionId).ToString())
    , replicaId_(replicaId)
    , serviceName_(serviceName)
    , testServiceLocation_()
    , partition_()
    , scheme_(::FABRIC_SERVICE_PARTITION_KIND_INVALID)
    , highKey_(-1)
    , lowKey_(-1)
    , partitionName_()
    , nodeId_(nodeId)
    , workDir_(workingDir)
    , state_(State::Initialized)
    , serviceType_(serviceType)
    , appId_(appId)
    , testSession_(testSession)
    , initDataStr_(initDataStr)
    , partitionWrapper_()
    , transactionalReplicator_()
{
    TestSession::WriteNoise(TraceSource, partitionId_, "TXRService created with name: {0} initialization-data:'{1}'", serviceName, initDataStr_);
}

TXRService::~TXRService()
{
    TestSession::WriteNoise(TraceSource, GetPartitionId(), "Dtor called for Service at {0}", nodeId_);

    if(state_ != State::Initialized)
    {
        TestSession::FailTestIfNot(state_ == State::Closed, "Invalid state during destruction");
        testSession_.RemovePendingItem(wformatString("TXRService.{0}-{1}-{2}", GetPartitionId(), replicaId_, nodeId_));
    }
}

TxnReplicator::ITransactionalReplicator::SPtr TXRService::GetTxnReplicator()
{
    TestSession::FailTestIfNot(transactionalReplicator_.RawPtr() != nullptr, "Invalid txnreplicator");
    return transactionalReplicator_;
}

KAllocator& TXRService::GetAllocator()
{
    TestSession::FailTestIfNot(allocator_ != nullptr, "Invalid allocator");
    return *allocator_;
}

::FABRIC_SERVICE_PARTITION_KIND TXRService::GetScheme()
{
    return scheme_;
}

wstring TXRService::GetSPNameFromKey(__int64 key)
{
    return wformatString(
        "{0}:{1}",
        TxnReplicator::TestCommon::TestStateProvider::FabricTestStateProviderNamePrefix , 
        key);
}

ErrorCode TXRService::OnOpen(
    __in ComPointer<IFabricStatefulServicePartition> const & partition,
    __in TxnReplicator::ITransactionalReplicator & transactionalReplicator,
    __in Common::ComPointer<IFabricPrimaryReplicator> & primaryReplicator,
    __in KAllocator & allocator)
{
    FABRIC_SERVICE_PARTITION_INFORMATION const *partitionInfo;
    HRESULT result = partition->GetPartitionInfo(&partitionInfo);

    if (!SUCCEEDED(result))
    {
        return ErrorCode::FromHResult(result);
    }

    oldPartition_ = partition;
    transactionalReplicator_ = &transactionalReplicator;
    primaryReplicator_ = primaryReplicator;
    allocator_ = &allocator;
    partition_ = ComPointer<IFabricStatefulServicePartition3>(partition, IID_IFabricStatefulServicePartition3);
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
        
    testServiceLocation_ = Guid::NewGuid().ToString();

    TestSession::WriteNoise(TraceSource, GetPartitionId(),
        "Open called for Service {0} at {1}. ServiceLocation {2}", 
        serviceName_, nodeId_, testServiceLocation_);

    wstring name = wformatString("TXRService.{0}-{1}-{2}", GetPartitionId(), replicaId_, nodeId_);

    // Wait for a maximum of 2 Client API duration in seconds for previous destructor to get invoked
    TimeoutHelper timeout(FabricTestSessionConfig::GetConfig().TXRServiceOperationTimeout + FabricTestSessionConfig::GetConfig().TXRServiceOperationTimeout);
    while(testSession_.ItemExists(name) && !timeout.IsExpired)
    {
        Sleep(2000);
    }

    testSession_.AddPendingItem(name);

    state_ = State::Opened;

    auto root = shared_from_this();
    TestServiceInfo testServiceInfo(
        appId_,
        serviceType_, 
        serviceName_, 
        GetPartitionId(), 
        testServiceLocation_, 
        true, 
        FABRIC_REPLICA_ROLE_NONE);

    OnOpenCallback(testServiceInfo, root, true);
    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode TXRService::OnChangeRole(::FABRIC_REPLICA_ROLE newRole)
{
    wstring newServiceLocation = Common::Guid::NewGuid().ToString();
    OnChangeRoleCallback(testServiceLocation_, newServiceLocation, newRole);
    testServiceLocation_ = newServiceLocation;

    TestSession::WriteNoise(TraceSource, GetPartitionId(),
         "ChangeRole called for Service {0} at {1} to {2}. ServiceLocation {3}",
         serviceName_, nodeId_, static_cast<int>(newRole), testServiceLocation_);

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode TXRService::OnClose()
{
    TestSession::WriteNoise(TraceSource, GetPartitionId(),
        "Close called for Service {0} at {1}",
        serviceName_, nodeId_);

    testSession_.ClosePendingItem(wformatString("TXRService.{0}-{1}-{2}", GetPartitionId(), replicaId_, nodeId_));

    OnCloseCallback(testServiceLocation_);
    Cleanup();
    return ErrorCode(ErrorCodeValue::Success);
}

void TXRService::OnAbort()
{
    TestSession::WriteNoise(TraceSource, GetPartitionId(),
        "Abort called for Service {0} at {1}",
        serviceName_, nodeId_);

    testSession_.ClosePendingItem(wformatString("TXRService.{0}-{1}-{2}", GetPartitionId(), replicaId_, nodeId_));

    if (OnCloseCallback)
    {
        OnCloseCallback(testServiceLocation_);
    }

    Cleanup();
}

void TXRService::Cleanup()
{
    if(state_ == State::Opened)
    {
        state_ = State::Closed;
        partition_.Release();
        OnOpenCallback = nullptr;
        OnCloseCallback = nullptr;
        OnChangeRoleCallback = nullptr;
    }
}

ErrorCode TXRService::RequestCheckpoint()
{
    ErrorCode error = InvokeKtlApi([&]
    {
        transactionalReplicator_->Test_RequestCheckpointAfterNextTransaction();
    });

    return error;
}

ErrorCode TXRService::Put(
    __int64 key,
    wstring const& value,
    FABRIC_SEQUENCE_NUMBER currentVersion,
    wstring serviceName,
    FABRIC_SEQUENCE_NUMBER & newVersion,
    ULONGLONG & dataLossVersion)
{
    wstring keyStateProvider = GetSPNameFromKey(key);
    
    TestSession::WriteNoise(TraceSource, GetPartitionId(), "Put called for Service. key {0} and value {1}, current version {2}", keyStateProvider, value, currentVersion);
    TestSession::FailTestIf(scheme_ != FABRIC_SERVICE_PARTITION_KIND_INT64_RANGE, "Put: Only INT64 scheme is supported");

    KUriView keyUri(&keyStateProvider[0]);
    KStringView typeName(
        (PWCHAR)TestStateProvider::TypeName.cbegin(),
        (ULONG)TestStateProvider::TypeName.size() + 1,
        (ULONG)TestStateProvider::TypeName.size());
    ErrorCode error;
    
    if(!VerifyServiceLocation(key, serviceName))
    {
        return ErrorCodeValue::FabricTestIncorrectServiceLocation;
    }

    error = InvokeKtlApiNoExcept([&]
    {
        FABRIC_EPOCH e;
        NTSTATUS status = transactionalReplicator_->GetCurrentEpoch(e);

        if (!NT_SUCCESS(status))
        {
            return status;
        }

        dataLossVersion = e.DataLossNumber;
        return status;
    });

    if (!error.IsSuccess())
    {
        return error;
    }

    KStringView kValue(&value[0]);
    
    {
        TxnReplicator::Transaction::SPtr transaction;
        TxnReplicator::IStateProvider2::SPtr stateProvider;
        TestStateProvider2::SPtr testStateProvider;

        {
            // Create the first transcation to call GetOrAddAsync
            error = InvokeKtlApiNoExcept([&]
            {
                return transactionalReplicator_->CreateTransaction(transaction);
            });

            if (!error.IsSuccess())
            {
                return error;
            }

            bool alreadyExist = false;

            // Get the state provider if exists, otherwise add the state provider 
            error = InvokeKtlApi([&]
            {
                Data::Utilities::BinaryWriter bw(*allocator_);
                bw.Write(kValue, Data::Utilities::UTF16);

                // Initialization data contains first value
                Data::Utilities::OperationData::SPtr op = Data::Utilities::OperationData::Create(*allocator_);
                op->Append(*bw.GetBuffer(0));
                NTSTATUS status = SyncAwait(transactionalReplicator_->GetOrAddAsync(
                    *transaction,
                    keyUri,
                    typeName,
                    stateProvider,
                    alreadyExist,
                    op.RawPtr()));
                THROW_ON_FAILURE(status);
            });
            
            if (error.IsSuccess())
            {
                error = CommitTransactionWrapper(*transaction, FabricTestSessionConfig::GetConfig().TXRServiceOperationTimeout);
            }
            else
            {
                transaction->Dispose();
            }

            if (alreadyExist)
            {
                TestSession::WriteNoise(
                    TraceSource,
                    GetPartitionId(),
                    "Put: Get a State Provider with key {0} and value {1} with error {2}",
                    keyStateProvider,
                    value,
                    error);

                if (!error.IsSuccess())
                {
                    return error;
                }
            }
            else
            {
                TestSession::WriteNoise(
                    TraceSource,
                    GetPartitionId(),
                    "Put: Add a State Provider with key {0} and value {1} with error {2}",
                    keyStateProvider,
                    value,
                    error);

                if (!error.IsSuccess())
                {
                    return error;
                }

                newVersion = TxnReplicator::TestCommon::TestStateProvider::StartingVersionNumber;
                return ErrorCodeValue::Success;
            }
        }
         
        // The state provider already exists, now try to update the value
        testStateProvider = dynamic_cast<TestStateProvider2 *>(stateProvider.RawPtr());
        ASSERT_IF(testStateProvider == nullptr, "Unexpected dynamic cast failure");

        {
            // Create another transaction because last one is committed in the GetOrAddAsync function 
            error = InvokeKtlApiNoExcept([&]
            {
                return transactionalReplicator_->CreateTransaction(transaction);
            });

            if (!error.IsSuccess())
            {
                return error;
            }

            bool updated;

            error = InvokeKtlApi([&]
            {
                updated = SyncAwait(testStateProvider->TryUpdateAsync(
                    FabricTestSessionConfig::GetConfig().TXRServiceOperationTimeout,
                    *transaction,
                    &value[0],
                    newVersion,
                    currentVersion));
            });

            if (!error.IsSuccess())
            {
                TestSession::WriteNoise(
                    TraceSource,
                    GetPartitionId(),
                    "Put: TryUpdateAsync failed for key {0} and value {1} with error {2}",
                    keyStateProvider,
                    value,
                    error);

                transaction->Dispose();
                return error;
            }

            if (!updated)
            {
                // Transaction will be aborted automatically

                TestSession::WriteNoise(
                    TraceSource,
                    GetPartitionId(),
                    "Put: Version check failed for key {0} and value {1} expected {2} actual {3}",
                    keyStateProvider,
                    value,
                    currentVersion,
                    newVersion);

                transaction->Dispose();
                return ErrorCode(ErrorCodeValue::FabricTestVersionDoesNotMatch);
            }

            error = CommitTransactionWrapper(*transaction, FabricTestSessionConfig::GetConfig().TXRServiceOperationTimeout);

            TestSession::WriteNoise(
                TraceSource,
                GetPartitionId(),
                "Put: TryUpdateAsync completed for key {0} and value{1} with new version {2} with error {3}",
                keyStateProvider,
                value,
                transaction->CommitSequenceNumber,
                error);

            if (!error.IsSuccess())
            {
                return error;
            }

            newVersion = transaction->CommitSequenceNumber;
        }
    }

    return error;
}

ErrorCode TXRService::Get(
    __int64 key,
    wstring serviceName,
    wstring & value,
    FABRIC_SEQUENCE_NUMBER & version,
    ULONGLONG & dataLossVersion)
{
    wstring keyStateProvider = GetSPNameFromKey(key);

    TestSession::WriteNoise(TraceSource, GetPartitionId(), "Get called for Service. key {0} and value", keyStateProvider);
    TestSession::FailTestIf(scheme_ != FABRIC_SERVICE_PARTITION_KIND_INT64_RANGE, "Get: Only INT64 scheme is supported");

    KUriView keyUri(&keyStateProvider[0]);
    ErrorCode error;
    
    if(!VerifyServiceLocation(key, serviceName))
    {
        return ErrorCodeValue::FabricTestIncorrectServiceLocation;
    }

    error = InvokeKtlApiNoExcept([&]
    {
        FABRIC_EPOCH e;
        NTSTATUS status = transactionalReplicator_->GetCurrentEpoch(e);

        if (!NT_SUCCESS(status))
        {
            return status;
        }

        dataLossVersion = e.DataLossNumber;
        return status;
    });

    if (!error.IsSuccess())
    {
        return error;
    }

    KStringView kValue(&value[0]);
    
    {
        TxnReplicator::Transaction::SPtr transaction;
        
        error = InvokeKtlApiNoExcept([&]
        {
            return transactionalReplicator_->CreateTransaction(transaction);
        });

        if (!error.IsSuccess())
        {
            return error;
        }

        TxnReplicator::IStateProvider2::SPtr stateProvider;
        TestStateProvider2::SPtr testStateProvider;

        NTSTATUS status = transactionalReplicator_->Get(keyUri, stateProvider);

        if (!NT_SUCCESS(status) && status != SF_STATUS_NAME_DOES_NOT_EXIST)
        {
            error = ErrorCode::FromHResult(Data::Utilities::StatusConverter::ToHResult(status));
            TestSession::WriteNoise(
                TraceSource,
                GetPartitionId(),
                "Get: Get failed to Get key {0} and value with error {1}",
                keyStateProvider,
                error);

            transaction->Dispose();
            return error;
        }

        if (status == SF_STATUS_NAME_DOES_NOT_EXIST)
        {
            TestSession::WriteNoise(
                TraceSource,
                GetPartitionId(),
                "Get: Get for key {0} and value failed as the state provider does not exist",
                keyStateProvider);

            transaction->Dispose();
            return ErrorCode(ErrorCodeValue::FabricTestKeyDoesNotExist);
        }
        
        testStateProvider = dynamic_cast<TestStateProvider2 *>(stateProvider.RawPtr());
        ASSERT_IF(testStateProvider == nullptr, "Unexpected dynamic cast failure");
        
        KString::SPtr lkValue;

        error = InvokeKtlApi([&]
        {
            SyncAwait(testStateProvider->GetAsync(
                FabricTestSessionConfig::GetConfig().TXRServiceOperationTimeout,
                version, 
                lkValue));
        });
    
        TestSession::WriteNoise(
            TraceSource, 
            GetPartitionId(),
            "GetAsync returned for key {0} and value with error {1}",
            keyStateProvider,
            error);

        if (!error.IsSuccess())
        {
            transaction->Dispose();
            return error;
        }

        error = CommitTransactionWrapper(*transaction, FabricTestSessionConfig::GetConfig().TXRServiceOperationTimeout);

        WStringLiteral valueStringLiteral(
            (wchar_t *)*lkValue,
            (wchar_t *)*lkValue + lkValue->Length());

        value.clear();
        value.append(valueStringLiteral.begin(), valueStringLiteral.end()); // Copy the value as the KString::SPtr could go out of scope
    }

    return error;
}

ErrorCode TXRService::Delete(
    __int64 key,
    FABRIC_SEQUENCE_NUMBER currentVersion,
    wstring serviceName,
    FABRIC_SEQUENCE_NUMBER & newVersion,
    ULONGLONG & dataLossVersion)
{
    wstring keyStateProvider = GetSPNameFromKey(key);

    TestSession::WriteNoise(TraceSource, GetPartitionId(), "Delete called for Service. key {0} and value current version {1}", key, currentVersion);
    TestSession::FailTestIf(scheme_ != FABRIC_SERVICE_PARTITION_KIND_INT64_RANGE, "Delete: Only INT64 scheme is supported");

    KUriView keyUri(&keyStateProvider[0]);
    ErrorCode error;
    
    if(!VerifyServiceLocation(key, serviceName))
    {
        return ErrorCodeValue::FabricTestIncorrectServiceLocation;
    }

    error = InvokeKtlApiNoExcept([&]
    {
        FABRIC_EPOCH e;
        NTSTATUS status = transactionalReplicator_->GetCurrentEpoch(e);

        if (!NT_SUCCESS(status))
        {
            return status;
        }

        dataLossVersion = e.DataLossNumber;
        return status;
    });

    if (!error.IsSuccess())
    {
        return error;
    }

    {
        TxnReplicator::Transaction::SPtr transaction;
        
        error = InvokeKtlApiNoExcept([&]
        {
            return transactionalReplicator_->CreateTransaction(transaction);
        });

        if (!error.IsSuccess())
        {
            return error;
        }

        TxnReplicator::IStateProvider2::SPtr stateProvider;
        TestStateProvider2::SPtr testStateProvider;

        NTSTATUS status = transactionalReplicator_->Get(keyUri, stateProvider);

        if (!NT_SUCCESS(status) && status != SF_STATUS_NAME_DOES_NOT_EXIST)
        {
            error = ErrorCode::FromHResult(Data::Utilities::StatusConverter::ToHResult(status));
            TestSession::WriteNoise(
                TraceSource,
                GetPartitionId(),
                "Delete failed to Get key {0} and value with error {1}",
                keyStateProvider,
                error);

            transaction->Dispose();
            return error;
        }

        if (status == SF_STATUS_NAME_DOES_NOT_EXIST)
        {
            // A racing thread already deleted the state provider
            transaction->Dispose();
            return ErrorCode(ErrorCodeValue::FabricTestKeyDoesNotExist);
        }
        
        testStateProvider = dynamic_cast<TestStateProvider2 *>(stateProvider.RawPtr());
        ASSERT_IF(testStateProvider == nullptr, "Unexpected dynamic cast failure");
        
        bool success;

        error = InvokeKtlApi([&]
        {
            success = SyncAwait(testStateProvider->TryRemoveMyself(
                FabricTestSessionConfig::GetConfig().TXRServiceOperationTimeout,
                *transaction, 
                *transactionalReplicator_, 
                newVersion, 
                currentVersion));
        });

        if (!error.IsSuccess())
        {
            TestSession::WriteNoise(
                TraceSource, 
                GetPartitionId(),
                "Delete: TryRemoveMyself failed for key {0} and value with error {1}",
                keyStateProvider,
                error);
            
            transaction->Dispose();
            return error;
        }

        if (!success)
        {
            TestSession::WriteNoise(
                TraceSource,
                GetPartitionId(),
                "Delete: Version does not match for key {0} and value. expected {1}, actual {2}",
                keyStateProvider,
                currentVersion,
                newVersion);

            transaction->Dispose();
            return ErrorCode(ErrorCodeValue::FabricTestVersionDoesNotMatch);
        }

        error = CommitTransactionWrapper(*transaction, FabricTestSessionConfig::GetConfig().TXRServiceOperationTimeout);

        TestSession::WriteNoise(
            TraceSource,
            GetPartitionId(),
            "Delete: TryRemoveMyself completed for key {0} with version {1} with error {3}",
            key,
            transaction->CommitSequenceNumber,
            error);

        if (!error.IsSuccess())
        {
            return error;
        }
        
        newVersion = transaction->CommitSequenceNumber;
    }

    return error;
}

AsyncOperationSPtr TXRService::BeginBackup(
    __in wstring const & backupDir,
    __in StoreBackupOption::Enum backupOption,
    __in Api::IStorePostBackupHandlerPtr const & postBackupHandler,
    __in AsyncCallback const & callback,
    __in AsyncOperationSPtr const & parent)
{
    KString::SPtr backupFolderPath = Data::KPath::CreatePath(backupDir.c_str(), GetAllocator());
    KPath::CombineInPlace(*backupFolderPath, L"BackupRestoreTest");

    if (!Common::Directory::Exists(static_cast<LPCWSTR>(*backupFolderPath)))
    {
        Common::ErrorCode errorCode = Common::Directory::Create2(static_cast<LPCWSTR>(*backupFolderPath));
    }

    FABRIC_BACKUP_OPTION fabricBackupOption;
    if (backupOption == StoreBackupOption::Full || backupOption == StoreBackupOption::Incremental)
    {
        fabricBackupOption = static_cast<FABRIC_BACKUP_OPTION>(backupOption);
    }
    else
    {
        // Error, since the backup option is invalid.
        TestSession::FailTest("TXRService::BeginBackup Store Backup Option is invalid.");
    }

    TxnReplicator::TestCommon::TestBackupCallbackHandler::SPtr backupCallbackHandler = TestBackupCallbackHandler::Create(
        *backupFolderPath,
        GetAllocator());

    return AsyncOperation::CreateAndStart<TxnReplicator::TestCommon::TestBackupAsyncOperation>(
        *transactionalReplicator_,
        *backupCallbackHandler,
        fabricBackupOption,
        postBackupHandler,
        callback,
        parent);
}

ErrorCode TXRService::EndBackup(
    __in AsyncOperationSPtr const & operation)
{
    return TestBackupAsyncOperation::End(operation);
}

ComPointer<IFabricStatefulServicePartition> const& TXRService::GetPartition() const
{
    return oldPartition_;
}

wstring const & TXRService::GetPartitionId() const
{
    return partitionId_;
}

wstring const & TXRService::GetServiceLocation() const
{
    return testServiceLocation_;
}

wstring const & TXRService::GetServiceName() const
{
    return serviceName_;
}

Federation::NodeId const & TXRService::GetNodeId() const
{
    return nodeId_;
}

void TXRService::ReportFault(::FABRIC_FAULT_TYPE faultType) const
{
    partitionWrapper_.ReportFault(faultType);
}

FABRIC_SERVICE_PARTITION_ACCESS_STATUS TXRService::GetWriteStatus() const
{
    return partitionWrapper_.GetWriteStatus();
}

FABRIC_SERVICE_PARTITION_ACCESS_STATUS TXRService::GetReadStatus() const
{
    return partitionWrapper_.GetReadStatus();
}

ErrorCode TXRService::ReportPartitionHealth(
    ::FABRIC_HEALTH_INFORMATION const *healthInfo,
    ::FABRIC_HEALTH_REPORT_SEND_OPTIONS const * sendOptions) const
{
    return  partitionWrapper_.ReportPartitionHealth(healthInfo, sendOptions);
}

ErrorCode TXRService::ReportReplicaHealth(
    ::FABRIC_HEALTH_INFORMATION const *healthInfo,
    ::FABRIC_HEALTH_REPORT_SEND_OPTIONS const * sendOptions) const
{
    return partitionWrapper_.ReportReplicaHealth(healthInfo, sendOptions);
}

void TXRService::ReportLoad(ULONG metricCount, ::FABRIC_LOAD_METRIC const *metrics) const
{
    partitionWrapper_.ReportLoad(metricCount, metrics);
}

void TXRService::ReportMoveCost(::FABRIC_MOVE_COST moveCost) const
{
    UNREFERENCED_PARAMETER(moveCost);
    TestSession::FailTest("TXRService::ReportMoveCost not implemented");
}

void TXRService::SetSecondaryPumpEnabled(bool value)
{
    UNREFERENCED_PARAMETER(value);
    TestSession::FailTest("TXRService::SetSecondaryPumpEnabled not implemented");
}

ErrorCode TXRService::Backup(wstring const & dir)
{
    ManualResetEvent waiter;

    AsyncOperationSPtr operation = BeginBackup(
        dir,
        StoreBackupOption::Incremental,
        Api::IStorePostBackupHandlerPtr(),
        [&](AsyncOperationSPtr const &) { waiter.Set(); },
        nullptr);

    waiter.WaitOne();

    return EndBackup(operation);
}

// Restore will not be directly called from script test, we invoke the restore Restore through reporting dataloss.
ErrorCode TXRService::Restore(wstring const & value)
{
    UNREFERENCED_PARAMETER(value);
    TestSession::FailTest("TXRService::SetSecondaryFaultInjectionEnabled not implemented");
}

AsyncOperationSPtr TXRService::BeginRestore(
    __in wstring const & backupDir,
    __in AsyncCallback const & callback,
    __in AsyncOperationSPtr const & parent)
{
    KString::SPtr backupFolderPath = Data::KPath::CreatePath(backupDir.c_str(), GetAllocator());

    return AsyncOperation::CreateAndStart<TxnReplicator::TestCommon::TestRestoreAsyncOperation>(
        *transactionalReplicator_,
        *backupFolderPath,
        FABRIC_RESTORE_POLICY::FABRIC_RESTORE_POLICY_FORCE,
        callback,
        parent);
}

ErrorCode TXRService::EndRestore(__in AsyncOperationSPtr const & operation)
{
    return TestRestoreAsyncOperation::End(operation);
}

ErrorCode TXRService::CompressionTest(vector<wstring> const & value)
{
    UNREFERENCED_PARAMETER(value);
    TestSession::FailTest("TXRService::getCurrentStoreTime not implemented");
}

ErrorCode TXRService::getCurrentStoreTime(int64 & currentStoreTime)
{
    UNREFERENCED_PARAMETER(currentStoreTime);
    TestSession::FailTest("TXRService::getCurrentStoreTime not implemented");
}

bool TXRService::IsStandby() const
{
    TestSession::FailTest("TXRService::IsStandby not implemented");
}

bool TXRService::VerifyServiceLocation(
    __int64 key, 
    wstring const& serviceName)
{
    __int64 partitionKey = static_cast<__int64>(key);
    if((serviceName_.compare(serviceName) == 0) && partitionKey >= lowKey_ && partitionKey <= highKey_)
    {
        return true;
    }

    TestSession::WriteInfo(
        TraceSource, 
        GetPartitionId(), 
        "Actual Service Name {0}, Expected {1}. key {2}, Range.High {3} and Range.Low {4}",
        serviceName_, 
        serviceName, 
        key, 
        highKey_, 
        lowKey_);

    return false;
}

ErrorCode TXRService::CommitTransactionWrapper(
    __in TxnReplicator::Transaction & transaction,
    __in Common::TimeSpan const & timeout) noexcept
{
    TxnReplicator::Transaction::SPtr txnSPtr = &transaction;
    ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acs;
    ktl::AwaitableCompletionSource<NTSTATUS>::Create(*allocator_, 'txrs', acs);
    shared_ptr<Common::ManualResetEvent> m = make_shared<Common::ManualResetEvent>();
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    Threadpool::Post([txnSPtr, timeout, acs, m]
    {
        KFinally([&] {txnSPtr->Dispose(); });

        NTSTATUS status = SyncAwait(txnSPtr->CommitAsync(
            timeout,
            ktl::CancellationToken::None));

        acs->SetResult(status);
        m->Set();
    });

    bool completed = m->WaitOne(timeout);

    if (completed)
    {
        status = acs->GetResult();
    }
    else
    {
        status = SF_STATUS_TIMEOUT;
    }

    return FromNTStatus(status);
}

ErrorCode TXRService::InvokeKtlApiNoExcept(function<NTSTATUS()> func) noexcept
{
    ErrorCode error = ErrorCodeValue::Success;

    NTSTATUS status = func();

    return FromNTStatus(status);
}

ErrorCode TXRService::InvokeKtlApi(function<void()> func)
{
    ErrorCode error = ErrorCodeValue::Success;

    try
    {
        func();
    }
    catch (ktl::Exception e)
    {
        return FromNTStatus(e.GetStatus());
    }
    catch (...)
    {
        TestSession::FailTest("Unexpected Exception in TXRService");
    }

    return error;
}

Common::ErrorCode TXRService::FromNTStatus(
    __in NTSTATUS status) noexcept
{
    ErrorCode error = ErrorCodeValue::Success;

    if (NT_SUCCESS(status))
    {
        return error;
    }

    switch (status)
    {
    case SF_STATUS_NAME_ALREADY_EXISTS:
        error = ErrorCodeValue::FabricTestVersionDoesNotMatch;
        break;
    case SF_STATUS_NAME_DOES_NOT_EXIST:
        error = ErrorCodeValue::FabricTestKeyDoesNotExist;
        break;
    default:
        error = ErrorCode::FromHResult(Data::Utilities::StatusConverter::ToHResult(status));
        break;
    }

    TestSession::FailTestIf(
        error.IsSuccess(),
        "TXRService hit an exception with NTSTATUS {0:x}, but the converted Fabric error code is a success {1}. This indicates a missed mapping in the StatusConverter Utility",
        status,
        error);

    return error;
}

ErrorCode TXRService::GetReplicationQueueCounters(__out FABRIC_INTERNAL_REPLICATION_QUEUE_COUNTERS & counters)
{
    ComPointer<IFabricInternalStateReplicator> internalStateReplicator;
    HRESULT hr = primaryReplicator_->QueryInterface(IID_IFabricInternalStateReplicator, internalStateReplicator.VoidInitializationAddress());

    if (SUCCEEDED(hr))
    {
        return ErrorCode::FromHResult(internalStateReplicator->GetReplicationQueueCounters(&counters));
    }

    return ErrorCodeValue::NotFound;
}

Common::ErrorCode TXRService::GetReplicatorQueryResult(__in ServiceModel::ReplicatorStatusQueryResultSPtr & result)
{
    ComPointer<IFabricInternalReplicator> internalReplicator;
    HRESULT hr = primaryReplicator_->QueryInterface(IID_IFabricInternalReplicator, internalReplicator.VoidInitializationAddress());

    if (SUCCEEDED(hr))
    {
        ComPointer<IFabricGetReplicatorStatusResult> replicatorStatusResult;
        hr = internalReplicator->GetReplicatorStatus(replicatorStatusResult.InitializationAddress());

        if (SUCCEEDED(hr))
        {
            FABRIC_REPLICA_ROLE role = replicatorStatusResult->get_ReplicatorStatus()->Role;
            result = ServiceModel::ReplicatorStatusQueryResult::CreateSPtr(role);

            if (role == FABRIC_REPLICA_ROLE_PRIMARY)
            {
                std::shared_ptr<ServiceModel::PrimaryReplicatorStatusQueryResult> primaryQueryResult = static_pointer_cast<ServiceModel::PrimaryReplicatorStatusQueryResult>(result);
                primaryQueryResult->FromPublicApi(*replicatorStatusResult->get_ReplicatorStatus());
                result = primaryQueryResult;
            }
            else
            {
                result->FromPublicApi(*replicatorStatusResult->get_ReplicatorStatus());
            }
        }
    }

    return ErrorCode::FromHResult(hr);
}

NTSTATUS TXRService::GetPeriodicCheckpointAndTruncationTimestamps(
    __out LONG64 & lastPeriodicCheckpoint,
    __out LONG64 & lastPeriodicTruncation)
{
    return transactionalReplicator_->Test_GetPeriodicCheckpointAndTruncationTimestampTicks(
        lastPeriodicCheckpoint,
        lastPeriodicTruncation);
}