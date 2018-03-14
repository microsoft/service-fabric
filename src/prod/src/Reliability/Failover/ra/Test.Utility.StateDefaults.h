// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

/*

The following default objects are defined for ease of use in the tests
Service Types:
- SL1_SP1: A stateless service type in service package 1
- SL2_SP2: A stateless service type in service package 2
- SV1_SP1: A stateful volatile service type in sp1
- SV2_SP2: A stateful volatile service type in sp1
- SP1_SP1: A stateful persisted service type in sp1
- SP2_SP2: A stateful persisted service type in sp1
- RMC: Exclusive host


SP1 is hosted in HostId "HostId1", Runtime "Runtime1"
SP2 is hosted in HostId "HostId2", Runtime "Runtime2"

Each of these has a different service type name

The initial Service Package Instance Id for all of these is 1
They are in Application version 1
Application Name = App
Application Number = 1

The following FTs are defined (short names):
- SL1
- SL2
- SL3 [Same service type as SL1] (different fuid but same service description)
- SV1
- SV2
- SP1
- SP2
- FM
//these are for resource monitor test
- RMC1
- RMC2

- ASP1 (AdHoc Stateful Persisted)

- RM (Repair Manager)
The default Closed state is written as C None 000/000/000 0:0 -"

*/
namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace ReliabilityUnitTest
        {
            namespace StateManagement
            {
                class Default
                {

                public:
                    static BOOL CALLBACK Init(PINIT_ONCE, PVOID, PVOID*);

                    static Default & GetInstance();
                    static Default * singleton_;
                    static INIT_ONCE initOnce_;

                    static std::wstring const Delimiter;

                    static const uint64 ApplicationUpgradeInstanceId = 2;
                    static const uint64 ServicePackage1InstanceId = 1;
                    static const uint64 ServicePackage2InstanceId = 1;

                    __declspec(property(get = get_SL1_SP1_STContext)) ServiceTypeReadContext const & SL1_SP1_STContext;
                    ServiceTypeReadContext const & get_SL1_SP1_STContext() const { return sL1_SP1_STContext_; }

                    __declspec(property(get = get_SL2_SP2_STContext)) ServiceTypeReadContext const & SL2_SP2_STContext;
                    ServiceTypeReadContext const & get_SL2_SP2_STContext() const { return sL2_SP2_STContext_; }

                    __declspec(property(get = get_SV1_SP1_STContext)) ServiceTypeReadContext const & SV1_SP1_STContext;
                    ServiceTypeReadContext const & get_SV1_SP1_STContext() const { return sV1_SP1_STContext_; }

                    __declspec(property(get = get_SV2_SP2_STContext)) ServiceTypeReadContext const & SV2_SP2_STContext;
                    ServiceTypeReadContext const & get_SV2_SP2_STContext() const { return sV2_SP2_STContext_; }

                    __declspec(property(get = get_SP1_SP1_STContext)) ServiceTypeReadContext const & SP1_SP1_STContext;
                    ServiceTypeReadContext const & get_SP1_SP1_STContext() const { return sP1_SP1_STContext_; }

                    __declspec(property(get = get_SP2_SP2_STContext)) ServiceTypeReadContext const & SP2_SP2_STContext;
                    ServiceTypeReadContext const & get_SP2_SP2_STContext() const { return sP2_SP2_STContext_; }

                    __declspec(property(get = get_ASP1_STContext)) ServiceTypeReadContext ASP1_STContext;
                    ServiceTypeReadContext get_ASP1_STContext() const { return aSP1_STContext_; }

                    __declspec(property(get = get_RM_STContext)) ServiceTypeReadContext RM_STContext;
                    ServiceTypeReadContext get_RM_STContext() const { return rm_STContext_; }

                    __declspec(property(get = get_FM_STContext)) ServiceTypeReadContext const & FM_STContext;
                    ServiceTypeReadContext const & get_FM_STContext() const { return fm_STContext_; }

                    __declspec(property(get = get_RMC_STContext)) ServiceTypeReadContext const & RMC_STContext;
                    ServiceTypeReadContext const & get_RMC_STContext() const { return rmc_STContext_; }

                    __declspec(property(get = get_SL1_FTContext)) SingleFTReadContext const & SL1_FTContext;
                    SingleFTReadContext const & get_SL1_FTContext() const { return sL1_FTContext_; }

                    __declspec(property(get = get_SL2_FTContext)) SingleFTReadContext const & SL2_FTContext;
                    SingleFTReadContext const & get_SL2_FTContext() const { return sL2_FTContext_; }

                    __declspec(property(get = get_SL3_FTContext)) SingleFTReadContext const & SL3_FTContext;
                    SingleFTReadContext const & get_SL3_FTContext() const { return sL3_FTContext_; }

                    __declspec(property(get = get_SV1_FTContext)) SingleFTReadContext const & SV1_FTContext;
                    SingleFTReadContext const & get_SV1_FTContext() const { return sV1_FTContext_; }

                    __declspec(property(get = get_SV2_FTContext)) SingleFTReadContext const & SV2_FTContext;
                    SingleFTReadContext const & get_SV2_FTContext() const { return sV2_FTContext_; }

                    __declspec(property(get = get_SP1_FTContext)) SingleFTReadContext const & SP1_FTContext;
                    SingleFTReadContext const & get_SP1_FTContext() const { return sP1_FTContext_; }

                    __declspec(property(get = get_SP2_FTContext)) SingleFTReadContext const & SP2_FTContext;
                    SingleFTReadContext const & get_SP2_FTContext() const { return sP2_FTContext_; }

                    __declspec(property(get = get_FmFTContext)) SingleFTReadContext const & FmFTContext;
                    SingleFTReadContext const & get_FmFTContext() const { return fm_FTContext_; }

                    __declspec(property(get = get_ASP1_FTContext)) SingleFTReadContext const & ASP1_FTContext;
                    SingleFTReadContext const & get_ASP1_FTContext() const { return aSP1_FTContext_; }

                    __declspec(property(get = get_RM_FTContext)) SingleFTReadContext const & RM_FTContext;
                    SingleFTReadContext const & get_RM_FTContext() const { return rm_FTContext_; }

                    __declspec(property(get = get_RMC1_FTContext)) SingleFTReadContext const & RMC1_FTContext;
                    SingleFTReadContext const & get_RMC1_FTContext() const { return rmc1_FTContext_; }

                    __declspec(property(get = get_RMC2_FTContext)) SingleFTReadContext const & RMC2_FTContext;
                    SingleFTReadContext const & get_RMC2_FTContext() const { return rmc2_FTContext_; }

                    __declspec(property(get = get_NodeId)) Federation::NodeId const & NodeId;
                    Federation::NodeId const & get_NodeId() const { return nodeId_; }

                    __declspec(property(get = get_NodeInstance)) Federation::NodeInstance const & NodeInstance;
                    Federation::NodeInstance const & get_NodeInstance() const { return nodeInstance_; }

                    __declspec(property(get = get_NodeVersionInstance)) Common::FabricVersionInstance const & NodeVersionInstance;
                    Common::FabricVersionInstance const & get_NodeVersionInstance() const { return nodeVersionInstance_; }

                    __declspec(property(get = get_ApplicationName)) std::wstring const & ApplicationName;
                    std::wstring const & get_ApplicationName() const { return applicationName_; }

                    __declspec(property(get = get_ApplicationIdentifier)) ServiceModel::ApplicationIdentifier const & ApplicationIdentifier;
                    ServiceModel::ApplicationIdentifier const & get_ApplicationIdentifier() const { return appId_; }

                    __declspec(property(get = get_ApplicationVersion)) ServiceModel::ApplicationVersion const & ApplicationVersion;
                    ServiceModel::ApplicationVersion const & get_ApplicationVersion() const { return appVersion_; }

                    __declspec(property(get = get_UpgradedApplicationVersion)) ServiceModel::ApplicationVersion const & UpgradedApplicationVersion;
                    ServiceModel::ApplicationVersion const & get_UpgradedApplicationVersion() const { return upgradedAppVersion_; }

                    __declspec(property(get = get_ServicePackage1Id)) ServiceModel::ServicePackageIdentifier const & ServicePackage1Id;
                    ServiceModel::ServicePackageIdentifier const & get_ServicePackage1Id() const { return servicePackage1Id_; }

                    __declspec(property(get = get_ServicePackage2Id)) ServiceModel::ServicePackageIdentifier const & ServicePackage2Id;
                    ServiceModel::ServicePackageIdentifier const & get_ServicePackage2Id() const { return servicePackage2Id_; }

                    __declspec(property(get = get_NotificationOnlyUpgrade)) ServiceModel::ApplicationUpgradeSpecification const & NotificationOnlyUpgrade;
                    ServiceModel::ApplicationUpgradeSpecification const & get_NotificationOnlyUpgrade() const { return notificationOnlyUpgrade_; }

                    __declspec(property(get = get_RollingUpgrade)) ServiceModel::ApplicationUpgradeSpecification const & RollingUpgrade;
                    ServiceModel::ApplicationUpgradeSpecification const & get_RollingUpgrade() const { return rollingUpgrade_; }

                    __declspec(property(get = get_AllDefaultFTReadContexts)) std::vector<SingleFTReadContext const*> const & AllDefaultFTReadContexts;
                    std::vector<SingleFTReadContext const*> const & get_AllDefaultFTReadContexts() const { return ftReadContexts_; }

                    __declspec(property(get = get_AllDefaultSDReadContexts)) std::vector<ServiceTypeReadContext const*> const & AllDefaultSDReadContexts;
                    std::vector<ServiceTypeReadContext const *> const & get_AllDefaultSDReadContexts() const { return sdReadContexts_; }

                    SingleFTReadContext const & LookupFTContext(std::wstring const & shortName) const;

                private:
                    Default()
                    {
                    }

                    ServiceTypeReadContext sL1_SP1_STContext_;
                    ServiceTypeReadContext sL2_SP2_STContext_;

                    ServiceTypeReadContext sV1_SP1_STContext_;
                    ServiceTypeReadContext sV2_SP2_STContext_;

                    ServiceTypeReadContext sP1_SP1_STContext_;
                    ServiceTypeReadContext sP2_SP2_STContext_;

                    ServiceTypeReadContext aSP1_STContext_;

                    ServiceTypeReadContext fm_STContext_;

                    ServiceTypeReadContext rm_STContext_;

                    ServiceTypeReadContext rmc_STContext_;

                    SingleFTReadContext sL1_FTContext_;
                    SingleFTReadContext sL2_FTContext_;
                    SingleFTReadContext sL3_FTContext_;

                    SingleFTReadContext sV1_FTContext_;
                    SingleFTReadContext sV2_FTContext_;

                    SingleFTReadContext sP1_FTContext_;
                    SingleFTReadContext sP2_FTContext_;

                    SingleFTReadContext aSP1_FTContext_;

                    SingleFTReadContext fm_FTContext_;
                    SingleFTReadContext rm_FTContext_;

                    SingleFTReadContext rmc1_FTContext_;
                    SingleFTReadContext rmc2_FTContext_;

                    std::wstring applicationName_;
                    ServiceModel::ApplicationIdentifier appId_;
                    ServiceModel::ApplicationVersion appVersion_;
                    ServiceModel::ApplicationVersion upgradedAppVersion_;
                    ServiceModel::ServicePackageIdentifier servicePackage1Id_;
                    ServiceModel::ServicePackageIdentifier servicePackage2Id_;

                    ServiceModel::ApplicationUpgradeSpecification notificationOnlyUpgrade_;
                    ServiceModel::ApplicationUpgradeSpecification rollingUpgrade_;

                    std::vector<SingleFTReadContext const *> ftReadContexts_;
                    std::vector<ServiceTypeReadContext const *> sdReadContexts_;

                    Common::FabricVersionInstance nodeVersionInstance_;

                    Federation::NodeId nodeId_;
                    Federation::NodeInstance nodeInstance_;

                };
            }
        }

    }

}
