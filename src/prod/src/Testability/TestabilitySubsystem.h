// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Testability
{   
    class TestabilitySubsystem :
        public Common::RootedObject
        , public ITestabilitySubsystem
        , public Common::ComponentRoot
        , Common::TextTraceComponent<Common::TraceTaskCodes::TestabilityComponent>
    {
        DENY_COPY(TestabilitySubsystem)

    public:
        TestabilitySubsystem(
            __in Common::ComponentRoot const & root,
            __in Federation::FederationSubsystem & federation,
            __in Common::FabricNodeConfigSPtr nodeConfig,
            __in Hosting2::IHostingSubsystemWPtr hostingSubSytem);

        virtual ~TestabilitySubsystem() = default;
    protected:
        virtual void InitializeHealthReportingComponent(
            Client::HealthReportingComponentSPtr const & healthClient) override;

        virtual Common::AsyncOperationSPtr OnBeginOpen(
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
       
        virtual Common::ErrorCode OnEndOpen(
            Common::AsyncOperationSPtr const &);

        virtual Common::AsyncOperationSPtr OnBeginClose(
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);

        virtual Common::ErrorCode OnEndClose(
            Common::AsyncOperationSPtr const &);

        virtual void OnAbort();

        virtual Common::AsyncOperationSPtr BeginTestabilityOperation(
            Query::QueryNames::Enum queryName,
            ServiceModel::QueryArgumentMap const & queryArgs,
            Common::ActivityId const & activityId,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;

        virtual Common::ErrorCode EndTestabilityOperation(
            Common::AsyncOperationSPtr const &operation,
            __out Transport::MessageUPtr & reply) override;

        void RequestMessageHandler(
            Transport::MessageUPtr & message,
            Federation::RequestReceiverContextUPtr & receiverContext);

        Common::AsyncOperationSPtr BeginProcessRequest(
            Transport::MessageUPtr &&,
            Federation::RequestReceiverContextUPtr &&,
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);

        Common::ErrorCode EndProcessRequest(
            Common::AsyncOperationSPtr const &,
            __out Transport::MessageUPtr &,
            __out Federation::RequestReceiverContextUPtr &);

        void OnProcessRequestComplete(
            Common::ActivityId const &, 
            Common::AsyncOperationSPtr const &);

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

        Common::ErrorCode AddUnreliableLeaseBehavior(
            ServiceModel::QueryArgumentMap const & queryArgument,
            Common::ActivityId const & activityId,
            __out ServiceModel::QueryResult & queryResult);

        Common::ErrorCode RemoveUnreliableLeaseBehavior(
            ServiceModel::QueryArgumentMap const & queryArgument,
            Common::ActivityId const & activityId,
            __out ServiceModel::QueryResult & queryResult);

        Common::ErrorCode TestabilitySubsystem::RestartDeployedCodePackage(
            ServiceModel::QueryArgumentMap const &,
            Common::ActivityId const & activityId_,
            __out ServiceModel::QueryResult &);

        Common::ErrorCode OnOpen();
        Common::ErrorCode OnClose();

        Common::ErrorCode StopNode(
            ServiceModel::QueryArgumentMap const & queryArgument,
            Common::ActivityId const & activityId,
            __out ServiceModel::QueryResult & queryResult);

        class OpenAsyncOperation;
        friend class OpenAsyncOperation;

        class CloseAsyncOperation;
        friend class CloseAsyncOperation;

        class TestabilitySubsystemRequestHandlerBase;
        class TestabilitySubsystemRequestHandler;
        class TestabilitySubsystemQueryMessageHandler;
        class QueryMessageHandlerJobItem;

        std::unique_ptr<Common::CommonTimedJobQueue<TestabilitySubsystem>> queryJobQueue_;

        Testability::UnreliableTransportHelperSPtr unreliableTx_;
        Testability::UnreliableLeaseHelperUPtr unreliableLease_;
        Query::QueryMessageHandlerUPtr queryMessageHandler_;

        Common::FabricNodeConfigSPtr nodeConfig_;
        Federation::FederationSubsystem & federation_;
        Hosting2::IHostingSubsystemWPtr hostingSubSytem_;
    };
}
