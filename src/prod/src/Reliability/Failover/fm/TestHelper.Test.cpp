// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

using namespace Common;
using namespace Client;
using namespace FailoverManagerUnitTest;
using namespace Federation;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;
using namespace std;


template void TestHelper::AssertEqual(int const& expected, int const& actual, wstring const& testName);
template void TestHelper::AssertEqual(wstring const& expected, wstring const& actual, wstring const& testName);
template void TestHelper::AssertEqual(vector<wstring> const& expected, vector<wstring> const& actual, wstring const& testName);

template <class T>
void TestHelper::AssertEqual(T const& expected, T const& actual, wstring const& testName)
{
    if (expected != actual)
    {
        BOOST_FAIL(formatString("\n{0}\nExpected:\n{1}\nActual:\n{2}\n", testName, expected, actual));
    }
}

void TestHelper::VerifyStringContainsCaseInsensitive(std::wstring const & bigStr, std::wstring const & smallStr)
{
    VERIFY_IS_TRUE(Common::StringUtility::ContainsCaseInsensitive(bigStr, smallStr));
}

void TestHelper::VerifyStringContains(std::wstring const & bigStr, std::wstring const & smallStr)
{
    VERIFY_IS_TRUE(Common::StringUtility::Contains(bigStr, smallStr));
}

void TestHelper::TrimExtraSpaces(wstring & s)
{
    while (s.find(L"  ") != wstring::npos)
    {
        StringUtility::Replace<wstring>(s, L"  ", L" ");
    }
}

NodeId TestHelper::CreateNodeId(int id)
{
    return NodeId(LargeInteger(0, id));
}

NodeInstance TestHelper::CreateNodeInstance(int id, uint64 instance)
{
    return NodeInstance(CreateNodeId(id), instance);
}

NodeInfoSPtr TestHelper::CreateNodeInfo(NodeInstance nodeInstance)
{
    return make_shared<NodeInfo>(nodeInstance, NodeDescription(), nodeInstance.InstanceId >= 1, true, false, DateTime::Now());
}

ReplicaStates::Enum TestHelper::ReplicaStateFromString(wstring const& value)
{
    if (value == L"RD")
    {
        return ReplicaStates::Ready;
    }
    else if (value == L"SB")
    {
        return ReplicaStates::StandBy;
    }
    else if (value == L"IB")
    {
        return ReplicaStates::InBuild;
    }
    else if (value == L"DD")
    {
        return ReplicaStates::Dropped;
    }
    else
    {
        Assert::CodingError("Invalid replica state: {0}", value);
    }
}

ReplicaFlags::Enum TestHelper::ReplicaFlagsFromString(wstring const& value)
{
    // D = ToBeDroppedByFM
    // R = ToBeDroppedByPLB
    // P = ToBePromoted
    // N = PendingRemove
    // Z = Deleted
    // L = PreferredPrimaryLocation
    // V = MoveInProgress

    ReplicaFlags::Enum flags = ReplicaFlags::None;
    if (value != L"-")
    {
        set<wchar_t> charSet;
        for (size_t i = 0; i < value.size(); i++)
        {
            charSet.insert(value[i]);
        }

        auto itEnd = charSet.end();

        if (charSet.find(L'D') != itEnd)
        {
            flags = static_cast<ReplicaFlags::Enum>(flags | ReplicaFlags::ToBeDroppedByFM);
        }

        if (charSet.find(L'R') != itEnd)
        {
            flags = static_cast<ReplicaFlags::Enum>(flags | ReplicaFlags::ToBeDroppedByPLB);
        }

        if (charSet.find(L'P') != itEnd)
        {
            flags = static_cast<ReplicaFlags::Enum>(flags | ReplicaFlags::ToBePromoted);
        }

        if (charSet.find(L'N') != itEnd)
        {
            flags = static_cast<ReplicaFlags::Enum>(flags | ReplicaFlags::PendingRemove);
        }

        if (charSet.find(L'Z') != itEnd)
        {
            flags = static_cast<ReplicaFlags::Enum>(flags | ReplicaFlags::Deleted);
        }

        if (charSet.find(L'L') != itEnd)
        {
            flags = static_cast<ReplicaFlags::Enum>(flags | ReplicaFlags::PreferredPrimaryLocation);
        }

        if (charSet.find(L'V') != itEnd)
        {
            flags = static_cast<ReplicaFlags::Enum>(flags | ReplicaFlags::MoveInProgress);
        }
    }
    return flags;
}

