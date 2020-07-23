// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

StringLiteral const TraceType("ComGuestServiceInstance");

ComGuestServiceInstance::ComGuestServiceInstance(
    IGuestServiceTypeHost & typeHost,
    wstring const & serviceName,
    wstring const & serviceTypeName,
    Guid const & partitionId,
    FABRIC_INSTANCE_ID instanceId)
    : IGuestServiceInstance()
    , ComUnknownBase()
    , typeHost_(typeHost)
    , serviceName_(serviceName)
    , serviceTypeName_(serviceTypeName)
    , partitionId_(partitionId)
    , instanceId_(instanceId)
    , endpointsAsJsonStr_()
    , partition_()
    , hasReportedFault_(false)
    , eventRegistrationHandle_(0)
    , serviceTraceInfo_(BuildTraceInfo(serviceTypeName, serviceName, partitionId, instanceId))
{
}

ComGuestServiceInstance::~ComGuestServiceInstance()
{
}

HRESULT ComGuestServiceInstance::BeginOpen(
    /* [in] */ IFabricStatelessServicePartition *partition,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    WriteNoise(TraceType, "BeginOpen {0}", serviceTraceInfo_);
    
    IFabricStatelessServicePartition2 * partition2;
    auto hr = partition->QueryInterface(&partition2);

    ASSERT_IF(FAILED(hr), "Partition must implement IFabricStatelessServicePartition2");

    partition_.SetAndAddRef(partition2);

    if (typeHost_.HostContext.IsCodePackageActivatorHost == true)
    {
        return typeHost_.ActivationManager->BeginActivateCodePackagesAndWaitForEvent(
            nullptr /* custom environment */,
            HostingConfig::GetConfig().ActivationTimeout.Milliseconds,
            callback,
            context);
    }
    
    return ComCompletedOperationHelper::BeginCompletedComOperation(callback, context);
}
        
HRESULT ComGuestServiceInstance::EndOpen( 
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricStringResult **serviceAddress)
{
    *serviceAddress = nullptr;

    HRESULT hr;
    if (typeHost_.HostContext.IsCodePackageActivatorHost == true)
    {
        hr = typeHost_.ActivationManager->EndActivateCodePackagesAndWaitForEvent(context);
        if (hr == S_OK)
        {
            this->RegisterForEvents();
        }
    }
    else
    {
        hr = ComCompletedOperationHelper::EndCompletedComOperation(context);
    }

    auto error = ErrorCode::FromHResult(hr);
    WriteTrace(
        error.ToLogLevel(),
        TraceType,
        "EndOpen {0}. Error={1}",
        serviceTraceInfo_,
        error);

    if (FAILED(hr)) 
    {
        return ComUtility::OnPublicApiReturn(hr); 
    }

    error = EndpointsHelper::ConvertToJsonString(serviceTraceInfo_, typeHost_.Endpoints, endpointsAsJsonStr_);
    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(error.ToHResult());
    }

    return ComStringResult::ReturnStringResult(endpointsAsJsonStr_, serviceAddress);
}

HRESULT ComGuestServiceInstance::BeginClose( 
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    WriteNoise(TraceType, "BeginClose: {0}", serviceTraceInfo_);

    if (typeHost_.HostContext.IsCodePackageActivatorHost == false)
    {
        return ComCompletedOperationHelper::BeginCompletedComOperation(callback, context);
    }

    this->UnregisterForEvents();

    if (hasReportedFault_.load() == true)
    {
        //
        // If replica has reported fault, it means one of CPs
        // continuous failure has exceeded failure threshold
        // and we want to trigger failover. Skip deativating
        // other CPs. If replica gets placed on other node
        // then these CPs will get deactivated as part of 
        // SP deactivation.
        //
        return ComCompletedOperationHelper::BeginCompletedComOperation(callback, context);
    }

    return typeHost_.ActivationManager->BeginDeactivateCodePackages(
        HostingConfig::GetConfig().ActivationTimeout.Milliseconds,
        callback,
        context);
}

HRESULT ComGuestServiceInstance::EndClose( 
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

void ComGuestServiceInstance::Abort(void)
{
    WriteNoise(TraceType, "Aborted {0}", serviceTraceInfo_);

    if (typeHost_.HostContext.IsCodePackageActivatorHost == true)
    {
        this->UnregisterForEvents();

        //
        // If replica has reported fault, it means one of CPs
        // continuous failure has exceeded failure threshold
        // and we want to trigger failover. Skip deativating
        // other CPs. If replica gets placed or other node
        // then these CPs will get deactivated as part of 
        // SP deactivation.
        //
        if (hasReportedFault_.load() == false)
        {
            typeHost_.ActivationManager->AbortCodePackages();
        }
    }
}

void ComGuestServiceInstance::OnActivatedCodePackageTerminated(
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

wstring ComGuestServiceInstance::BuildTraceInfo(
    wstring const & serviceName,
    wstring const & serviceTypeName,
    Guid const & partitionId,
    FABRIC_INSTANCE_ID instanceId)
{
    auto traceInfo = wformatString(
        "ServiceTypeName={0}, ServiceName={1}, PartitionId={2}, ReplicaId={3}",
        serviceTypeName,
        serviceName,
        partitionId,
        instanceId);

    return move(traceInfo);
}

void ComGuestServiceInstance::RegisterForEvents()
{
    ComPointer<IGuestServiceInstance> eventHandler;
    eventHandler.SetAndAddRef(this);

    typeHost_.ActivationManager->RegisterServiceInstance(eventHandler, eventRegistrationHandle_);
}

void ComGuestServiceInstance::UnregisterForEvents()
{
    typeHost_.ActivationManager->UnregisterServiceReplicaOrInstance(eventRegistrationHandle_);
}

bool ComGuestServiceInstance::Test_IsInstanceFaulted()
{
    return (hasReportedFault_.load() == true);
}

void ComGuestServiceInstance::Test_SetReplicaFaulted(bool value)
{
    hasReportedFault_.store(value);
}
