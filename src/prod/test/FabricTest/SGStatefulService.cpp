// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace TestCommon;
using namespace TestHooks;
using namespace FabricTest;

class SGStatefulService::SGCopyContextReceiver
    : public ComponentRoot
{
    DENY_COPY(SGCopyContextReceiver);

public:
    SGCopyContextReceiver(
        ComPointer<IFabricOperationDataStream> && copyContextEnumCPtr,
        NodeId const & nodeId,
        wstring const & serviceName,
        wstring const & serviceType)
        : copyContextEnumCPtr_(move(copyContextEnumCPtr))
        , nodeId_(nodeId)
        , serviceName_(serviceName)
        , serviceType_(serviceType)
        , emptyCopyContextDataSeen_(false)
    {
        TestSession::WriteNoise(
            TraceSource, 
            "Node({0}) - ({1}) - SGCopyContextReceiver::ctor - serviceType({2}) serviceName({3})",
            nodeId_,
            this,
            serviceType_,
            serviceName_);
    }

    ~SGCopyContextReceiver()
    {
        TestSession::WriteNoise(
            TraceSource, 
            "Node({0}) - ({1}) - SGCopyContextReceiver::~dtor - serviceType({2}) serviceName({3})",
            nodeId_,
            this,
            serviceType_,
            serviceName_);
    }
    
    static HRESULT CopyContextProcessing(
        IFabricOperationDataStream * copyContextEnumerator,
        NodeId const & nodeId,
        wstring const & serviceName,
        wstring const & serviceType)
    {
        ComPointer<IFabricOperationDataStream> copyContextEnumCPtr;
        copyContextEnumCPtr.SetAndAddRef(copyContextEnumerator);
        std::shared_ptr<SGCopyContextReceiver> receiverSPtr = make_shared<SGCopyContextReceiver>(move(copyContextEnumCPtr), nodeId, serviceName, serviceType);

        while (receiverSPtr->GetNextCopyContextOperationData());
        return S_OK;
    }
    
    bool GetNextCopyContextOperationData()
    {
        TestSession::WriteNoise(
            TraceSource, 
            "Node({0}) - ({1}) - SGCopyContextReceiver::GetNextCopyContextOperationData - serviceType({2}) serviceName({3})",
            nodeId_,
            this,
            serviceType_,
            serviceName_);
    
        AsyncOperationSPtr inner = AsyncOperation::CreateAndStart<SGComProxyOperationDataEnumAsyncOperation>(
            copyContextEnumCPtr_,
            [this](AsyncOperationSPtr const & asyncOperation) -> void
            {
                if (!asyncOperation->CompletedSynchronously)
                {
                    if (this->FinishGetNextCopyContext(asyncOperation))
                    {
                        GetNextCopyContextOperationData();
                    }
                }
            },
            CreateAsyncOperationRoot());

        if (inner->CompletedSynchronously)
        {
            TestSession::WriteNoise(
                TraceSource, 
                "Node({0}) - ({1}) - SGCopyContextReceiver::GetNextCopyContextOperationData - serviceType({2}) serviceName({3}) - Completed synchronously",
                nodeId_,
                this,
                serviceType_,
                serviceName_);
    
            if (FinishGetNextCopyContext(inner))
            {
                return true;
            }
        }
        else
        {
            TestSession::WriteNoise(
                TraceSource, 
                "Node({0}) - ({1}) - SGCopyContextReceiver::GetNextCopyContextOperationData - serviceType({2}) serviceName({3}) - asyncOperation({4}) - Completed Asynchronously",
                nodeId_,
                this,
                serviceType_,
                serviceName_,
                inner.get());
        }
    
        return false;
    }
    
    bool FinishGetNextCopyContext(
        AsyncOperationSPtr const & asyncOperation)
    {
        auto enumerator = AsyncOperation::Get<SGComProxyOperationDataEnumAsyncOperation>(asyncOperation);
    
        if (enumerator->OperationData)
        {
            ULONG bufferCount = 0;
            FABRIC_OPERATION_DATA_BUFFER const * replicaBuffers = nullptr;
            
            enumerator->OperationData->GetData(&bufferCount, &replicaBuffers);
    
            if (0 == bufferCount)
            {
                TestSession::WriteNoise(
                    TraceSource, 
                    "Node({0}) - ({1}) - SGCopyContextReceiver::FinishGetNextCopyContext - serviceType({2}) serviceName({3}) - asyncOperation({4}) - empty operation data)",
                    nodeId_,
                    this,
                    serviceType_,
                    serviceName_,
                    asyncOperation.get());
                emptyCopyContextDataSeen_ = true;
            }
            else
            {
                TestSession::FailTestIf(!emptyCopyContextDataSeen_,
                    "Node({0}) - ({1}) serviceType({2}) serviceName({3}) - empty operation data has not been seen in copy context!",
                    nodeId_,
                    this,
                    serviceType_,
                    serviceName_);
                emptyCopyContextDataSeen_ = false;
    
                TestSession::FailTestIf(bufferCount != 2, "buffer count must be 2");
                TestSession::FailTestIf(replicaBuffers[0].BufferSize != sizeof(ULONGLONG), "wrong buffer size");
                TestSession::FailTestIf(replicaBuffers[1].BufferSize != sizeof(ULONGLONG), "wrong buffer size");
                TestSession::FailTestIf(replicaBuffers[0].Buffer == NULL, "null replica buffer");
                TestSession::FailTestIf(replicaBuffers[1].Buffer == NULL, "null replica buffer");
        
                TestSession::WriteNoise(
                    TraceSource, 
                    "Node({0}) - ({1}) - SGCopyContextReceiver::FinishGetNextCopyContext - serviceType({2}) serviceName({3}) - asyncOperation({4}) value({5})",
                    nodeId_,
                    this,
                    serviceType_,
                    serviceName_,
                    asyncOperation.get(),
                    (__int64)((*(ULONGLONG*)replicaBuffers[0].Buffer)*3 + (*(ULONGLONG*)replicaBuffers[1].Buffer)));
            }
            return true;
        }
    
        TestSession::WriteNoise(
            TraceSource, 
            "Node({0}) - ({1}) - SGCopyContextReceiver::FinishGetNextCopyContext - serviceType({2}) serviceName({3}) - asyncOperation({4}) - NULL operationData",
            nodeId_,
            this,
            serviceType_,
            serviceName_,
            asyncOperation.get());
    
        return false;
    }

private:
    bool emptyCopyContextDataSeen_;
    ComPointer<IFabricOperationDataStream> copyContextEnumCPtr_;
    NodeId nodeId_;
    wstring serviceName_;
    wstring serviceType_;

    static Common::StringLiteral const TraceSource;
};

//
// Constructor/Destructor.
//
SGStatefulService::SGStatefulService(
    __in NodeId nodeId,
    __in LPCWSTR serviceType,
    __in LPCWSTR serviceName,
    __in ULONG initializationDataLength,
    __in const byte* initializationData,
    __in FABRIC_REPLICA_ID replicaId)
    : serviceType_(serviceType)
    , serviceName_(serviceName)
    , emptyOperationDataSeen_(true)
    , replicateEmptyOperationData_(true)
    , rollbackCount_(0)
    , catchupDone_(true)
    , replicationDone_(true)
    , isStopping_(false)
    , isGetCopyStateCalled_(false)
{
    UNREFERENCED_PARAMETER(initializationDataLength);
    UNREFERENCED_PARAMETER(initializationData);

    TestSession::WriteNoise(
        TraceSource, 
        "Node({0}) - ({1}) - SGStatefulService::SGStatefulService - ctor - serviceType({2}) serviceName({3})",
        nodeId,
        this,
        serviceType,
        serviceName);

    nodeId_ = nodeId;
    replicaId_ = replicaId;

    state_ = 0;
    currentSequenceNumber_ = 0;
    previousEpochLastLsn_ = 0;
    operationCount_ = 0;
    currentReplicaRole_ = FABRIC_REPLICA_ROLE_NONE;
    majorReplicaRole_ = FABRIC_REPLICA_ROLE_NONE;
    workItemReplication_ = NULL;
    selfReplication_ = nullptr;
    workItemCatchUpCopy_ = NULL;
    selfCatchUpCopy_ = nullptr;
    workItemReportMetrics_ = NULL;
    selfReportMetrics_ = nullptr;

    returnEmptyCopyContext_ = false;
    returnEmptyCopyState_ = false;
    returnNullCopyContext_ = false;
    returnNullCopyState_ = false;
}

SGStatefulService::~SGStatefulService()
{
    TestSession::WriteNoise(
        TraceSource, 
        "Node({0}) - ({1}) - SGStatefulService::~SGStatefulService - dtor - serviceType({2}) serviceName({3})",
        nodeId_,
        this,
        serviceType_,
        serviceName_);
}

//
// Initialization/Cleanup methods.
//
HRESULT SGStatefulService::FinalConstruct(
    __in bool returnEmptyCopyContext,
    __in bool returnEmptyCopyState,
    __in bool returnNullCopyContext,
    __in bool returnNullCopyState,
    __in wstring const & initData)
{
    returnEmptyCopyContext_ = returnEmptyCopyContext;
    returnEmptyCopyState_ = returnEmptyCopyState;
    returnNullCopyContext_ = returnNullCopyContext;
    returnNullCopyState_ = returnNullCopyState;

    StringCollection collection;
    StringUtility::Split(initData, collection, wstring(L" "));

    settings_ = make_unique<TestCommon::CommandLineParser>(collection);

    return S_OK;
}