Reliability::ReplicaRole::Enum TestHelper::ReplicaRoleFromString(wstring const& value)
{
    if (value == L"N")
    {
        return Reliability::ReplicaRole::None;
    }
    else if (value == L"I")
    {
        return Reliability::ReplicaRole::Idle;
    }
    else if (value == L"S")
    {
        return Reliability::ReplicaRole::Secondary;
    }
    else if (value == L"P")
    {
        return Reliability::ReplicaRole::Primary;
    }
    else
    {
        Assert::CodingError("Invalid replica status: {0}", value);
    }
}

wstring TestHelper::ReplicaRoleToString(Reliability::ReplicaRole::Enum replicaRole)
{
    switch (replicaRole)
    {
    case Reliability::ReplicaRole::None:
        return L"N";
    case Reliability::ReplicaRole::Idle:
        return L"I";
    case Reliability::ReplicaRole::Primary:
        return L"P";
    case Reliability::ReplicaRole::Secondary:
        return L"S";
    default:
        Common::Assert::CodingError("Unknown ReplicaRole");
    }
}


ReplicaInfo TestHelper::ReplicaInfoFromString(wstring const& replicaInfoStr, __out ReplicaFlags::Enum & flags, __out bool & isCreating)
{
    vector<wstring> tempStrings;
    StringUtility::Split<wstring>(replicaInfoStr, tempStrings, L" ");

    int replicaNodeId;
    uint64 replicaNodeInstance = 1;
    int64 replicaInstanceId = 1;
    if (tempStrings[0].find(L':') != wstring::npos)
    {
        vector<wstring> temp;
        StringUtility::Split<wstring>(tempStrings[0], temp, L":");

        replicaNodeId = Int32_Parse(temp[0]);
        replicaNodeInstance = static_cast<uint64>(Int32_Parse(temp[1]));
        if (temp.size() > 2)
        {
            replicaInstanceId = Int64_Parse(temp[2]);
        }
    }
    else
    {
        replicaNodeId = Int32_Parse(tempStrings[0]);
    }

    vector<wstring> roles;
    StringUtility::Split<wstring>(tempStrings[1], roles, L"/");

    ReplicaRole::Enum pcRole, icRole, ccRole;
    if (roles.size() == 3)
    {
        pcRole = ReplicaRoleFromString(roles[0]);
        icRole = ReplicaRoleFromString(roles[1]);
        ccRole = ReplicaRoleFromString(roles[2]);
    }
    else if (roles.size() == 2)
    {
        pcRole = ReplicaRoleFromString(roles[0]);
        icRole = ReplicaRoleFromString(roles[1]);
        ccRole = icRole;
    }
    else
    {
        pcRole = ReplicaRole::None;
        icRole = ReplicaRole::None;
        ccRole = ReplicaRoleFromString(roles[0]);
    }

    auto state = ReplicaStateFromString(tempStrings[2]);
    flags = ReplicaFlagsFromString(tempStrings[3]);

    isCreating = tempStrings[3].find('C') != wstring::npos;

    bool isReplicaUp = (tempStrings[4] == L"Up");

    NodeInstance nodeInstance = CreateNodeInstance(replicaNodeId, replicaNodeInstance);
    int64 replicaId = replicaNodeId;

    return ReplicaInfo(
        ReplicaDescription(
            nodeInstance,
            replicaId,
            replicaInstanceId,
            pcRole,
            ccRole,
            state,
            isReplicaUp,
            tempStrings.size() > 5 ? Int64_Parse(tempStrings[5]) : FABRIC_INVALID_SEQUENCE_NUMBER,
            tempStrings.size() > 6 ? Int64_Parse(tempStrings[6]) : FABRIC_INVALID_SEQUENCE_NUMBER,
            L"",
            L"",
            ServiceModel::ServicePackageVersionInstance()),
        icRole);
}

Reliability::Epoch TestHelper::EpochFromString(wstring const& value)
{
    size_t len = value.size();
    int64 v0 = (len >= 1 ? value[len - 1] - L'0' : 0);
    int64 v1 = (len >= 2 ? value[len - 2] - L'0' : 0);
    int64 v2 = (len >= 3 ? value[len - 3] - L'0' : 0);

    return Reliability::Epoch(v2, (v1 << 32) + v0);
}

