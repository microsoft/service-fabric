// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class InfrastructureTaskContext : public RolloutContext
        {
            DENY_COPY(InfrastructureTaskContext)

        public:
            static const RolloutContextType::Enum ContextType;

            InfrastructureTaskContext();

            InfrastructureTaskContext(
                InfrastructureTaskContext &&);

            InfrastructureTaskContext & operator=(
                InfrastructureTaskContext &&);

            InfrastructureTaskContext(
                TaskInstance const &);

            InfrastructureTaskContext(
                Common::ComponentRoot const &,
                ClientRequestSPtr const &,
                InfrastructureTaskDescription const &);

            virtual ~InfrastructureTaskContext();

            __declspec(property(get=get_Description)) InfrastructureTaskDescription const & Description;
            InfrastructureTaskDescription const & get_Description() const { return description_; }

            __declspec(property(get=get_State)) InfrastructureTaskState::Enum const State;
            InfrastructureTaskState::Enum const get_State() const { return state_; }

            __declspec(property(get=get_IsInterrupted)) bool IsInterrupted;
            bool get_IsInterrupted() const { return isInterrupted_; }

            __declspec(property(get=get_HealthCheckElapsedTime)) Common::TimeSpan const HealthCheckElapsedTime;
            Common::TimeSpan const get_HealthCheckElapsedTime() const { return healthCheckElapsedTime_; }

            __declspec(property(get=get_LastHealthCheckResult)) bool LastHealthCheckResult;
            bool get_LastHealthCheckResult() const { return lastHealthCheckResult_; }

            void OnFailRolloutContext() override { } // no-op

            Common::ErrorCode TryAcceptStartTask(InfrastructureTaskDescription const &, __out bool & taskComplete, __out bool & shouldCommit);
            Common::ErrorCode TryAcceptFinishTask(TaskInstance const &, __out bool & taskComplete, __out bool & shouldCommit);
            void UpdateState(InfrastructureTaskState::Enum state);

            void UpdateHealthCheckElapsedTime(Common::TimeSpan const elapsed);
            void SetHealthCheckElapsedTime(Common::TimeSpan const value);
            void SetLastHealthCheckResult(bool isHealthy);

            virtual std::wstring const & get_Type() const;
            virtual void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            FABRIC_FIELDS_05(description_, state_, isInterrupted_, healthCheckElapsedTime_, lastHealthCheckResult_);

        protected:
            virtual std::wstring ConstructKey() const;

        private:
            void UpdateDescription(InfrastructureTaskDescription const &);

            InfrastructureTaskDescription description_;
            InfrastructureTaskState::Enum state_;
            bool isInterrupted_;

            // persist elapsed times across failover
            Common::TimeSpan healthCheckElapsedTime_;
            bool lastHealthCheckResult_;
        };
    }
}