HRESULT SGStatefulService::OnOpen(
    __in ComPointer<IFabricStatefulServicePartition> && statefulPartition,
    __in ComPointer<IFabricStateReplicator> && stateReplicator)
{
    TestSession::WriteNoise(
        TraceSource, 
        "Node({0}) - ({1}) - SGStatefulService::OnOpen - serviceType({2}) serviceName({3}) - statefulPartition({4}) stateReplicator({5})",
        nodeId_,
        this,
        serviceType_,
        serviceName_,
        statefulPartition.GetRawPointer(),
        stateReplicator.GetRawPointer());

    statefulPartition_ = std::move(statefulPartition);
    stateReplicator_ = std::move(stateReplicator);
    return S_OK;
}

HRESULT SGStatefulService::OnChangeRole(
    __in FABRIC_REPLICA_ROLE newRole)
{
    TestSession::WriteNoise(
        TraceSource, 
        "Node({0}) - ({1}) - SGStatefulService::OnChangeRole - serviceType({2}) serviceName({3}) Role({4} ==> {5})",
        nodeId_,
        this,
        serviceType_,
        serviceName_,
        currentReplicaRole_,
        newRole);

    if (!IsSignalSet(ServiceGroupSkipValidation))
    {
        if (FABRIC_REPLICA_ROLE_PRIMARY == currentReplicaRole_)
        {
            if (settings_->GetBool(L"onchangerole.primary_to_secondary.failtest"))
            {
                TestSession::FailTestIf(FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY == newRole, "Unexpected role change");
            }
        }
    }

    CreateWorkload(newRole);

    if (FABRIC_REPLICA_ROLE_PRIMARY == newRole || FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY == newRole)
    {
        majorReplicaRole_ = newRole;
    }
    currentReplicaRole_ = newRole;

    if (!IsSignalSet(ServiceGroupSkipValidation))
    {
        if (FABRIC_REPLICA_ROLE_NONE == newRole)
        {
            int desiredRollbackCount = 0;
            if (settings_->TryGetInt(L"onchangerole.none.assert.rollbackcount", desiredRollbackCount, 0))
            {
                TestSession::FailTestIfNot(rollbackCount_ == desiredRollbackCount, "Expected rollbackCount {0} was {1}", desiredRollbackCount, rollbackCount_);
            }

            int newRollbackCount = 0;
            if (settings_->TryGetInt(L"onchangerole.none.put.rollbackcount", newRollbackCount, 0))
            {
                rollbackCount_ = newRollbackCount;
            }

            int64 desiredPreviousEpochLastLsnOnSecondary = 0;
            if (settings_->TryGetInt64(L"onchangerole.none.assert.previousEpochLastLsn", desiredPreviousEpochLastLsnOnSecondary, 0))
            {
                TestSession::FailTestIfNot(previousEpochLastLsn_ == static_cast<FABRIC_SEQUENCE_NUMBER>(desiredPreviousEpochLastLsnOnSecondary), "Expected previousEpochLastLsn {0} was {1}", desiredPreviousEpochLastLsnOnSecondary, previousEpochLastLsn_);
            }
        }
    }

    return S_OK;
}

HRESULT SGStatefulService::OnClose()
{
    TestSession::WriteNoise(
        TraceSource, 
        "Node({0}) - ({1}) - SGStatefulService::OnClose - serviceType({2}) serviceName({3})",
        nodeId_,
        this,
        serviceType_,
        serviceName_);

    if (!IsSignalSet(ServiceGroupSkipValidation))
    {
        if (majorReplicaRole_ == FABRIC_REPLICA_ROLE_PRIMARY)
        {
            int64 desiredState = 0;
            if (settings_->TryGetInt64(L"primary.onclose.assert.state", desiredState, 0))
            {
                TestSession::FailTestIfNot(state_ == static_cast<ULONGLONG>(desiredState), "Expected state {0} was {1}", desiredState, state_);
            }

            FABRIC_SEQUENCE_NUMBER desiredLastCommited = 0;
            if (settings_->TryGetInt64(L"primary.onclose.assert.lastcommited", desiredLastCommited, 0))
            {
                TestSession::FailTestIfNot(currentSequenceNumber_ == desiredLastCommited, "Expected lsn {0} was {1}", desiredLastCommited, currentSequenceNumber_);
            }

            int64 desiredPreviousEpochLastLsnOnPrimary = 0;
            if (settings_->TryGetInt64(L"primary.onclose.assert.previousEpochLastLsn", desiredPreviousEpochLastLsnOnPrimary, 0))
            {
                TestSession::FailTestIfNot(previousEpochLastLsn_ == static_cast<FABRIC_SEQUENCE_NUMBER>(desiredPreviousEpochLastLsnOnPrimary), "Expected previousEpochLastLsn {0} was {1}", desiredPreviousEpochLastLsnOnPrimary, previousEpochLastLsn_);
            }
        }

        if (majorReplicaRole_ == FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY)
        {
            int64 desiredPreviousEpochLastLsnOnSecondary = 0;
            if (settings_->TryGetInt64(L"secondary.onclose.assert.previousEpochLastLsn", desiredPreviousEpochLastLsnOnSecondary, 0))
            {
                TestSession::FailTestIfNot(previousEpochLastLsn_ == static_cast<FABRIC_SEQUENCE_NUMBER>(desiredPreviousEpochLastLsnOnSecondary), "Expected previousEpochLastLsn {0} was {1}", desiredPreviousEpochLastLsnOnSecondary, previousEpochLastLsn_);
            }
        }

        int desiredRollbackCount = 0;
        if (settings_->TryGetInt(L"onclose.assert.rollbackcount", desiredRollbackCount, 0))
        {
            TestSession::FailTestIfNot(rollbackCount_ == desiredRollbackCount, "Expected rollbackCount {0} was {1}", desiredRollbackCount, rollbackCount_);
        }

        int64 desiredPreviousEpochLastLsn = 0;
        if (settings_->TryGetInt64(L"onclose.assert.previousEpochLastLsn", desiredPreviousEpochLastLsn, 0))
        {
            TestSession::FailTestIfNot(previousEpochLastLsn_ == static_cast<FABRIC_SEQUENCE_NUMBER>(desiredPreviousEpochLastLsn), "Expected previousEpochLastLsn {0} was {1}", desiredPreviousEpochLastLsn, previousEpochLastLsn_);
        }

        if (settings_->GetBool(L"onclose.assert.roleisprimaryorsecondary"))
        {
            TestSession::FailTestIfNot(
                currentReplicaRole_ == FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY || currentReplicaRole_ == FABRIC_REPLICA_ROLE_PRIMARY,
                "Expected currentReplicaRole primary or secondary, was {0}", (int)currentReplicaRole_);
        }
    }

    isStopping_ = true;
    HRESULT hr = StopWorkload();

    if (!IsSignalSet(ServiceGroupSkipValidation))
    {
        if (settings_->GetBool(L"after.close.check"))
        {
            IFabricStatefulServicePartition* statefulPartition = statefulPartition_.GetRawPointer();
            statefulPartition->AddRef();

            IFabricStateReplicator* stateReplicator = stateReplicator_.GetRawPointer();
            stateReplicator->AddRef();

            IFabricStateProvider* stateProvider = make_com<SGComStatefulService, IFabricStateProvider>(*this).DetachNoRelease();

            Threadpool::Post(
                [statefulPartition, stateReplicator, stateProvider]() -> void
                {
                    FABRIC_SERVICE_PARTITION_ACCESS_STATUS status;
                    HRESULT hr = statefulPartition->GetWriteStatus(&status);
                    TestSession::FailTestIfNot(ErrorCode::FromHResult(hr).IsError(ErrorCodeValue::ObjectClosed), "GetWriteStatus expected ObjectClosed");

                    FABRIC_SERVICE_PARTITION_INFORMATION const * partitionInfo;
                    hr = statefulPartition->GetPartitionInfo(&partitionInfo);
                    TestSession::FailTestIfNot(ErrorCode::FromHResult(hr).IsError(ErrorCodeValue::ObjectClosed), "GetPartitionInfo expected ObjectClosed");

                    FABRIC_LOAD_METRIC metrics = { 0 };
                    hr = statefulPartition->ReportLoad(1, &metrics);
                    TestSession::FailTestIfNot(ErrorCode::FromHResult(hr).IsError(ErrorCodeValue::ObjectClosed), "ReportLoad expected ObjectClosed");

                    hr = statefulPartition->ReportFault(FABRIC_FAULT_TYPE_PERMANENT);
                    TestSession::FailTestIfNot(ErrorCode::FromHResult(hr).IsError(ErrorCodeValue::ObjectClosed), "ReportFault expected ObjectClosed");

                    hr = statefulPartition->GetReadStatus(&status);
                    TestSession::FailTestIfNot(ErrorCode::FromHResult(hr).IsError(ErrorCodeValue::ObjectClosed), "GetReadStatus expected ObjectClosed");

                    IFabricReplicator* replicator;
                    IFabricStateReplicator* dummyStateReplicator;
                    hr = statefulPartition->CreateReplicator(stateProvider, NULL, &replicator, &dummyStateReplicator);
                    TestSession::FailTestIfNot(ErrorCode::FromHResult(hr).IsError(ErrorCodeValue::ObjectClosed), "CreateReplicator expected ObjectClosed");

                    IFabricServiceGroupPartition* serviceGroupPartition = NULL;
                    hr = statefulPartition->QueryInterface(IID_IFabricServiceGroupPartition, (void**) &serviceGroupPartition);
                    TestSession::FailTestIf(FAILED(hr), "Expected ServiceGroupPartition");
                    IUnknown* member;
                    hr = serviceGroupPartition->ResolveMember(L"fabric:/group/member", IID_IUnknown, (void**)(&member));
                    TestSession::FailTestIfNot(ErrorCode::FromHResult(hr).IsError(ErrorCodeValue::ObjectClosed), "ResolveMember expected ObjectClosed");
                    statefulPartition->Release();

                    Common::ComPointer<IFabricOperationData> replicationData = Common::make_com<SGEmptyOperationData, IFabricOperationData>();
                    FABRIC_SEQUENCE_NUMBER sequenceNumber;
                    IFabricAsyncOperationContext* context;
                    hr = stateReplicator->BeginReplicate(replicationData.GetRawPointer(), NULL, &sequenceNumber, &context);
                    TestSession::FailTestIfNot(ErrorCode::FromHResult(hr).IsError(ErrorCodeValue::ObjectClosed), "BeginReplicate expected ObjectClosed");

                    IFabricOperationStream* stream;
                    hr = stateReplicator->GetCopyStream(&stream);
                    TestSession::FailTestIfNot(ErrorCode::FromHResult(hr).IsError(ErrorCodeValue::ObjectClosed), "GetCopyStream expected ObjectClosed");

                    hr = stateReplicator->GetReplicationStream(&stream);
                    TestSession::FailTestIfNot(ErrorCode::FromHResult(hr).IsError(ErrorCodeValue::ObjectClosed), "GetReplicationStream expected ObjectClosed");

                    IFabricAtomicGroupStateReplicator* atomicGroupStateReplicator = NULL;
                    hr = stateReplicator->QueryInterface(IID_IFabricAtomicGroupStateReplicator, (void**) &atomicGroupStateReplicator);
                    if (SUCCEEDED(hr))
                    {
                        FABRIC_ATOMIC_GROUP_ID atomicGroupId;
                        hr = atomicGroupStateReplicator->CreateAtomicGroup(&atomicGroupId);
                        TestSession::FailTestIfNot(ErrorCode::FromHResult(hr).IsError(ErrorCodeValue::ObjectClosed), "CreateAtomicGroup expected ObjectClosed");

                        atomicGroupId = 1;

                        hr = atomicGroupStateReplicator->BeginReplicateAtomicGroupOperation(atomicGroupId, replicationData.GetRawPointer(), NULL, &sequenceNumber, &context);
                        TestSession::FailTestIfNot(ErrorCode::FromHResult(hr).IsError(ErrorCodeValue::ObjectClosed), "BeginReplicateAtomicGroupOperation expected ObjectClosed");

                        hr = atomicGroupStateReplicator->BeginReplicateAtomicGroupCommit(atomicGroupId, NULL, &sequenceNumber, &context);
                        TestSession::FailTestIfNot(ErrorCode::FromHResult(hr).IsError(ErrorCodeValue::ObjectClosed), "BeginReplicateAtomicGroupCommit expected ObjectClosed");

                        hr = atomicGroupStateReplicator->BeginReplicateAtomicGroupRollback(atomicGroupId, NULL, &sequenceNumber, &context);
                        TestSession::FailTestIfNot(ErrorCode::FromHResult(hr).IsError(ErrorCodeValue::ObjectClosed), "BeginReplicateAtomicGroupRollback expected ObjectClosed");

                        atomicGroupStateReplicator->Release();
                    }

                    statefulPartition->Release();
                    stateReplicator->Release();
                    stateProvider->Release();
                },
                TimeSpan::FromMilliseconds(2000.0)
                );
        }
    }

    return hr;
}