wstring TestHelper::EpochToString(Reliability::Epoch const& epoch)
{
    wstring result;
    result.push_back(static_cast<wchar_t>(L'0' + epoch.DataLossVersion));
    result.push_back(static_cast<wchar_t>(L'0' + (epoch.ConfigurationVersion >> 32)));
    result.push_back(static_cast<wchar_t>(L'0' + (epoch.ConfigurationVersion & 0xffffffff)));

    return result;
}

FailoverUnitFlags::Flags TestHelper::FailoverUnitFlagsFromString(wstring const& value)
{
    // S:IsStateful, P:HasPersistedState, E:IsNoData, D:IsToBeDeleted
    bool isStateful = false;
    bool hasPersistedState = false;
    bool isNoData = false;
    bool isToBeDeleted = false;
    bool isSwappingPrimary = false;

    if (value != L"-")
    {
        set<wchar_t> charSet;
        for (size_t i = 0; i < value.size(); i++)
        {
            charSet.insert(value[i]);
        }

        auto itEnd = charSet.end();
        isStateful = (charSet.find(L'S') != itEnd);
        hasPersistedState = (charSet.find(L'P') != itEnd);
        isNoData = (charSet.find(L'E') != itEnd);
        isToBeDeleted = (charSet.find(L'D') != itEnd);
        isSwappingPrimary = (charSet.find(L'W') != itEnd);
    }

    return FailoverUnitFlags::Flags(isStateful, hasPersistedState, isNoData, isToBeDeleted, isSwappingPrimary);
}

LoadBalancingComponent::FailoverUnitMovementType::Enum TestHelper::PlbMovementActionTypeFromString(std::wstring const & value)
{
    if (value == L"SwapPrimarySecondary")
    {
        return LoadBalancingComponent::FailoverUnitMovementType::SwapPrimarySecondary;
    }
    else if (value == L"MoveSecondary")
    {
        return LoadBalancingComponent::FailoverUnitMovementType::MoveSecondary;
    }
    else if (value == L"MoveInstance")
    {
        return LoadBalancingComponent::FailoverUnitMovementType::MoveInstance;
    }
    else if (value == L"MovePrimary")
    {
        return LoadBalancingComponent::FailoverUnitMovementType::MovePrimary;
    }
    else if (value == L"AddPrimary")
    {
        return LoadBalancingComponent::FailoverUnitMovementType::AddPrimary;
    }
    else if (value == L"AddSecondary")
    {
        return LoadBalancingComponent::FailoverUnitMovementType::AddSecondary;
    }
    else if (value == L"AddInstance")
    {
        return LoadBalancingComponent::FailoverUnitMovementType::AddInstance;
    }
    else if (value == L"PromoteSecondary")
    {
        return LoadBalancingComponent::FailoverUnitMovementType::PromoteSecondary;
    }
    else if (value == L"RequestedPlacementNotPossible")
    {
        return LoadBalancingComponent::FailoverUnitMovementType::RequestedPlacementNotPossible;
    }
    else if (value == L"DropPrimary")
    {
        return LoadBalancingComponent::FailoverUnitMovementType::DropPrimary;
    }
    else if (value == L"DropSecondary")
    {
        return LoadBalancingComponent::FailoverUnitMovementType::DropSecondary;
    }
    else if (value == L"DropInstance")
    {
        return LoadBalancingComponent::FailoverUnitMovementType::DropInstance;
    }
    else
    {
        Assert::CodingError("Invalid PLB movement action: {0}", value);
    }
}

