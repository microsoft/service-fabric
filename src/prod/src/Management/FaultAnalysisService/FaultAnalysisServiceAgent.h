// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class FaultAnalysisServiceAgent
            : public Api::IFaultAnalysisServiceAgent
            , public Common::ComponentRoot
            , private Federation::NodeTraceComponent<Common::TraceTaskCodes::FaultAnalysisService>
        {
            using Federation::NodeTraceComponent<Common::TraceTaskCodes::FaultAnalysisService>::WriteNoise;
            using Federation::NodeTraceComponent<Common::TraceTaskCodes::FaultAnalysisService>::WriteInfo;
            using Federation::NodeTraceComponent<Common::TraceTaskCodes::FaultAnalysisService>::WriteWarning;
            using Federation::NodeTraceComponent<Common::TraceTaskCodes::FaultAnalysisService>::WriteError;

        public:
            virtual ~FaultAnalysisServiceAgent();

            static Common::ErrorCode Create(__out std::shared_ptr<FaultAnalysisServiceAgent> & result);

            __declspec(property(get=get_TraceId)) std::wstring const & TraceId;
            std::wstring const & get_TraceId() const { return NodeTraceComponent::TraceId; }

            virtual void Release();

            virtual void RegisterFaultAnalysisService(
                ::FABRIC_PARTITION_ID,
                ::FABRIC_REPLICA_ID,
                Api::IFaultAnalysisServicePtr const &,
                __out std::wstring & serviceAddress);

            virtual void UnregisterFaultAnalysisService(
                ::FABRIC_PARTITION_ID,
                ::FABRIC_REPLICA_ID);

            Common::ErrorCode ProcessQuery(
                Query::QueryNames::Enum queryName,
                ServiceModel::QueryArgumentMap const & queryArgs,
                Common::ActivityId const & activityId,
                __out Transport::MessageUPtr & reply);

            Common::TimeSpan GetRandomizedOperationRetryDelay(Common::ErrorCode const &) const;

            // Send the query from the FaultAnalysisServiceAgent to the service itself
            Common::AsyncOperationSPtr BeginSendQueryToService(
            FABRIC_TEST_COMMAND_STATE_FILTER stateFilter,
            FABRIC_TEST_COMMAND_TYPE_FILTER typeFilter,
                Common::ActivityId const & activityId,
                Common::TimeSpan timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            Common::ErrorCode EndSendQueryToService(
                Common::AsyncOperationSPtr const & operation,
                Transport::MessageUPtr & reply);

            Common::AsyncOperationSPtr BeginGetStoppedNodeList(
                Common::ActivityId const & activityId,
                Common::TimeSpan timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            Common::ErrorCode EndGetStoppedNodeList(
                Common::AsyncOperationSPtr const & operation,
                Transport::MessageUPtr & reply);

        private:
            class DispatchMessageAsyncOperationBase;
            class InvokeDataLossAsyncOperation;
            class GetInvokeDataLossProgressAsyncOperation;
            class InvokeQuorumLossAsyncOperation;
            class GetInvokeQuorumLossProgressAsyncOperation;
            class RestartPartitionAsyncOperation;
            class GetRestartPartitionProgressAsyncOperation;
            class CancelTestCommandAsyncOperation;
            class StartChaosAsyncOperation;
            class StopChaosAsyncOperation;
            class GetChaosReportAsyncOperation;
            class StartNodeTransitionAsyncOperation;
            class GetNodeTransitionProgressAsyncOperation;

            FaultAnalysisServiceAgent(Federation::NodeInstance const &, __in Transport::IpcClient &, Common::ComPointer<IFabricRuntime> const &);

            Common::ErrorCode Initialize();

            Common::AsyncOperationSPtr BeginProcessQuery(
                Query::QueryNames::Enum queryName,
                ServiceModel::QueryArgumentMap const & queryArgs,
                Common::ActivityId const & activityId,
                Common::TimeSpan timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            Common::ErrorCode EndProcessQuery(
                Common::AsyncOperationSPtr const & operation,
                Transport::MessageUPtr & reply);

            void ReplaceServiceLocation(SystemServices::SystemServiceLocation const & location);
            void DispatchMessage(Api::IFaultAnalysisServicePtr const &, __in Transport::MessageUPtr &, __in Transport::IpcReceiverContextUPtr &);
            void OnDispatchMessageComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

            void OnDispatchMessageComplete2(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

            SystemServices::ServiceRoutingAgentProxyUPtr routingAgentProxyUPtr_;
            std::list<SystemServices::SystemServiceLocation> registeredServiceLocations_;
            Common::ExclusiveLock lock_;

            Common::ComPointer<IFabricRuntime> runtimeCPtr_;

            Query::QueryMessageHandlerUPtr queryMessageHandler_;

            Api::IFaultAnalysisServicePtr servicePtr_;
        };
    }
}