void SGStatefulService::OnAbort()
{
    TestSession::WriteNoise(
            TraceSource, 
            "Node({0}) - ({1}) - SGStatefulService::OnAbort - serviceType({2}) serviceName({3})",
            nodeId_,
            this,
            serviceType_,
            serviceName_);

    isStopping_ = true;
    StopWorkload();
}

HRESULT SGStatefulService::OnDataLoss()
{
    TestSession::WriteNoise(
        TraceSource, 
        "Node({0}) - ({1}) - SGStatefulService::OnDataLoss - serviceType({2}) serviceName({3})",
        nodeId_,
        this,
        serviceType_,
        serviceName_);

    return S_OK;
}

HRESULT SGStatefulService::UpdateEpoch(
    __in FABRIC_EPOCH const* epoch, 
    __in FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber)
{
    TestSession::WriteNoise(
        TraceSource, 
        "Node({0}) - ({1}) - SGStatefulService::UpdateEpoch - serviceType({2}) serviceName({3})",
        nodeId_,
        this,
        serviceType_,
        serviceName_);

    if (NULL == epoch) { return E_POINTER; }

    previousEpochLastLsn_ = previousEpochLastSequenceNumber;

    if (FABRIC_REPLICA_ROLE_PRIMARY == currentReplicaRole_)
    {
        if (settings_->GetBool(L"primary.onupdateepoch.restartreplication"))
        {
            // on update epoch restart replicating as if we just became primary
            currentReplicaRole_ = FABRIC_REPLICA_ROLE_NONE;
            CreateWorkload(FABRIC_REPLICA_ROLE_PRIMARY);
            currentReplicaRole_ = FABRIC_REPLICA_ROLE_PRIMARY;
        }
    }

    return S_OK;
}

HRESULT SGStatefulService::GetCurrentProgress(
    __out FABRIC_SEQUENCE_NUMBER* sequenceNumber)
{
    if (NULL == sequenceNumber) { return E_POINTER; }

    FABRIC_SEQUENCE_NUMBER s;

    if (settings_->TryGetInt64(L"getcurrentprogress", s, currentSequenceNumber_))
    {
        TestSession::WriteNoise(
            TraceSource, 
            "Node({0}) - ({1}) - SGStatefulService::GetCurrentProgress - serviceType({2}) serviceName({3}) SequenceNumber({4}) - using dummy sequenceNumber",
            nodeId_,
            this,
            serviceType_,
            serviceName_,
            s);
    }
    else
    {
        TestSession::WriteNoise(
            TraceSource, 
            "Node({0}) - ({1}) - SGStatefulService::GetCurrentProgress - serviceType({2}) serviceName({3}) SequenceNumber({4})",
            nodeId_,
            this,
            serviceType_,
            serviceName_,
            s);
    }

    *sequenceNumber = s;
    return S_OK;
}

HRESULT SGStatefulService::GetCopyContext(
    __out IFabricOperationDataStream** copyContextEnumerator)
{
    TestSession::WriteNoise(
        TraceSource, 
        "Node({0}) - ({1}) - SGStatefulService::GetCopyContext - serviceType({2}) serviceName({3})",
        nodeId_,
        this,
        serviceType_,
        serviceName_);

    if (returnNullCopyContext_)
    {
        TestSession::WriteNoise(
            TraceSource, 
            "Node({0}) - ({1}) - SGStatefulService::GetCopyContext - serviceType({2}) serviceName({3}) - Return NULL enumerator",
            nodeId_,
            this,
            serviceType_,
            serviceName_);

        *copyContextEnumerator = NULL;
        return S_OK;
    }

    Common::ComPointer<SGUlonglongOperationData> operationData = Common::make_com<SGUlonglongOperationData>((ULONGLONG)-1);
    Common::ComPointer<IFabricOperationData> opData;
    opData.SetNoAddRef(operationData.DetachNoRelease());
    Common::ComPointer<SGOperationDataAsyncEnumerator> operationDataEnumerator = Common::make_com<SGOperationDataAsyncEnumerator>(
        std::move(opData),
        ShouldFailOn(ApiFaultHelper::Provider, L"BeginGetNextCopyContext"),
        ShouldFailOn(ApiFaultHelper::Provider, L"EndGetNextCopyContext"),
        returnEmptyCopyContext_);
    operationDataEnumerator->QueryInterface(IID_IFabricOperationDataStream, reinterpret_cast<void**>(copyContextEnumerator));

    TestSession::WriteNoise(
        TraceSource, 
        "Node({0}) - ({1}) - SGStatefulService::GetCopyContext - serviceType({2}) serviceName({3}) - Return copyContextEnumerator({4})",
        nodeId_,
        this,
        serviceType_,
        serviceName_,
        *copyContextEnumerator);

    return S_OK;
}

HRESULT SGStatefulService::GetCopyState(
    __in FABRIC_SEQUENCE_NUMBER uptoSequenceNumber,
    __in IFabricOperationDataStream* copyContextEnumerator,
    __out IFabricOperationDataStream** copyStateEnumerator)
{
    TestSession::WriteNoise(
        TraceSource, 
        "Node({0}) - ({1}) - SGStatefulService::GetCopyState - serviceType({2}) serviceName({3}) uptoSequenceNumber({4}) currentSequenceNumber_({5})",
        nodeId_,
        this,
        serviceType_,
        serviceName_,
        uptoSequenceNumber,
        currentSequenceNumber_);

    isGetCopyStateCalled_ = true;

    if (NULL != copyContextEnumerator)
    {
        SGCopyContextReceiver::CopyContextProcessing(copyContextEnumerator, nodeId_, serviceName_, serviceType_);
    }

    if (returnNullCopyState_)
    {
        TestSession::WriteNoise(
            TraceSource, 
            "Node({0}) - ({1}) - SGStatefulService::GetCopyState - serviceType({2}) serviceName({3}) - Return NULL enumerator",
            nodeId_,
            this,
            serviceType_,
            serviceName_);
        *copyStateEnumerator = NULL;
        return S_OK;
    }

    if (uptoSequenceNumber < currentSequenceNumber_)
    {
        TestSession::WriteNoise(
            TraceSource, 
            "Node({0}) - ({1}) - SGStatefulService::GetCopyState - serviceType({2}) serviceName({3}) - Return NULL enumerator: uptoSequenceNumber < currentSequenceNumber_",
            nodeId_,
            this,
            serviceType_,
            serviceName_);
        *copyStateEnumerator = NULL;
        return S_OK;
    }

    ULONGLONG value = 0;
    GetCurrentState(&value);
    Common::ComPointer<SGUlonglongOperationData> operationData = Common::make_com<SGUlonglongOperationData>(value);
    Common::ComPointer<IFabricOperationData> opData;
    opData.SetNoAddRef(operationData.DetachNoRelease());
    Common::ComPointer<SGOperationDataAsyncEnumerator> operationDataEnumerator = Common::make_com<SGOperationDataAsyncEnumerator>(
        std::move(opData),
        ShouldFailOn(ApiFaultHelper::Provider, L"BeginGetNextCopyState"),
        ShouldFailOn(ApiFaultHelper::Provider, L"EndGetNextCopyState"),
        returnEmptyCopyState_);
    operationDataEnumerator->QueryInterface(IID_IFabricOperationDataStream, reinterpret_cast<void**>(copyStateEnumerator));

    TestSession::WriteNoise(
        TraceSource, 
        "Node({0}) - ({1}) - SGStatefulService::GetCopyState - serviceType({2}) serviceName({3}) - Return copyStateEnumerator({4}) - empty operation data followed by state({5}).",
        nodeId_,
        this,
        serviceType_,
        serviceName_,
        *copyStateEnumerator,
        value);

    return S_OK;
}

