// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Management
{
    namespace HealthManager
    {
        namespace EntityKind 
        {
            void WriteToTextWriter(__in Common::TextWriter & w, Enum e)
            {
                switch(e)
                {
                case Invalid: w << "Invalid"; return;
                case Node: w << "Node"; return;
                case Replica: w << "Replica"; return;
                case Partition: w << "Partition"; return;
                case Service: w << "Service"; return;
                case Application: w << "Application"; return;
                case DeployedApplication: w << "DeployedApplication"; return;
                case DeployedServicePackage: w << "DeployedServicePackage"; return;
                case Cluster: w << "Cluster"; return;
                default: w << "EntityKind(" << static_cast<uint>(e) << ')'; return;
                }
            }

            Enum FromPublicApi(FABRIC_HEALTH_ENTITY_KIND publicType)
            {
                switch (publicType)
                {
                case FABRIC_HEALTH_ENTITY_KIND_CLUSTER:
                    return Cluster;

                case FABRIC_HEALTH_ENTITY_KIND_NODE:
                    return Node;

                case FABRIC_HEALTH_ENTITY_KIND_REPLICA:
                    return Replica;

                case FABRIC_HEALTH_ENTITY_KIND_PARTITION:
                    return Partition;

                case FABRIC_HEALTH_ENTITY_KIND_SERVICE:
                    return Service;

                case FABRIC_HEALTH_ENTITY_KIND_APPLICATION:
                    return Application;

                case FABRIC_HEALTH_ENTITY_KIND_DEPLOYED_APPLICATION:
                    return DeployedApplication;

                case FABRIC_HEALTH_ENTITY_KIND_DEPLOYED_SERVICE_PACKAGE:
                    return DeployedServicePackage;

                default:
                    return Invalid;
                }
            }

            FABRIC_HEALTH_ENTITY_KIND ToPublicApi(Enum const nativeType)
            {
                switch (nativeType)
                {
                case Cluster:
                    return FABRIC_HEALTH_ENTITY_KIND_CLUSTER;

                case Node:
                    return FABRIC_HEALTH_ENTITY_KIND_NODE;

                case Replica:
                    return FABRIC_HEALTH_ENTITY_KIND_REPLICA;

                case Partition:
                    return FABRIC_HEALTH_ENTITY_KIND_PARTITION;

                case Service:
                    return FABRIC_HEALTH_ENTITY_KIND_SERVICE;

                case Application:
                    return FABRIC_HEALTH_ENTITY_KIND_APPLICATION;

                case DeployedApplication:
                    return FABRIC_HEALTH_ENTITY_KIND_DEPLOYED_APPLICATION;

                case DeployedServicePackage: 
                    return FABRIC_HEALTH_ENTITY_KIND_DEPLOYED_SERVICE_PACKAGE;

                default:
                    return FABRIC_HEALTH_ENTITY_KIND_INVALID;
                }
            }

            BEGIN_ENUM_STRUCTURED_TRACE( EntityKind )

            ADD_CASTED_ENUM_MAP_VALUE_RANGE( EntityKind, Invalid, LAST_STATE)

            END_ENUM_STRUCTURED_TRACE( EntityKind )
        }
    }
}
