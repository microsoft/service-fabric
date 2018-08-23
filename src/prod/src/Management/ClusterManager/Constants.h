// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class Constants
        {
        public:
            // -------------------------------
            // General
            // -------------------------------

            static Common::GlobalWString TokenDelimeter;
            static Common::GlobalWString QualifiedTypeDelimeter;

            static uint64 const InitialApplicationVersionInstance;

            // -------------------------------
            // Service Description
            // -------------------------------
            
            static Common::GlobalWString ClusterManagerServicePrimaryCountName;
            static Common::GlobalWString ClusterManagerServiceReplicaCountName;
            
            // -------------------------------
            // Store Data Keys
            // -------------------------------

            static Common::GlobalWString DatabaseDirectory;
            static Common::GlobalWString DatabaseFilename;
            static Common::GlobalWString SharedLogFilename;

            static Common::GlobalWString StoreType_GlobalApplicationInstanceCounter;
            static Common::GlobalWString StoreKey_GlobalApplicationInstanceCounter;

            static Common::GlobalWString StoreType_VerifiedFabricUpgradeDomains;
            static Common::GlobalWString StoreKey_VerifiedFabricUpgradeDomains;

            static Common::GlobalWString StoreType_FabricProvisionContext;
            static Common::GlobalWString StoreKey_FabricProvisionContext;

            static Common::GlobalWString StoreType_FabricUpgradeContext;
            static Common::GlobalWString StoreKey_FabricUpgradeContext;

            static Common::GlobalWString StoreType_FabricUpgradeStateSnapshot;
            static Common::GlobalWString StoreKey_FabricUpgradeStateSnapshot;

            static Common::GlobalWString StoreType_ApplicationTypeContext;
            static Common::GlobalWString StoreType_ApplicationContext;
            static Common::GlobalWString StoreType_ApplicationUpgradeContext;
            static Common::GlobalWString StoreType_GoalStateApplicationUpgradeContext;
            static Common::GlobalWString StoreType_ComposeDeploymentContext;
            static Common::GlobalWString StoreType_SingleInstanceDeploymentContext;
            static Common::GlobalWString StoreType_ServiceContext;
            static Common::GlobalWString StoreType_InfrastructureTaskContext;
            static Common::GlobalWString StoreType_ApplicationUpdateContext;
            static Common::GlobalWString StoreType_ComposeDeploymentUpgradeContext;
            static Common::GlobalWString StoreType_SingleInstanceDeploymentUpgradeContext;
            
            static Common::GlobalWString StoreType_ApplicationIdMap;
            static Common::GlobalWString StoreType_ServicePackage;
            static Common::GlobalWString StoreType_ServiceTemplate;
            static Common::GlobalWString StoreType_VerifiedUpgradeDomains;

            static Common::GlobalWString StoreType_ApplicationManifest;
            static Common::GlobalWString StoreType_ServiceManifest;
            static Common::GlobalWString StoreType_ComposeDeploymentInstance;
            static Common::GlobalWString StoreType_ContainerGroupDeploymentInstance;
            static Common::GlobalWString StoreType_SingleInstanceApplicationInstance;

            static Common::GlobalWString StoreType_ComposeDeploymentInstanceCounter;
            static Common::GlobalWString StoreKey_ComposeDeploymentInstanceCounter;

            static Common::GlobalWString StoreType_Volume;

            // -------------------------------
            // Naming Data Keys
            // -------------------------------
            
            static Common::GlobalWString ApplicationPropertyName;
            static Common::GlobalWString ComposeDeploymentTypePrefix;
            static Common::GlobalWString SingleInstanceDeploymentTypePrefix;

            // -------------------------------
            // Test parameters
            // -------------------------------
            // When enabled, CM reports the application type as part of application attributes of the health entity. Used by tests to simulate upgrades from and to versions when the application type reporting is disabled.
            static bool ReportHealthEntityApplicationType;
            static void Test_SetReportHealthEntityApplicationType(bool includeAppType) { ReportHealthEntityApplicationType = includeAppType; }

        };
    }
}