HRESULT SGStatefulService::OnAtomicGroupCommit(
    __in FABRIC_ATOMIC_GROUP_ID atomicGroupId, 
    __in FABRIC_SEQUENCE_NUMBER commitSequenceNumber)
{
    TestSession::WriteNoise(
        TraceSource, 
        "Node({0}) - ({1}) - SGStatefulService::OnAtomicGroupCommit - serviceType({2}) serviceName({3}) - atomicGroupId ({4}) commitSequenceNumber ({5})",
        nodeId_,
        this,
        serviceType_,
        serviceName_,
        atomicGroupId,
        commitSequenceNumber);

    return S_OK;
}

HRESULT SGStatefulService::OnAtomicGroupRollback(
    __in FABRIC_ATOMIC_GROUP_ID atomicGroupId, 
    __in FABRIC_SEQUENCE_NUMBER rollbackSequenceNumber)
{
    TestSession::WriteNoise(
        TraceSource, 
        "Node({0}) - ({1}) - SGStatefulService::OnAtomicGroupRollback - serviceType({2}) serviceName({3}) - atomicGroupId ({4}) rollbackSequenceNumber ({5})",
        nodeId_,
        this,
        serviceType_,
        serviceName_,
        atomicGroupId,
        rollbackSequenceNumber);

    rollbackCount_ += 1;

    return S_OK;
}

HRESULT SGStatefulService::OnUndoProgress(__in FABRIC_SEQUENCE_NUMBER fromCommitSequenceNumber)
{
    TestSession::WriteNoise(
        TraceSource, 
        "Node({0}) - ({1}) - SGStatefulService::OnUndoProgress - serviceType({2}) serviceName({3}) - fromCommitSequenceNumber ({4})",
        nodeId_,
        this,
        serviceType_,
        serviceName_,
        fromCommitSequenceNumber);

    return S_OK;
}

HRESULT SGStatefulService::ApplyState(
    __in BOOLEAN isCopy,
    __in FABRIC_SEQUENCE_NUMBER operationSequenceNumber, 
    __in ULONGLONG state)
{
    Common::AcquireWriteLock grab(lock_);

    TestSession::WriteNoise(
        TraceSource, 
        "Node({0}) - ({1}) - SGStatefulService::ApplyState - serviceType({2}) serviceName({3}) Type({4}) SequenceNumber({5} - {6}) State({7} ==> {8})",
        nodeId_,
        this,
        serviceType_,
        serviceName_,
        isCopy ? "COPY" : "REPL",
        currentSequenceNumber_,
        operationSequenceNumber,
        state_,
        state);

    currentSequenceNumber_ = operationSequenceNumber;
    state_ = state;

    return S_OK;
}

HRESULT SGStatefulService::ApplyStateInPrimary(
    __in FABRIC_SEQUENCE_NUMBER operationSequenceNumber, 
    __in ULONGLONG state)
{
    Common::AcquireWriteLock grab(lock_);

    TestSession::WriteNoise(
        TraceSource, 
        "Node({0}) - ({1}) - SGStatefulService::ApplyStateInPrimary - serviceType({2}) serviceName({3}) SequenceNumber({4} - {5}) State({6} ==> {7})",
        nodeId_,
        this,
        serviceType_,
        serviceName_,
        currentSequenceNumber_,
        operationSequenceNumber,
        state_,
        state);

    currentSequenceNumber_ = operationSequenceNumber;
    state_ = state;

    return S_OK;
}

HRESULT SGStatefulService::GetCurrentState(
    __out ULONGLONG* state)
{
    TestSession::FailTestIf(NULL == state, "state");

    Common::AcquireReadLock grab(lock_);
    *state = state_;

    return S_OK;
}

void CALLBACK SGStatefulService::ThreadPoolWorkItemCatchUpCopyAndReplication(
    __inout PTP_CALLBACK_INSTANCE Instance,
    __inout_opt PVOID Context,
    __inout PTP_WORK Work)
{
    UNREFERENCED_PARAMETER(Instance);
    UNREFERENCED_PARAMETER(Work);

    SGStatefulService* service = (SGStatefulService*)Context;
    TestSession::WriteNoise(
        TraceSource, 
        "Node({0}) - ({1}) - SGStatefulService::ThreadPoolWorkItemCatchUpCopyAndReplication - serviceType({2}) serviceName({3}) - Context({4})",
        service->nodeId_,
        service,
        service->serviceType_,
        service->serviceName_,
        Context);

    service->CatchUpCopyProcessing();
    service->selfCatchUpCopy_.reset();
}

void CALLBACK SGStatefulService::ThreadPoolWorkItemReplication(
    __inout PTP_CALLBACK_INSTANCE Instance,
    __inout_opt PVOID Context,
    __inout PTP_WORK Work)
{
    UNREFERENCED_PARAMETER(Instance);
    UNREFERENCED_PARAMETER(Work);

    SGStatefulService* service = (SGStatefulService*)Context;
    TestSession::WriteNoise(
        TraceSource, 
        "Node({0}) - ({1}) - SGStatefulService::ThreadPoolWorkItemReplication - serviceType({2}) serviceName({3}) - Context({4})",
        service->nodeId_,
        service,
        service->serviceType_,
        service->serviceName_,
        Context);

    ULONG retryCount = 60;
    FABRIC_SERVICE_PARTITION_ACCESS_STATUS spwas;
    HRESULT hr = service->statefulPartition_->GetWriteStatus(&spwas);
    while (FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED != spwas)
    {
        TestSession::WriteNoise(
            TraceSource, 
            "Node({0}) - ({1}) - SGStatefulService::ThreadPoolWorkItemReplication - serviceType({2}) serviceName({3}) - Write access not granted, will retry",
            service->nodeId_,
            service,
            service->serviceType_,
            service->serviceName_);

        if (0 == --retryCount) { break; }
        ::Sleep(500);
        hr = service->statefulPartition_->GetWriteStatus(&spwas);
    }
    if (SUCCEEDED(hr))
    {
        service->ResetReplicateStatus();
        hr = service->ReplicationProcessing();
    }
    else
    {
        TestSession::WriteNoise(
            TraceSource, 
            "Node({0}) - ({1}) - SGStatefulService::ReplicaNextState - serviceType({2}) serviceName({3}) - Replication completed (no write access)",
            service->nodeId_,
            service,
            service->serviceType_,
            service->serviceName_
            );

        service->replicationDone_.Set();
    }
    service->selfReplication_.reset();
}

void CALLBACK SGStatefulService::ThreadPoolWorkItemReportMetrics(
    __inout PTP_CALLBACK_INSTANCE Instance,
    __inout_opt PVOID Context,
    __inout PTP_WORK Work)
{
    UNREFERENCED_PARAMETER(Instance);
    UNREFERENCED_PARAMETER(Work);

    SGStatefulService* service = (SGStatefulService*)Context;
    TestSession::WriteNoise(
        TraceSource, 
        "Node({0}) - ({1}) - SGStatefulService::ThreadPoolWorkItemReportMetrics - serviceType({2}) serviceName({3}) - Context({4})",
        service->nodeId_,
        service,
        service->serviceType_,
        service->serviceName_,
        Context);

    for (int i = 0; i < 10; ++i)
    {
        service->PrimaryReportMetricsRandom();
        ::Sleep(500);
    }

    service->selfReportMetrics_.reset();
}

HRESULT SGStatefulService::CatchUpCopyProcessing()
{
    if (!copyStream_)
    {
        HRESULT hr = stateReplicator_->GetCopyStream(copyStream_.InitializationAddress());

        if (FAILED(hr))
        {
            TestSession::WriteNoise(
                TraceSource, 
                "Node({0}) - ({1}) - SGStatefulService::CatchUpCopyProcessing - serviceType({2}) serviceName({3}) - GetCopyStream failed with ({4})",
                nodeId_,
                this,
                serviceType_,
                serviceName_,
                hr);
            return hr;
        }
        else
        {
            TestSession::WriteNoise(
                TraceSource, 
                "Node({0}) - ({1}) - SGStatefulService::CatchUpCopyProcessing - serviceType({2}) serviceName({3}) - copyStream_({4})",
                nodeId_,
                this,
                serviceType_,
                serviceName_,
                copyStream_.GetRawPointer());
        }

        if (IsSignalSet(ServiceGroupAfterGetCopyStreamReportFaultPermanent))
        {
            hr = statefulPartition_->ReportFault(FABRIC_FAULT_TYPE_PERMANENT);
            TestSession::FailTestIf(FAILED(hr), "ReportFault failed unexpectedly.");

            return S_OK;
        }
    }

    while (ProcessNextCatchupOperation(copyStream_, true));
    return S_OK;
}

