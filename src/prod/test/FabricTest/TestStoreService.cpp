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

StringLiteral const TraceSource("TestStatefulService");

static size_t const DataBufferSize = 2000;

class TestStoreService::TestFabricOperationData : public IFabricOperationData, private Common::ComUnknownBase, public Serialization::FabricSerializable
{
    DENY_COPY_ASSIGNMENT(TestFabricOperationData)
        COM_INTERFACE_LIST1(
        TestFabricOperationData,
        IID_IFabricOperationData,
        IFabricOperationData)

public:

    TestFabricOperationData()
        : key_(), 
        value_(), 
        version_()
    {
    }

    TestFabricOperationData(__int64 key, wstring const& value, int64 version)
        : key_(key), 
        value_(value), 
        version_(version)
    {
    }

    __declspec (property(get=get_Key)) __int64 Key;
    __int64 get_Key() const {return key_;}

    __declspec (property(get=get_Value)) wstring const& Value;
    wstring const& get_Value() const {return value_;}

    __declspec (property(get=get_Version)) FABRIC_SEQUENCE_NUMBER Version;
    FABRIC_SEQUENCE_NUMBER get_Version() const {return version_;}

    FABRIC_FIELDS_03(key_, value_, version_);

    HRESULT STDMETHODCALLTYPE GetData(
        /*[out]*/ ULONG * count, 
        /*[out, retval]*/ FABRIC_OPERATION_DATA_BUFFER const ** buffers)
    {
        *count = 1;

        if (buffer_ == nullptr)
        {
            buffer_ = make_unique<std::vector<byte>>();

            FabricSerializer::Serialize(this, *buffer_);
        }

        replicaBuffer_.BufferSize = static_cast<ULONG>(buffer_->size());
        replicaBuffer_.Buffer = buffer_->data();

        *buffers = &replicaBuffer_;

        return S_OK;
    }

    void WriteTo(TextWriter & w, FormatOptions const &) const
    {
        w.Write("Key {0}, Value {1}, Version {2}", key_, value_, version_);
    }

private:
    __int64 key_;
    wstring value_;
    FABRIC_SEQUENCE_NUMBER version_;
    std::unique_ptr<std::vector<byte>> buffer_;
    FABRIC_OPERATION_DATA_BUFFER replicaBuffer_;
};

struct TestStoreService::TestServiceEntry
{
public:

    TestServiceEntry(__int64 key, wstring const& value, int64 version)
        : key_(key), 
        value_(value), 
        version_(version), 
        nextVersion_(TestServiceEntry::InvalidVersion),
        nextValue_(),
        dataLock_(), 
        replicationLock_()
    {
    }

    __declspec (property(get=get_Key)) __int64 Key;
    __int64 get_Key() const {return key_;}

    __declspec (property(get=get_ReplicationLock)) RwLock & ReplicationLock;
    RwLock & get_ReplicationLock() const {return replicationLock_;}

    HRESULT BeginReplicate(ComPointer<IFabricStateReplicator> & replicator, 
        ComPointer<ComAsyncOperationCallbackTestHelper> & callback, 
        ComPointer<IFabricAsyncOperationContext> & context, 
        FABRIC_SEQUENCE_NUMBER & newVersion,
        wstring const& newValue) 
    { 
        AcquireWriteLock grab(dataLock_);
        ComPointer<TestFabricOperationData> operationData = make_com<TestFabricOperationData>(key_, newValue, version_);
        TestSession::WriteNoise(TraceSource, "Replicating operation key {0}, value {1}", key_, newValue);
        TestSession::FailTestIfNot(nextVersion_ == TestServiceEntry::InvalidVersion, "NextVersion in TestServiceEntry should be invalid: {0}", nextVersion_);
        
        HRESULT hr = replicator->BeginReplicate(
            operationData.GetRawPointer(),
            callback.GetRawPointer(),
            &newVersion,
            context.InitializationAddress());

        // According to the BeginReplicate contract, operation is accepted only if S_OK and valid version is returned.
        if(SUCCEEDED(hr) && newVersion > 0)
        {
            nextVersion_ = newVersion;
            nextValue_ = newValue;
        }

        return hr;
    }

    void GetValueAndVersion(wstring & value, FABRIC_SEQUENCE_NUMBER & version) 
    { 
        AcquireReadLock grab(dataLock_);
        value = value_;
        version = version_; 
    }

    void Commit(wstring const & value, FABRIC_SEQUENCE_NUMBER version) 
    { 
        AcquireWriteLock grab(dataLock_);
        version_ = version;
        value_ = value; 
        nextVersion_ = TestServiceEntry::InvalidVersion;
        nextValue_.clear();
    }

    void Rollback() 
    { 
        AcquireWriteLock grab(dataLock_);
        nextVersion_ = TestServiceEntry::InvalidVersion;
        nextValue_.clear();
    }

