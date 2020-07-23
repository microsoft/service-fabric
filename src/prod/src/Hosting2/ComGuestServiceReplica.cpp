//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

StringLiteral const TraceType("ComGuestServiceReplica");

ComGuestServiceReplica::ComGuestServiceReplica(
    IGuestServiceTypeHost & typeHost,
    wstring const & serviceName,
    wstring const & serviceTypeName,
    Guid const & partitionId,
    ::FABRIC_REPLICA_ID replicaId)
    : IGuestServiceReplica()
    , ComUnknownBase()
    , typeHost_(typeHost)
    , serviceName_(serviceName)
    , serviceTypeName_(serviceTypeName)
    , partitionId_(partitionId)
    , replicaId_(replicaId)
    , currenRole_(FABRIC_REPLICA_ROLE_UNKNOWN)
    , endpointsAsJsonStr_()
    , partition_()
    , hasReportedFault_(false)
    , isActivationInProgress_(false)
    , isDeactivationInProgress_(false)
    , eventRegistrationHandle_(0)
    , serviceTraceInfo_(BuildTraceInfo(serviceTypeName, serviceName, partitionId, replicaId))
    , dummyReplicator_()
    , partitionName_()
{
}

ComGuestServiceReplica::~ComGuestServiceReplica()
{
}

HRESULT ComGuestServiceReplica::BeginOpen(
    /* [in] */ FABRIC_REPLICA_OPEN_MODE openMode,
    /* [in] */ IFabricStatefulServicePartition *partition,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    UNREFERENCED_PARAMETER(openMode);
    UNREFERENCED_PARAMETER(partition);

    WriteNoise(TraceType, "BeginOpen {0}", serviceTraceInfo_);

    IFabricStatefulServicePartition2 * partition2;
    auto hr = partition->QueryInterface(&partition2);

    ASSERT_IF(FAILED(hr), "Partition must implement IFabricStatefulServicePartition2");

    partition_.SetAndAddRef(partition2);
    FABRIC_SERVICE_PARTITION_INFORMATION const * partitionInfo;
    hr = partition_->GetPartitionInfo(&partitionInfo);
    ASSERT_IF(FAILED(hr), "Unable to get partition information");

    if (partitionInfo->Kind == FABRIC_SERVICE_PARTITION_KIND_NAMED)
    {
        FABRIC_NAMED_PARTITION_INFORMATION * namedPartitionInfo = (FABRIC_NAMED_PARTITION_INFORMATION *)partitionInfo->Value;
        partitionName_= namedPartitionInfo->Name;
    }
    return ComCompletedOperationHelper::BeginCompletedComOperation(callback, context);
}

HRESULT ComGuestServiceReplica::EndOpen(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricReplicator **replicator)
{
    WriteNoise(TraceType, "EndOpen {0}", serviceTraceInfo_);

    HRESULT hr = ComCompletedOperationHelper::EndCompletedComOperation(context);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    dummyReplicator_ = 
        make_com<ComDummyReplicator>(
            serviceName_, 
            partitionId_, 
            replicaId_);

    return dummyReplicator_->QueryInterface(IID_IFabricReplicator, (void**)(replicator));
}

HRESULT ComGuestServiceReplica::BeginChangeRole(
    /* [in] */ FABRIC_REPLICA_ROLE newRole,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    WriteNoise(
        TraceType, 
        "BeginChangeRole: {0}, CurrentRole={1}, NewRole={2}.", 
        serviceTraceInfo_, 
        currenRole_,
        newRole);

    if (typeHost_.HostContext.IsCodePackageActivatorHost == false)
    {
        currenRole_ = newRole;
        return ComCompletedOperationHelper::BeginCompletedComOperation(callback, context);
    }

    HRESULT hr;
    if (newRole == FABRIC_REPLICA_ROLE_PRIMARY)
    {
        isActivationInProgress_ = true;
        EnvironmentMap envMap;
        ScopedHeap heap;
        FABRIC_STRING_MAP environment = {};

        auto epoch = dummyReplicator_->GetCurrentEpoch();
        envMap[Constants::EnvironmentVariable::Epoch] = Common::wformatString(L"{0}:{1}", epoch.DataLossNumber, epoch.ConfigurationNumber);

        if (!partitionName_.empty())
        {
            envMap[Constants::EnvironmentVariable::ReplicaName] = partitionName_;
        }

        auto error = PublicApiHelper::ToPublicApiStringMap(heap, envMap, environment);
        if (!error.IsSuccess())
        {
            WriteNoise(
                TraceType, 
                "BeginChangeRole: Getting Epoch failed. {0}", 
                error);

            return ComUtility::OnPublicApiReturn(error.ToHResult());
        }

        hr = typeHost_.ActivationManager->BeginActivateCodePackagesAndWaitForEvent(
            &environment /* custom environment */,
            HostingConfig::GetConfig().ActivationTimeout.Milliseconds,
            callback,
            context);
    }
    else if (currenRole_ == FABRIC_REPLICA_ROLE_PRIMARY)
    {
        isDeactivationInProgress_ = true;
        this->UnregisterForEvents();

        hr = typeHost_.ActivationManager->BeginDeactivateCodePackages(
                HostingConfig::GetConfig().ActivationTimeout.Milliseconds,
                callback,
                context);
    }
    else
    {
        hr = ComCompletedOperationHelper::BeginCompletedComOperation(callback, context);
    }

    currenRole_ = newRole;
    return hr;
}