bool SGStatefulService::ProcessNextCatchupOperation(
    Common::ComPointer<IFabricOperationStream> const & stream,
    bool isCopy)
{
    TestSession::WriteNoise(
        TraceSource, 
        "Node({0}) - ({1}) - SGStatefulService::ProcessNextCatchupOperation - serviceType({2}) serviceName({3}) - Type({4})",
        nodeId_,
        this,
        serviceType_,
        serviceName_,
        isCopy ? "COPY" : "REPL");

    AsyncOperationSPtr inner = AsyncOperation::CreateAndStart<SGComProxyOperationStreamAsyncOperation>(
        stream,
        isCopy,
        [this, isCopy](AsyncOperationSPtr const & asyncOperation) -> void
        {
            if (!asyncOperation->CompletedSynchronously)
            {
                bool isLast = false;
                if (this->FinishProcessingCatchupOperation(asyncOperation, isLast))
                {
                    if (!isLast)
                    {
                        if (isCopy)
                        {
                            CatchUpCopyProcessing();
                        }
                        else
                        {
                            CatchUpReplicationProcessing();
                        }
                    }
                    else if (isCopy)
                    {
                        CatchUpReplicationProcessing();
                    }
                }
            }
        },
        CreateAsyncOperationRoot());

    if (inner->CompletedSynchronously)
    {
        TestSession::WriteNoise(
            TraceSource, 
            "Node({0}) - ({1}) - SGStatefulService::ProcessNextCatchupOperation - serviceType({2}) serviceName({3}) - Type({4}) - Completed synchronously",
            nodeId_,
            this,
            serviceType_,
            serviceName_,
            isCopy ? "COPY" : "REPL");

        bool isLast = false;
        if (FinishProcessingCatchupOperation(inner, isLast))
        {
            if (!isLast)
            {
                return true;
            }
            else if (isCopy)
            {
                CatchUpReplicationProcessing();
            }
        }
    }
    else
    {
        TestSession::WriteNoise(
            TraceSource, 
            "Node({0}) - ({1}) - SGStatefulService::ProcessNextCatchupOperation - serviceType({2}) serviceName({3}) - Type({4}) - Waiting for operation to asynchronously",
            nodeId_,
            this,
            serviceType_,
            serviceName_,
            isCopy ? "COPY" : "REPL");
    }

    return false;
}

void SGStatefulService::PrimaryReportMetricsRandom()
{
    const ULONG maxMetricCount = 10;
    const ULONG minMetricCount = 1;

    const int maxScale = 7000;

    map<wstring, ULONG> loads;
    vector<FABRIC_LOAD_METRIC> metrics;

    ULONG count = random_.Next(minMetricCount, maxMetricCount);

    wstring metricTrace;
    StringWriter metricWriter(metricTrace);

    for (ULONG i = 0; i < count; ++i)
    {
        wstring name;
        StringWriter(name).Write("metric{0}", i);

        ULONG load = static_cast<ULONG>(random_.Next(maxScale));

        loads.insert(make_pair(name, load));

        FABRIC_LOAD_METRIC metric = { 0 };
        metric.Name = loads.find(name)->first.c_str();
        metric.Value = load;

        metrics.push_back(metric);

        if (metricTrace.empty())
        {
            metricWriter.Write("{0} = {1}", name, load);
        }
        else
        {
            wstring current = metricTrace;
            metricTrace.clear();
            metricWriter.Write("{0}; {1} = {2}", current, name, load);
        }
    }

    TestSession::WriteNoise(
        TraceSource, 
        "Node({0}) - ({1}) - SGStatefulService::PrimaryReportMetricsRandom - serviceType({2}) serviceName({3}) - Calling ReportLoad with {4} metrics - {5}",
        nodeId_,
        this,
        serviceType_,
        serviceName_,
        count,
        metricTrace);

    HRESULT hr = this->Partition->ReportLoad(static_cast<ULONG>(metrics.size()), metrics.data());

    if (FAILED(hr))
    {
        TestSession::FailTest(
            "Node({0}) - ({1}) - SGStatefulService::PrimaryReportMetricsRandom - serviceType({2}) serviceName({3}) - ReportLoad should succeed but failed with {4}",
            nodeId_,
            this,
            serviceType_,
            serviceName_,
            hr
            );
    }
    else
    {
        TestSession::WriteNoise(
            TraceSource, 
            "Node({0}) - ({1}) - SGStatefulService::PrimaryReportMetricsRandom - serviceType({2}) serviceName({3}) - ReportLoad completed as expected (hr={4})",
            nodeId_,
            this,
            serviceType_,
            serviceName_,
            hr);
    }
}

bool SGStatefulService::FinishProcessingCatchupOperation(
    AsyncOperationSPtr const & asyncOperation,
    bool& isLast)
{
    auto operation = AsyncOperation::Get<SGComProxyOperationStreamAsyncOperation>(asyncOperation);

    FABRIC_OPERATION_METADATA const * operationMetadata;
    HRESULT hr = S_OK;

    if (!operation->Operation)
    {
        TestSession::WriteNoise(
            TraceSource, 
            "Node({0}) - ({1}) - SGStatefulService::FinishProcessingCatchupOperation - serviceType({2}) serviceName({3}) - asyncOperation({4}) Type({5})- NULL operation (Completed successfully).  Close {5} stream",
            nodeId_,
            this,
            serviceType_,
            serviceName_,
            asyncOperation.get(),
            operation->IsCopy ? "COPY" : "REPL");

        Common::ComPointer<IFabricOperationStream> cleanup(std::move(operation->IsCopy ? copyStream_ : replicationStream_));
        isLast = true;
    }
    else
    {
        operationMetadata = operation->Operation->get_Metadata();
     
        ULONG bufferCount = 0;
        FABRIC_OPERATION_DATA_BUFFER const * replicaBuffers = NULL;

        hr = operation->Operation->GetData(&bufferCount, &replicaBuffers);
        if (0 == bufferCount)
        {
            TestSession::WriteNoise(
                TraceSource, 
                "Node({0}) - ({1}) - SGStatefulService::FinishProcessingCatchupOperation - serviceType({2}) serviceName({3}) sequenceNumber({5}) - empty {4} operation",
                nodeId_,
                this,
                serviceType_,
                serviceName_,
                operation->IsCopy ? "COPY" : "REPL",
                operationMetadata->SequenceNumber
                );
            emptyOperationDataSeen_ = true;
        }
        else
        {
            TestSession::WriteNoise(
                TraceSource, 
                "Node({0}) - ({1}) - SGStatefulService::FinishProcessingCatchupOperation - serviceType({2}) serviceName({3}) sequenceNumber({5}) - {4} operation",
                nodeId_,
                this,
                serviceType_,
                serviceName_,
                operation->IsCopy ? "COPY" : "REPL",
                operationMetadata->SequenceNumber
                );

            if (!settings_->GetBool(L"replicateatomic") && !settings_->GetBool(L"donotassertemptyoperation")) // in the atomic group test, we don't replicate empty operations
            {
                TestSession::FailTestIf(!emptyOperationDataSeen_, 
                    "Node({0}) - ({1}) serviceType({2}) serviceName({3}) - empty operation data has not been seen in {4} stream!",
                    nodeId_,
                    this,
                    serviceType_,
                    serviceName_,
                    operation->IsCopy ? "COPY" : "REPL");
                emptyOperationDataSeen_ = false;
            }

            if (operationMetadata->Type == FABRIC_OPERATION_TYPE_NORMAL)
            {
                TestSession::FailTestIf(bufferCount != 2, "buffer count must be 2");
                TestSession::FailTestIf(replicaBuffers[0].BufferSize != sizeof(ULONGLONG), "wrong buffer size");
                TestSession::FailTestIf(replicaBuffers[1].BufferSize != sizeof(ULONGLONG), "wrong buffer size");
                TestSession::FailTestIf(replicaBuffers[0].Buffer == NULL, "null replica buffer");
                TestSession::FailTestIf(replicaBuffers[1].Buffer == NULL, "null replica buffer");
                ApplyState(operation->IsCopy, operationMetadata->SequenceNumber, (*(ULONGLONG*)replicaBuffers[0].Buffer)*3 + (*(ULONGLONG*)replicaBuffers[1].Buffer));
            }
        }

        if (operationMetadata->Type == FABRIC_OPERATION_TYPE_ROLLBACK_ATOMIC_GROUP)
        {
            TestSession::WriteNoise(
                TraceSource, 
                "Node({0}) - ({1}) - SGStatefulService::FinishProcessingCatchupOperation - serviceType({2}) serviceName({3}) sequenceNumber({4}) - Rollback",
                nodeId_,
                this,
                serviceType_,
                serviceName_,
                operationMetadata->SequenceNumber
                );

            rollbackCount_++;
        }

        operation->Operation->Acknowledge();
        isLast = false;
    }

    if (isLast && !operation->IsCopy)
    {
        TestSession::WriteNoise(
            TraceSource, 
            "Node({0}) - ({1}) - SGStatefulService::FinishProcessingCatchupOperation - serviceType({2}) serviceName({3}) - Catchup processing completed",
            nodeId_,
            this,
            serviceType_,
            serviceName_
            );

        catchupDone_.Set();
    }

    return true;
}