FailoverUnitUPtr TestHelper::FailoverUnitFromString(wstring const& failoverUnitStr, vector<Reliability::ServiceScalingPolicyDescription> scalingPolicies)
{
    vector<wstring> tokens;
    StringUtility::Split<wstring>(failoverUnitStr, tokens, L" ");

    wstring const serviceName = L"TestService";
    int targetReplicaSetSize = Int32_Parse(tokens[0]);
    int minReplicaSetSize = Int32_Parse(tokens[1]);
    FailoverUnitFlags::Flags flags = FailoverUnitFlagsFromString(tokens[2]);

    vector<wstring> versions;
    StringUtility::Split<wstring>(tokens[3], versions, L"/");

    Reliability::Epoch pcEpoch = EpochFromString(versions[0]);
    Reliability::Epoch ccEpoch = EpochFromString(versions[1]);

    ServiceModel::ApplicationIdentifier appId;
    ServiceModel::ApplicationIdentifier::FromString(L"TestApp_App0", appId);
    ApplicationInfoSPtr applicationInfo = make_shared<ApplicationInfo>(appId, NamingUri(L"fabric:/TestApp"), 1);
    ApplicationEntrySPtr applicationEntry = make_shared<CacheEntry<ApplicationInfo>>(move(applicationInfo));
    wstring serviceTypeName = L"TestServiceType";
    ServiceModel::ServiceTypeIdentifier typeId(ServiceModel::ServicePackageIdentifier(appId, L"TestPackage"), serviceTypeName);
    wstring placementConstraints = L"";
    int scaleoutCount = 0;
    auto serviceType = make_shared<ServiceType>(typeId, applicationEntry);
    auto serviceInfo = make_shared<ServiceInfo>(
        ServiceDescription(serviceName, 0, 0, 1, targetReplicaSetSize, minReplicaSetSize, flags.IsStateful(), 
            flags.HasPersistedState(), TimeSpan::FromSeconds(60.0), TimeSpan::MaxValue, TimeSpan::FromSeconds(300.0), typeId,
            vector<ServiceCorrelationDescription>(), placementConstraints, scaleoutCount, vector<ServiceLoadMetricDescription>(), 
            0, vector<byte>(), L"", std::vector<ServiceModel::ServicePlacementPolicyDescription>(), ServiceModel::ServicePackageActivationMode::SharedProcess,
            L"", vector<Reliability::ServiceScalingPolicyDescription>(scalingPolicies)),
        serviceType,
        FABRIC_INVALID_SEQUENCE_NUMBER,
        false);

    serviceInfo->AddFailoverUnitId(FailoverUnitId());

    FailoverUnitUPtr failoverUnit = FailoverUnitUPtr(new FailoverUnit(
        FailoverUnitId(),
        ConsistencyUnitDescription(),
        serviceInfo,
        flags,
        pcEpoch,
        ccEpoch,
        0,
        DateTime::Now(),
        0,
        0,
        PersistenceState::ToBeInserted));

    if (tokens.size() > 4)
    {
        tokens.clear();
        StringUtility::Split<wstring>(failoverUnitStr, tokens, L"[");

        for (size_t i = 1; i < tokens.size(); i++)
        {
            StringUtility::TrimSpaces(tokens[i]);
            StringUtility::Trim<wstring>(tokens[i], L"]");

            bool isCreating = false;
            ReplicaFlags::Enum flags1;
            ReplicaInfo replicaInfo = ReplicaInfoFromString(tokens[i], flags1, isCreating);
            ReplicaDescription const & replica = replicaInfo.Description;
            NodeInstance const & nodeInstance = replica.FederationNodeInstance;

            NodeInfoSPtr nodeInfo = TestHelper::CreateNodeInfo(nodeInstance);

            auto fmReplica = failoverUnit->AddReplica(
                move(nodeInfo),
                replica.ReplicaId,
                replica.InstanceId,
                replica.State,
                flags1,
                replica.PreviousConfigurationRole,
                replica.CurrentConfigurationRole,
                replica.IsUp,
                serviceInfo->ServiceDescription.UpdateVersion,
                ServiceModel::ServicePackageVersionInstance(),
                PersistenceState::NoChange,
                DateTime::Now());

            fmReplica->ReplicaDescription.LastAcknowledgedLSN = replica.LastAcknowledgedLSN;
            fmReplica->ReplicaDescription.FirstAcknowledgedLSN = replica.FirstAcknowledgedLSN;

            if (isCreating)
            {
                fmReplica->VersionInstance = ServiceModel::ServicePackageVersionInstance::Invalid;
            }
        }

        failoverUnit->PostUpdate(DateTime::Now());
        failoverUnit->PostCommit(failoverUnit->OperationLSN + 1);
    }

    return failoverUnit;
}

