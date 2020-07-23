// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Reliability {
namespace ReplicationComponent {

using Common::AcquireExclusiveLock;
using Common::AcquireReadLock;
using Common::Assert;
using Common::ErrorCode;
using Common::RwLock;
using Common::StringWriter;
using Common::Timer;
using Common::TimerSPtr;
using Common::TimeSpan;
using std::wstring;
using std::move;

using namespace Common::ApiMonitoring;

ApiMonitoringWrapperSPtr ApiMonitoringWrapper::Create(
    IReplicatorHealthClientSPtr & healthClient,
    REInternalSettingsSPtr const & config,
    Common::Guid const & partitionId,
    ReplicationEndpointId const & endpointId)
{
    auto scanInterval = config->SlowApiMonitoringInterval;

    auto created = std::shared_ptr<ApiMonitoringWrapper>(new ApiMonitoringWrapper(healthClient, partitionId, endpointId));
    
    if (scanInterval != TimeSpan::Zero)
    {
        created->Open(scanInterval);
    }

    return created;
}

ApiMonitoringWrapper::ApiMonitoringWrapper(
    IReplicatorHealthClientSPtr & healthClient,
    Common::Guid const & partitionId,
    ReplicationEndpointId const & endpointId)
    : ComponentRoot(),
    healthClient_(healthClient),
    partitionId_(partitionId),
    endpointUniqueId_(endpointId),
    monitor_(),
    isActive_(false),
    lock_()
{
}

ApiMonitoringWrapper::~ApiMonitoringWrapper()
{
    ASSERT_IF(isActive_, "{0}:{1} - ApiMonitoringWrapper should be closed before destruction", partitionId_, endpointUniqueId_);
}

void ApiMonitoringWrapper::Open(TimeSpan const & timeSpan)
{
    AcquireExclusiveLock grab(lock_);
    if (isActive_)
    {
        return;
    }

    ASSERT_IF(timeSpan == TimeSpan::Zero, "{0}:{1} - ApiMonitoringWrapper cannot be opened with scan interval 0", partitionId_, endpointUniqueId_);

    Common::ApiMonitoring::MonitoringComponentConstructorParameters parameters;
    parameters.Root = this;
    parameters.ScanInterval = timeSpan;
    parameters.SlowHealthReportCallback = [this](MonitoringHealthEventList const & events, MonitoringComponentMetadata const &) { this->SlowHealthCallback(events); };
    parameters.ClearSlowHealthReportCallback = [this](MonitoringHealthEventList const & events, MonitoringComponentMetadata const &) { this->ClearSlowHealth(events); };

    monitor_ = MonitoringComponent::Create(parameters);

    // Curently we add NodeName and NodeInstance information for failover APIs. These information can be added for Replicator APIs in the future.
    monitor_->Open(MonitoringComponentMetadata(L""/*NodeName*/, Federation::NodeInstance()));
    isActive_ = true;
}

void ApiMonitoringWrapper::Close()
{
    AcquireExclusiveLock grab(lock_);
    if (isActive_)
    {
        isActive_ = false;
        monitor_->Close();
        healthClient_.reset();
    }
}

void ApiMonitoringWrapper::StartMonitoring(ApiCallDescriptionSPtr const & desc)
{
    {
        AcquireReadLock grab(lock_);
        if (!isActive_)
        {
            return;
        }
    }

    monitor_->StartMonitoring(desc);
}

void ApiMonitoringWrapper::StopMonitoring(ApiCallDescriptionSPtr const & desc, ErrorCode const & errorCode)
{ 
    {
        AcquireReadLock grab(lock_);
        if (!isActive_)
        {
            return;
        }
    }

    monitor_->StopMonitoring(desc, errorCode);
}

void ApiMonitoringWrapper::SlowHealthCallback(Common::ApiMonitoring::MonitoringHealthEventList const & slowList)
{
    IReplicatorHealthClientSPtr healthClientSnap;

    {
        AcquireReadLock grab(lock_);
        if (!isActive_)
        {
            return;
        }
        
        healthClientSnap = this->healthClient_;
    }

    ASSERT_IFNOT(healthClientSnap, "{0}:{1}: ApiMonitoringWrapper::SlowHealthCallback - Health client not expected to be empty", partitionId_, endpointUniqueId_);

    for (auto const & item : slowList)
    {
        ReportHealth(partitionId_, endpointUniqueId_, FABRIC_HEALTH_STATE_WARNING, item, healthClientSnap);
    }
}

void ApiMonitoringWrapper::ClearSlowHealth(Common::ApiMonitoring::MonitoringHealthEventList const & slowList)
{
    IReplicatorHealthClientSPtr healthClientSnap;

    {
        AcquireReadLock grab(lock_);
        if (!isActive_)
        {
            return;
        }
        
        healthClientSnap = this->healthClient_;
    }

    ASSERT_IFNOT(healthClientSnap, "{0}:{1}: ApiMonitoringWrapper::ClearSlowHealth - Health client not expected to be empty", partitionId_, endpointUniqueId_);

    for (auto const & item : slowList)
    {
        ReportHealth(partitionId_, endpointUniqueId_, FABRIC_HEALTH_STATE_OK, item, healthClientSnap);
    }
}


void ApiMonitoringWrapper::ReportHealth(
    Common::Guid const & partitionId,
    ReplicationEndpointId const & endpointId,
    FABRIC_HEALTH_STATE toReport,
    Common::ApiMonitoring::MonitoringHealthEvent const & monitoringEvent,
    IReplicatorHealthClientSPtr healthClient)
{
    wstring dynamicProperty = monitoringEvent.first->Api.Metadata; // metadata contains the dynamic property
    Common::SystemHealthReportCode::Enum reportCode = HealthReportType::GetHealthReportCode(toReport, monitoringEvent.first->Api.Api);
    TimeSpan ttl = HealthReportType::GetHealthReportTTL(toReport, monitoringEvent.first->Api.Api);
 
    if (toReport == FABRIC_HEALTH_STATE_OK)
    {
        ReplicatorEventSource::Events->ApiMonitoringSend(
            partitionId,
            endpointId,
            Common::wformatString(toReport),
            monitoringEvent.second,
            dynamicProperty);
    }
    else 
    {
        ReplicatorEventSource::Events->ApiMonitoringSendWarning(
            partitionId,
            endpointId,
            Common::wformatString(toReport),
            monitoringEvent.second,
            dynamicProperty);
    }

    healthClient->ReportReplicatorHealth(
        reportCode,
        dynamicProperty,
        L"",
        monitoringEvent.second,
        ttl);
}

ApiCallDescriptionSPtr ApiMonitoringWrapper::GetApiCallDescriptionFromName(
    ReplicationEndpointId const & currentReplicatorId,
    ApiNameDescription const & nameDesc, 
    Common::TimeSpan const & slowApiTime)
{
    switch (nameDesc.Api)
    {
        case ApiName::GetNextCopyContext:
        case ApiName::GetNextCopyState:
        {
            auto monitoringData = MonitoringData(currentReplicatorId.PartitionId, currentReplicatorId.ReplicaId, 0, nameDesc, Common::StopwatchTime::FromDateTime(Common::DateTime::Now()));
            auto monitoringParameters = MonitoringParameters(
                true, // healthReportEnabled
                true, // apiSlowTraceEnabled
                false, // apiLifecycleTraceEnabled
                slowApiTime);

            return std::make_shared<ApiCallDescription>(monitoringData, monitoringParameters);
        }
        
        case ApiName::UpdateEpoch:
        case ApiName::OnDataLoss:
        {
            auto monitoringData = MonitoringData(currentReplicatorId.PartitionId, currentReplicatorId.ReplicaId, 0, nameDesc, Common::StopwatchTime::FromDateTime(Common::DateTime::Now()));
            auto monitoringParameters = MonitoringParameters(
                false, // healthReportEnabled
                true, // apiSlowTraceEnabled
                true, // apiLifecycleTraceEnabled
                slowApiTime);

            return std::make_shared<ApiCallDescription>(monitoringData, monitoringParameters);
        }

        default:
            Common::Assert::CodingError("Replicator.GetApiCallDescriptionFromName unknown api name - {0}", nameDesc.Api);
    }
}

}
}