HRESULT SGStatefulService::CatchUpReplicationProcessing()
{
    if (!replicationStream_)
    {
        HRESULT hr = stateReplicator_->GetReplicationStream(replicationStream_.InitializationAddress());

        if (FAILED(hr))
        {
            TestSession::WriteNoise(
                TraceSource, 
                "Node({0}) - ({1}) - SGStatefulService::CatchUpReplicationProcessing - serviceType({2}) serviceName({3}) - GetReplicationStream failed with ({4})",
                nodeId_,
                this,
                serviceType_,
                serviceName_,
                hr);

            return hr;
        }
        else
        {
            TestSession::WriteNoise(
                TraceSource, 
                "Node({0}) - ({1}) - SGStatefulService::CatchUpReplicationProcessing - serviceType({2}) serviceName({3}) - replicationStream_({4})",
                nodeId_,
                this,
                serviceType_,
                serviceName_,
                replicationStream_.GetRawPointer());
            emptyOperationDataSeen_ = true;
        }

        if (IsSignalSet(ServiceGroupAfterGetReplicationStreamReportFaultPermanent))
        {
            hr = statefulPartition_->ReportFault(FABRIC_FAULT_TYPE_PERMANENT);
            TestSession::FailTestIf(FAILED(hr), "ReportFault failed unexpectedly.");

            return S_OK;
        }
    }
    while (ProcessNextCatchupOperation(replicationStream_, false));
    return S_OK;
}

HRESULT SGStatefulService::ReplicationProcessing()
{
    bool more = false;
    do 
    {
        if (settings_->GetBool(L"replicateatomic"))
        {
            more = ReplicateAtomicGroup();
        }
        else
        {
            more = ReplicaNextState();
        }
    }
    while (more);

    return S_OK;
}

bool SGStatefulService::ReplicateAtomicGroup()
{
    TestSession::WriteNoise(
            TraceSource, 
            "Node({0}) - ({1}) - SGStatefulService::ReplicateAtomicGroup - serviceType({2}) serviceName({3})",
            nodeId_,
            this,
            serviceType_,
            serviceName_);

    if (settings_->GetBool(L"rollback"))
    {
        if (!isGetCopyStateCalled_)
        {
            // delay replication until secondary is copied (assume one secondary) so that replication can be observed on replication stream

            TestSession::WriteNoise(
                TraceSource, 
                "Node({0}) - ({1}) - SGStatefulService::ReplicateAtomicGroup - serviceType({2}) serviceName({3}) - GetCopyState has not been called, retrying",
                nodeId_,
                this,
                serviceType_,
                serviceName_);

            Threadpool::Post(
            [this]()
            {
                this->ReplicateAtomicGroup();
            },
            TimeSpan::FromMilliseconds(500));

            return false;
        }
    }

    ComPointer<IFabricAtomicGroupStateReplicator> atomicReplicator;

    auto hr = stateReplicator_->QueryInterface(IID_IFabricAtomicGroupStateReplicator, atomicReplicator.VoidInitializationAddress());

    TestSession::FailTestIf(
        FAILED(hr), 
        "Node({0}) - ({1}) - SGStatefulService::ReplicateAtomicGroup - serviceType({2}) serviceName({3}) - QueryInterface failed with {4}",
        nodeId_,
        this,
        serviceType_,
        serviceName_,
        hr);

    FABRIC_ATOMIC_GROUP_ID groupId;

    hr = atomicReplicator->CreateAtomicGroup(&groupId);

    TestSession::FailTestIf(
        FAILED(hr), 
        "Node({0}) - ({1}) - SGStatefulService::ReplicateAtomicGroup - serviceType({2}) serviceName({3}) - CreateAtomicGroup failed with {4}",
        nodeId_,
        this,
        serviceType_,
        serviceName_,
        hr);

    TestSession::WriteNoise(
        TraceSource, 
        "Node({0}) - ({1}) - SGStatefulService::ReplicateAtomicGroup - serviceType({2}) serviceName({3}) - atomicGroupId {4}",
        nodeId_,
        this,
        serviceType_,
        serviceName_,
        groupId);

    ULONGLONG state = state_ + 1;

    auto replicationData = Common::make_com<SGUlonglongOperationData, IFabricOperationData>(state);

    auto replicate = AsyncOperation::CreateAndStart<SGComProxyReplicateAtomicGroupAsyncOperation>(
        atomicReplicator,
        groupId,
        replicationData.DetachNoRelease(),

        [this](AsyncOperationSPtr const & asyncOperation) -> void
        {
            this->FinishReplicateAtomicGroupOperation(asyncOperation, false);
        },

        CreateAsyncOperationRoot());

    return this->FinishReplicateAtomicGroupOperation(replicate, true);
}

bool SGStatefulService::FinishReplicateAtomicGroupOperation(AsyncOperationSPtr const & asyncOperation, bool expectCompletedSynchronously)
{
    if (asyncOperation->CompletedSynchronously != expectCompletedSynchronously)
    {
        return false;
    }

    if (asyncOperation->CompletedSynchronously)
    {
        TestSession::WriteNoise(
            TraceSource, 
            "Node({0}) - ({1}) - SGStatefulService::FinishReplicateAtomicGroupOperation - serviceType({2}) serviceName({3})  Completed synchronously",
            nodeId_,
            this,
            serviceType_,
            serviceName_);
    }
    else
    {
        TestSession::WriteNoise(
            TraceSource, 
            "Node({0}) - ({1}) - SGStatefulService::FinishReplicateAtomicGroupOperation - serviceType({2}) serviceName({3})  Completed asynchronously",
            nodeId_,
            this,
            serviceType_,
            serviceName_);
    }

    auto replicate = AsyncOperation::End<SGComProxyReplicateAtomicGroupAsyncOperation>(asyncOperation);

    if (replicate->Error.IsError(ErrorCodeValue::NoWriteQuorum) || replicate->Error.IsError(ErrorCodeValue::ReconfigurationPending))
    {
        bool stopping = isStopping_;

        TestSession::WriteNoise(
            TraceSource, 
            "Node({0}) - ({1}) - SGStatefulService::FinishReplicateAtomicGroupOperation - serviceType({2}) serviceName({3}) - failed with {4}, {5}",
            nodeId_,
            this,
            serviceType_,
            serviceName_,
            replicate->Error,
            (stopping ? "stopping" : "retrying"));

        if (!stopping)
        {
            Threadpool::Post(
                [this]()
                {
                    this->ReplicateAtomicGroup();
                },
                TimeSpan::FromMilliseconds(100));
        }

        return false;
    }
    else
    {
        if (!replicate->Error.IsSuccess() && settings_->GetBool(L"ignore.replicationerror"))
        {
            //
            // Reported fault during previous incarnation. This may have led to data loss. While data loss is ongoin this will fail. It's safe to ignore here.
            //
            replicationDone_.Set();
            return false;
        }

        TestSession::FailTestIf(
            !replicate->Error.IsSuccess(), 
            "Node({0}) - ({1}) - SGStatefulService::FinishReplicateAtomicGroupOperation - serviceType({2}) serviceName({3}) - End failed with {4}",
            nodeId_,
            this,
            serviceType_,
            serviceName_,
            replicate->Error);
    
        return CommitAtomicGroup(replicate->AtomicGroupId, replicate->ReplicationData);
    }
}

bool SGStatefulService::CommitAtomicGroup(FABRIC_ATOMIC_GROUP_ID groupId, IFabricOperationData* data)
{
    if (settings_->GetBool(L"nocommit"))
    {
         TestSession::WriteNoise(
            TraceSource, 
            "Node({0}) - ({1}) - SGStatefulService::CommitAtomicGroup - serviceType({2}) serviceName({3}) - Atomic replication completed (nocommit)",
            nodeId_,
            this,
            serviceType_,
            serviceName_
            );

        replicationDone_.Set();

        return false;
    }

    if (IsSignalSet(ServiceGroupDelayCommit))
    {
        TestSession::WriteNoise(
            TraceSource, 
            "Node({0}) - ({1}) - SGStatefulService::CommitAtomicGroup - serviceType({2}) serviceName({3}) Delaying commit",
            nodeId_,
            this,
            serviceType_,
            serviceName_);

        Threadpool::Post(
            [this, groupId, data]()
            {
                this->CommitAtomicGroup(groupId, data);
            },
            TimeSpan::FromMilliseconds(200));

        return false;
    }
    else
    {
        TestSession::WriteNoise(
            TraceSource, 
            "Node({0}) - ({1}) - SGStatefulService::CommitAtomicGroup - serviceType({2}) serviceName({3})",
            nodeId_,
            this,
            serviceType_,
            serviceName_);

        ComPointer<IFabricAtomicGroupStateReplicator> atomicReplicator;

        auto hr = stateReplicator_->QueryInterface(IID_IFabricAtomicGroupStateReplicator, atomicReplicator.VoidInitializationAddress());

        TestSession::FailTestIf(
            FAILED(hr), 
            "Node({0}) - ({1}) - SGStatefulService::CommitAtomicGroup - serviceType({2}) serviceName({3}) - QueryInterface failed with {4}",
            nodeId_,
            this,
            serviceType_,
            serviceName_,
            hr);

        bool doCommit = true;
        if (settings_->GetBool(L"rollback"))
        {
            doCommit = false;
        }

        int endCommitDelay = 0;
        if (settings_->TryGetInt(L"endcommit.delay", endCommitDelay, 0))
        {
            TestSession::WriteNoise(
                TraceSource, 
                "Node({0}) - ({1}) - SGStatefulService::CommitAtomicGroup - serviceType({2}) serviceName({3}) - End will be delayed by {4} s",
                nodeId_,
                this,
                serviceType_,
                serviceName_,
                endCommitDelay);
        }

        auto commit = AsyncOperation::CreateAndStart<SGComProxyCommitOrRollbackAtomicGroupAsyncOperation>(
            atomicReplicator,
            groupId,
            data,
            doCommit,
            endCommitDelay,
            [this](AsyncOperationSPtr const & asyncOperation) -> void
            {
                this->FinishReplicateAtomicGroupCommit(asyncOperation, false);
            },

            CreateAsyncOperationRoot());

        if (IsSignalSet(ServiceGroupAfterBeginReplicateAtomicGroupCommitReportFaultPermanent))
        {
            statefulPartition_->ReportFault(FABRIC_FAULT_TYPE_PERMANENT);
        }

        return FinishReplicateAtomicGroupCommit(commit, true);
    }
}

