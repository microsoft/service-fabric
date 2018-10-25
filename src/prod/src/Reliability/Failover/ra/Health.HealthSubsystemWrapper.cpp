// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;

using namespace Common;
using namespace Client;
using namespace Federation;
using namespace ServiceModel;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Health;
using namespace Transport;
using namespace Infrastructure;

const Common::TimeSpan ClearErrorHealthReportTTL = Common::TimeSpan::FromMinutes(5);

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Health
        {
            namespace ReplicaHealthEvent
            {
                void ReplicaHealthEvent::WriteToTextWriter(TextWriter & w, Enum const & e)
                {
                    switch (e)
                    {
                    case OK:
                        w << "OK";
                        break;

                    case Restart:
                        w << "Restart";
                        break;

                    case Close:
                        w << "Close";
                        break;

                    case OpenWarning:
                        w << "OpenWarning";
                        break;

                    case ClearOpenWarning:
                        w << "ClearOpenWarning";
                        break;

                    case CloseWarning:
                        w << "CloseWarning";
                        break;

                    case ClearCloseWarning:
                        w << "ClearCloseWarning";
                        break;

                    case Error:
                        w << "Error";
                        break;

                    case ClearError:
                        w << "ClearError";
                        break;

                    case ServiceTypeRegistrationWarning:
                        w << "ServiceTypeRegistrationWarning";
                        break;

                    case ClearServiceTypeRegistrationWarning:
                        w << "ClearServiceTypeRegistrationWarning";
                        break;

                    case ReconfigurationStuckWarning:
                        w << "ReconfigurationStuckWarning";
                        break;

                    case ClearReconfigurationStuckWarning:
                        w << "ClearReconfigurationStuckWarning";
                        break;

                    default:
                        Assert::CodingError("Invalid state for internal enum {0}", static_cast<int>(e));
                    }
                }

                ENUM_STRUCTURED_TRACE(ReplicaHealthEvent, OK, LastValidEnum);
            }
        }
    }
}

void IHealthSubsystemWrapper::EnqueueReplicaHealthAction(
    ReplicaHealthEvent::Enum type,
    FailoverUnitId const & ftId,
    bool isStateful,
    uint64 replicaId,
    uint64 instanceId,
    StateMachineActionQueue & queue,
    IHealthDescriptorSPtr const & reportDescriptor)
{
    auto reportHealthAction = make_unique<Health::ReportReplicaHealthStateMachineAction>(
        type,
        ftId,
        isStateful,
        replicaId,
        instanceId,
        move(reportDescriptor));
    queue.Enqueue(move(reportHealthAction));
}

HealthSubsystemWrapper::HealthSubsystemWrapper(
    IReliabilitySubsystemWrapperSPtr const & reliability, 
    IFederationWrapper & federation,
    bool isHealthReportingEnabled,
    wstring const nodeName) : 
    reliability_(reliability),
    federation_(federation),
    isHealthReportingEnabled_(isHealthReportingEnabled),
    healthContext_(nodeName)
{
}

ErrorCode HealthSubsystemWrapper::ReportStoreProviderEvent(HealthReport && healthReport)
{
    if (!isHealthReportingEnabled_)
    {
        return ErrorCodeValue::Success;
    }

    return reliability_->AddHealthReport(move(healthReport));
}