HRESULT ComGuestServiceReplica::EndChangeRole(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricStringResult **serviceAddress)
{
    WriteNoise(
        TraceType, 
        "EndChangeRole: ServiceName={0}, NewRole={1}.", 
        serviceTraceInfo_, 
        currenRole_);

    if (typeHost_.HostContext.IsCodePackageActivatorHost == false)
    {
        return ComCompletedOperationHelper::EndCompletedComOperation(context);
    }

    HRESULT hr;
    if (isActivationInProgress_)
    {
        isActivationInProgress_ = false;
        hr = typeHost_.ActivationManager->EndActivateCodePackagesAndWaitForEvent(context);
        if(hr == S_OK)
        {
            this->RegisterForEvents();
        }
    }
    else if (isDeactivationInProgress_)
    {
        isDeactivationInProgress_ = false;
        hr = typeHost_.ActivationManager->EndDeactivateCodePackages(context);
    }
    else
    {
        hr = ComCompletedOperationHelper::EndCompletedComOperation(context);
    }

    if (FAILED(hr))
    { 
        return ComUtility::OnPublicApiReturn(hr); 
    }

    auto error = EndpointsHelper::ConvertToJsonString(serviceTraceInfo_, typeHost_.Endpoints, endpointsAsJsonStr_);
    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(error.ToHResult());
    }

    return ComStringResult::ReturnStringResult(endpointsAsJsonStr_, serviceAddress);
}

HRESULT ComGuestServiceReplica::BeginClose(
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    WriteNoise(TraceType, "BeginClose: {0}", serviceTraceInfo_);

    if (typeHost_.HostContext.IsCodePackageActivatorHost == false)
    {
        return ComCompletedOperationHelper::BeginCompletedComOperation(callback, context);
    }

    this->UnregisterForEvents();

    //
    // If replica has reported fault, it means one of CPs
    // continuous failure has exceeded failure threshold
    // and we want to trigger failover. Skip deativating
    // other CPs. If replica gets placed on other node 
    // (unlikely for stateful replica) then these CPs will
    // get deactivated as part of SP deactivation.
    //
    if (hasReportedFault_.load() == true)
    {
        return ComCompletedOperationHelper::BeginCompletedComOperation(callback, context);
    }

    return typeHost_.ActivationManager->BeginDeactivateCodePackages(
        HostingConfig::GetConfig().ActivationTimeout.Milliseconds,
        callback,
        context);
}

HRESULT ComGuestServiceReplica::EndClose(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    HRESULT hr;
    if (typeHost_.HostContext.IsCodePackageActivatorHost == true &&
        hasReportedFault_.load() == false)
    {
        hr = typeHost_.ActivationManager->EndDeactivateCodePackages(context);
    }
    else
    {
        hr = ComCompletedOperationHelper::EndCompletedComOperation(context);
    }

    auto error = ErrorCode::FromHResult(hr);
    WriteTrace(
        error.ToLogLevel(),
        TraceType,
        "EndClose {0}. Error={1}",
        serviceTraceInfo_,
        error);

    return ComUtility::OnPublicApiReturn(hr);
}

void ComGuestServiceReplica::Abort(void)
{
    WriteNoise(TraceType, "Aborted {0}", serviceTraceInfo_);

    if (typeHost_.HostContext.IsCodePackageActivatorHost == true)
    {
        this->UnregisterForEvents();

        //
        // If replica has reported fault, it means one of CPs
        // continuous failure has exceeded failure threshold
        // and we want to trigger failover. Skip deativating
        // other CPs. If replica gets placed on other node 
        // (unlikely for stateful replica) then these CPs will
        // get deactivated as part of SP deactivation.
        //
        if (hasReportedFault_.load() == false)
        {
            typeHost_.ActivationManager->AbortCodePackages();
        }
    }
}

void ComGuestServiceReplica::OnActivatedCodePackageTerminated(
    CodePackageEventDescription && eventDesc)
{
    WriteWarning(
        TraceType,
        "OnActivatedCodePackageTerminated - Reporting FaultTransient: {0}. {1}",
        serviceTraceInfo_,
        eventDesc);

    // TODO: Report Health.
    hasReportedFault_.store(true);
    partition_->ReportFault(FABRIC_FAULT_TYPE_TRANSIENT);
}

std::wstring ComGuestServiceReplica::BuildTraceInfo(
    wstring const & serviceName,
    wstring const & serviceTypeName,
    Guid const & partitionId,
    FABRIC_REPLICA_ID instanceId)
{
    auto traceInfo = wformatString(
        "ServiceTypeName={0}, ServiceName={1}, PartitionId={2}, ReplicaId={3}",
        serviceTypeName,
        serviceName,
        partitionId,
        instanceId);

    return move(traceInfo);
}

void ComGuestServiceReplica::RegisterForEvents()
{
    ComPointer<IGuestServiceReplica> eventHandler;
    eventHandler.SetAndAddRef(this);

    typeHost_.ActivationManager->RegisterServiceReplica(eventHandler, eventRegistrationHandle_);
}

void ComGuestServiceReplica::UnregisterForEvents()
{
    typeHost_.ActivationManager->UnregisterServiceReplicaOrInstance(eventRegistrationHandle_);
}

bool ComGuestServiceReplica::Test_IsReplicaFaulted()
{
    return (hasReportedFault_.load() == true);
}

void ComGuestServiceReplica::Test_SetReplicaFaulted(bool value)
{
    hasReportedFault_.store(value);
}