bool SGStatefulService::FinishReplicateAtomicGroupCommit(AsyncOperationSPtr const & asyncOperation, bool expectCompletedSynchronously)
{
    if (asyncOperation->CompletedSynchronously != expectCompletedSynchronously)
    {
        return false;
    }

    if (asyncOperation->CompletedSynchronously)
    {
        TestSession::WriteNoise(
            TraceSource, 
            "Node({0}) - ({1}) - SGStatefulService::FinishReplicateAtomicGroupCommit - serviceType({2}) serviceName({3})  Completed synchronously",
            nodeId_,
            this,
            serviceType_,
            serviceName_);
    }
    else
    {
        TestSession::WriteNoise(
            TraceSource, 
            "Node({0}) - ({1}) - SGStatefulService::FinishReplicateAtomicGroupCommit - serviceType({2}) serviceName({3})  Completed asynchronously",
            nodeId_,
            this,
            serviceType_,
            serviceName_);
    }

    auto commit = AsyncOperation::End<SGComProxyCommitOrRollbackAtomicGroupAsyncOperation>(asyncOperation);

    if (commit->Error.IsError(ErrorCodeValue::NoWriteQuorum) || commit->Error.IsError(ErrorCodeValue::ReconfigurationPending))
    {
        bool stopping = isStopping_;

        TestSession::WriteNoise(
            TraceSource, 
            "Node({0}) - ({1}) - SGStatefulService::FinishReplicateAtomicGroupCommit - serviceType({2}) serviceName({3}) - failed with {4}, {5}",
            nodeId_,
            this,
            serviceType_,
            serviceName_,
            commit->Error,
            (stopping ? "stopping" : "retrying"));

        if (!stopping)
        {
            auto groupId = commit->AtomicGroupId;
            auto data = commit->ReplicationData;

            Threadpool::Post(
                [this, groupId, data]()
                {
                    this->CommitAtomicGroup(groupId, data);
                },
                TimeSpan::FromMilliseconds(200));
        }

        return false;
    }
    else 
    {
        if (!commit->Error.IsSuccess() && settings_->GetBool(L"ignore.replicationerror"))
        {
            //
            // If we reported fault, we don't care about the commit result.
            //
            replicationDone_.Set();
            return false;
        }

        TestSession::FailTestIf(
            !commit->Error.IsSuccess(), 
            "Node({0}) - ({1}) - SGStatefulService::FinishReplicateAtomicGroupCommit - serviceType({2}) serviceName({3}) - End failed with {4}",
            nodeId_,
            this,
            serviceType_,
            serviceName_,
            commit->Error);
    }

    TestSession::FailTestIf(NULL == commit->ReplicationData, "ReplicationData");

    ULONG bufferCount = 0;
    FABRIC_OPERATION_DATA_BUFFER const * replicaBuffers = nullptr;

    commit->ReplicationData->GetData(&bufferCount, &replicaBuffers);
    if (0 == bufferCount)
    {
        TestSession::WriteNoise(
            TraceSource, 
            "Node({0}) - ({1}) - SGStatefulService::FinishReplicateAtomicGroupCommit - serviceType({2}) serviceName({3}) - asyncOperation({4}) - empty operation",
            nodeId_,
            this,
            serviceType_,
            serviceName_,
            asyncOperation.get());
    }
    else
    {
        TestSession::FailTestIf(bufferCount != 2, "buffer count must be 2");
        TestSession::FailTestIf(replicaBuffers[0].BufferSize != sizeof(ULONGLONG), "wrong buffer size");
        TestSession::FailTestIf(replicaBuffers[1].BufferSize != sizeof(ULONGLONG), "wrong buffer size");
        TestSession::FailTestIf(replicaBuffers[0].Buffer == NULL, "null replica buffer");
        TestSession::FailTestIf(replicaBuffers[1].Buffer == NULL, "null replica buffer");
    
        TestSession::WriteNoise(
            TraceSource, 
            "Node({0}) - ({1}) - SGStatefulService::FinishReplicateAtomicGroupCommit - serviceType({2}) serviceName({3}) - asyncOperation({4}) value({5})",
            nodeId_,
            this,
            serviceType_,
            serviceName_,
            asyncOperation.get(),
            (*(ULONGLONG*)replicaBuffers[0].Buffer)*3 + (*(ULONGLONG*)replicaBuffers[1].Buffer));
    
        ApplyStateInPrimary(commit->SequenceNumber, (*(ULONGLONG*)replicaBuffers[0].Buffer)*3 + (*(ULONGLONG*)replicaBuffers[1].Buffer));
    }

    TestSession::WriteNoise(
        TraceSource, 
        "Node({0}) - ({1}) - SGStatefulService::FinishReplicateAtomicGroupCommit - serviceType({2}) serviceName({3}) - Atomic replication completed",
        nodeId_,
        this,
        serviceType_,
        serviceName_
        );

    replicationDone_.Set();
    return false;
}

bool SGStatefulService::ReplicaNextState()
{
    if (operationCount_++ >= 10) 
    { 
        TestSession::WriteNoise(
            TraceSource, 
            "Node({0}) - ({1}) - SGStatefulService::ReplicaNextState - serviceType({2}) serviceName({3}) - Replication completed",
            nodeId_,
            this,
            serviceType_,
            serviceName_
            );

        replicationDone_.Set();
        return false; 
    }
    
    ULONGLONG state = state_ + 1;
    bool emptyOperationReplicated = replicateEmptyOperationData_;
    Common::ComPointer<IFabricOperationData> replicationData;
    if (replicateEmptyOperationData_)
    {
        replicationData = Common::make_com<SGEmptyOperationData, IFabricOperationData>();
    }
    else
    {
        replicationData = Common::make_com<SGUlonglongOperationData, IFabricOperationData>(state);
    }
    
    AsyncOperationSPtr inner = AsyncOperation::CreateAndStart<SGComProxyReplicateAsyncOperation>(
        stateReplicator_,
        replicationData.DetachNoRelease(),
        [this](AsyncOperationSPtr const & asyncOperation) -> void
        {
            if (!asyncOperation->CompletedSynchronously)
            {
                if (this->FinishReplicateState(asyncOperation))
                {
                    ReplicationProcessing();
                }
            }
        },
        CreateAsyncOperationRoot());

    if (emptyOperationReplicated)
    {
        if (inner->CompletedSynchronously)
        {
            TestSession::WriteNoise(
                TraceSource, 
                "Node({0}) - ({1}) - SGStatefulService::ReplicaNextState - serviceType({2}) serviceName({3}) - empty operation data - Completed synchronously",
                nodeId_,
                this,
                serviceType_,
                serviceName_);
    
            if (FinishReplicateState(inner))
            {
                return true;
            }
        }
        else
        {
            TestSession::WriteNoise(
                TraceSource, 
                "Node({0}) - ({1}) - SGStatefulService::ReplicaNextState - serviceType({2}) serviceName({3}) - empty operation data - asyncOperation({4}) - Completed Asynchronously",
                nodeId_,
                this,
                serviceType_,
                serviceName_,
                inner.get());
        }
    }
    else
    {
        if (inner->CompletedSynchronously)
        {
            TestSession::WriteNoise(
                TraceSource, 
                "Node({0}) - ({1}) - SGStatefulService::ReplicateNextState - serviceType({2}) serviceName({3}) - state({4}) - Completed synchronously",
                nodeId_,
                this,
                serviceType_,
                serviceName_,
                state);
    
            if (FinishReplicateState(inner))
            {
                return true;
            }
        }
        else
        {
            TestSession::WriteNoise(
                TraceSource, 
                "Node({0}) - ({1}) - SGStatefulService::ReplicateNextState - serviceType({2}) serviceName({3}) - state({4}) asyncOperation({5}) - Completed Asynchronously",
                nodeId_,
                this,
                serviceType_,
                serviceName_,
                state,
                inner.get());
        }
    }

    return false;
}

