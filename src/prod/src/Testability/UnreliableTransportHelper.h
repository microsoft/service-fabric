// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Testability
{
    class UnreliableTransportHelper;
    typedef std::unique_ptr<UnreliableTransportHelper> UnreliableTransportHelperUPtr;
    typedef std::shared_ptr<UnreliableTransportHelper> UnreliableTransportHelperSPtr;

    // The class is responsible for handling out reads and writes to persistent store of transport behaviors.
    // maintaining list of the tranportbehavior elapsed time on its application. Raising healthwarnings if the behavior
    // elapsed time goes beyond config threshold.
    class UnreliableTransportHelper :
        public Common::ComponentRoot,
        public Common::TextTraceComponent<Common::TraceTaskCodes::TestabilityComponent>
    {
        DENY_COPY(UnreliableTransportHelper)

    public:
        UnreliableTransportHelper(
            __in Common::FabricNodeConfigSPtr nodeConfig);

        void InitializeHealthReportingComponent(
            Client::HealthReportingComponentSPtr const & healthClient);

        Common::ErrorCode Open();

        Common::ErrorCode Close();

        ~UnreliableTransportHelper();

        static UnreliableTransportHelperSPtr CreateSPtr(
            __in Common::FabricNodeConfigSPtr nodeConfig);

        static UnreliableTransportHelperUPtr CreateUPtr(
            __in Common::FabricNodeConfigSPtr nodeConfig);

        Common::ErrorCode AddUnreliableTransportBehavior(
            ServiceModel::QueryArgumentMap const & queryArgument,
            Common::ActivityId const & activityId,
            __out ServiceModel::QueryResult & queryResult);

        Common::ErrorCode RemoveUnreliableTransportBehavior(
            ServiceModel::QueryArgumentMap const & queryArgument,
            Common::ActivityId const & activityId,
            __out ServiceModel::QueryResult & queryResult);

        Common::ErrorCode GetTransportBehaviors(
            ServiceModel::QueryArgumentMap const & queryArgument,
            Common::ActivityId const & activityId,
            __out ServiceModel::QueryResult & queryResult);

    private:
        void StartTimer();

        void StopTimer();

        class TransportBehavior
        {
        public:
            TransportBehavior(const std::wstring& behaviorName):
                behaviorName_(behaviorName)
                , initedTime_(Common::DateTime::Now())
            {}

            bool operator < (TransportBehavior const& rhs) const 
            { return initedTime_ < rhs.initedTime_; }

            bool operator == (std::wstring const& behaviorName) const
            { return behaviorName_ == behaviorName; }

            std::wstring behaviorName_;
            Common::DateTime initedTime_;
        };

        std::vector<TransportBehavior> transportBehaviors_;
        Common::RwLock transportBehaviorsListLock_;

        Common::ErrorCode GetTransportBehaviors(
            std::vector<std::pair<std::wstring, std::wstring>>& transportBehaviors);

        static void UnreliableTransportTimerCallback(
            std::weak_ptr<ComponentRoot> const & wPtr,
            std::wstring behaviorName);
        void HandleUnreliableTransportTimerCallback(const std::wstring& behaviorName);

        Common::FabricNodeConfigSPtr nodeConfig_;
        Common::TimerSPtr unrelibleTxTimer_;
        Client::HealthReportingComponentSPtr healthClient_;
        Testability::NodeHealthReportManagerUPtr nodeHealthReportManager_;
    };
}