wstring TestHelper::FailoverUnitToString(FailoverUnitUPtr const& failoverUnit, ServiceDescription const& serviceDescription)
{
    std::wstring result;
    StringWriter s(result);

    s <<
        (failoverUnit->ServiceInfoObj ? failoverUnit->TargetReplicaSetSize : serviceDescription.TargetReplicaSetSize) << L' ' <<
        (failoverUnit->ServiceInfoObj ? failoverUnit->MinReplicaSetSize : serviceDescription.MinReplicaSetSize) << L' ' <<
        failoverUnit->FailoverUnitFlags << L' ';

    s.Write("{0}/{1}", EpochToString(failoverUnit->PreviousConfigurationEpoch), EpochToString(failoverUnit->CurrentConfigurationEpoch));

    for (auto it = failoverUnit->BeginIterator; it != failoverUnit->EndIterator; it++)
    {
        wstring flags = wformatString(it->Flags);
        if (it->IsCreating)
        {
            if (it->Flags == ReplicaFlags::None)
            {
                flags = L"C";
            }
            else
            {
                flags.append(L"C");
            }
        }

        s <<
            L" [" <<
            it->FederationNodeId.IdValue << // only low will be written when high = 0
            (it->FederationNodeInstance.InstanceId == 1 && it->InstanceId == 1 ? L"" : wformatString(":{0}", it->FederationNodeInstance.InstanceId)) <<
            (it->InstanceId == 1 ? L" " : wformatString(":{0} ", it->InstanceId)) <<
            ReplicaRoleToString(it->PreviousConfigurationRole) << L'/' <<
            ReplicaRoleToString(it->CurrentConfigurationRole) << L' ' <<
            it->ReplicaState << L' ' <<
            flags << L' ' <<
            (it->IsUp ? L"Up" : L"Down") <<
            L"]";
    }

    return result;
}

vector<wstring> TestHelper::ActionsToString(vector<StateMachineActionUPtr> const& actions)
{
    vector<wstring> result;

    for (StateMachineActionUPtr const& action : actions)
    {
        wstring actionStr;
        StringWriter writer(actionStr);
        writer.Write("{0:s}", *action);
        result.push_back(actionStr);
    }

    sort(result.begin(), result.end());

    return result;
}

NodeInstance TestHelper::FailoverUnitInfoFromString(wstring const& report, __out unique_ptr<FailoverUnitInfo> & failoverUnitInfo)
{
    vector<wstring> temp;
    StringUtility::Split<wstring>(report, temp, L"|");

    int nodeId;
    int64 nodeInstanceId;

    if (temp[0].find(L':') != wstring::npos)
    {
        vector<wstring> tempStrings;
        StringUtility::Split<wstring>(temp[0], tempStrings, L":");

        nodeId = Int32_Parse(tempStrings[0]);
        nodeInstanceId = Int64_Parse(tempStrings[1]);
    }
    else
    {
        nodeId = Int32_Parse(temp[0]);
        nodeInstanceId = 1;
    }

    NodeInstance orign = CreateNodeInstance(nodeId, nodeInstanceId);

    if (temp.size() > 1)
    {
        vector<wstring> tokens;
        StringUtility::Split<wstring>(temp[1], tokens, L" ");

        wstring const serviceName = L"TestService";
        int targetReplicaSetSize = Int32_Parse(tokens[0]);
        int minReplicaSetSize = Int32_Parse(tokens[1]);
        FailoverUnitFlags::Flags flags = FailoverUnitFlagsFromString(tokens[2]);

        vector<wstring> versions;
        StringUtility::Split<wstring>(tokens[3], versions, L"/");

        Reliability::Epoch pcEpoch, icEpoch, ccEpoch;
        if (versions.size() == 3)
        {
            pcEpoch = EpochFromString(versions[0]);
            icEpoch = EpochFromString(versions[1]);
            ccEpoch = EpochFromString(versions[2]);
        }
        else if (versions.size() == 2)
        {
            pcEpoch = EpochFromString(versions[0]);
            icEpoch = EpochFromString(versions[1]);
            ccEpoch = icEpoch;
        }
        else
        {
            pcEpoch = Epoch::InvalidEpoch();
            icEpoch = Epoch::InvalidEpoch();
            ccEpoch = EpochFromString(versions[0]);
        }

        bool isReportFromPrimary = tokens[4] == L"T" ? true : false;

        wstring serviceTypeName = L"TestServiceType";
        ServiceModel::ServiceTypeIdentifier typeId(ServiceModel::ServicePackageIdentifier(L"TestApp_App0", L"TestPackage"), serviceTypeName);
        wstring placementConstraints = L"";
        int scaleoutCount = 0;
        ServiceDescription serviceDescription(
            serviceName,
            0,
            0,
            1,
            targetReplicaSetSize,
            minReplicaSetSize,
            flags.IsStateful(),
            flags.HasPersistedState(),
            TimeSpan::FromSeconds(60.0),
            TimeSpan::MaxValue,
            TimeSpan::FromSeconds(300.0),
            typeId,
            vector<ServiceCorrelationDescription>(),
            placementConstraints,
            scaleoutCount,
            vector<ServiceLoadMetricDescription>(),
            0,
            vector<byte>());

        vector<ReplicaInfo> replicas;
        if (tokens.size() > 5)
        {
            tokens.clear();
            StringUtility::Split<wstring>(temp[1], tokens, L"[");

            for (size_t i = 1; i < tokens.size(); i++)
            {
                StringUtility::TrimSpaces(tokens[i]);
                StringUtility::Trim<wstring>(tokens[i], L"]");

                bool isCreating = false;
                ReplicaFlags::Enum flags1;
                replicas.push_back(ReplicaInfoFromString(tokens[i], flags1, isCreating));
            }
        }

        FailoverUnitDescription failoverUnitDescription(
            FailoverUnitId(),
            ConsistencyUnitDescription(),
            ccEpoch,
            pcEpoch);

        failoverUnitInfo = make_unique<FailoverUnitInfo>(
            move(serviceDescription),
            move(failoverUnitDescription),
            icEpoch,
            isReportFromPrimary,
            false, // local replica deleted
            move(replicas));
    }

    return orign;
}