bool SGStatefulService::FinishReplicateState(
    AsyncOperationSPtr const & asyncOperation)
{
    auto operation = AsyncOperation::Get<SGComProxyReplicateAsyncOperation>(asyncOperation);
    TestSession::FailTestIf(NULL == operation->ReplicationData, "ReplicationData");

    if (operation->Error.IsError(ErrorCodeValue::NoWriteQuorum) || operation->Error.IsError(ErrorCodeValue::ReconfigurationPending))
    {
        bool stopping = isStopping_;

        TestSession::WriteNoise(
            TraceSource, 
            "Node({0}) - ({1}) - SGStatefulService::FinishReplicateState - serviceType({2}) serviceName({3}) - asyncOperation({4}) - error({5}) - {6}",
            nodeId_,
            this,
            serviceType_,
            serviceName_,
            operation,
            operation->Error,
            (stopping ? "stopping" : "retrying"));

        // operation was not applied
        // compensate
        operationCount_--;

        if (!stopping)
        {
            // retry operation
            Threadpool::Post(
                [this]()
                {
                    this->ReplicationProcessing();
                },
                TimeSpan::FromMilliseconds(50));
        }
        return false;
    }
    else
    {
        TestSession::WriteNoise(
            TraceSource, 
            "Node({0}) - ({1}) - SGStatefulService::FinishReplicateState - serviceType({2}) serviceName({3}) - asyncOperation({4}) - error({5})",
            nodeId_,
            this,
            serviceType_,
            serviceName_,
            operation,
            operation->Error);
    }

    replicateEmptyOperationData_ = !replicateEmptyOperationData_;

    ULONG bufferCount = 0;
    FABRIC_OPERATION_DATA_BUFFER const * replicaBuffers = nullptr;
    
    operation->ReplicationData->GetData(&bufferCount, &replicaBuffers);
    if (0 == bufferCount)
    {
        TestSession::WriteNoise(
            TraceSource, 
            "Node({0}) - ({1}) - SGStatefulService::FinishReplicateState - serviceType({2}) serviceName({3}) - asyncOperation({4}) - empty operation",
            nodeId_,
            this,
            serviceType_,
            serviceName_,
            asyncOperation.get());

        // next operation must not be empty
        TestSession::FailTestIf(replicateEmptyOperationData_, "Unexpected state");
    }
    else
    {
        TestSession::FailTestIf(bufferCount != 2, "buffer count must be 2");
        TestSession::FailTestIf(replicaBuffers[0].BufferSize != sizeof(ULONGLONG), "wrong buffer size");
        TestSession::FailTestIf(replicaBuffers[1].BufferSize != sizeof(ULONGLONG), "wrong buffer size");
        TestSession::FailTestIf(replicaBuffers[0].Buffer == NULL, "null replica buffer");
        TestSession::FailTestIf(replicaBuffers[1].Buffer == NULL, "null replica buffer");
    
        TestSession::WriteNoise(
            TraceSource, 
            "Node({0}) - ({1}) - SGStatefulService::FinishReplicateState - serviceType({2}) serviceName({3}) - asyncOperation({4}) value({5})",
            nodeId_,
            this,
            serviceType_,
            serviceName_,
            asyncOperation.get(),
            (*(ULONGLONG*)replicaBuffers[0].Buffer)*3 + (*(ULONGLONG*)replicaBuffers[1].Buffer));

        // next operation must be empty
        TestSession::FailTestIfNot(replicateEmptyOperationData_, "Unexpected state");
    
        ApplyStateInPrimary(operation->SequenceNumber, (*(ULONGLONG*)replicaBuffers[0].Buffer)*3 + (*(ULONGLONG*)replicaBuffers[1].Buffer));
    }
    return true;
}

HRESULT SGStatefulService::CreateWorkload(
    __in FABRIC_REPLICA_ROLE newRole)
{
    HRESULT hr = S_OK;

    TestSession::WriteNoise(
        TraceSource, 
        "Node({0}) - ({1}) - SGStatefulService::CreateWorkload - serviceType({2}) serviceName({3}) - Role({4} ==> {5})",
        nodeId_,
        this,
        serviceType_,
        serviceName_,
        currentReplicaRole_,
        newRole);

    if (!IsSecondaryRole(currentReplicaRole_) && IsSecondaryRole(newRole))
    {
        isStopping_ = true;
        hr = StopWorkload();
        isStopping_ = false;

        catchupDone_.Reset();

        workItemCatchUpCopy_ = ::CreateThreadpoolWork(&ThreadPoolWorkItemCatchUpCopyAndReplication, this, NULL);
        selfCatchUpCopy_ = shared_from_this();
        ::SubmitThreadpoolWork(workItemCatchUpCopy_);
    }
    else if (!IsPrimaryRole(currentReplicaRole_) && IsPrimaryRole(newRole))
    {
        isStopping_ = true;
        hr = StopWorkload();
        isStopping_ = false;

        operationCount_ = 0;

        if (!settings_->GetBool(L"donotreplicate"))
        {
            replicationDone_.Reset();

            workItemReplication_ = ::CreateThreadpoolWork(&ThreadPoolWorkItemReplication, this, NULL);
            selfReplication_ = shared_from_this();
            ::SubmitThreadpoolWork(workItemReplication_);
        }

        if (settings_->GetBool(L"primary.reportmetrics"))
        {
            workItemReportMetrics_ = ::CreateThreadpoolWork(&ThreadPoolWorkItemReportMetrics, this, NULL);
            selfReportMetrics_ = shared_from_this();
            ::SubmitThreadpoolWork(workItemReportMetrics_);
        }
    }
    else if (newRole == FABRIC_REPLICA_ROLE_NONE)
    {
        isStopping_ = true;
        hr = StopWorkload();
        isStopping_ = false;
    }

    return S_OK;
}

HRESULT SGStatefulService::StopWorkload()
{
    TestSession::WriteNoise(
        TraceSource, 
        "Node({0}) - ({1}) - SGStatefulService::StopWorkload - serviceType({2}) serviceName({3}) - Wating for replication or catchup work to complete",
        nodeId_,
        this,
        serviceType_,
        serviceName_);
    
    bool catchupDone = catchupDone_.WaitOne(10000);
    bool replicationDone = replicationDone_.WaitOne(10000);

    TestSession::WriteNoise(
        TraceSource, 
        "Node({0}) - ({1}) - SGStatefulService::StopWorkload - serviceType({2}) serviceName({3}) - Replication or catchup work to completed or timed out - catchupDone({4}) replicationDone({5})",
        nodeId_,
        this,
        serviceType_,
        serviceName_,
        catchupDone,
        replicationDone
        );

    if (NULL != workItemReplication_)
    {
        TestSession::WriteNoise(
            TraceSource, 
            "Node({0}) - ({1}) - SGStatefulService::StopWorkload - serviceType({2}) serviceName({3}) - Stopping replication",
            nodeId_,
            this,
            serviceType_,
            serviceName_);

        ::WaitForThreadpoolWorkCallbacks(workItemReplication_, FALSE);
        ::CloseThreadpoolWork(workItemReplication_);
        workItemReplication_ = NULL;

        TestSession::WriteNoise(
            TraceSource, 
            "Node({0}) - ({1}) - SGStatefulService::StopWorkload - serviceType({2}) serviceName({3}) - Stopped replication",
            nodeId_,
            this,
            serviceType_,
            serviceName_);
    }
    if (NULL != workItemCatchUpCopy_)
    {
        TestSession::WriteNoise(
            TraceSource, 
            "Node({0}) - ({1}) - SGStatefulService::StopWorkload - serviceType({2}) serviceName({3}) - Stopping catchup",
            nodeId_,
            this,
            serviceType_,
            serviceName_);

        ::WaitForThreadpoolWorkCallbacks(workItemCatchUpCopy_, FALSE);
        ::CloseThreadpoolWork(workItemCatchUpCopy_);
        workItemCatchUpCopy_ = NULL;

        TestSession::WriteNoise(
            TraceSource, 
            "Node({0}) - ({1}) - SGStatefulService::StopWorkload - serviceType({2}) serviceName({3}) - Stopped catchup",
            nodeId_,
            this,
            serviceType_,
            serviceName_);
    }
    if (NULL != workItemReportMetrics_)
    {
        TestSession::WriteNoise(
            TraceSource, 
            "Node({0}) - ({1}) - SGStatefulService::StopWorkload - serviceType({2}) serviceName({3}) - Stopping report metrics",
            nodeId_,
            this,
            serviceType_,
            serviceName_);

        ::WaitForThreadpoolWorkCallbacks(workItemReportMetrics_, FALSE);
        ::CloseThreadpoolWork(workItemReportMetrics_);
        workItemReportMetrics_ = NULL;

        TestSession::WriteNoise(
            TraceSource, 
            "Node({0}) - ({1}) - SGStatefulService::StopWorkload - serviceType({2}) serviceName({3}) - Stopped report metrics",
            nodeId_,
            this,
            serviceType_,
            serviceName_);
    }
    return S_OK;
}

bool SGStatefulService::ShouldFailOn(ApiFaultHelper::ComponentName compName, wstring operationName, ApiFaultHelper::FaultType faultType)
{
    return ApiFaultHelper::Get().ShouldFailOn(nodeId_, serviceName_, compName, operationName, faultType);
}
/*
bool SGStatefulService::ShouldFailOn(ApiFaultHelper::ApiName apiName, ApiFaultHelper::ApiAction actionType)
{
    return ApiFaultHelper::Get().ShouldFailOn(nodeId_, serviceName_, compName, operationName, faultType);
} */

bool SGStatefulService::IsSignalSet(std::wstring strFlag)
{
    return ApiSignalHelper::IsSignalSet(nodeId_, serviceName_, strFlag);
}

std::wstring const SGStatefulService::StatefulServiceType = L"SGStatefulService";
std::wstring const SGStatefulService::StatefulServiceECCType = L"SGStatefulServiceECC";
std::wstring const SGStatefulService::StatefulServiceECSType = L"SGStatefulServiceECS";
std::wstring const SGStatefulService::StatefulServiceNCCType = L"SGStatefulServiceNCC";
std::wstring const SGStatefulService::StatefulServiceNCSType = L"SGStatefulServiceNCS";

StringLiteral const SGStatefulService::TraceSource("FabricTest.ServiceGroup.SGStatefulService");
StringLiteral const SGComProxyOperationDataEnumAsyncOperation::TraceSource("FabricTest.ServiceGroup.SGComProxyOperationDataEnumAsyncOperation");
StringLiteral const SGComProxyOperationStreamAsyncOperation::TraceSource("FabricTest.ServiceGroup.SGComProxyOperationStreamAsyncOperation");
StringLiteral const SGComProxyReplicateAsyncOperation::TraceSource("FabricTest.ServiceGroup.SGComProxyReplicateAsyncOperation");
StringLiteral const SGComProxyReplicateAtomicGroupAsyncOperation::TraceSource("FabricTest.ServiceGroup.SGComProxyReplicateAtomicGroupAsyncOperation");
StringLiteral const SGComProxyCommitOrRollbackAtomicGroupAsyncOperation::TraceSource("FabricTest.ServiceGroup.SGComProxyCommitAtomicGroupAsyncOperation");
StringLiteral const SGStatefulService::SGCopyContextReceiver::TraceSource("FabricTest.ServiceGroup.SGCopyContextReceiver");
