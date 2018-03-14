// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        // Internal health entity kind enumeration used to map from HEALTH_REPORT_KIND
        // to different internal report types for different entities.
        namespace HealthEntityKind 
        {
            enum Enum 
            { 
                Unknown = 0, 
                Node = 1,
                Partition = 2,
                Service = 3,
                Application = 4,
                Replica = 5,
                DeployedApplication = 6,
                DeployedServicePackage = 7,
                Cluster = 8,
                LAST_STATE = Cluster,
            };

            void WriteToTextWriter(__in Common::TextWriter & w, Enum e);

            bool CanAccept(FABRIC_HEALTH_REPORT_KIND publicEntityKind);

            HealthEntityKind::Enum FromHealthInformationKind(FABRIC_HEALTH_REPORT_KIND publicEntityKind);

            FABRIC_HEALTH_REPORT_KIND GetHealthInformationKind(HealthEntityKind::Enum e, bool hasInstance);

            std::wstring const & GetEntityTypeString(HealthEntityKind::Enum entityKind);

            DECLARE_ENUM_STRUCTURED_TRACE( HealthEntityKind )
        }
    }
}
