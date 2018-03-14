// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Federation;
using namespace ServiceModel;
using namespace Query;
using namespace Store;
using namespace SystemServices;
using namespace Management;
using namespace Management::HealthManager;
using namespace ServiceModel;
using namespace std;
using namespace Testability;

StringLiteral const TraceType("TestabilitySubsystemSubsystem");

// note: this class should be moved to common as this can be used across subcomponents.
NodeHealthReportManager::NodeHealthReportManager(
    Common::ComponentRoot const & root,
    Client::HealthReportingComponentSPtr const & healthClient,
    __in Common::FabricNodeConfigSPtr nodeConfig)
    : RootedObject(root),
    nodeConfig_(nodeConfig),
    healthClient_(healthClient),
    registeredComponents_(),
    lock_(),
    isClosed_(false)
{
}

Common::ErrorCode NodeHealthReportManager::RegisterSource(wstring const & componentId)
{
    ASSERT_IF(componentId.empty(), "ComponentId cannot be empty.");
    {
        AcquireWriteLock lock(lock_);
        if (isClosed_)
        {
            return ErrorCodeValue::ObjectClosed;
        }

        auto componentIter = registeredComponents_.find(componentId);
        ASSERT_IF(
            componentIter != registeredComponents_.end(),
            "ComponentId:{0} is already registered",
            componentId);

        registeredComponents_.insert(make_pair(componentId, false));
    }

    WriteNoise(
        TraceType,
        Root.TraceId,
        "RegisterSource: ComponentId:{0}.",
        componentId);

    return ErrorCodeValue::Success;
}

void NodeHealthReportManager::UnregisterSource(wstring const & componentId)
{
    ASSERT_IF(componentId.empty(), "ComponentId cannot be empty.");

    FABRIC_SEQUENCE_NUMBER healthSequenceNumber;
    bool componentReportedHealth = false;
    {
        AcquireWriteLock lock(lock_);

        if (isClosed_)
        {
            // Object is closed
            return;
        }

        auto componentIter = registeredComponents_.find(componentId);
        if (componentIter == registeredComponents_.end())
        {
            WriteNoise(
                TraceType,
                Root.TraceId,
                "UnregisterSource: ComponentId:{0} is not registered.",
                componentId);
            return;
        }

        componentReportedHealth = componentIter->second;

        registeredComponents_.erase(componentIter);

        healthSequenceNumber = SequenceNumber::GetNext();
    }

    WriteNoise(
        TraceType,
        Root.TraceId,
        "UnregisterSource: ComponentId:{0} is unregistered.",
        componentId);

    if (componentReportedHealth)
    {
        this->DeleteHealthEvent(
            componentId,
            healthSequenceNumber);
    }
}

void NodeHealthReportManager::ReportHealth(
    wstring const & componentId,
    SystemHealthReportCode::Enum reportCode,
    wstring const & healthDescription,
    FABRIC_SEQUENCE_NUMBER sequenceNumber)
{
    ASSERT_IF(componentId.empty(), "ComponentId cannot be empty.");
    {
        AcquireReadLock lock(lock_);

        if (isClosed_) { return; }

        auto componentIter = registeredComponents_.find(componentId);
        if (componentIter == registeredComponents_.end())
        {
            WriteNoise(
                TraceType,
                Root.TraceId,
                "ReportHealth: Component is not registered for the entity. ComponentId:{0}",
                componentId);
            return;
        }

        componentIter->second = true;
    }

    this->ReportHealthEvent(componentId, reportCode, healthDescription, sequenceNumber, TimeSpan::MaxValue);
}

