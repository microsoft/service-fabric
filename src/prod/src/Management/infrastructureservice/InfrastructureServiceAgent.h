// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace InfrastructureService
    {
        class InfrastructureServiceAgent 
            : public Api::IInfrastructureServiceAgent
            , public Common::ComponentRoot
            , private Federation::NodeTraceComponent<Common::TraceTaskCodes::InfrastructureService>
        {
            using Federation::NodeTraceComponent<Common::TraceTaskCodes::InfrastructureService>::WriteNoise;
            using Federation::NodeTraceComponent<Common::TraceTaskCodes::InfrastructureService>::WriteInfo;
            using Federation::NodeTraceComponent<Common::TraceTaskCodes::InfrastructureService>::WriteWarning;
            using Federation::NodeTraceComponent<Common::TraceTaskCodes::InfrastructureService>::WriteError;

        public:
            virtual ~InfrastructureServiceAgent();

            static Common::ErrorCode Create(__out std::shared_ptr<InfrastructureServiceAgent> & result);

            __declspec(property(get=get_TraceId)) std::wstring const & TraceId;
            std::wstring const & get_TraceId() const { return NodeTraceComponent::TraceId; }

            virtual void Release();

            virtual Common::ErrorCode RegisterInfrastructureServiceFactory(
                IFabricStatefulServiceFactory * );

            virtual void RegisterInfrastructureService(
                ::FABRIC_PARTITION_ID,
                ::FABRIC_REPLICA_ID,
                Api::IInfrastructureServicePtr const &,  
                __out std::wstring & serviceAddress);

            virtual void UnregisterInfrastructureService(
                ::FABRIC_PARTITION_ID,
                ::FABRIC_REPLICA_ID);

            virtual Common::AsyncOperationSPtr BeginStartInfrastructureTask(
                ::FABRIC_INFRASTRUCTURE_TASK_DESCRIPTION * description,
                Common::TimeSpan const,
                Common::AsyncCallback const &, 
                Common::AsyncOperationSPtr const &);
            virtual Common::ErrorCode EndStartInfrastructureTask(
                Common::AsyncOperationSPtr const & asyncOperation);

            virtual Common::AsyncOperationSPtr BeginFinishInfrastructureTask(
                std::wstring const & taskId,
                uint64 instanceId,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & parent);
            virtual Common::ErrorCode EndFinishInfrastructureTask(
                Common::AsyncOperationSPtr const & asyncOperation);

            virtual Common::AsyncOperationSPtr BeginQueryInfrastructureTask(
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & parent);
            virtual Common::ErrorCode EndQueryInfrastructureTask(
                Common::AsyncOperationSPtr const & asyncOperation,
                __out ServiceModel::QueryResult &);

        private:
            class DispatchMessageAsyncOperationBase;
            class RunCommandAsyncOperation;
            class ReportTaskAsyncOperationBase;
            class ReportStartTaskSuccessAsyncOperation;
            class ReportFinishTaskSuccessAsyncOperation;
            class ReportTaskFailureAsyncOperation;

            class InfrastructureTaskAsyncOperationBase;
            class StartInfrastructureTaskAsyncOperation;
            class FinishInfrastructureTaskAsyncOperation;
            class QueryInfrastructureTaskAsyncOperation;

            InfrastructureServiceAgent(Federation::NodeInstance const &, __in Transport::IpcClient &, Common::ComPointer<IFabricRuntime> const &);

            Common::ErrorCode Initialize();

            void ReplaceServiceLocation(SystemServices::SystemServiceLocation const & location);
            void DispatchMessage(Api::IInfrastructureServicePtr const &, __in Transport::MessageUPtr &, __in Transport::IpcReceiverContextUPtr &);
            void OnDispatchMessageComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

            SystemServices::ServiceRoutingAgentProxyUPtr routingAgentProxyUPtr_;
            std::list<SystemServices::SystemServiceLocation> registeredServiceLocations_;
            Common::ExclusiveLock lock_;

            Common::ComPointer<IFabricRuntime> runtimeCPtr_;
        };
    }
}
