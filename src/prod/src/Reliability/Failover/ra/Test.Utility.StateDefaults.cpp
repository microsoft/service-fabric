// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


using namespace std;
using namespace Federation;
using namespace Common;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateManagement;

StateManagement::Default* StateManagement::Default::singleton_ = nullptr;
INIT_ONCE StateManagement::Default::initOnce_ = INIT_ONCE_STATIC_INIT;

wstring const Default::Delimiter(L"---------------------------------");

StateManagement::Default & StateManagement::Default::GetInstance()
{
    PVOID lpContext = NULL;
    BOOL bStatus = FALSE;
    bStatus = ::InitOnceExecuteOnce(
        &Default::initOnce_,
        Default::Init,
        NULL,
        &lpContext);

    ASSERT_IF(!bStatus, "Failed to init UpgradeStateMapCollection");
    return *singleton_;
}

namespace
{
    class ServiceStateType
    {
    public:
        enum Enum
        {
            Stateless,
            StatefulVolatile,
            StatefulPersisted
        };

        static bool HasPersistedState(Enum e)
        {
            return e == StatefulPersisted;
        }

        static bool IsStateful(Enum e)
        {
            return e != Stateless;
        }
    };

    vector<ServiceModel::ServicePackageUpgradeSpecification> CreatePackageUpgradeSpecification(
        std::wstring const & packageName,
        ServiceModel::RolloutVersion const & version)
    {
        vector<ServiceModel::ServicePackageUpgradeSpecification> v;
        v.push_back(ServiceModel::ServicePackageUpgradeSpecification(
            packageName, 
            version,
            vector<wstring>())); // code package names
        return v;
    }

    ServiceDescription CreateServiceDescription(
        wstring const & name, 
        ServiceStateType::Enum stateType,
        ServiceModel::ServiceTypeIdentifier const & typeID, 
        std::wstring const & appName,
        ServiceModel::ServicePackageVersionInstance const & versionInstance,
        ServiceModel::ServicePackageActivationMode::Enum activationMode)
    {
        auto rv = ServiceDescription(
            name,
            1,
            0,
            1,
            1,
            1,
            ServiceStateType::IsStateful(stateType),
            ServiceStateType::HasPersistedState(stateType),
            TimeSpan::Zero,
            TimeSpan::Zero,
            TimeSpan::Zero,
            typeID,
            vector<ServiceCorrelationDescription>(),
            L"",
            1,
            vector<ServiceLoadMetricDescription>(),
            0,
            vector<BYTE>(),
            appName,
            vector<ServiceModel::ServicePlacementPolicyDescription> (),
            activationMode,
            L"");
        rv.PackageVersionInstance = versionInstance;
        return rv;
    }

    void AddServiceDescription(ServiceTypeReadContext & context, wstring const & name,  ServiceStateType::Enum stateType, wstring const & appName, ServiceModel::ServicePackageActivationMode::Enum activationMode = ServiceModel::ServicePackageActivationMode::SharedProcess)
    {
        context.SD = CreateServiceDescription(name, stateType, context.ServiceTypeId, appName, context.ServicePackageVersionInstance, activationMode);
    }

}