    bool CheckVersion(FABRIC_SEQUENCE_NUMBER uptoSequenceNumber, ComPointer<TestFabricOperationData> & operationData) 
    { 
        AcquireWriteLock grab(dataLock_);
        operationData = make_com<TestFabricOperationData>(key_, value_, version_);

        if(nextVersion_ != TestServiceEntry::InvalidVersion && nextVersion_ <= uptoSequenceNumber)
        {
            // Replication in flight has next version less than upto that means it is quorum acked and done
            // we should return the next version and next value.

            operationData = make_com<TestFabricOperationData>(key_, nextValue_, nextVersion_);
            return true;
        }
        else if (version_ <= uptoSequenceNumber)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    void WriteTo(TextWriter & w, FormatOptions const &) const
    {
        AcquireReadLock grab(dataLock_);
        w.Write("Key {0}, Value {1}, Version {2}", key_, value_, version_);
    }

private:
    __int64 key_;
    wstring value_;
    FABRIC_SEQUENCE_NUMBER version_;

    wstring nextValue_;
    FABRIC_SEQUENCE_NUMBER nextVersion_;

    static FABRIC_SEQUENCE_NUMBER const InvalidVersion = -1;

    mutable RwLock dataLock_;
    mutable RwLock replicationLock_;
};

std::wstring const TestStoreService::DefaultServiceType = L"TestStoreServiceType";

TestStoreService::TestStoreService(
    FABRIC_PARTITION_ID partitionId,
    wstring const& serviceName,
    Federation::NodeId nodeId,
    bool changeServiceLocationOnChangeRole,
    wstring const& initDataStr,
    wstring const& serviceType,
    std::wstring const& appId)
    :serviceName_(serviceName),
    partitionId_(Common::Guid(partitionId).ToString()),
    serviceLocation_(),
    isOpen_(false),
    replicaRole_(FABRIC_REPLICA_ROLE_UNKNOWN),
    partition_(),
    oldPartition_(),
    replicator_(),
    stateLock_(),
    entryMapLock_(),
    entryMap_(),
    scheme_(::FABRIC_SERVICE_PARTITION_KIND_INVALID),
    highKey_(-1),
    lowKey_(-1),
    partitionName_(),
    dataLossNumber_(0),
    configurationNumber_(0),
    nodeId_(nodeId),
    stateProviderCPtr_(),
    pumpState_(PumpNotStarted),
    changeRoleCalled_(false),
    serviceType_(serviceType),
    appId_(appId),
    secondaryPumpEnabled_(true),
    changeServiceLocationOnChangeRole_(changeServiceLocationOnChangeRole),
    initDataStr_(initDataStr),
    streamFaultsAndRequireServiceAckEnabled_(false),
    pumpingCompleteEvent_(true)
{
    TestSession::WriteNoise(TraceSource, partitionId_, "TestStoreService created with name: {0} initialization-data:'{1}'", serviceName, initDataStr_);
}

TestStoreService::~TestStoreService()
{
    TestSession::WriteNoise(TraceSource, partitionId_, "TestStoreService destructed");
}

ErrorCode TestStoreService::OnOpen(ComPointer<IFabricStatefulServicePartition3> const& partition, ComPointer<IFabricStateReplicator> const& replicator, bool streamFaultsEnabled)
{
    TestSession::WriteNoise(TraceSource, partitionId_, "Open called for Service at {0}. StreamFaultsEnabled = {1}", nodeId_, streamFaultsEnabled);

    {
        AcquireWriteLock grab(stateLock_);
        TestSession::FailTestIf(isOpen_, "Service open called twice");
        partition_ = partition;
        oldPartition_ = Common::ComPointer<IFabricStatefulServicePartition>(partition_, IID_IFabricStatefulServicePartition3);
        replicator_ = replicator;
        isOpen_ = true;
        serviceLocation_ = Common::Guid::NewGuid().ToString();
        streamFaultsAndRequireServiceAckEnabled_ = streamFaultsEnabled;
    }

    partitionWrapper_.SetPartition(partition);

    FABRIC_SERVICE_PARTITION_INFORMATION const *partitionInfo;
    HRESULT result = partition->GetPartitionInfo(&partitionInfo);

    if (!SUCCEEDED(result))
    {
        return ErrorCode::FromHResult(result);
    }

    TestSession::FailTestIf(partitionInfo->Value == nullptr, "HighKey not found.");

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

    auto root = shared_from_this();
    TestServiceInfo testServiceInfo(
        appId_,
        serviceType_, 
        serviceName_, 
        partitionId_, 
        ServiceLocation, 
        true, 
        FABRIC_REPLICA_ROLE_NONE);

    OnOpenCallback(testServiceInfo, root, false);

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode TestStoreService::OnChangeRole(FABRIC_REPLICA_ROLE newRole)
{
    TestSession::WriteNoise(
        TraceSource, 
        partitionId_, 
        "Change Role called: original role {0}, new role {1} at {2}", 
        replicaRole_, 
        newRole, 
        nodeId_);
    {
        AcquireReadLock grab(stateLock_);
        TestSession::FailTestIfNot(isOpen_, "ChangeRole called on unopened service");
    }

    bool isGetOperationsPumpActive = (replicaRole_ == ::FABRIC_REPLICA_ROLE_IDLE_SECONDARY || 
        replicaRole_ == FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
    replicaRole_ = newRole;

    if (!isGetOperationsPumpActive && secondaryPumpEnabled_)
    {
        ComPointer<IFabricStateReplicator> replicator;
        {
            AcquireReadLock grab(stateLock_);
            if(isOpen_)
            {
                replicator = replicator_;
            }
        }
        
        auto root = this->CreateComponentRoot();
        if (replicaRole_ == ::FABRIC_REPLICA_ROLE_IDLE_SECONDARY)
        {
            // Post this since StartCopyOperationPump can get blocked due to a signal
            Threadpool::Post([root, this, replicator]
            {
                StartCopyOperationPump(replicator);
            });
        }
        else if (replicaRole_ == ::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY)
        {
            // Post this since StartReplicationOperationPump can get blocked due to a signal
            Threadpool::Post([root, this, replicator]
            {
                StartReplicationOperationPump(replicator);
            });
        }
    }
    else if (replicaRole_ == ::FABRIC_REPLICA_ROLE_PRIMARY)
    {
        bool result = pumpingCompleteEvent_.WaitOne(TimeSpan::FromMinutes(5));
        TESTASSERT_IFNOT(result, "Wait for pumping to complete inside TestSoreService in change role did not complete in 5 minutes");
    }

    wstring newServiceLocation;
    if (changeServiceLocationOnChangeRole_)
    {
        newServiceLocation = Common::Guid::NewGuid().ToString();
    }

    OnChangeRoleCallback(ServiceLocation, newServiceLocation, replicaRole_);
    if(changeServiceLocationOnChangeRole_)
    {
        AcquireWriteLock grab(stateLock_);
        serviceLocation_ = newServiceLocation;
    }

    changeRoleCalled_ = true;
    return ErrorCode(ErrorCodeValue::Success);
}

void TestStoreService::StartCopyOperationPump(Common::ComPointer<IFabricStateReplicator> const & replicator)
{
    ComPointer<IFabricOperationStream> stream;
    WaitForSignalReset(StartPumpCopyBlock);

    HRESULT hr = replicator->GetCopyStream(stream.InitializationAddress());
    if (FAILED(hr)) 
    {
        if (hr == ErrorCodeValue::InvalidState || hr == ErrorCodeValue::ObjectClosed)
        {
            // The replicator state changed due to close/ChangeRole
            TestSession::WriteWarning(TraceSource, "GetCopyStream: Error {0} at {1}", hr, ServiceLocation);
        }
        else
        {
            TestSession::FailTest("GetCopyStream failed with HR {0} at {1}", hr, ServiceLocation);
        }
    }
    else
    {
        pumpState_ = PumpCopy;
        BeginGetOperations(replicator, stream);
    }
}

ErrorCode TestStoreService::GetReplicationQueueCounters(
    __out FABRIC_INTERNAL_REPLICATION_QUEUE_COUNTERS & counters)
{
    ComPointer<IFabricInternalStateReplicator> internalStateReplicator;
    HRESULT hr = replicator_->QueryInterface(IID_IFabricInternalStateReplicator, internalStateReplicator.VoidInitializationAddress());

    if(SUCCEEDED(hr))
    {
        return ErrorCode::FromHResult(internalStateReplicator->GetReplicationQueueCounters(&counters));
    }

    return ErrorCodeValue::NotFound;
}

ErrorCode TestStoreService::GetReplicatorQueryResult(__out ServiceModel::ReplicatorStatusQueryResultSPtr & result)
{
    ComPointer<IFabricInternalReplicator> internalReplicator;
    HRESULT hr = replicator_->QueryInterface(IID_IFabricInternalReplicator, internalReplicator.VoidInitializationAddress());

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

void TestStoreService::StartReplicationOperationPump(Common::ComPointer<IFabricStateReplicator> const & replicator)
{
    ComPointer<IFabricOperationStream> stream;
    WaitForSignalReset(StartPumpReplicationBlock);

    HRESULT hr = replicator->GetReplicationStream(stream.InitializationAddress());
    if (FAILED(hr)) 
    {
        if (hr == ErrorCodeValue::InvalidState || hr == ErrorCodeValue::ObjectClosed)
        {
            // The replicator state changed due to close/ChangeRole
            TestSession::WriteWarning(TraceSource, "GetReplicationStream: Error {0} at {1}", hr, ServiceLocation);
        }
        else
        {
            TestSession::FailTest("GetReplicationStream failed with HR {0} at {1}", hr, ServiceLocation); 
        }
    }
    else
    {
        pumpState_ = PumpReplication;
        BeginGetOperations(replicator, stream);
    }
}

void TestStoreService::BeginGetOperations(ComPointer<IFabricStateReplicator> const & replicator, ComPointer<IFabricOperationStream> const & stream)
{
    if (!secondaryPumpEnabled_) { return; }

    BOOLEAN completedSynchronously = false;
    pumpingCompleteEvent_.Reset();

    do
    {
        auto root = CreateComponentRoot();
        ComPointer<ComAsyncOperationCallbackTestHelper> callback = make_com<ComAsyncOperationCallbackTestHelper>(
            [this, stream, replicator, root](IFabricAsyncOperationContext * context)
        {
            BOOLEAN inCallbackCompletedSynchronously = GetCompletedSynchronously(context);
            if(!inCallbackCompletedSynchronously)
            {
                if (FinishProcessingOperation(context, replicator, stream))
                {
                    BeginGetOperations(replicator, stream);
                }
            }
        });

        ComPointer<IFabricAsyncOperationContext> beginContext;
        HRESULT hr = stream->BeginGetOperation(callback.GetRawPointer(), beginContext.InitializationAddress());
        if (FAILED(hr)) { TestSession::FailTest("BeginGetOperation failed with HR {0} at {1}", hr, ServiceLocation); }

        completedSynchronously = GetCompletedSynchronously(beginContext.GetRawPointer());

        if(completedSynchronously)
        {
            if (!FinishProcessingOperation(beginContext.GetRawPointer(), replicator, stream))
            {
                break;
            }
        }
    }
    while(completedSynchronously && secondaryPumpEnabled_);
}

bool TestStoreService::FinishProcessingOperation(IFabricAsyncOperationContext * context, ComPointer<IFabricStateReplicator> const & replicator, ComPointer<IFabricOperationStream> const & stream)
{
    ComPointer<IFabricOperation> operation;
    HRESULT hr = stream->EndGetOperation(context, operation.InitializationAddress());
    if (FAILED(hr)) { TestSession::FailTest("EndGetOperation failed with HR {0} at {1}", hr, ServiceLocation); }

    std::wstring faultInjectionComponentName;
    switch (pumpState_)
    {
    case PumpReplication:
        faultInjectionComponentName = L"replication";
        if (IsSignalSet(PumpReplicationBlock))
        {
            WaitForSignalReset(PumpReplicationBlock);
        }
        break;
    default:
        faultInjectionComponentName = L"copy";
        if (IsSignalSet(PumpCopyBlock))
        {
            WaitForSignalReset(PumpCopyBlock);
        }
        break;
    }

    if (this->ShouldFailOn(ApiFaultHelper::ServiceOperationPump, faultInjectionComponentName, ApiFaultHelper::Delay))
    {
        ::Sleep(static_cast<DWORD>(ApiFaultHelper::Get().GetApiDelayInterval().TotalMilliseconds()));
    }
    if (this->ShouldFailOn(ApiFaultHelper::ServiceOperationPump, faultInjectionComponentName, ApiFaultHelper::ReportFaultPermanent) &&
        operation)
    {
        ComPointer<IFabricOperationStream2> stream2;
        hr = stream->QueryInterface(IID_IFabricOperationStream2, stream2.VoidInitializationAddress());
        ASSERT_IFNOT(SUCCEEDED(hr), "QI on IFabricOperationStream2 failed");
        hr = stream2->ReportFault(FABRIC_FAULT_TYPE::FABRIC_FAULT_TYPE_PERMANENT);

        if (streamFaultsAndRequireServiceAckEnabled_)
        {
            ASSERT_IF(
                !SUCCEEDED(hr) && 
                !ErrorCode::FromHResult(hr).IsError(ErrorCodeValue::ObjectClosed),
                "Stream Fault on {0} returned unexpected return code {1}", partitionId_, hr);
        }

        if (SUCCEEDED(hr))
        {
            TestSession::WriteNoise(TraceSource, partitionId_, "finishprocessingoperation reported fault permanent on stream");
        }
        else
        {
            // This means the config for stream faults is not enabled. Report fault on partition
            this->ReportFault(FABRIC_FAULT_TYPE_PERMANENT);
            TestSession::WriteNoise(TraceSource, partitionId_, "finishprocessingoperation reported fault permanent on partition as stream returned invalidoperation");
        }
    }
    if (this->ShouldFailOn(ApiFaultHelper::ServiceOperationPump, faultInjectionComponentName, ApiFaultHelper::ReportFaultTransient) &&
        operation)
    {
        ComPointer<IFabricOperationStream2> stream2;
        hr = stream->QueryInterface(IID_IFabricOperationStream2, stream2.VoidInitializationAddress());
        ASSERT_IFNOT(SUCCEEDED(hr), "QI on IFabricOperationStream2 failed");
        hr = stream2->ReportFault(FABRIC_FAULT_TYPE::FABRIC_FAULT_TYPE_TRANSIENT);

        if (streamFaultsAndRequireServiceAckEnabled_)
        {
            ASSERT_IF(
                !SUCCEEDED(hr) && 
                !ErrorCode::FromHResult(hr).IsError(ErrorCodeValue::ObjectClosed),
                "Stream Fault on {0} returned unexpected return code {1}", partitionId_, hr);
        }

        if (SUCCEEDED(hr))
        {
            TestSession::WriteNoise(TraceSource, partitionId_, "finishprocessingoperation reported fault transient on stream");
        }
        else
        {
            // This means the config for stream faults is not enabled. Report fault on partition
            this->ReportFault(FABRIC_FAULT_TYPE_TRANSIENT);
            TestSession::WriteNoise(TraceSource, partitionId_, "finishprocessingoperation reported fault transient on partition as stream returned invalidoperation");
        }
    }

    if (operation && operation->get_Metadata()->Type != FABRIC_OPERATION_TYPE_END_OF_STREAM)
    {
        ULONG size;
        BYTE const * data;

        ULONG bufferCount = 0;
        FABRIC_OPERATION_DATA_BUFFER const * replicaBuffers = nullptr;

        hr = operation->GetData(&bufferCount, &replicaBuffers);
        if (FAILED(hr)) { TestSession::FailTest("GetData failed with HR {0} at {1}", hr, ServiceLocation); }

        TestSession::FailTestIf(bufferCount > 1, "Unexpected multiple buffers");

        size = (bufferCount > 0 ? replicaBuffers[0].BufferSize : 0);
        data = (bufferCount > 0 ? replicaBuffers[0].Buffer : nullptr);

        ByteBique biqueBuffer(DataBufferSize);
        biqueBuffer.reserve_back(DataBufferSize);

        BYTE * nonConstData = const_cast<BYTE *>(data);
        for (size_t ix = 0; ix < size; ++ix)
        {
            biqueBuffer.push_back(move(nonConstData[ix]));
        }

        ComPointer<TestFabricOperationData> operationData = make_com<TestFabricOperationData>();

        FabricSerializer::Deserialize((*operationData.GetRawPointer()), size, nonConstData);

        {
            shared_ptr<TestServiceEntry> const& currentEntry = GetEntry(operationData->Key);

            AcquireWriteLock grab(currentEntry->ReplicationLock);
            FABRIC_SEQUENCE_NUMBER version = operationData->Version;

            FABRIC_OPERATION_METADATA const *metadata = operation->get_Metadata();
            if (pumpState_ == PumpReplication)
            {
                version = metadata->SequenceNumber;
            }

            currentEntry->Commit(operationData->Value, version);

            operation->Acknowledge();

            TestSession::WriteNoise(
                TraceSource,
                partitionId_,
                "Operation of type {0} received. key {1}, value {2}, version {3} at {4}",
                metadata->Type,
                currentEntry->Key,
                operationData->Value,
                version,
                nodeId_);
        }
    }
    else
    {
        if (operation)
        {
            operation->Acknowledge();
            TestSession::WriteNoise(TraceSource, partitionId_, "EndGetOperation returned end of stream operation");
        }
        else
        {
            TestSession::WriteNoise(TraceSource, partitionId_, "EndGetOperation returned null");
        }

        // Pumping complete set the reset event.
        pumpingCompleteEvent_.Set();

        if (pumpState_ == PumpCopy)
        {
            // The copy stream is done.
            TestSession::WriteNoise(TraceSource, partitionId_, "Last copy operation received at {0} {1}", ServiceLocation, nodeId_);
            // Start replication pump
            // Post this since StartReplicationOperationPump can get blocked due to a signal
            auto root = this->CreateComponentRoot();
            Threadpool::Post([this, replicator, root]
            {
                StartReplicationOperationPump(replicator);    
            });
        }

        return false;
    }

    return true;
}

BOOLEAN TestStoreService::GetCompletedSynchronously(IFabricAsyncOperationContext * context)
{
    return context->CompletedSynchronously();
}

ErrorCode TestStoreService::OnClose()
{
    TestSession::WriteNoise(TraceSource, partitionId_, "Close called for Service at {0}", nodeId_);
    TestSession::FailTestIfNot(isOpen_, "Close called on unopened service");
    OnCloseCallback(ServiceLocation);
    Cleanup();
    return ErrorCode(ErrorCodeValue::Success);
}

void TestStoreService::OnAbort()
{
    TestSession::WriteNoise(TraceSource, partitionId_, "Abort called for Service at {0}", nodeId_);

    if (OnCloseCallback)
    {
        OnCloseCallback(ServiceLocation);
    }

    Cleanup();
}

void TestStoreService::Cleanup()
{
    AcquireWriteLock grab(stateLock_);
    if(isOpen_)
    {
        isOpen_ = false;
        replicator_.Release();
        partition_.Release();
        oldPartition_.Release();
        OnOpenCallback = nullptr;
        OnCloseCallback = nullptr;
        OnChangeRoleCallback = nullptr;
    }
}

ErrorCode TestStoreService::Put(
    /* [in] */ __int64 key,
    /* [in] */ std::wstring const& newValue,
    /* [in] */ FABRIC_SEQUENCE_NUMBER currentClientVersion,
    /* [in] */ std::wstring serviceName,
    /* [retval][out] */ FABRIC_SEQUENCE_NUMBER & newVersion,
    /* [retval][out] */ ULONGLONG & dataLossVersion)
{
    TestSession::WriteNoise(TraceSource, partitionId_, "Put called for Service. key {0}, value {1}", key, newValue);
    TestSession::FailTestIf(scheme_ != FABRIC_SERVICE_PARTITION_KIND_INT64_RANGE, "Put: Only INT64 scheme is supported");

    ComPointer<IFabricStateReplicator> replicator;
    ComPointer<IFabricStatefulServicePartition3> partition;
    {
        AcquireReadLock grab(stateLock_);
        if(!isOpen_)
        {
            TestSession::WriteNoise(TraceSource, partitionId_, "Put called on closed service");
            return ErrorCode(ErrorCodeValue::FabricTestServiceNotOpen);
        }

        replicator = replicator_;
        partition = partition_;
    }

    if(!VerifyServiceLocation(key, serviceName))
    {
        return ErrorCode(ErrorCodeValue::FabricTestIncorrectServiceLocation);
    }

    ::FABRIC_SERVICE_PARTITION_ACCESS_STATUS status;
    partition->GetWriteStatus(&status);
    if(status != ::FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED)
    {
        TestSession::WriteNoise(TraceSource, partitionId_, "Write status not granted");
        return ErrorCode(ErrorCodeValue::FabricTestStatusNotGranted);
    }

    dataLossVersion = dataLossNumber_;
    shared_ptr<TestServiceEntry> const& entry = GetEntry(key);
    AcquireWriteLock grab(entry->ReplicationLock);
    wstring currentEntryValue; 
    FABRIC_SEQUENCE_NUMBER currentEntryVersion;
    entry->GetValueAndVersion(currentEntryValue, currentEntryVersion);
    if(currentEntryVersion != currentClientVersion)
    {
        TestSession::WriteNoise(TraceSource, partitionId_, "Version does not match for key {0}. expected {1}, actual {2}", key, currentClientVersion, currentEntryVersion);
        newVersion = currentEntryVersion;
        return ErrorCode(ErrorCodeValue::FabricTestVersionDoesNotMatch);
    }

    shared_ptr<ManualResetEvent> resetEvent = make_shared<ManualResetEvent>(false);

    HRESULT replicationResult = S_OK;
    ComPointer<ComAsyncOperationCallbackTestHelper > callback = make_com<ComAsyncOperationCallbackTestHelper>(
        [resetEvent](IFabricAsyncOperationContext *)
    {
        resetEvent->Set();
    });

    ErrorCode error;
    ComPointer<IFabricAsyncOperationContext> context;
    HRESULT hr = entry->BeginReplicate(replicator, callback, context, newVersion, newValue);

    if(SUCCEEDED(hr))
    {
        // Wait for 5 minutes for replication to complete else the FailoverUnit must be stuck
        // in quorum loss so timeout
        TimeSpan replicationTimeout(FabricTestCommonConfig::GetConfig().StoreReplicationTimeout);
        bool waitResult = resetEvent->WaitOne(replicationTimeout);
        if(waitResult)
        {
            replicationResult = replicator->EndReplicate(context.GetRawPointer(), nullptr);
            if(SUCCEEDED(replicationResult))
            {
                entry->Commit(newValue, newVersion);
                TestSession::WriteNoise(TraceSource, partitionId_, "Put operation for key {0} succeeded. value {1}, version {2}, dataLoss {3}", 
                    entry->Key, newValue, newVersion, dataLossNumber_);
            }
            else
            {
                entry->Rollback();
                TestSession::WriteNoise(TraceSource, partitionId_, "EndReplicate failed with HR {0}", replicationResult);
                error = ErrorCode(ErrorCodeValue::FabricTestReplicationFailed);
            }
        }
        else
        {
            entry->Rollback();
            TestSession::WriteNoise(TraceSource, partitionId_, "Timeout waiting for replication to complete");
            Threadpool::Post([resetEvent, replicator, context] () mutable
            { 
                // Ignore errors
                resetEvent->WaitOne();
                replicator->EndReplicate(context.GetRawPointer(), nullptr);
            });

            error = ErrorCode(ErrorCodeValue::Timeout);
        }
    }
    else
    {
        TestSession::WriteNoise(TraceSource, partitionId_, "BeginReplicate failed with HR {0}", hr);
        ASSERT_IF(ErrorCode::FromHResult(hr).IsError(ErrorCodeValue::REOperationTooLarge), "BeginReplicate failed with operation too large error and this is not expected");
        error =  ErrorCode(ErrorCodeValue::FabricTestReplicationFailed);
    }

    return error;
}

ErrorCode TestStoreService::Get(
    /* [in] */ __int64 key,
    /* [in] */ std::wstring serviceName,
    /* [retval][out] */ std::wstring & value,
    /* [retval][out] */ FABRIC_SEQUENCE_NUMBER & version,
    /* [retval][out] */ ULONGLONG & dataLossVersion)
{
    TestSession::FailTestIf(scheme_ != FABRIC_SERVICE_PARTITION_KIND_INT64_RANGE, "Get: Only INT64 scheme is supported");

    ComPointer<IFabricStatefulServicePartition3> partition;
    {
        AcquireReadLock grab(stateLock_);
        if(!isOpen_)
        {
            TestSession::WriteNoise(TraceSource, partitionId_, "Get called for closed service");
            return ErrorCode(ErrorCodeValue::FabricTestServiceNotOpen);
        }


        partition = partition_;
    }

    if(!VerifyServiceLocation(key, serviceName))
    {
        return ErrorCode(ErrorCodeValue::FabricTestIncorrectServiceLocation);
    }

    ::FABRIC_SERVICE_PARTITION_ACCESS_STATUS status;
    partition->GetReadStatus(&status);
    if(status != ::FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED)
    {
        TestSession::WriteNoise(TraceSource, partitionId_, "Read status not granted");
        return ErrorCode(ErrorCodeValue::FabricTestStatusNotGranted);
    }

    dataLossVersion = dataLossNumber_;

    shared_ptr<TestServiceEntry> entry;
    if(!TryGetEntry(key, entry))
    {
        TestSession::WriteNoise(TraceSource, partitionId_, "key {0} does not exist. Current DataLossVersion {1}", key, dataLossNumber_);
        return ErrorCode(ErrorCodeValue::FabricTestKeyDoesNotExist);
    }

    entry->GetValueAndVersion(value, version);
    TestSession::WriteNoise(
        TraceSource, 
        partitionId_, 
        "Get operation for key {0} succeeded. value {1}, version {2}, dataLoss {3}", 
        key, 
        value, 
        version, 
        dataLossNumber_);
    return ErrorCode::Success();
}

bool TestStoreService::VerifyServiceLocation(__int64 key, wstring const& serviceName)
{
    __int64 partitionKey = static_cast<__int64>(key);
    if((serviceName_.compare(serviceName) == 0) && partitionKey >= lowKey_ && partitionKey <= highKey_)
    {
        return true;
    }

    TestSession::WriteInfo(TraceSource, partitionId_, "Actual Service Name {0}, Expected {1}. key {2}, Range.High {3} and Range.Low {4}",
        serviceName_, serviceName, key, highKey_, lowKey_);
    return false;
}

shared_ptr<TestStoreService::TestServiceEntry> & TestStoreService::GetEntry(__int64 key)
{
    AcquireWriteLock grab(entryMapLock_);
    auto iterator = entryMap_.find(key);
    if(iterator == entryMap_.end())
    {
        entryMap_[key] = make_shared<TestServiceEntry>(key, L"", 0);
    }

    return entryMap_[key];
}

bool TestStoreService::TryGetEntry(__int64 key, shared_ptr<TestServiceEntry> & entry)
{
    AcquireReadLock grab(entryMapLock_);
    auto iterator = entryMap_.find(key);
    if(iterator != entryMap_.end())
    {
        entry = entryMap_[key];
        return true;
    }

    return false;
}

static const GUID CLSID_TestGetNextOperation = { 0x86aef2e3, 0xf375, 0x4440, { 0xab, 0x1d, 0xa1, 0xbe, 0x58, 0x8d, 0x92, 0x18 } };
class TestStoreService::ComTestGetNextOperation : public ComAsyncOperationContext
{
    DENY_COPY(ComTestGetNextOperation)

        COM_INTERFACE_LIST2(
        ComTestGetNextOperation,
        IID_IFabricAsyncOperationContext,
        IFabricAsyncOperationContext,
        CLSID_TestGetNextOperation,
        ComTestGetNextOperation)

public:
    explicit ComTestGetNextOperation(){ }

    virtual ~ComTestGetNextOperation(){ }

    HRESULT Initialize(
        __in IFabricAsyncOperationCallback * inCallback)
    {
        //Dont need to pass in root pointer. The callback posted to threadpool will keep it alive. 
        HRESULT hr = this->ComAsyncOperationContext::Initialize(ComponentRootSPtr(), inCallback);
        if (FAILED(hr)) { return hr; }
        return S_OK;
    }

    static HRESULT End(
        __in IFabricAsyncOperationContext * context,
        __out IFabricOperationData ** operation)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<ComTestGetNextOperation> thisOperation(context, CLSID_TestGetNextOperation);
        HRESULT hr = thisOperation->ComAsyncOperationContext::End();;
        if (FAILED(hr)) { return hr; }
        *operation = thisOperation->innerComOperationPointer_.DetachNoRelease();
        return thisOperation->Result;
    }

    void SetOperation(ComPointer<TestFabricOperationData> && copy)
    {
        innerComOperationPointer_.SetNoAddRef(copy.DetachNoRelease());
    }

    bool IsInnerComOperationPointerNull()
    {
        return innerComOperationPointer_.GetRawPointer() == nullptr;
    }

private:
    virtual void OnStart(AsyncOperationSPtr const &)
    {
    }

    ComPointer<TestFabricOperationData> innerComOperationPointer_;
};

class TestStoreService::ComTestAsyncEnumOperationData : public IFabricOperationDataStream, private Common::ComUnknownBase
{
    DENY_COPY(ComTestAsyncEnumOperationData)
        COM_INTERFACE_LIST1(
        ComTestAsyncEnumOperationData,
        IID_IFabricOperationDataStream,
        IFabricOperationDataStream)

public:

    ComTestAsyncEnumOperationData(TestStoreService & storeService, vector<__int64> && keys, wstring partitionId, FABRIC_SEQUENCE_NUMBER uptoSequenceNumber) 
        : storeService_(storeService), root_(storeService_.CreateComponentRoot()), keys_(keys), index_(0), partitionId_(partitionId), uptoSequenceNumber_(uptoSequenceNumber)
    {
    }

    virtual ~ComTestAsyncEnumOperationData() 
    {
    }

    HRESULT STDMETHODCALLTYPE BeginGetNext(
        /*[in]*/ IFabricAsyncOperationCallback * callback,
        /*[out, retval]*/ IFabricAsyncOperationContext ** context)
    {
        TestSession::WriteNoise(TraceSource, partitionId_, " {0}: BeginGetNext called", static_cast<void*>(this));

        if (storeService_.ShouldFailOn(ApiFaultHelper::Provider, L"BeginGetNextCopyState")) return E_FAIL;

        if (storeService_.IsSignalSet(ProviderBeginGetNextCopyStateBlock))
        {
            storeService_.WaitForSignalReset(ProviderBeginGetNextCopyStateBlock);
        }
        
        // The delay injection is intentionally done after the signal check above.
        // This is to enable the API to be delayed after a specific event of interest occurs in the fabric test script
        if (storeService_.ShouldFailOn(ApiFaultHelper::Provider, L"BeginGetNextCopyState", ApiFaultHelper::Delay))
        {
            DWORD delay = (DWORD)ApiFaultHelper::Get().GetApiDelayInterval().TotalSeconds();
            ::Sleep(delay * 1000);
        }

        if (storeService_.ShouldFailOn(ApiFaultHelper::Provider, L"EndGetNextCopyState")) 
        {
            ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();
            HRESULT hr = operation->Initialize(E_FAIL, root_, callback);
            TestSession::FailTestIf(FAILED(hr), "operation->Initialize should not fail");
            return ComAsyncOperationContext::StartAndDetach<ComCompletedAsyncOperationContext>(std::move(operation), context);
        }
        else
        {
            ComPointer<ComTestGetNextOperation> operation = make_com<ComTestGetNextOperation>();
            HRESULT hr = operation->Initialize(
                callback);
            if (FAILED(hr)) { return hr; }

            ComPointer<ComTestGetNextOperation> operationCopy(operation);
            MoveCPtr<ComTestGetNextOperation> mover(move(operationCopy));

            hr = ComAsyncOperationContext::StartAndDetach(std::move(operation), context);
            if (FAILED(hr)) { return hr; }

            Random random;
            if(index_ == 0 || random.NextDouble() > 0.5)
            {
                ComPointer<ComTestAsyncEnumOperationData> thisCPtr;
                thisCPtr.SetAndAddRef(this);
                Threadpool::Post([this, mover, thisCPtr] () mutable
                { 
                    GetNextOperation(move(mover.TakeSPtr())); 
                });
            }
            else
            {
                GetNextOperation(move(mover.TakeSPtr())); 
            }

            return S_OK;
        }
    }

    HRESULT STDMETHODCALLTYPE EndGetNext(
        /*[in]*/ IFabricAsyncOperationContext * context,
        /*[out, retval]*/ IFabricOperationData ** operation)
    {
        TestSession::WriteNoise(TraceSource, partitionId_, "{0}: EndGetNext called", static_cast<void*>(this));
        auto castedContext = dynamic_cast<ComCompletedAsyncOperationContext*>(context);
        if(castedContext != nullptr)
        {
            return ComCompletedAsyncOperationContext::End(context);
        }
        else
        {
            return ComTestGetNextOperation::End(context, operation);
        }
    }

private:
    TestStoreService & storeService_;
    ComponentRootSPtr root_;
    vector<__int64> keys_;
    size_t index_;
    wstring partitionId_;
    FABRIC_SEQUENCE_NUMBER uptoSequenceNumber_;

    //Currently there is only one loop for dispatching copy operations and for simplicity the test depends on this.
    void GetNextOperation(ComPointer<ComTestGetNextOperation> && operation)
    {
        bool entryFound = false;
        while(index_ < keys_.size())
        {
            shared_ptr<TestServiceEntry> const& entry = storeService_.GetEntry(keys_[index_]);
            ComPointer<TestFabricOperationData> operationData;
            index_++;

            if(entry->CheckVersion(uptoSequenceNumber_, operationData))
            {
                TestSession::WriteNoise(
                    TraceSource, 
                    partitionId_, 
                    "{0}: Copying operation with entry = {1}",
                    static_cast<void*>(this),
                    *operationData.GetRawPointer());

                operation->SetOperation(move(operationData));
                entryFound = true;
                break;
            }
            else
            {
                TestSession::WriteNoise(
                    TraceSource, 
                    partitionId_, 
                    "{0}: Not Copying operation with entry = {1}",
                    static_cast<void*>(this), 
                    *operationData.GetRawPointer());
            }
        }

        if(index_ == keys_.size() && !entryFound)
        {
            TestSession::WriteNoise(TraceSource, partitionId_, "{0}: Done copying", static_cast<void*>(this));
            ASSERT_IFNOT(operation->IsInnerComOperationPointerNull(), "Operation should be NULL after copy is done");
        }

        ComAsyncOperationContextCPtr baseOperation;
        baseOperation.SetNoAddRef(operation.DetachNoRelease());
        if (baseOperation->TryStartComplete())
        {
            baseOperation->FinishComplete(baseOperation);
        }
    }
};

HRESULT STDMETHODCALLTYPE TestStoreService::UpdateEpoch( 
    /* [in] */ FABRIC_EPOCH const * epoch,
    /* [in] */ FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber)
{
    UNREFERENCED_PARAMETER(previousEpochLastSequenceNumber);

    ASSERT_IF(streamFaultsAndRequireServiceAckEnabled_ && replicaRole_ == FABRIC_REPLICA_ROLE_NONE, "BeginUpdateEpoch should not be called when role is none");

    LONGLONG dataLossNumber = epoch->DataLossNumber;
    LONGLONG configurationNumber = epoch->ConfigurationNumber;
    TestSession::FailTestIf(
        dataLossNumber < dataLossNumber_ || (dataLossNumber == dataLossNumber_ && configurationNumber <= configurationNumber_), 
        "UpdateEpoch called with lower/equal epoch ({0}.{1} < {2}.{3})", dataLossNumber, configurationNumber, dataLossNumber_, configurationNumber_);
    TestSession::WriteNoise(TraceSource, partitionId_, "StateProvider.UpdateEpoch called: Epoch {0}.{1}, old DataLoss {2} at {3}", 
        dataLossNumber, configurationNumber, dataLossNumber_, nodeId_);
    dataLossNumber_ = dataLossNumber;
    configurationNumber_ = configurationNumber;

    return S_OK;
}


HRESULT STDMETHODCALLTYPE TestStoreService::GetCurrentProgress( 
    /* [retval][out] */ FABRIC_SEQUENCE_NUMBER *sequenceNumber)
{
    //Since there is no persisted data progreess is always 0
    *sequenceNumber = 0;
    return S_OK;
}


//The test does not recover from data loss
HRESULT TestStoreService::BeginOnDataLoss( 
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();

    HRESULT hr = operation->Initialize(
        ComponentRootSPtr(),
        callback);
    if (FAILED(hr)) { return hr; }

    ComAsyncOperationContextCPtr baseOperation;
    baseOperation.SetNoAddRef(operation.DetachNoRelease());
    hr = baseOperation->Start(baseOperation);
    if (FAILED(hr)) { return hr; }

    *context = baseOperation.DetachNoRelease();
    return S_OK;
}

HRESULT TestStoreService::EndOnDataLoss( 
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][string][out] */ BOOLEAN * isStateChanged)
{
    isStateChanged = FALSE;
    return ComCompletedAsyncOperationContext::End(context);
}

HRESULT TestStoreService::GetCopyContext(
    /*[out, retval]*/ IFabricOperationDataStream **)
{
    ASSERT_IFNOT(replicaRole_ == FABRIC_REPLICA_ROLE_IDLE_SECONDARY, "GetCopyContext should be called when role is idle");
    return E_NOTIMPL;
}

//TODO: think about batching the copy operations
HRESULT TestStoreService::GetCopyState(
    /*[in]*/ FABRIC_SEQUENCE_NUMBER uptoSequenceNumber,
    /*[in]*/ IFabricOperationDataStream *,
    /*[out, retval]*/ IFabricOperationDataStream ** copyStateEnumerator)
{
    vector<__int64> keys;
    AcquireReadLock grab(entryMapLock_);
    TestSession::WriteNoise(TraceSource, partitionId_, "Get Copy operations called upto SequenceNumber {0}, map size {1}", uptoSequenceNumber, entryMap_.size());
    for (auto iterator = entryMap_.begin() ; iterator != entryMap_.end(); iterator++ )
    {
        keys.push_back((*iterator).first);
    }

    *copyStateEnumerator = make_com<ComTestAsyncEnumOperationData>(*this, move(keys), partitionId_, uptoSequenceNumber).DetachNoRelease();
    return S_OK;
}
