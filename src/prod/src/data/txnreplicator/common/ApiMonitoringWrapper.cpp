// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace TxnReplicator;
using namespace Data::Utilities;
using namespace Common::ApiMonitoring;

Common::StringLiteral const TraceComponent("ApiMonitoringWraper");

#pragma region ApiMonitoringWrapper

ApiMonitoringWrapper::ApiMonitoringWrapper(
  __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
  __in Common::TimeSpan const & scanInterval,
    __in Data::Utilities::PartitionedReplicaId const & traceId)
    : KObject()
    , KShared()
    , PartitionedReplicaTraceComponent(traceId)
{
  component_ = ApiMonitoringWrapper::ApiMonitoringComponent::Create(healthClient, scanInterval, traceId.ReplicaId, Common::Guid(traceId.PartitionId));
}

ApiMonitoringWrapper::~ApiMonitoringWrapper()
{
}

ApiMonitoringWrapper::SPtr ApiMonitoringWrapper::Create(
  __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
  __in Common::TimeSpan const & scanInterval,
  __in Data::Utilities::PartitionedReplicaId const & traceId,
    __in KAllocator & allocator)
{
    ApiMonitoringWrapper * pointer = _new(APIMONITORING_TAG, allocator) ApiMonitoringWrapper(healthClient, scanInterval, traceId);

    THROW_ON_ALLOCATION_FAILURE(pointer);

    return ApiMonitoringWrapper::SPtr(pointer);
}

Common::ApiMonitoring::ApiCallDescriptionSPtr ApiMonitoringWrapper::GetApiCallDescriptionFromName(
  __in Data::Utilities::PartitionedReplicaId & partitionedReplicaId,
  __in Common::ApiMonitoring::ApiNameDescription const & nameDesc,
  __in Common::TimeSpan const & slowApiTime)
{

  return ApiMonitoringWrapper::ApiMonitoringComponent::GetApiCallDescriptionFromName(
    partitionedReplicaId,
    nameDesc,
    slowApiTime);
}

#pragma endregion

#pragma region ApiMonitoringComponent

ApiMonitoringWrapper::ApiMonitoringComponentSPtr ApiMonitoringWrapper::ApiMonitoringComponent::Create(
  __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
  __in Common::TimeSpan const & scanInterval,
  __in ::FABRIC_REPLICA_ID const & endpointUniqueId,
  __in Common::Guid const & partitionId)
{
  auto created = std::shared_ptr<ApiMonitoringComponent>(new ApiMonitoringComponent(healthClient, endpointUniqueId, partitionId));

  if (scanInterval != Common::TimeSpan::Zero)
  {
    created->Open(scanInterval);
  }

  return created;
}

ApiMonitoringWrapper::ApiMonitoringComponent::ApiMonitoringComponent(
  __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
  __in ::FABRIC_REPLICA_ID const & endpointUniqueId,
  __in Common::Guid const & partitionId)
  : healthClient_(healthClient)
  , endpointUniqueId_(endpointUniqueId)
  , partitionId_(partitionId)
  , monitor_()
  , isActive_(false)
{
}

ApiMonitoringWrapper::ApiMonitoringComponent::~ApiMonitoringComponent()
{
  ASSERT_IF(
    isActive_,
    "{0} ApiMonitoringWrapper should be closed before destruction.", TraceId);
}

Common::ApiMonitoring::ApiCallDescriptionSPtr ApiMonitoringWrapper::ApiMonitoringComponent::GetApiCallDescriptionFromName(
    __in Data::Utilities::PartitionedReplicaId & partitionedReplicaId,
    __in Common::ApiMonitoring::ApiNameDescription const & nameDesc,
    __in Common::TimeSpan const & slowApiTime)
{
    Common::Guid partitionId = Common::Guid(partitionedReplicaId.PartitionId);
    MonitoringData monitoringData = MonitoringData(partitionId, partitionedReplicaId.ReplicaId, 0, nameDesc, Common::StopwatchTime::FromDateTime(Common::DateTime::Now()));
    MonitoringParameters monitoringParameters = MonitoringParameters(true, true, false, slowApiTime);
    
    return std::make_shared<ApiCallDescription>(monitoringData, monitoringParameters);
}