ErrorCode HealthSubsystemWrapper::ReportReplicaEvent(
    ReplicaHealthEvent::Enum type,
    FailoverUnitId const & ftId,
    bool isStateful,
    uint64 replicaId,
    uint64 instanceId,
    HealthReportDescriptorBase const & reportDescriptor)
{
    if (!isHealthReportingEnabled_)
    {
        return ErrorCodeValue::Success;
    }

    HealthReport healthReport;

    AttributeList attributeList;
    attributeList.AddAttribute(*HealthAttributeNames::NodeId, wformatString(federation_.Instance.Id));
    attributeList.AddAttribute(*HealthAttributeNames::NodeInstanceId, wformatString(federation_.Instance.InstanceId));

    EntityHealthInformationSPtr entityInformation;
    if (isStateful)
    {
        entityInformation = EntityHealthInformation::CreateStatefulReplicaEntityHealthInformation(ftId.Guid, replicaId, instanceId);
    }
    else
    {
        entityInformation = EntityHealthInformation::CreateStatelessInstanceEntityHealthInformation(ftId.Guid, replicaId);
    }
        
    auto description = reportDescriptor.GenerateReportDescription(healthContext_);

    switch (type)
    {
    case ReplicaHealthEvent::OK:
        healthReport = HealthReport::CreateSystemHealthReport(
            SystemHealthReportCode::RA_ReplicaCreated,
            move(entityInformation),
            description,
            move(attributeList));
        break;

    case ReplicaHealthEvent::Restart:
        ASSERT_IFNOT(isStateful, "Cannot have a restart event for stateless {0}", ftId);
        healthReport = HealthReport::CreateSystemHealthReport(
            SystemHealthReportCode::RA_ReplicaCreated,
            move(entityInformation),
            description,
            move(attributeList));
        break;

    case ReplicaHealthEvent::OpenWarning:
        healthReport = HealthReport::CreateSystemHealthReport(
            SystemHealthReportCode::RA_ReplicaOpenStatusWarning,
            move(entityInformation),
            description,
            move(attributeList));
        break;

    case ReplicaHealthEvent::ClearOpenWarning:
        healthReport = HealthReport::CreateSystemHealthReport(
            SystemHealthReportCode::RA_ReplicaOpenStatusHealthy,
            move(entityInformation),
            L""/*dynamicProperty*/,
            description,
            ClearErrorHealthReportTTL,
            move(attributeList));
        break;

    case ReplicaHealthEvent::CloseWarning:
        healthReport = HealthReport::CreateSystemHealthReport(
            SystemHealthReportCode::RA_ReplicaCloseStatusWarning,
            move(entityInformation),
            description,
            move(attributeList));
        break;

    case ReplicaHealthEvent::ClearCloseWarning:
        healthReport = HealthReport::CreateSystemHealthReport(
            SystemHealthReportCode::RA_ReplicaCloseStatusHealthy,
            move(entityInformation),
            L""/*dynamicProperty*/,
            description,
            ClearErrorHealthReportTTL,
            move(attributeList));
        break;

    case ReplicaHealthEvent::ServiceTypeRegistrationWarning:
        healthReport = HealthReport::CreateSystemHealthReport(
            SystemHealthReportCode::RA_ReplicaServiceTypeRegistrationStatusWarning,
            move(entityInformation),
            description,
            move(attributeList));
        break;

    case ReplicaHealthEvent::ClearServiceTypeRegistrationWarning:
        healthReport = HealthReport::CreateSystemHealthReport(
            SystemHealthReportCode::RA_ReplicaServiceTypeRegistrationStatusHealthy,
            move(entityInformation),
            L""/*dynamicProperty*/,
            description,
            ClearErrorHealthReportTTL,
            move(attributeList));
        break;

    case ReplicaHealthEvent::Error:
        healthReport = HealthReport::CreateSystemHealthReport(
            SystemHealthReportCode::RA_ReplicaChangeRoleStatusError,
            move(entityInformation),
            description,
            move(attributeList));
        break;

    case ReplicaHealthEvent::ClearError:
        healthReport = HealthReport::CreateSystemHealthReport(
            SystemHealthReportCode::RA_ReplicaChangeRoleStatusHealthy,
            move(entityInformation),
            L""/*dynamicProperty*/,
            description,
            ClearErrorHealthReportTTL,
            move(attributeList));
        break;

    case ReplicaHealthEvent::Close:
        healthReport = HealthReport::CreateSystemDeleteEntityHealthReport(
            SystemHealthReportCode::RA_DeleteReplica, 
            move(entityInformation));
        break;

    case ReplicaHealthEvent::ReconfigurationStuckWarning:
        healthReport = HealthReport::CreateSystemHealthReport(
            SystemHealthReportCode::RA_ReconfigurationStuckWarning,
            move(entityInformation),
            description,
            move(attributeList));
        break;

    case ReplicaHealthEvent::ClearReconfigurationStuckWarning:
        healthReport = HealthReport::CreateSystemHealthReport(
            SystemHealthReportCode::RA_ReconfigurationHealthy,
            move(entityInformation),
            L""/*dynamicProperty*/,
            description,
            ClearErrorHealthReportTTL,
            move(attributeList));
        break;
        

    default:
        Assert::CodingError("unknown replica health event {0}", type);
    }

    return reliability_->AddHealthReport(move(healthReport));
}

void HealthSubsystemWrapper::SendToHealthManager(
    MessageUPtr && messagePtr,
    IpcReceiverContextUPtr && ipcTransportContext)
{
    reliability_->ForwardHealthReportFromRAPToHealthManager(move(messagePtr), move(ipcTransportContext));
}
