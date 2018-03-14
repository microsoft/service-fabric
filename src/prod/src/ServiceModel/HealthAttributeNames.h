// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class HealthAttributeNames
    {
    public:
        static Common::GlobalWString IpAddressOrFqdn;
        static Common::GlobalWString UpgradeDomain;
        static Common::GlobalWString FaultDomain;

        static Common::GlobalWString ApplicationName;
        static Common::GlobalWString ApplicationDefinitionKind;
        static Common::GlobalWString ApplicationTypeName;
        static Common::GlobalWString ServiceTypeName;
        static Common::GlobalWString ServiceManifestName;
        static Common::GlobalWString ServicePackageActivationId;
        static Common::GlobalWString ServiceName;

        static Common::GlobalWString NodeId;
        static Common::GlobalWString NodeName;
        static Common::GlobalWString NodeInstanceId;
        static Common::GlobalWString ProcessId;
        static Common::GlobalWString ProcessName;

        static Common::GlobalWString ApplicationHealthPolicy;
        static Common::GlobalWString ClusterHealthPolicy;
        static Common::GlobalWString ClusterUpgradeHealthPolicy;
        static Common::GlobalWString ApplicationHealthPolicies;
    };
}
