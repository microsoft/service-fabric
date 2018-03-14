// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        // Entity kind exposed through Public API when getting health statistics.
        namespace EntityKind 
        {
            enum Enum 
            { 
                Invalid = 0, 
                Node = 0x0001,
                Partition = 0x0002,
                Service = 0x0003,
                Application = 0x0004,
                Replica = 0x0005,
                DeployedApplication = 0x0006,
                DeployedServicePackage = 0x0007,
                Cluster = 0x0008,
                LAST_STATE = Cluster,
            };

            void WriteToTextWriter(__in Common::TextWriter & w, Enum e);

            Enum FromPublicApi(FABRIC_HEALTH_ENTITY_KIND entityKind);
            FABRIC_HEALTH_ENTITY_KIND ToPublicApi(Enum const);

            DECLARE_ENUM_STRUCTURED_TRACE( EntityKind )

            BEGIN_DECLARE_ENUM_JSON_SERIALIZER(Enum)
                ADD_ENUM_VALUE(Invalid)
                ADD_ENUM_VALUE(Node)
                ADD_ENUM_VALUE(Partition)
                ADD_ENUM_VALUE(Service)
                ADD_ENUM_VALUE(Application)
                ADD_ENUM_VALUE(Replica)
                ADD_ENUM_VALUE(DeployedApplication)
                ADD_ENUM_VALUE(DeployedServicePackage)
                ADD_ENUM_VALUE(Cluster)
            END_DECLARE_ENUM_SERIALIZER()
        }
    }
}
