// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
using namespace ServiceModel;

namespace Management
{
    namespace HealthManager
    {
        namespace HealthEntityKind 
        {
            void WriteToTextWriter(__in Common::TextWriter & w, Enum e)
            {
                switch(e)
                {
                case Unknown: w << "Unknown"; return;
                case Node: w << "Node"; return;
                case Replica: w << "Replica"; return;
                case Partition: w << "Partition"; return;
                case Service: w << "Service"; return;
                case Application: w << "Application"; return;
                case DeployedApplication: w << "DeployedApplication"; return;
                case DeployedServicePackage: w << "DeployedServicePackage"; return;
                case Cluster: w << "Cluster"; return;
                default: w << "HealthEntityKind(" << static_cast<uint>(e) << ')'; return;
                }
            }

            std::wstring const & GetEntityTypeString(HealthEntityKind::Enum entityKind)
            {
                switch(entityKind)
                {
                case Node: return Constants::StoreType_NodeTypeString;
                case Replica: return Constants::StoreType_ReplicaTypeString;
                case Partition: return Constants::StoreType_PartitionTypeString;
                case Service: return Constants::StoreType_ServiceTypeString;
                case Application: return Constants::StoreType_ApplicationTypeString;
                case DeployedApplication: return Constants::StoreType_DeployedApplicationTypeString;
                case DeployedServicePackage: return Constants::StoreType_DeployedServicePackageTypeString;
                case Cluster: return Constants::StoreType_ClusterTypeString;
                default: Common::Assert::CodingError("unsupported entity kind {0}", entityKind);
                }
            }

            bool HealthEntityKind::CanAccept(FABRIC_HEALTH_REPORT_KIND publicEntityKind)
            {
                switch (publicEntityKind)
                {
                case FABRIC_HEALTH_REPORT_KIND_NODE:
                case FABRIC_HEALTH_REPORT_KIND_STATEFUL_SERVICE_REPLICA:
                case FABRIC_HEALTH_REPORT_KIND_STATELESS_SERVICE_INSTANCE:
                case FABRIC_HEALTH_REPORT_KIND_PARTITION:
                case FABRIC_HEALTH_REPORT_KIND_SERVICE:
                case FABRIC_HEALTH_REPORT_KIND_APPLICATION:
                case FABRIC_HEALTH_REPORT_KIND_DEPLOYED_APPLICATION:
                case FABRIC_HEALTH_REPORT_KIND_DEPLOYED_SERVICE_PACKAGE:
                case FABRIC_HEALTH_REPORT_KIND_CLUSTER:
                    return true;
                default:
                    return false;
                }
            }

            HealthEntityKind::Enum HealthEntityKind::FromHealthInformationKind(FABRIC_HEALTH_REPORT_KIND publicEntityKind)
            {
                switch (publicEntityKind)
                {
                case FABRIC_HEALTH_REPORT_KIND_NODE:
                    return HealthEntityKind::Node;
                case FABRIC_HEALTH_REPORT_KIND_STATEFUL_SERVICE_REPLICA:
                case FABRIC_HEALTH_REPORT_KIND_STATELESS_SERVICE_INSTANCE:
                    return HealthEntityKind::Replica;
                case FABRIC_HEALTH_REPORT_KIND_PARTITION:
                    return HealthEntityKind::Partition;
                case FABRIC_HEALTH_REPORT_KIND_SERVICE:
                    return HealthEntityKind::Service;
                case FABRIC_HEALTH_REPORT_KIND_APPLICATION:
                    return HealthEntityKind::Application;
                case FABRIC_HEALTH_REPORT_KIND_DEPLOYED_APPLICATION:
                    return HealthEntityKind::DeployedApplication;
                case FABRIC_HEALTH_REPORT_KIND_DEPLOYED_SERVICE_PACKAGE:
                    return HealthEntityKind::DeployedServicePackage;
                case FABRIC_HEALTH_REPORT_KIND_CLUSTER:
                    return HealthEntityKind::Cluster;
                default:
                    Common::Assert::CodingError("{0}: Unsupported entity kind", static_cast<int>(publicEntityKind));
                }
            }

            FABRIC_HEALTH_REPORT_KIND HealthEntityKind::GetHealthInformationKind(HealthEntityKind::Enum e, bool hasInstance)
            {
                switch (e)
                {
                case HealthEntityKind::Node:
                    return FABRIC_HEALTH_REPORT_KIND_NODE;
                case HealthEntityKind::Replica:
                    return hasInstance ? FABRIC_HEALTH_REPORT_KIND_STATEFUL_SERVICE_REPLICA : FABRIC_HEALTH_REPORT_KIND_STATELESS_SERVICE_INSTANCE;
                case HealthEntityKind::Partition:
                    return FABRIC_HEALTH_REPORT_KIND_PARTITION;
                case HealthEntityKind::Service:
                    return FABRIC_HEALTH_REPORT_KIND_SERVICE;
                case HealthEntityKind::Application:
                    return FABRIC_HEALTH_REPORT_KIND_APPLICATION;
                case HealthEntityKind::DeployedApplication:
                    return FABRIC_HEALTH_REPORT_KIND_DEPLOYED_APPLICATION;
                case HealthEntityKind::DeployedServicePackage:
                    return FABRIC_HEALTH_REPORT_KIND_DEPLOYED_SERVICE_PACKAGE;
                case HealthEntityKind::Cluster:
                    return FABRIC_HEALTH_REPORT_KIND_CLUSTER;
                default:
                    Common::Assert::CodingError("{0}: Unsupported entity kind", static_cast<int>(e));
                }
            }

            BEGIN_ENUM_STRUCTURED_TRACE( HealthEntityKind )

            ADD_CASTED_ENUM_MAP_VALUE_RANGE( HealthEntityKind, Unknown, LAST_STATE)

            END_ENUM_STRUCTURED_TRACE( HealthEntityKind )
        }
    }
}
