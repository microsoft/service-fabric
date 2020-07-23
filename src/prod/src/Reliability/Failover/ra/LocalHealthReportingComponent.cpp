// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Api;
using namespace Client;
using namespace Common;
using namespace Common::ApiMonitoring;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace ServiceModel;
using namespace Transport;

LocalHealthReportingComponent::LocalHealthReportingComponent(
	Common::ComponentRoot const & root,
	IpcHealthReportingClientSPtr && healthClient) :
	Common::RootedObject(root),
	healthClient_(move(healthClient))
{
    ApiMonitoring::MonitoringComponentConstructorParameters parameters;
    parameters.Root = &root;
    parameters.ScanInterval = FailoverConfig::GetConfig().LocalHealthReportingTimerInterval;
    parameters.SlowHealthReportCallback = [this] (ApiMonitoring::MonitoringHealthEventList const & p, ApiMonitoring::MonitoringComponentMetadata const & metaData)
    {
        OnHealthEvent(SystemHealthReportCode::RAP_ApiSlow, p, metaData, TimeSpan::MaxValue, L"stuck");
    };

    parameters.ClearSlowHealthReportCallback = [this](ApiMonitoring::MonitoringHealthEventList const & p, ApiMonitoring::MonitoringComponentMetadata const & metaData)
    {
        OnHealthEvent(SystemHealthReportCode::RAP_ApiOk, p, metaData, FailoverConfig::GetConfig().RAPApiOKHealthEventDuration, L"complete");
    };

    monitoringComponent_ = ApiMonitoring::MonitoringComponent::Create(parameters);
}

void LocalHealthReportingComponent::Open(ReconfigurationAgentProxyId const & , Federation::NodeInstance const & nodeInstance, wstring const & nodeName)
{
    auto metaData = MonitoringComponentMetadata(nodeName, nodeInstance);
    monitoringComponent_->Open(metaData);
}

void LocalHealthReportingComponent::Close()
{
    monitoringComponent_->Close();
}

void LocalHealthReportingComponent::StartMonitoring(ApiCallDescriptionSPtr const & description)
{
    monitoringComponent_->StartMonitoring(description);
}

void LocalHealthReportingComponent::StopMonitoring(
    ApiMonitoring::ApiCallDescriptionSPtr const & apiCallDescription,
    Common::ErrorCode const & error)
{
    monitoringComponent_->StopMonitoring(apiCallDescription, error);
}

void LocalHealthReportingComponent::ReportHealth(vector<HealthReport> && healthReports, HealthReportSendOptionsUPtr && sendOptions)
{
    healthClient_->AddReports(move(healthReports), move(sendOptions));
}

void LocalHealthReportingComponent::ReportHealth(HealthReport && healthReport, HealthReportSendOptionsUPtr && sendOptions)
{
    healthClient_->AddReport(move(healthReport), move(sendOptions));
}

HealthReport CreateHealthReport(
	Common::SystemHealthReportCode::Enum code,
	MonitoringHealthEvent const & it,
	Common::TimeSpan const & ttl,
	std::wstring const & nodeName,
	Federation::NodeInstance const & nodeInstance,
    std::wstring const & status)
{
    ApiNameDescription const & name = it.first->Api;
    wstring formattedName;
    if (name.Metadata.empty())
    {
        formattedName = wformatString("{0}.{1}", name.Interface, name.Api);
    }
    else
    {
        formattedName = wformatString("{0}.{1}({2})", name.Interface, name.Api, name.Metadata);
    }

    // Note the use of StopwatchTime::ToDateTime
    // The ticks from a stopwatch time cannot directly be converted to datetime
    wstring description = wformatString(" {0} on node {1} is {2}. Start Time (UTC): {3}.", formattedName, nodeName, status, it.first->StartTime.ToDateTime());

    AttributeList attributeList;
    attributeList.AddAttribute(*HealthAttributeNames::NodeId, wformatString(nodeInstance.Id));
    attributeList.AddAttribute(*HealthAttributeNames::NodeInstanceId, wformatString(nodeInstance.InstanceId));

    EntityHealthInformationSPtr entityInformation;
    if (name.Interface == InterfaceName::IStatelessServiceInstance)
    {
        entityInformation = EntityHealthInformation::CreateStatelessInstanceEntityHealthInformation(
            it.first->PartitionId,
            it.first->ReplicaId);
    }
    else
    {
        entityInformation = EntityHealthInformation::CreateStatefulReplicaEntityHealthInformation(
            it.first->PartitionId,
            it.first->ReplicaId,
            it.first->ReplicaInstanceId);
    }

    return HealthReport::CreateSystemHealthReport(
        code,
        move(entityInformation),
        formattedName + *Reliability::Constants::HealthReportDurationTag,
        description,
        it.second,
        ttl,
        move(attributeList));
}

void LocalHealthReportingComponent::OnHealthEvent(
    SystemHealthReportCode::Enum code, 
    ApiMonitoring::MonitoringHealthEventList const & events, 
	ApiMonitoring::MonitoringComponentMetadata const & metaData,
    Common::TimeSpan const & ttl,
    std::wstring const & status)
{
    vector<ServiceModel::HealthReport> reports;

    for (auto const & it : events)
    {
        reports.push_back(CreateHealthReport(code, it, ttl, metaData.NodeName, metaData.NodeInstance, status));
    }

    ReportHealth(move(reports), nullptr);
}