void ApiMonitoringWrapper::ApiMonitoringComponent::Open(__in Common::TimeSpan const & timeSpan)
{
    K_LOCK_BLOCK(lock_)
    {
        if (isActive_)
        {
            return;
        }

        Common::ApiMonitoring::MonitoringComponentConstructorParameters parameters;
        parameters.Root = this;
        parameters.ScanInterval = timeSpan;
        parameters.SlowHealthReportCallback = [this](MonitoringHealthEventList const & events, MonitoringComponentMetadata const &) { SlowHealthCallback(events); };
        parameters.ClearSlowHealthReportCallback = [this](MonitoringHealthEventList const & events, MonitoringComponentMetadata const &) { ClearSlowHealth(events); };
        
        monitor_ = MonitoringComponent::Create(parameters);

        // Curently we add NodeName and NodeInstance information for failover APIs. These information can be added for Replicator APIs in the future. 
        monitor_->Open(MonitoringComponentMetadata(L""/*NodeName*/, Federation::NodeInstance()));
        isActive_ = true;
    }
}

void ApiMonitoringWrapper::ApiMonitoringComponent::Close()
{
    K_LOCK_BLOCK(lock_)
    {
        if (isActive_)
        {
            monitor_->Close();
        }
    }
}

void ApiMonitoringWrapper::ApiMonitoringComponent::StartMonitoring(__in ApiCallDescriptionSPtr const & desc)
{
    K_LOCK_BLOCK(lock_)
    {
        if (!isActive_)
        {
            return;
        }
    }

    monitor_->StartMonitoring(desc);
}

void ApiMonitoringWrapper::ApiMonitoringComponent::StopMonitoring(__in ApiCallDescriptionSPtr const & desc, __out Common::ErrorCode const & errorCode)
{
    K_LOCK_BLOCK(lock_)
    {
        if (!isActive_)
        {
            return;
        }
    }

    monitor_->StopMonitoring(desc, errorCode);
}

void ApiMonitoringWrapper::ApiMonitoringComponent::SlowHealthCallback(Common::ApiMonitoring::MonitoringHealthEventList const & slowList)
{
    Reliability::ReplicationComponent::IReplicatorHealthClientSPtr healthClientSnap;

    K_LOCK_BLOCK(lock_)
    {
        if (!isActive_)
        {
            return;
        }
        
        healthClientSnap = healthClient_;
    }

    ASSERT_IFNOT(
        healthClientSnap,
        "{0} ApiMonitoringWrapper::SlowHealthCallback - Health client not expected to be empty", TraceId);

    for (Common::ApiMonitoring::MonitoringHealthEvent const & item : slowList)
    {
        ReportHealth(TraceId, FABRIC_HEALTH_STATE_WARNING, item, healthClientSnap);
    }
}

void ApiMonitoringWrapper::ApiMonitoringComponent::ClearSlowHealth(Common::ApiMonitoring::MonitoringHealthEventList const & slowList)
{
    Reliability::ReplicationComponent::IReplicatorHealthClientSPtr healthClientSnap;

    K_LOCK_BLOCK(lock_)
    {
        if (!isActive_)
        {
            return;
        }

        healthClientSnap = healthClient_;
    }

    ASSERT_IFNOT(
        healthClientSnap,
        "{0} ApiMonitoringWrapper::ClearSlowHealth - Health client not expected to be empty", TraceId);

    for (Common::ApiMonitoring::MonitoringHealthEvent const & item : slowList)
    {
        ReportHealth(TraceId, FABRIC_HEALTH_STATE_OK, item, healthClientSnap);
    }
}

void ApiMonitoringWrapper::ApiMonitoringComponent::ReportHealth(
  const std::wstring traceId,
    FABRIC_HEALTH_STATE toReport,
    Common::ApiMonitoring::MonitoringHealthEvent const & monitoringEvent,
    Reliability::ReplicationComponent::IReplicatorHealthClientSPtr healthClient)
{
    wstring dynamicProperty = monitoringEvent.first->Api.Metadata; // metadata contains the dynamic property
    Common::SystemHealthReportCode::Enum reportCode = toReport == FABRIC_HEALTH_STATE_OK ? Common::SystemHealthReportCode::RE_ApiOk : Common::SystemHealthReportCode::RE_ApiSlow;
    Common::TimeSpan ttl = toReport == FABRIC_HEALTH_STATE_OK ? Common::TimeSpan::FromSeconds(300) : Common::TimeSpan::MaxValue;

  if (toReport == FABRIC_HEALTH_STATE_OK)
    {
      TREventSource::Events->ApiMonitoringWrapperInfo(
          monitoringEvent.first->PartitionId,
          monitoringEvent.first->ReplicaId,
          dynamicProperty,
          L"OK");
    }
    else
    {
        TREventSource::Events->ApiMonitoringWrapperWarning(
            monitoringEvent.first->PartitionId,
            monitoringEvent.first->ReplicaId,
            dynamicProperty,
            L"Warning");
    }

    healthClient->ReportReplicatorHealth(
        reportCode,
        dynamicProperty,
        L"",
        monitoringEvent.second,
        ttl);
}

#pragma endregion