void NodeHealthReportManager::ReportHealthWithTTL(
    wstring const & componentId,
    SystemHealthReportCode::Enum reportCode,
    wstring const & healthDescription,
    FABRIC_SEQUENCE_NUMBER sequenceNumber,
    Common::TimeSpan const timeToLive)
{
    ASSERT_IF(componentId.empty(), "ComponentId cannot be empty.");

    {
        AcquireReadLock lock(lock_);

        if (isClosed_) { return; }

        auto componentIter = registeredComponents_.find(componentId);
        ASSERT_IF(registeredComponents_.find(componentId) != registeredComponents_.end(),
            "'{0}' is reporting health with TTL. The ComponentId should not be registered since the lifetime management of the event is done based on TTL.",
            componentId);
    }

    this->ReportHealthEvent(componentId, reportCode, healthDescription, sequenceNumber, timeToLive);
}

Common::ErrorCode NodeHealthReportManager::OnOpen()
{
    isClosed_ = false;
    return ErrorCodeValue::Success;
}

Common::ErrorCode NodeHealthReportManager::OnClose()
{
    map<wstring, bool> map;
    {
        AcquireWriteLock lock(this->lock_);
        map = registeredComponents_;
        registeredComponents_.clear();
        isClosed_ = true;
    }

    for (auto iter = map.begin(); iter != map.end(); ++iter)
    {
        // Delete only if health is reported for this component
        if (iter->second)
        {
            this->DeleteHealthEvent(
                iter->first,
                SequenceNumber::GetNext());
        }
    }

    return ErrorCodeValue::Success;
}

void NodeHealthReportManager::OnAbort()
{
    this->OnClose().ReadValue();
}

void NodeHealthReportManager::ReportHealthEvent(
    wstring const & propertyName,
    SystemHealthReportCode::Enum reportCode,
    wstring const & healthDescription,
    FABRIC_SEQUENCE_NUMBER healthSequenceNumber,
    Common::TimeSpan const timeToLive)
{
    LargeInteger nodeIdAsLargeInteger;

    if (!LargeInteger::TryParse(nodeConfig_->get_NodeId(), nodeIdAsLargeInteger))
    {
        Common::Assert::CodingError("Failed to parse {0} as large NodeId", nodeConfig_->get_NodeId());
    }
    
    auto entityInfo = EntityHealthInformation::CreateNodeEntityHealthInformation(
        nodeIdAsLargeInteger,
        nodeConfig_->InstanceName,
        nodeConfig_->NodeVersion.get_InstanceId());
    AttributeList attributeList;
    attributeList.AddAttribute(*HealthAttributeNames::NodeName, nodeConfig_->InstanceName);

    auto healthReport = HealthReport::CreateSystemHealthReport(
        reportCode,
        move(entityInfo),
        propertyName,
        healthDescription,
        healthSequenceNumber,
        timeToLive,
        move(attributeList));

    WriteInfo(
        TraceType,
        Root.TraceId,
        "Node ReportHealth: {0}",
        healthReport);

    healthClient_->AddHealthReport(move(healthReport));
}

void NodeHealthReportManager::DeleteHealthEvent(
    wstring const & propertyName,
    FABRIC_SEQUENCE_NUMBER healthSequenceNumber)
{
    LargeInteger nodeIdAsLargeInteger;

    if (!LargeInteger::TryParse(nodeConfig_->get_NodeId(), nodeIdAsLargeInteger))
    {
        Common::Assert::CodingError("Failed to parse {0} as large NodeId", nodeConfig_->get_NodeId());
    }

    auto entityInfo = EntityHealthInformation::CreateNodeEntityHealthInformation(
        nodeIdAsLargeInteger,
        nodeConfig_->InstanceName,
        nodeConfig_->NodeVersion.get_InstanceId());
    AttributeList attributeList;
    attributeList.AddAttribute(*HealthAttributeNames::NodeName, nodeConfig_->InstanceName);

    auto healthReport = HealthReport::CreateSystemRemoveHealthReport(
        SystemHealthReportCode::Hosting_DeleteEvent,
        move(entityInfo),
        propertyName,
        healthSequenceNumber);

    WriteInfo(
        TraceType,
        Root.TraceId,
        "Node DeleteEvent: {0}",
        healthReport);

    healthClient_->AddHealthReport(move(healthReport));
}
