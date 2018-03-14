// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace UpgradeOrchestrationService
    {
        class UpgradeOrchestrationServiceAgent
            : public Api::IUpgradeOrchestrationServiceAgent
            , public Common::ComponentRoot
            , private Federation::NodeTraceComponent<Common::TraceTaskCodes::UpgradeOrchestrationService>
        {
            using Federation::NodeTraceComponent<Common::TraceTaskCodes::UpgradeOrchestrationService>::WriteNoise;
            using Federation::NodeTraceComponent<Common::TraceTaskCodes::UpgradeOrchestrationService>::WriteInfo;
            using Federation::NodeTraceComponent<Common::TraceTaskCodes::UpgradeOrchestrationService>::WriteWarning;
            using Federation::NodeTraceComponent<Common::TraceTaskCodes::UpgradeOrchestrationService>::WriteError;

        public:
            virtual ~UpgradeOrchestrationServiceAgent();

            static Common::ErrorCode Create(__out std::shared_ptr<UpgradeOrchestrationServiceAgent> & result);

            __declspec(property(get=get_TraceId)) std::wstring const & TraceId;
            std::wstring const & get_TraceId() const { return NodeTraceComponent::TraceId; }

            virtual void Release();

            virtual void RegisterUpgradeOrchestrationService(
                ::FABRIC_PARTITION_ID,
                ::FABRIC_REPLICA_ID,
                Api::IUpgradeOrchestrationServicePtr const &,
                __out std::wstring & serviceAddress);

            virtual void UnregisterUpgradeOrchestrationService(
                ::FABRIC_PARTITION_ID,
                ::FABRIC_REPLICA_ID);

            Common::TimeSpan GetRandomizedOperationRetryDelay(Common::ErrorCode const &) const;

        private:
            class DispatchMessageAsyncOperationBase;
            class StartClusterConfigurationUpgradeOperation;
            class GetClusterConfigurationUpgradeStatusOperation;
            class GetClusterConfigurationOperation;
            class GetUpgradesPendingApprovalOperation;
            class StartApprovedUpgradesOperation;
            class GetUpgradeOrchestrationServiceStateOperation;

            UpgradeOrchestrationServiceAgent(Federation::NodeInstance const &, __in Transport::IpcClient &, Common::ComPointer<IFabricRuntime> const &);

            Common::ErrorCode Initialize();

            void ReplaceServiceLocation(SystemServices::SystemServiceLocation const & location);
            void DispatchMessage(Api::IUpgradeOrchestrationServicePtr const &, __in Transport::MessageUPtr &, __in Transport::IpcReceiverContextUPtr &);
            void OnDispatchMessageComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

            void OnDispatchMessageComplete2(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

            SystemServices::ServiceRoutingAgentProxyUPtr routingAgentProxyUPtr_;
            std::list<SystemServices::SystemServiceLocation> registeredServiceLocations_;
            Common::ExclusiveLock lock_;

            Common::ComPointer<IFabricRuntime> runtimeCPtr_;

            Api::IUpgradeOrchestrationServicePtr servicePtr_;
        };
    }
}