FailoverManagerSPtr TestHelper::CreateFauxFM(ComponentRoot const& root, bool useFauxStore)
{
    FailoverManagerSPtr fm;

    auto healthTransport = make_unique<TestHelper::DummyHealthReportingTransport>(root);
    auto healthClient = make_shared<HealthReportingComponent>(*healthTransport, L"Dummy", false);

    Federation::NodeConfig nodeConfig(Federation::NodeId(), L"TestAddress", L"TestAddress", L"");
    Transport::SecuritySettings defaultSettings;
    auto fs = make_shared<FederationSubsystem>(nodeConfig, FabricCodeVersion(), nullptr, Common::Uri(), defaultSettings, root);

    unique_ptr<Store::IReplicatedStore> replicatedStore;
    if (useFauxStore)
    {
        replicatedStore = make_unique<FauxLocalStore>();
    }
    else
    {
        wstring const & directory = FailoverConfig::GetConfig().FMStoreDirectory;
        wstring const & fileName = FailoverConfig::GetConfig().FMStoreFilename;

        replicatedStore = Store::KeyValueStoreReplica::CreateForUnitTests(
            Guid::NewGuid(),
            0,
            Store::EseLocalStoreSettings(fileName, directory),
            root);
        auto error = replicatedStore->InitializeLocalStoreForUnittests();
        if (!error.IsSuccess())
        {
            Assert::CodingError("InitializeLocalStoreForUnittests failed: {0}", error);
        }
    }

    auto fmStore = make_unique<FailoverManagerStore>(move(replicatedStore));
    ComPointer<IFabricStatefulServicePartition> servicePartition;

    auto fabricNodeConfig = make_shared<FabricNodeConfig>();
    Api::IClientFactoryPtr clientFactoryPtr;
    auto error = ClientTest::TestClientFactory::CreateLocalClientFactory(fabricNodeConfig, clientFactoryPtr);
    if (!error.IsSuccess())
    {
        Assert::CodingError("Failed to create local client factory: error={0}", error);
    }

    fm = FailoverManager::CreateFM(
        fs->shared_from_this(),
        healthClient,
        Api::IServiceManagementClientPtr(),
        clientFactoryPtr,
        fabricNodeConfig,
        move(fmStore),
        servicePartition,
        Guid(),
        0);

    AutoResetEvent wait(false);
    fm->RegisterFailoverManagerReadyEvent([&wait](EventArgs const &) { wait.Set(); });

    fm->Open(EventHandler(), EventHandler());
    wait.WaitOne();

    return fm;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TestHelper::DummyHealthReportingTransport::DummyHealthReportingTransport(ComponentRoot const& root)
    : HealthReportingTransport(root)
{
}

AsyncOperationSPtr TestHelper::DummyHealthReportingTransport::BeginReport(
    Transport::MessageUPtr && message,
    TimeSpan timeout,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& parent)
{
    UNREFERENCED_PARAMETER(message);
    UNREFERENCED_PARAMETER(timeout);

    return make_shared<CompletedAsyncOperation>(ErrorCodeValue::OperationCanceled, callback, parent);
}

ErrorCode TestHelper::DummyHealthReportingTransport::EndReport(
    AsyncOperationSPtr const& operation,
    __out Transport::MessageUPtr & reply)
{
    UNREFERENCED_PARAMETER(reply);

    return CompletedAsyncOperation::End(operation);
}