BOOL Default::Init(PINIT_ONCE, PVOID, PVOID*)
{
    wstring const HostId1(L"HostId1");
    wstring const RuntimeId1(L"RuntimeId1");
    wstring const HostId2(L"HostId2");
    wstring const RuntimeId2(L"RuntimeId2");
    wstring const HostId3(L"HostId3");
    wstring const RuntimeId3(L"RuntimeId3");
    wstring const CodePackage1(L"CP1");
    wstring const CodePackage2(L"CP2");
    wstring const CodePackage3(L"CP3");

    singleton_ = new Default();

    singleton_->applicationName_ = L"AppName";
    singleton_->appId_ = ServiceModel::ApplicationIdentifier(L"AppType", 0);
    ServiceModel::RolloutVersion rollOutVersion(1, 0);
    singleton_->appVersion_ = ServiceModel::ApplicationVersion(rollOutVersion);

    singleton_->upgradedAppVersion_ = ServiceModel::ApplicationVersion(ServiceModel::RolloutVersion(2, 0));

    singleton_->servicePackage1Id_ = ServiceModel::ServicePackageIdentifier(singleton_->appId_, L"SP1");
    singleton_->servicePackage2Id_ = ServiceModel::ServicePackageIdentifier(singleton_->appId_, L"SP2");

    singleton_->notificationOnlyUpgrade_ = ServiceModel::ApplicationUpgradeSpecification(
        singleton_->appId_,
        singleton_->upgradedAppVersion_,
        singleton_->ApplicationName,
        Default::ApplicationUpgradeInstanceId,
        ServiceModel::UpgradeType::Rolling_NotificationOnly,
        false,
        false,
        CreatePackageUpgradeSpecification(singleton_->servicePackage1Id_.ServicePackageName, singleton_->ApplicationVersion.RolloutVersionValue),
        vector<ServiceModel::ServiceTypeRemovalSpecification>());

    singleton_->rollingUpgrade_ = ServiceModel::ApplicationUpgradeSpecification(
        singleton_->appId_,
        singleton_->upgradedAppVersion_,
        singleton_->ApplicationName,
        Default::ApplicationUpgradeInstanceId,
        ServiceModel::UpgradeType::Rolling,
        false,
        false,
        CreatePackageUpgradeSpecification(singleton_->servicePackage1Id_.ServicePackageName, singleton_->ApplicationVersion.RolloutVersionValue),
        vector<ServiceModel::ServiceTypeRemovalSpecification>());
    
    ServiceModel::ServicePackageVersionInstance ServicePackage1VersionInstance(
        ServiceModel::ServicePackageVersion(rollOutVersion, rollOutVersion),
        Default::ServicePackage1InstanceId);
    ServiceModel::ServicePackageVersionInstance ServicePackage2VersionInstance(
        ServiceModel::ServicePackageVersion(rollOutVersion, rollOutVersion),
        Default::ServicePackage2InstanceId);

    singleton_->sL1_SP1_STContext_.ShortName = L"SL1";
    singleton_->sL1_SP1_STContext_.HostId = HostId1;
    singleton_->sL1_SP1_STContext_.RuntimeId = RuntimeId1;
    singleton_->sL1_SP1_STContext_.CodePackageName = CodePackage1;
    singleton_->sL1_SP1_STContext_.ServiceTypeId = ServiceModel::ServiceTypeIdentifier(singleton_->servicePackage1Id_, L"SL1_ST");
    singleton_->sL1_SP1_STContext_.ServicePackageInstanceId = Default::ServicePackage1InstanceId;
    singleton_->sL1_SP1_STContext_.ServicePackageVersionInstance = ServicePackage1VersionInstance;
    AddServiceDescription(singleton_->sL1_SP1_STContext_, L"fabric:/sl1", ServiceStateType::Stateless, singleton_->applicationName_);
    
    singleton_->sL2_SP2_STContext_.ShortName = L"SL2";
    singleton_->sL2_SP2_STContext_.HostId = HostId2;
    singleton_->sL2_SP2_STContext_.RuntimeId = RuntimeId2;
    singleton_->sL2_SP2_STContext_.CodePackageName = CodePackage2;
    singleton_->sL2_SP2_STContext_.ServiceTypeId = ServiceModel::ServiceTypeIdentifier(singleton_->servicePackage2Id_, L"SL2_ST");
    singleton_->sL2_SP2_STContext_.ServicePackageInstanceId = Default::ServicePackage2InstanceId;
    singleton_->sL2_SP2_STContext_.ServicePackageVersionInstance = ServicePackage2VersionInstance;
    AddServiceDescription(singleton_->sL2_SP2_STContext_, L"fabric:/sl2", ServiceStateType::Stateless, singleton_->applicationName_);

    singleton_->sV1_SP1_STContext_.ShortName = L"SV1";
    singleton_->sV1_SP1_STContext_.HostId = HostId1;
    singleton_->sV1_SP1_STContext_.RuntimeId = RuntimeId1;
    singleton_->sV1_SP1_STContext_.CodePackageName = CodePackage1;
    singleton_->sV1_SP1_STContext_.ServiceTypeId = ServiceModel::ServiceTypeIdentifier(singleton_->servicePackage1Id_, L"SV1_ST");
    singleton_->sV1_SP1_STContext_.ServicePackageInstanceId = ServicePackage1InstanceId;
    singleton_->sV1_SP1_STContext_.ServicePackageVersionInstance = ServicePackage1VersionInstance;
    AddServiceDescription(singleton_->sV1_SP1_STContext_, L"fabric:/sv1", ServiceStateType::StatefulVolatile, singleton_->applicationName_);

    singleton_->sV2_SP2_STContext_.ShortName = L"SV2";
    singleton_->sV2_SP2_STContext_.HostId = HostId2;
    singleton_->sV2_SP2_STContext_.RuntimeId = RuntimeId2;
    singleton_->sV2_SP2_STContext_.CodePackageName = CodePackage2;
    singleton_->sV2_SP2_STContext_.ServiceTypeId = ServiceModel::ServiceTypeIdentifier(singleton_->servicePackage2Id_, L"SV2_ST");
    singleton_->sV2_SP2_STContext_.ServicePackageInstanceId = ServicePackage2InstanceId;
    singleton_->sV2_SP2_STContext_.ServicePackageVersionInstance = ServicePackage2VersionInstance;
    AddServiceDescription(singleton_->sV2_SP2_STContext_, L"fabric:/sv2", ServiceStateType::StatefulVolatile, singleton_->applicationName_);
    
    singleton_->sP1_SP1_STContext_.ShortName = L"SP1";
    singleton_->sP1_SP1_STContext_.HostId = HostId1;
    singleton_->sP1_SP1_STContext_.RuntimeId = RuntimeId1;
    singleton_->sP1_SP1_STContext_.CodePackageName = CodePackage1;
    singleton_->sP1_SP1_STContext_.ServiceTypeId = ServiceModel::ServiceTypeIdentifier(singleton_->servicePackage1Id_, L"SP1_ST");
    singleton_->sP1_SP1_STContext_.ServicePackageInstanceId = ServicePackage1InstanceId;
    singleton_->sP1_SP1_STContext_.ServicePackageVersionInstance = ServicePackage1VersionInstance;
    AddServiceDescription(singleton_->sP1_SP1_STContext_, L"fabric:/sp1", ServiceStateType::StatefulPersisted, singleton_->applicationName_);

    singleton_->sP2_SP2_STContext_.ShortName = L"SP2";
    singleton_->sP2_SP2_STContext_.HostId = HostId2;
    singleton_->sP2_SP2_STContext_.RuntimeId = RuntimeId2;
    singleton_->sP2_SP2_STContext_.CodePackageName = CodePackage2;
    singleton_->sP2_SP2_STContext_.ServiceTypeId = ServiceModel::ServiceTypeIdentifier(singleton_->servicePackage2Id_, L"SP2_ST");
    singleton_->sP2_SP2_STContext_.ServicePackageInstanceId = ServicePackage2InstanceId;
    singleton_->sP2_SP2_STContext_.ServicePackageVersionInstance = ServicePackage2VersionInstance;
    AddServiceDescription(singleton_->sP2_SP2_STContext_, L"fabric:/sp2", ServiceStateType::StatefulPersisted, singleton_->applicationName_);

    singleton_->rm_STContext_.HostId = L"RMHostId";
    singleton_->rm_STContext_.RuntimeId = L"RMRuntimeId";
    singleton_->rm_STContext_.ServiceTypeId = *ServiceModel::ServiceTypeIdentifier::RepairManagerServiceTypeId;
    singleton_->rm_STContext_.SD = ServiceDescription(
        ServiceModel::SystemServiceApplicationNameHelper::InternalRepairManagerServiceName,
        1,      /* Instance */
        0,      /* UpdateVersion */
        1,      /* PartitionCount */
        7,
        3,
        true,   /* IsStateful */
        true,   /* HasPersistedState */
        TimeSpan::MinValue,
        TimeSpan::MaxValue,
        TimeSpan::MinValue,
        *ServiceModel::ServiceTypeIdentifier::RepairManagerServiceTypeId,
        vector<ServiceCorrelationDescription>(),
        L"",
        0,      /* ScaleoutCount */
        vector<ServiceLoadMetricDescription>(),
        0,      /* DefaultMoveCost */
        vector<byte>(),
        ServiceModel::SystemServiceApplicationNameHelper::SystemServiceApplicationName);

    singleton_->rmc_STContext_.ShortName = L"RMC";
    singleton_->rmc_STContext_.HostId = HostId3;
    singleton_->rmc_STContext_.RuntimeId = RuntimeId3;
    singleton_->rmc_STContext_.CodePackageName = CodePackage3;
    singleton_->rmc_STContext_.ServiceTypeId = ServiceModel::ServiceTypeIdentifier(singleton_->servicePackage2Id_, L"SP2_RMC");
    singleton_->rmc_STContext_.ServicePackageInstanceId = ServicePackage2InstanceId;
    singleton_->rmc_STContext_.ServicePackageVersionInstance = ServicePackage2VersionInstance;
    AddServiceDescription(singleton_->rmc_STContext_, L"fabric:/rmc1", ServiceStateType::Stateless, singleton_->applicationName_, ServiceModel::ServicePackageActivationMode::ExclusiveProcess);

    singleton_->fm_STContext_.ShortName = L"FM";
    singleton_->fm_STContext_.HostId = L"FMServiceHostId";
    singleton_->fm_STContext_.RuntimeId = L"FMServiceRuntimeId";
    singleton_->fm_STContext_.ServiceTypeId = *ServiceModel::ServiceTypeIdentifier::FailoverManagerServiceTypeId;
    singleton_->fm_STContext_.IsHostedInFabricExe = true;
    singleton_->fm_STContext_.SD = ServiceDescription(
        ServiceModel::SystemServiceApplicationNameHelper::InternalFMServiceName,
        1,      /* Instance */
        0,      /* UpdateVersion */
        1,      /* PartitionCount */
        7,
        3,
        true,   /* IsStateful */
        true,   /* HasPersistedState */
        TimeSpan::MinValue,
        TimeSpan::MaxValue,
        TimeSpan::MinValue,
        *ServiceModel::ServiceTypeIdentifier::FailoverManagerServiceTypeId,
        vector<ServiceCorrelationDescription>(),
        L"",
        0,      /* ScaleoutCount */
        vector<ServiceLoadMetricDescription>(),
        0,      /* DefaultMoveCost */
        vector<byte>());

    singleton_->aSP1_STContext_.ShortName = L"ASP1";
    singleton_->aSP1_STContext_.HostId = HostId1;
    singleton_->aSP1_STContext_.RuntimeId = RuntimeId1;
    singleton_->aSP1_STContext_.CodePackageName = L"";
    
    ServiceModel::ApplicationIdentifier appId(L"", 0);
    ServiceModel::ServicePackageIdentifier aspId(appId, L"");
    singleton_->aSP1_STContext_.ServiceTypeId = ServiceModel::ServiceTypeIdentifier(aspId, L"ASP1_ST");
    singleton_->aSP1_STContext_.ServicePackageInstanceId = 1;
    singleton_->aSP1_STContext_.ServicePackageVersionInstance = ServicePackage1VersionInstance;
    singleton_->aSP1_STContext_.SD = ServiceDescription(
        L"fabric:/asp1",
        1,
        0,
        1,
        1,
        1,
        false,
        false,
        TimeSpan::Zero,
        TimeSpan::Zero,
        TimeSpan::Zero,
        ServiceModel::ServiceTypeIdentifier(aspId, L"ASP1_ST"),
        vector<ServiceCorrelationDescription>(),
        L"",
        1,
        vector<ServiceLoadMetricDescription>(),
        0,
        vector<BYTE>(),
        L"");

    singleton_->sL1_FTContext_.ShortName = L"SL1";
    singleton_->sL1_FTContext_.FUID = FailoverUnitId(Guid(L"AAAAAAAA-AAAA-AAAA-AAAA-AAAAAAAAAAAA"));
    singleton_->sL1_FTContext_.MinReplicaSetSize = 1;
    singleton_->sL1_FTContext_.TargetReplicaSetSize = 1;
    singleton_->sL1_FTContext_.STInfo = singleton_->sL1_SP1_STContext_;

    singleton_->sL2_FTContext_.ShortName = L"SL2";
    singleton_->sL2_FTContext_.FUID = FailoverUnitId(Guid(L"BBBBBBBB-BBBB-BBBB-BBBB-BBBBBBBBBBBB"));
    singleton_->sL2_FTContext_.MinReplicaSetSize = 1;
    singleton_->sL2_FTContext_.TargetReplicaSetSize = 1;
    singleton_->sL2_FTContext_.STInfo = singleton_->sL2_SP2_STContext_;

    singleton_->sL3_FTContext_.ShortName = L"SL3";
    singleton_->sL3_FTContext_.FUID = FailoverUnitId(Guid(L"AAAAAAAA-AAAA-AAAA-AAAA-BBBBBBBBBBBB"));
    singleton_->sL3_FTContext_.MinReplicaSetSize = 1;
    singleton_->sL3_FTContext_.TargetReplicaSetSize = 1;
    singleton_->sL3_FTContext_.STInfo = singleton_->sL1_SP1_STContext_;

    singleton_->sV1_FTContext_.ShortName = L"SV1";
    singleton_->sV1_FTContext_.FUID = FailoverUnitId(Guid(L"CCCCCCCC-CCCC-CCCC-CCCC-CCCCCCCCCCCC"));
    singleton_->sV1_FTContext_.MinReplicaSetSize = 1;
    singleton_->sV1_FTContext_.TargetReplicaSetSize = 1;
    singleton_->sV1_FTContext_.STInfo = singleton_->sV1_SP1_STContext_;

    singleton_->sV2_FTContext_.ShortName = L"SV2";
    singleton_->sV2_FTContext_.FUID = FailoverUnitId(Guid(L"DDDDDDDD-DDDD-DDDD-DDDD-DDDDDDDDDDDD"));
    singleton_->sV2_FTContext_.MinReplicaSetSize = 1;
    singleton_->sV2_FTContext_.TargetReplicaSetSize = 1;
    singleton_->sV2_FTContext_.STInfo = singleton_->sV2_SP2_STContext_;

    singleton_->sP1_FTContext_.ShortName = L"SP1";
    singleton_->sP1_FTContext_.FUID = FailoverUnitId(Guid(L"EEEEEEEE-EEEE-EEEE-EEEE-EEEEEEEEEEEE"));
    singleton_->sP1_FTContext_.MinReplicaSetSize = 1;
    singleton_->sP1_FTContext_.TargetReplicaSetSize = 1;
    singleton_->sP1_FTContext_.STInfo = singleton_->sP1_SP1_STContext_;

    singleton_->sP2_FTContext_.ShortName = L"SP2";
    singleton_->sP2_FTContext_.FUID = FailoverUnitId(Guid(L"FFFFFFFF-FFFF-FFFF-FFFF-FFFFFFFFFFFF"));
    singleton_->sP2_FTContext_.MinReplicaSetSize = 1;
    singleton_->sP2_FTContext_.TargetReplicaSetSize = 1;
    singleton_->sP2_FTContext_.STInfo = singleton_->sP2_SP2_STContext_;

    singleton_->aSP1_FTContext_.ShortName = L"ASP1";
    singleton_->aSP1_FTContext_.FUID = FailoverUnitId(Guid(L"AAAAAAAA-AAAA-0000-0000-AAAAAAAAAAAA"));
    singleton_->aSP1_FTContext_.MinReplicaSetSize = 1;
    singleton_->aSP1_FTContext_.TargetReplicaSetSize = 1;
    singleton_->aSP1_FTContext_.STInfo = singleton_->aSP1_STContext_;
    
    singleton_->fm_FTContext_.ShortName = L"FM";
    singleton_->fm_FTContext_.FUID = FailoverUnitId(Reliability::Constants::FMServiceGuid);
    singleton_->fm_FTContext_.CUID = ConsistencyUnitId(Reliability::Constants::FMServiceGuid);
    singleton_->fm_FTContext_.MinReplicaSetSize = 3;
    singleton_->fm_FTContext_.TargetReplicaSetSize = 7;
    singleton_->fm_FTContext_.STInfo = singleton_->fm_STContext_;

    Guid rmId = ConsistencyUnitId::RMIdRange->CreateReservedGuid(0);
    singleton_->rm_FTContext_.ShortName = L"RM";
    singleton_->rm_FTContext_.FUID = FailoverUnitId(rmId);
    singleton_->rm_FTContext_.CUID = ConsistencyUnitId(rmId);
    singleton_->rm_FTContext_.MinReplicaSetSize = 3;
    singleton_->rm_FTContext_.TargetReplicaSetSize = 7;
    singleton_->rm_FTContext_.STInfo = singleton_->rm_STContext_;

    singleton_->rmc1_FTContext_.ShortName = L"RMC1";
    singleton_->rmc1_FTContext_.FUID = FailoverUnitId(Guid(L"EEEEEEEE-AAAA-EEEE-EEEE-EEEEEEEEEEEE"));
    singleton_->rmc1_FTContext_.MinReplicaSetSize = 1;
    singleton_->rmc1_FTContext_.TargetReplicaSetSize = 2;
    singleton_->rmc1_FTContext_.STInfo = singleton_->rmc_STContext_;

    singleton_->rmc2_FTContext_.ShortName = L"RMC2";
    singleton_->rmc2_FTContext_.FUID = FailoverUnitId(Guid(L"FFFFFFFF-BBBB-FFFF-FFFF-FFFFFFFFFFFF"));
    singleton_->rmc2_FTContext_.MinReplicaSetSize = 2;
    singleton_->rmc2_FTContext_.TargetReplicaSetSize = 3;
    singleton_->rmc2_FTContext_.STInfo = singleton_->rmc_STContext_;

    singleton_->nodeId_ = Federation::NodeId(LargeInteger(0, 1));
    singleton_->nodeInstance_= Federation::NodeInstance(Federation::NodeId(LargeInteger(0, 1)), 1);

    FabricConfigVersion configVersion;
    auto error = configVersion.FromString(L"cfg1");
    if (!error.IsSuccess()) { Assert::CodingError("Invalid FabricConfigVersion"); }

    singleton_->nodeVersionInstance_ = FabricVersionInstance(FabricVersion(
        FabricCodeVersion(1, 1, 1, 0), 
        configVersion),
        1);

    singleton_->ftReadContexts_.push_back(&singleton_->sL1_FTContext_);
    singleton_->ftReadContexts_.push_back(&singleton_->sL2_FTContext_);
    singleton_->ftReadContexts_.push_back(&singleton_->sL3_FTContext_);
    singleton_->ftReadContexts_.push_back(&singleton_->sV1_FTContext_);
    singleton_->ftReadContexts_.push_back(&singleton_->sV2_FTContext_);
    singleton_->ftReadContexts_.push_back(&singleton_->sP1_FTContext_);
    singleton_->ftReadContexts_.push_back(&singleton_->sP2_FTContext_);
    singleton_->ftReadContexts_.push_back(&singleton_->aSP1_FTContext_);
    singleton_->ftReadContexts_.push_back(&singleton_->fm_FTContext_);
    singleton_->ftReadContexts_.push_back(&singleton_->rm_FTContext_);
    singleton_->ftReadContexts_.push_back(&singleton_->rmc1_FTContext_);
    singleton_->ftReadContexts_.push_back(&singleton_->rmc2_FTContext_);

    singleton_->sdReadContexts_.push_back(&singleton_->sL1_SP1_STContext_);
    singleton_->sdReadContexts_.push_back(&singleton_->sL2_SP2_STContext_);
    singleton_->sdReadContexts_.push_back(&singleton_->sV1_SP1_STContext_);
    singleton_->sdReadContexts_.push_back(&singleton_->sV2_SP2_STContext_);
    singleton_->sdReadContexts_.push_back(&singleton_->sP1_SP1_STContext_);
    singleton_->sdReadContexts_.push_back(&singleton_->sP2_SP2_STContext_);
    singleton_->sdReadContexts_.push_back(&singleton_->aSP1_STContext_);
    singleton_->sdReadContexts_.push_back(&singleton_->fm_STContext_);
    singleton_->sdReadContexts_.push_back(&singleton_->rm_STContext_);
    singleton_->sdReadContexts_.push_back(&singleton_->rmc_STContext_);

    return TRUE;
}

SingleFTReadContext const & Default::LookupFTContext(wstring const & shortName) const
{
    for(size_t i = 0; i < ftReadContexts_.size(); i++)
    {
        if (ftReadContexts_[i]->ShortName == shortName)
        {
            return *ftReadContexts_[i];
        }
    }

    Assert::CodingError("ShortName: {0} not found", shortName);
}

