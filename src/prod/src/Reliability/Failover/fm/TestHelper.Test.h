// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FailoverManagerUnitTest
{
    class TestHelper
    {
    public:

        template <class T>
        static void AssertEqual(T const& expected, T const& actual, std::wstring const& testName);

        static void VerifyStringContainsCaseInsensitive(std::wstring const & bigStr, std::wstring const & smallStr);
        static void VerifyStringContains(std::wstring const & bigStr, std::wstring const & smallStr);

        static void TrimExtraSpaces(std::wstring & s);

        static Federation::NodeId CreateNodeId(int id);
        static Federation::NodeInstance CreateNodeInstance(int id, uint64 instance);
        static Reliability::FailoverManagerComponent::NodeInfoSPtr CreateNodeInfo(Federation::NodeInstance nodeInstance);

        static Reliability::ReplicaStates::Enum ReplicaStateFromString(std::wstring const& value);

        static Reliability::ReplicaFlags::Enum ReplicaFlagsFromString(std::wstring const& value);

        static Reliability::ReplicaRole::Enum ReplicaRoleFromString(std::wstring const& value);
        static std::wstring ReplicaRoleToString(Reliability::ReplicaRole::Enum replicaRole);

        static Reliability::ReplicaInfo ReplicaInfoFromString(std::wstring const& value, __out Reliability::ReplicaFlags::Enum & flags, __out bool & isCreating);

        static Reliability::Epoch EpochFromString(std::wstring const& value);
        static std::wstring EpochToString(Reliability::Epoch const& epoch);

        static Reliability::FailoverUnitFlags::Flags FailoverUnitFlagsFromString(std::wstring const& value);

        static Reliability::LoadBalancingComponent::FailoverUnitMovementType::Enum PlbMovementActionTypeFromString(std::wstring const& value);

        //
        // FailoverUnit String:
        //
        // [ServiceName] [TargetReplicaSetSize] [MinReplicaSetSize] [FailoverUnitFlags] [pcEpoch]/[ccEpoch] [Replica 1] [Replica 2] ...
        // Replica #: [ReplicaNodeId]:[ReplicaNodeInstance]:[ReplicaInstanceId]  [pcRole]/[ccRole] [ReplicaState] [ReplicaFlags][ReplicaUpOrDown]
        //
        // Note: Default ReplicaNodeInstance = 1
        //       Default ReplicaInstanceId = 1
        //       ReplicaId = ReplicaNodeId
        //       ReplicaNodeInstance value '0' implies hosting node of replica is Down
        // 
        // FailoverUnitFlags: Combination of S:IsStateful, P:HasPersistedState, E:IsNoData, D:IsToBeDeleted
        // Epoch: "ijk", where i = DataLossVersion, j = PrimaryChangeVersion, k = ConfigurationVersion
        // REPLICA_STATUS: StandBy, InBuild, Ready, Dropped
        // ReplicaFlags: Combination of R:ToBeDropped, P:ToBePromoted, N:PendingRemove, C:Creating
        // ReplicaRole: One of None, Idle, Secondary, or Primary
        // ReplicaUpOrDown: One of Up or Down
        //
        // Example: "3 2 SP 0 0 [0 0 - Idle Primary Up] [1 0 - Idle Secondary Up]"
        //
        static Reliability::FailoverManagerComponent::FailoverUnitUPtr FailoverUnitFromString(std::wstring const& fuStr, vector<Reliability::ServiceScalingPolicyDescription> scalingPolicies = vector<Reliability::ServiceScalingPolicyDescription>());
        static std::wstring FailoverUnitToString(Reliability::FailoverManagerComponent::FailoverUnitUPtr const& failoverUnit, Reliability::ServiceDescription const& serviceDescription = Reliability::ServiceDescription());

        static std::vector<std::wstring> ActionsToString(std::vector<Reliability::FailoverManagerComponent::StateMachineActionUPtr> const& actions);

        static Federation::NodeInstance FailoverUnitInfoFromString(std::wstring const& report, __out std::unique_ptr<Reliability::FailoverUnitInfo> & failoverUnitInfo);

        static Reliability::FailoverManagerComponent::FailoverManagerSPtr CreateFauxFM(Common::ComponentRoot const& root, bool useFauxStore = true);

    private:

        class DummyHealthReportingTransport : public Client::HealthReportingTransport
        {
        public:
            DummyHealthReportingTransport(Common::ComponentRoot const& root);

            virtual Common::AsyncOperationSPtr BeginReport(
                Transport::MessageUPtr && message,
                Common::TimeSpan timeout,
                Common::AsyncCallback const& callback,
                Common::AsyncOperationSPtr const& parent) override;

            virtual Common::ErrorCode EndReport(
                Common::AsyncOperationSPtr const& operation,
                __out Transport::MessageUPtr & reply) override;
        };
    };
}
