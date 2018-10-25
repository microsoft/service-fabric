// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Management
{
    namespace HealthManager
    {
        Common::Global<HealthManagerEventSource> HealthManagerEventSource::Trace = Common::make_global<HealthManagerEventSource>();

        bool HealthManagerEventSource::ShouldTraceToOperationalChannel(FABRIC_HEALTH_STATE previousState, FABRIC_HEALTH_STATE currentState)
        {
            if (currentState == FABRIC_HEALTH_STATE_WARNING || currentState == FABRIC_HEALTH_STATE_ERROR)
            {
                return true;
            }
            else if (currentState == FABRIC_HEALTH_STATE_OK)
            {
                if (previousState == FABRIC_HEALTH_STATE_WARNING || previousState == FABRIC_HEALTH_STATE_ERROR)
                {
                    return true;
                }
            }

            return false;
        }

        void HealthManagerEventSource::TraceEventExpiredOperational(
            HealthEntityKind::Enum entityKind,
            HealthEventStoreDataUPtr const & eventEntry,
            AttributesStoreDataSPtr const & attributes)
        {
            if (entityKind == HealthEntityKind::Partition)
            {
                auto & castedAttributes = *dynamic_cast<PartitionAttributesStoreData*>(attributes.get());
                auto & healthId = castedAttributes.EntityId;

                HMEvents::Trace->ExpiredPartitionEventOperational(
                    Common::Guid::NewGuid(),
                    healthId,
                    eventEntry->SourceId, // User Input
                    eventEntry->Property, // User Input
                    ServiceModel::HealthState::ConvertToServiceModelHealthState(eventEntry->State),
                    eventEntry->TimeToLive,
                    eventEntry->ReportSequenceNumber,
                    eventEntry->Description,
                    eventEntry->RemoveWhenExpired,
                    eventEntry->SourceUtcTimestamp);
            }
            else if (entityKind == HealthEntityKind::Node)
            {
                auto & castedAttributes = *dynamic_cast<NodeAttributesStoreData*>(attributes.get());

                HMEvents::Trace->ExpiredNodeEventOperational(
                    Guid::NewGuid(),
                    castedAttributes.NodeName, // wstring
                    castedAttributes.NodeInstanceId, // FABRIC_NODE_INSTANCE_ID
                    eventEntry->SourceId, // User Input
                    eventEntry->Property, // User Input
                    ServiceModel::HealthState::ConvertToServiceModelHealthState(eventEntry->State),
                    eventEntry->TimeToLive,
                    eventEntry->ReportSequenceNumber,
                    eventEntry->Description,
                    eventEntry->RemoveWhenExpired,
                    eventEntry->SourceUtcTimestamp);
            }
            else if (entityKind == HealthEntityKind::Service)
            {
                auto & castedAttributes = *dynamic_cast<ServiceAttributesStoreData*>(attributes.get());
                auto & healthId = castedAttributes.EntityId;

                HMEvents::Trace->ExpiredServiceEventOperational(
                    Guid::NewGuid(),
                    healthId.ServiceName,
                    castedAttributes.InstanceId, // FABRIC_INSTANCE_ID
                    eventEntry->SourceId, // User Input
                    eventEntry->Property, // User Input
                    ServiceModel::HealthState::ConvertToServiceModelHealthState(eventEntry->State),
                    eventEntry->TimeToLive,
                    eventEntry->ReportSequenceNumber,
                    eventEntry->Description,
                    eventEntry->RemoveWhenExpired,
                    eventEntry->SourceUtcTimestamp);
            }
            else if (entityKind == HealthEntityKind::Application)
            {
                auto & castedAttributes = *dynamic_cast<ApplicationAttributesStoreData*>(attributes.get());
                auto & healthId = castedAttributes.EntityId;

                HMEvents::Trace->ExpiredApplicationEventOperational(
                    Guid::NewGuid(),
                    healthId.ApplicationName,
                    castedAttributes.InstanceId, // FABRIC_INSTANCE_ID
                    eventEntry->SourceId, // User Input
                    eventEntry->Property, // User Input
                    ServiceModel::HealthState::ConvertToServiceModelHealthState(eventEntry->State),
                    eventEntry->TimeToLive,
                    eventEntry->ReportSequenceNumber,
                    eventEntry->Description,
                    eventEntry->RemoveWhenExpired,
                    eventEntry->SourceUtcTimestamp);
            }
            else if (entityKind == HealthEntityKind::DeployedApplication)
            {
                auto & castedAttributes = *dynamic_cast<DeployedApplicationAttributesStoreData*>(attributes.get());
                auto & healthId = castedAttributes.EntityId;

                HMEvents::Trace->ExpiredDeployedApplicationEventOperational(
                    Common::Guid::NewGuid(),
                    healthId.ApplicationName, // wstring
                    castedAttributes.InstanceId, // FABRIC_INSTANCE_ID
                    castedAttributes.NodeName, // wstring
                    eventEntry->SourceId, // User Input
                    eventEntry->Property, // User Input
                    ServiceModel::HealthState::ConvertToServiceModelHealthState(eventEntry->State),
                    eventEntry->TimeToLive,
                    eventEntry->ReportSequenceNumber,
                    eventEntry->Description,
                    eventEntry->RemoveWhenExpired,
                    eventEntry->SourceUtcTimestamp);
            }
            else if (entityKind == HealthEntityKind::DeployedServicePackage)
            {
                auto & castedAttributes = *dynamic_cast<DeployedServicePackageAttributesStoreData*>(attributes.get());
                auto & healthId = castedAttributes.EntityId;

                HMEvents::Trace->ExpiredDeployedServicePackageEventOperational(
                    Guid::NewGuid(),
                    healthId.ApplicationName, // wstring
                    healthId.ServiceManifestName, // wstring
                    castedAttributes.InstanceId, // FABRIC_INSTANCE_ID - ServicePackageInstanceId
                    healthId.ServicePackageActivationId, // wstring
                    castedAttributes.NodeName, // wstring
                    eventEntry->SourceId, // User Input
                    eventEntry->Property, // User Input
                    ServiceModel::HealthState::ConvertToServiceModelHealthState(eventEntry->State),
                    eventEntry->TimeToLive,
                    eventEntry->ReportSequenceNumber,
                    eventEntry->Description,
                    eventEntry->RemoveWhenExpired,
                    eventEntry->SourceUtcTimestamp);
            }
            else if (entityKind == HealthEntityKind::Cluster)
            {
                HMEvents::Trace->ExpiredClusterEventOperational(
                    Guid::NewGuid(),
                    eventEntry->SourceId, // User Input
                    eventEntry->Property, // User Input
                    ServiceModel::HealthState::ConvertToServiceModelHealthState(eventEntry->State),
                    eventEntry->TimeToLive,
                    eventEntry->ReportSequenceNumber,
                    eventEntry->Description,
                    eventEntry->RemoveWhenExpired,
                    eventEntry->SourceUtcTimestamp);
            }
            else if (entityKind == HealthEntityKind::Replica)
            {
                auto & castedAttributes = *dynamic_cast<ReplicaAttributesStoreData*>(attributes.get());
                auto & healthId = castedAttributes.EntityId;

                if (castedAttributes.Kind == FABRIC_SERVICE_KIND_STATEFUL)
                {
                    HMEvents::Trace->ExpiredStatefulReplicaEventOperational(
                        Guid::NewGuid(),
                        healthId.PartitionId, // Common::Guid
                        healthId.ReplicaId, // FABRIC_REPLICA_ID
                        castedAttributes.InstanceId, // FABRIC_INSTANCE_ID
                        eventEntry->SourceId, // User Input
                        eventEntry->Property, // User Input
                        ServiceModel::HealthState::ConvertToServiceModelHealthState(eventEntry->State),
                        eventEntry->TimeToLive,
                        eventEntry->ReportSequenceNumber,
                        eventEntry->Description,
                        eventEntry->RemoveWhenExpired,
                        eventEntry->SourceUtcTimestamp);
                }
                else
                {
                    HMEvents::Trace->ExpiredStatelessInstanceEventOperational(
                        Guid::NewGuid(),
                        healthId.PartitionId, // Common::Guid
                        healthId.ReplicaId, // FABRIC_REPLICA_ID
                        eventEntry->SourceId, // User Input
                        eventEntry->Property, // User Input
                        ServiceModel::HealthState::ConvertToServiceModelHealthState(eventEntry->State),
                        eventEntry->TimeToLive,
                        eventEntry->ReportSequenceNumber,
                        eventEntry->Description,
                        eventEntry->RemoveWhenExpired,
                        eventEntry->SourceUtcTimestamp);
                }
            }
        }

        void HealthManagerEventSource::TraceProcessReportOperational(
            ServiceModel::HealthReport const & healthReport)
        {
            FABRIC_HEALTH_REPORT_KIND entityKind = healthReport.Kind;

            if (entityKind == FABRIC_HEALTH_REPORT_KIND_PARTITION)
            {
#ifdef DBG
                auto & entityInfo = * dynamic_cast<ServiceModel::PartitionEntityHealthInformation const*>(healthReport.EntityInformation.get());
#else
                auto & entityInfo = *static_cast<ServiceModel::PartitionEntityHealthInformation const*>(healthReport.EntityInformation.get());
#endif

                HMEvents::Trace->ProcessPartitionReportOperational(
                    Guid::NewGuid(),
                    entityInfo.PartitionId, // Guid
                    healthReport.SourceId, // User Input
                    healthReport.Property, // User Input
                    ServiceModel::HealthState::ConvertToServiceModelHealthState(healthReport.State),
                    healthReport.TimeToLive,
                    healthReport.SequenceNumber,
                    healthReport.Description,
                    healthReport.RemoveWhenExpired,
                    healthReport.SourceUtcTimestamp);
            }
            else if (entityKind == FABRIC_HEALTH_REPORT_KIND_NODE)
            {
#ifdef DBG
                auto & entityInfo = *dynamic_cast<ServiceModel::NodeEntityHealthInformation const*>(healthReport.EntityInformation.get());
#else
                auto & entityInfo = *static_cast<ServiceModel::NodeEntityHealthInformation const*>(healthReport.EntityInformation.get());
#endif

                HMEvents::Trace->ProcessNodeReportOperational(
                    Guid::NewGuid(),
                    entityInfo.NodeName, // wstring
                    entityInfo.NodeInstanceId, // FABRIC_NODE_INSTANCE_ID
                    healthReport.SourceId, // User Input
                    healthReport.Property, // User Input
                    ServiceModel::HealthState::ConvertToServiceModelHealthState(healthReport.State),
                    healthReport.TimeToLive,
                    healthReport.SequenceNumber,
                    healthReport.Description,
                    healthReport.RemoveWhenExpired,
                    healthReport.SourceUtcTimestamp);
            }
            else if (entityKind == FABRIC_HEALTH_REPORT_KIND_SERVICE)
            {
#ifdef DBG
                auto & entityInfo = *dynamic_cast<ServiceModel::ServiceEntityHealthInformation const*>(healthReport.EntityInformation.get());
#else
                auto & entityInfo = *static_cast<ServiceModel::ServiceEntityHealthInformation const*>(healthReport.EntityInformation.get());
#endif

                HMEvents::Trace->ProcessServiceReportOperational(
                    Guid::NewGuid(),
                    entityInfo.ServiceName,
                    entityInfo.EntityInstance, // FABRIC_INSTANCE_ID
                    healthReport.SourceId, // User Input
                    healthReport.Property, // User Input
                    ServiceModel::HealthState::ConvertToServiceModelHealthState(healthReport.State),
                    healthReport.TimeToLive,
                    healthReport.SequenceNumber,
                    healthReport.Description,
                    healthReport.RemoveWhenExpired,
                    healthReport.SourceUtcTimestamp);
            }
            else if (entityKind == FABRIC_HEALTH_REPORT_KIND_APPLICATION)
            {
#ifdef DBG
                auto & entityInfo = *dynamic_cast<ServiceModel::ApplicationEntityHealthInformation const*>(healthReport.EntityInformation.get());
#else
                auto & entityInfo = *static_cast<ServiceModel::ApplicationEntityHealthInformation const*>(healthReport.EntityInformation.get());
#endif

                HMEvents::Trace->ProcessApplicationReportOperational(
                    Guid::NewGuid(),
                    entityInfo.ApplicationName,
                    entityInfo.EntityInstance, // FABRIC_INSTANCE_ID
                    healthReport.SourceId, // User Input
                    healthReport.Property, // User Input
                    ServiceModel::HealthState::ConvertToServiceModelHealthState(healthReport.State),
                    healthReport.TimeToLive,
                    healthReport.SequenceNumber,
                    healthReport.Description,
                    healthReport.RemoveWhenExpired,
                    healthReport.SourceUtcTimestamp);
            }
            else if (entityKind == FABRIC_HEALTH_REPORT_KIND_DEPLOYED_APPLICATION)
            {
#ifdef DBG
                auto & entityInfo = *dynamic_cast<ServiceModel::DeployedApplicationEntityHealthInformation const*>(healthReport.EntityInformation.get());
#else
                auto & entityInfo = *static_cast<ServiceModel::DeployedApplicationEntityHealthInformation const*>(healthReport.EntityInformation.get());
#endif

                HMEvents::Trace->ProcessDeployedApplicationReportOperational(
                    Guid::NewGuid(),
                    entityInfo.ApplicationName, // wstring
                    entityInfo.EntityInstance, // FABRIC_INSTANCE_ID
                    entityInfo.NodeName, // wstring
                    healthReport.SourceId, // User Input
                    healthReport.Property, // User Input
                    ServiceModel::HealthState::ConvertToServiceModelHealthState(healthReport.State),
                    healthReport.TimeToLive,
                    healthReport.SequenceNumber,
                    healthReport.Description,
                    healthReport.RemoveWhenExpired,
                    healthReport.SourceUtcTimestamp);
            }
            else if (entityKind == FABRIC_HEALTH_REPORT_KIND_DEPLOYED_SERVICE_PACKAGE)
            {
#ifdef DBG
                auto & entityInfo = *dynamic_cast<ServiceModel::DeployedServicePackageEntityHealthInformation const*>(healthReport.EntityInformation.get());
#else
                auto & entityInfo = *static_cast<ServiceModel::DeployedServicePackageEntityHealthInformation const*>(healthReport.EntityInformation.get());
#endif

                HMEvents::Trace->ProcessDeployedServicePackageReportOperational(
                    Guid::NewGuid(),
                    entityInfo.ApplicationName, // wstring
                    entityInfo.ServiceManifestName, // wstring
                    entityInfo.EntityInstance, // FABRIC_INSTANCE_ID
                    entityInfo.ServicePackageActivationId, // wstring
                    entityInfo.NodeName, // wstring
                    healthReport.SourceId, // User Input
                    healthReport.Property, // User Input
                    ServiceModel::HealthState::ConvertToServiceModelHealthState(healthReport.State),
                    healthReport.TimeToLive,
                    healthReport.SequenceNumber,
                    healthReport.Description,
                    healthReport.RemoveWhenExpired,
                    healthReport.SourceUtcTimestamp);
            }
            else if (entityKind == FABRIC_HEALTH_REPORT_KIND_CLUSTER)
            {
                HMEvents::Trace->ProcessClusterReportOperational(
                    Guid::NewGuid(),
                    healthReport.SourceId, // User Input
                    healthReport.Property, // User Input
                    ServiceModel::HealthState::ConvertToServiceModelHealthState(healthReport.State),
                    healthReport.TimeToLive,
                    healthReport.SequenceNumber,
                    healthReport.Description,
                    healthReport.RemoveWhenExpired,
                    healthReport.SourceUtcTimestamp);
            }
            else if (entityKind == FABRIC_HEALTH_REPORT_KIND_STATEFUL_SERVICE_REPLICA)
            {
#ifdef DBG
                auto & entityInfo = *dynamic_cast<ServiceModel::StatefulReplicaEntityHealthInformation const*>(healthReport.EntityInformation.get());
#else
                auto & entityInfo = *static_cast<ServiceModel::StatefulReplicaEntityHealthInformation const*>(healthReport.EntityInformation.get());
#endif

                HMEvents::Trace->ProcessStatefulReplicaReportOperational(
                    Guid::NewGuid(),
                    entityInfo.PartitionId, // Common::Guid
                    entityInfo.ReplicaId, // FABRIC_REPLICA_ID
                    entityInfo.EntityInstance, // FABRIC_INSTANCE_ID
                    healthReport.SourceId, // User Input
                    healthReport.Property, // User Input
                    ServiceModel::HealthState::ConvertToServiceModelHealthState(healthReport.State),
                    healthReport.TimeToLive,
                    healthReport.SequenceNumber,
                    healthReport.Description,
                    healthReport.RemoveWhenExpired,
                    healthReport.SourceUtcTimestamp);
            }
            else if (entityKind == FABRIC_HEALTH_REPORT_KIND_STATELESS_SERVICE_INSTANCE)
            {
#ifdef DBG
                auto & entityInfo = *dynamic_cast<ServiceModel::StatelessInstanceEntityHealthInformation const*>(healthReport.EntityInformation.get());
#else
                auto & entityInfo = *static_cast<ServiceModel::StatelessInstanceEntityHealthInformation const*>(healthReport.EntityInformation.get());
#endif

                HMEvents::Trace->ProcessStatelessInstanceReportOperational(
                    Guid::NewGuid(),
                    entityInfo.PartitionId, // Common::Guid
                    entityInfo.ReplicaId, // FABRIC_REPLICA_ID
                    healthReport.SourceId, // User Input
                    healthReport.Property, // User Input
                    ServiceModel::HealthState::ConvertToServiceModelHealthState(healthReport.State),
                    healthReport.TimeToLive,
                    healthReport.SequenceNumber,
                    healthReport.Description,
                    healthReport.RemoveWhenExpired,
                    healthReport.SourceUtcTimestamp);
            }
        }
    }
}
