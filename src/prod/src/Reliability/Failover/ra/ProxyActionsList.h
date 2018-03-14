// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class ProxyActionsList : public Serialization::FabricSerializable
        {
        public:
            static bool AreAcceptableActionsListsForParallelExecution(std::vector<ActionListInfo> const & runningActions, ProxyActionsListTypes::Enum const toRun);
            static bool AreStandaloneActionsLists(ProxyActionsListTypes::Enum const actionList);
            static bool IsAbort(ProxyActionsListTypes::Enum const actionList);
            static bool IsClose(ProxyActionsListTypes::Enum const actionList);
            static ProxyActionsList const & GetActionsListByType(ProxyActionsListTypes::Enum actionListName);

            __declspec (property(get=get_Actions)) std::vector<ProxyActions::Enum> const & Actions;
            std::vector<ProxyActions::Enum> const & get_Actions() const { return actions_; }

            void WriteTo(Common::TextWriter& w, Common::FormatOptions const& options) const;

            ProxyActionsList() 
            {
            }

            ProxyActionsList(ProxyActions::Enum action) 
            {
                actions_.push_back(action);
            }

            ProxyActionsList(std::initializer_list<ProxyActions::Enum> actions) :
                actions_(actions.begin(), actions.end())
            {
            }

        private:

            static ProxyActionsList const & GetEmpty() { return Empty; }

            // Stateless service actions
            static ProxyActionsList const & GetStatelessServiceOpen() { return StatelessServiceOpen; }
            static ProxyActionsList const & GetStatelessServiceClose() { return StatelessServiceClose; }
            static ProxyActionsList const & GetStatelessServiceAbort() { return StatelessServiceAbort; }

            // Stateful service actions
            static ProxyActionsList const & GetStatefulServiceOpenIdle() { return StatefulServiceOpenIdle; }
            static ProxyActionsList const & GetStatefulServiceOpenPrimary() { return StatefulServiceOpenPrimary; }
            static ProxyActionsList const & GetStatefulServiceReopen() { return StatefulServiceReopen; }
            static ProxyActionsList const & GetStatefulServiceClose() { return StatefulServiceClose; }
            static ProxyActionsList const & GetStatefulServiceDrop() { return StatefulServiceDrop; }
            static ProxyActionsList const & GetStatefulServiceAbort() { return StatefulServiceAbort; }
            static ProxyActionsList const & GetStatefulServiceChangeRole() { return StatefulServiceChangeRole; }
            static ProxyActionsList const & GetStatefulServiceFinalizeDemoteToSecondary() { return StatefulServiceFinalizeDemoteToSecondary; }
            static ProxyActionsList const & GetStatefulServicePromoteToPrimary() { return StatefulServicePromoteToPrimary; }
            static ProxyActionsList const & GetStatefulServiceDemoteToSecondary() { return StatefulServiceDemoteToSecondary; }
            static ProxyActionsList const & GetStatefulServiceFinishDemoteToSecondary() { return StatefulServiceFinishDemoteToSecondary; }
            static ProxyActionsList const & GetStatefulServiceEndReconfiguration() { return StatefulServiceEndReconfiguration; }

            // Replicator only
            static ProxyActionsList const & GetReplicatorBuildIdleReplica() { return ReplicatorBuildIdleReplica; }
            static ProxyActionsList const & GetReplicatorRemoveIdleReplica() { return ReplicatorRemoveIdleReplica; }
            static ProxyActionsList const & GetReplicatorGetStatus() { return ReplicatorGetStatus; }
            static ProxyActionsList const & GetReplicatorGetQuery() { return ReplicatorGetQuery; }
            static ProxyActionsList const & GetReplicatorUpdateEpochAndGetStatus() { return ReplicatorUpdateEpochAndGetStatus; }
            static ProxyActionsList const & GetReplicatorUpdateReplicas() { return ReplicatorUpdateReplicas; }
            static ProxyActionsList const & GetReplicatorUpdateAndCatchupQuorum() { return ReplicatorUpdateAndCatchupQuorum; }
            static ProxyActionsList const & GetCancelCatchupReplicaSet() { return CancelCatchupReplicaSet; }

            // FUP/Partition only
            static ProxyActionsList const & GetUpdateServiceDescription() { return UpdateServiceDescription; }

            static Common::Global<ProxyActionsList> Empty;
            static Common::Global<ProxyActionsList> StatelessServiceOpen;
            static Common::Global<ProxyActionsList> StatelessServiceClose;
            static Common::Global<ProxyActionsList> StatelessServiceAbort;

            static Common::Global<ProxyActionsList> StatefulServiceOpenIdle;
            static Common::Global<ProxyActionsList> StatefulServiceOpenPrimary;
            static Common::Global<ProxyActionsList> StatefulServiceReopen;
            static Common::Global<ProxyActionsList> StatefulServiceClose;
            static Common::Global<ProxyActionsList> StatefulServiceDrop;
            static Common::Global<ProxyActionsList> StatefulServiceAbort;
            static Common::Global<ProxyActionsList> StatefulServiceChangeRole;
            static Common::Global<ProxyActionsList> StatefulServicePromoteToPrimary;
            static Common::Global<ProxyActionsList> StatefulServiceDemoteToSecondary;
            static Common::Global<ProxyActionsList> StatefulServiceFinishDemoteToSecondary;
            static Common::Global<ProxyActionsList> StatefulServiceEndReconfiguration;

            static Common::Global<ProxyActionsList> ReplicatorBuildIdleReplica;
            static Common::Global<ProxyActionsList> ReplicatorRemoveIdleReplica;
            static Common::Global<ProxyActionsList> ReplicatorGetStatus;
            static Common::Global<ProxyActionsList> ReplicatorGetQuery;
            static Common::Global<ProxyActionsList> ReplicatorUpdateEpochAndGetStatus;
            static Common::Global<ProxyActionsList> ReplicatorUpdateReplicas;
            static Common::Global<ProxyActionsList> ReplicatorUpdateAndCatchupQuorum;
            static Common::Global<ProxyActionsList> CancelCatchupReplicaSet;

            static Common::Global<ProxyActionsList> StatefulServiceFinalizeDemoteToSecondary;

            static Common::Global<ProxyActionsList> UpdateServiceDescription;

            class ConcurrentExecutionCompatibilityLookupTable;

            static Common::Global<ConcurrentExecutionCompatibilityLookupTable> CompatibilityTable;

            std::vector<ProxyActions::Enum> actions_;
        };
    }
}
