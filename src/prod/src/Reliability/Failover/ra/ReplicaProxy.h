// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        // Represents a replica within RA Proxy
        class ReplicaProxy
        {       
            DENY_COPY_CONSTRUCTOR(ReplicaProxy);

        public:
            ReplicaProxy(ReplicaDescription const replicaDescription) :
              replicaDescription_(replicaDescription),
              failoverUnitProxyState_(ReplicaProxyStates::Ready),
              replicatorState_(ReplicaProxyStates::Ready),
              stateUpdateTime_(Common::DateTime::Now())
            {
            }

            ReplicaProxy(ReplicaProxy && other) :
               replicaDescription_(std::move(other.replicaDescription_)),
               failoverUnitProxyState_(other.failoverUnitProxyState_),
               replicatorState_(other.replicatorState_),
               buildIdleOperation_(std::move(other.buildIdleOperation_)),
               stateUpdateTime_(other.stateUpdateTime_)
            {
            }

            __declspec(property(put=set_State)) ReplicaProxyStates::Enum State;
            void set_State(ReplicaProxyStates::Enum value) { replicatorState_ = value; failoverUnitProxyState_ = value; stateUpdateTime_ = Common::DateTime::Now(); }

            __declspec(property(get=get_StateUpdateTime)) Common::DateTime StateUpdateTime;
            Common::DateTime get_StateUpdateTime() const { return stateUpdateTime_; }

            __declspec(property(get=get_FailoverUnitProxyState, put=set_FailoverUnitProxyState)) ReplicaProxyStates::Enum FailoverUnitProxyState;
            ReplicaProxyStates::Enum get_FailoverUnitProxyState() const { return failoverUnitProxyState_; }
            void set_FailoverUnitProxyState(ReplicaProxyStates::Enum value) { failoverUnitProxyState_ = value; }

            __declspec(property(get=get_ReplicatorState, put=set_ReplicatorState)) ReplicaProxyStates::Enum ReplicatorState;
            ReplicaProxyStates::Enum get_ReplicatorState() const { return replicatorState_; }
            void set_ReplicatorState(ReplicaProxyStates::Enum value) { replicatorState_ = value; }

            __declspec(property(get=get_ReplicaDescription, put=set_ReplicaDescription)) Reliability::ReplicaDescription const & ReplicaDescription;
            Reliability::ReplicaDescription const & get_ReplicaDescription() const { return replicaDescription_; }
            void set_ReplicaDescription(Reliability::ReplicaDescription const & value) { replicaDescription_ = value; }

            __declspec(property(get=get_CurrentConfigurationRole, put=set_CurrentConfigurationRole)) ReplicaRole::Enum CurrentConfigurationRole;
            ReplicaRole::Enum get_CurrentConfigurationRole() const { return replicaDescription_.CurrentConfigurationRole; }
            void set_CurrentConfigurationRole(ReplicaRole::Enum value) { replicaDescription_.CurrentConfigurationRole = value; }

            __declspec(property(get = get_BuildIdleOperation, put=set_BuildIdleOperation)) ExecutingOperation & BuildIdleOperation;
            ExecutingOperation const & get_BuildIdleOperation() const { return buildIdleOperation_; }
            ExecutingOperation & get_BuildIdleOperation() { return buildIdleOperation_; }
            void set_BuildIdleOperation(ExecutingOperation const & value) { buildIdleOperation_ = value;}

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
            void WriteToEtw(uint16 contextSequenceId) const;

        private:
            Reliability::ReplicaDescription replicaDescription_;
            // replicator's view of the proxy
            ReplicaProxyStates::Enum replicatorState_;
            // failoverunitproxy's view of the proxy
            ReplicaProxyStates::Enum failoverUnitProxyState_;
            ExecutingOperation buildIdleOperation_;

            Common::DateTime stateUpdateTime_;
        };
    }
}
