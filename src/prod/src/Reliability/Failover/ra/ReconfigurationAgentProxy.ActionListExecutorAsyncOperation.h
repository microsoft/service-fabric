// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent 
    {
        class ReconfigurationAgentProxy::ActionListExecutorAsyncOperation : public Common::AsyncOperation
        {
            DENY_COPY(ActionListExecutorAsyncOperation);
            friend class ReconfigurationAgentProxy::ActionListExecutorReplicatorQueryAsyncOperation;
        public:
            ActionListExecutorAsyncOperation(
                ReconfigurationAgentProxy & owner,
                FailoverUnitProxyContext<ProxyRequestMessageBody> && context,
                ProxyActionsListTypes::Enum actionsList,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & parent,
                bool continueOnFailure = false) :
                owner_(owner),
                context_(std::move(context)),
                currentStep_(0),
                AsyncOperation(callback, parent),
                actionsList_(ProxyActionsList::GetActionsListByType(actionsList)),
                actionListName_(actionsList),
                continueOnFailure_(continueOnFailure),
                proxyErrorCode_(ProxyErrorCode::Success())
            {
                replyFuDesc_ = context_.Body.FailoverUnitDescription;
                replyLocalReplica_ = context_.Body.LocalReplicaDescription;
                replyRemoteReplicas_ = context_.Body.RemoteReplicaDescriptions;
            }

            virtual ~ActionListExecutorAsyncOperation() {}

            static Common::ErrorCode End(
                Common::AsyncOperationSPtr const & asyncOperation, 
                __out FailoverUnitProxyContext<ProxyRequestMessageBody>* & context,
                __out ProxyActionsListTypes::Enum & actionsList, 
                __out ProxyReplyMessageBody & reply);

            static void CreateAndStart(
                ReconfigurationAgentProxy & owner,
                FailoverUnitProxyContext<ProxyRequestMessageBody> && context,
                ProxyActionsListTypes::Enum actionsList,
                std::function<void(Common::AsyncOperationSPtr const &)> completionCallback,
                bool continueOnFailure = false);
            
        protected:
            void OnStart(Common::AsyncOperationSPtr const & thisSPtr);
               
        private:            
            void ProcessNextAction(Common::AsyncOperationSPtr const & thisSPtr);
            bool ProcessActionFailure(Common::AsyncOperationSPtr const & thisSPtr, ProxyErrorCode && failingErrorCode);

            ProxyErrorCode DoOpenInstance(Common::AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously);
            void OpenInstanceCompletedCallback(Common::AsyncOperationSPtr const & openInstanceAsyncOperation);
            ProxyErrorCode FinishOpenInstance(Common::AsyncOperationSPtr const & openInstanceAsyncOperation);

            ProxyErrorCode DoCloseInstance(Common::AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously);
            void CloseInstanceCompletedCallback(Common::AsyncOperationSPtr const & closeInstanceAsyncOperation);
            ProxyErrorCode FinishCloseInstance(Common::AsyncOperationSPtr const & closeInstanceAsyncOperation);
    
             // TODO: Make this sync when service Abort support is added
            ProxyErrorCode DoAbortInstance(Common::AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously);
            void AbortInstanceCompletedCallback(Common::AsyncOperationSPtr const & closeReplicaAsyncOperation);
            ProxyErrorCode FinishAbortInstance(Common::AsyncOperationSPtr const & closeReplicaAsyncOperation);

            ProxyErrorCode DoOpenReplica(Common::AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously);
            void OpenReplicaCompletedCallback(Common::AsyncOperationSPtr const & openReplicaAsyncOperation);
            ProxyErrorCode FinishOpenReplica(Common::AsyncOperationSPtr const & openReplicaAsyncOperation);

            ProxyErrorCode DoChangeReplicaRole(Common::AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously);
            void ChangeReplicaRoleCompletedCallback(Common::AsyncOperationSPtr const & changeRoleAsyncOperation);
            ProxyErrorCode FinishChangeReplicaRole(Common::AsyncOperationSPtr const & changeRoleAsyncOperation);

            ProxyErrorCode DoCloseReplica(Common::AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously);
            void CloseReplicaCompletedCallback(Common::AsyncOperationSPtr const & closeReplicaAsyncOperation);
            ProxyErrorCode FinishCloseReplica(Common::AsyncOperationSPtr const & closeReplicaAsyncOperation);

            // TODO: Make this sync when service Abort support is added
            ProxyErrorCode DoAbortReplica(Common::AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously);
            void AbortReplicaCompletedCallback(Common::AsyncOperationSPtr const & closeReplicaAsyncOperation);
            ProxyErrorCode FinishAbortReplica(Common::AsyncOperationSPtr const & closeReplicaAsyncOperation);

            ProxyErrorCode DoOpenReplicator(Common::AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously);
            void OpenReplicatorCompletedCallback(Common::AsyncOperationSPtr const & openReplicatorAsyncOperation);
            ProxyErrorCode FinishOpenReplicator(Common::AsyncOperationSPtr const & openReplicatorAsyncOperation);

            ProxyErrorCode DoChangeReplicatorRole(Common::AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously);
            void ChangeReplicatorRoleCompletedCallback(Common::AsyncOperationSPtr const & changeRoleAsyncOperation);
            ProxyErrorCode FinishChangeReplicatorRole(Common::AsyncOperationSPtr const & changeRoleAsyncOperation);

            ProxyErrorCode DoAbortReplicator(Common::AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously);
            
            ProxyErrorCode DoCloseReplicator(Common::AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously);
            void CloseReplicatorCompletedCallback(Common::AsyncOperationSPtr const & closeReplicatorAsyncOperation);
            ProxyErrorCode FinishCloseReplicator(Common::AsyncOperationSPtr const & closeReplicatorAsyncOperation);

            ProxyErrorCode DoReplicatorGetStatus(Common::AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously);
            ProxyErrorCode DoReplicatorGetStatusIfDataLoss(Common::AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously);

            virtual ProxyErrorCode DoReplicatorGetQuery(Common::AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously);

            ProxyErrorCode DoReplicatorUpdateConfiguration(UpdateConfigurationReason::Enum reason, Common::AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously);

            ProxyErrorCode DoReplicatorCatchupReplicas(CatchupType::Enum catchupType, Common::AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously);
            void ReplicatorCatchupReplicasCompletedCallback(Common::AsyncOperationSPtr const & catchupReplicasAsyncOperation);
            ProxyErrorCode FinishReplicatorCatchupReplicas(Common::AsyncOperationSPtr const & catchupReplicasAsyncOperation);

            ProxyErrorCode DoReplicatorCancelCatchupReplicas(Common::AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously);

            ProxyErrorCode DoReplicatorBuildIdleReplica(Common::AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously);
            void ReplicatorBuildIdleReplicaCompletedCallback(Common::AsyncOperationSPtr const & buildIdleReplicaAsyncOperation);
            ProxyErrorCode FinishReplicatorBuildIdleReplica(Common::AsyncOperationSPtr const & buildIdleReplicaAsyncOperation);
            
            ProxyErrorCode DoReplicatorRemoveIdleReplica(Common::AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously);

            ProxyErrorCode DoReplicatorReportAnyDataLoss(Common::AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously);
            void ReplicatorOnDataLossCompletedCallback(Common::AsyncOperationSPtr const & onDataLossAsyncOperation);
            ProxyErrorCode FinishReplicatorOnDataLoss(Common::AsyncOperationSPtr const & onDataLossAsyncOperation);

            ProxyErrorCode DoReplicatorUpdateEpoch(Common::AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously);
            void ReplicatorUpdateEpochCompletedCallback(Common::AsyncOperationSPtr const & replicatorUpdateEpochAsyncOperation);
            ProxyErrorCode FinishReplicatorUpdateEpoch(Common::AsyncOperationSPtr const & replicatorUpdateEpochAsyncOperation);

            ProxyErrorCode DoReconfigurationStarting(Common::AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously);
            ProxyErrorCode DoReconfigurationEnding(Common::AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously);

            ProxyErrorCode DoUpdateReadWriteStatus(Common::AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously);
            ProxyErrorCode DoUpdateServiceDescription(Common::AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously);
            ProxyErrorCode DoFinalizeDemoteToSecondary(Common::AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously);

            ProxyErrorCode DoOpenPartition(Common::AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously);
            ProxyErrorCode DoClosePartition(Common::AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously);

            ProxyActionsList actionsList_;
            ReconfigurationAgentProxy & owner_;
            FailoverUnitProxyContext<ProxyRequestMessageBody> context_;
            size_t currentStep_;
            FailoverUnitDescription replyFuDesc_;
            ReplicaDescription replyLocalReplica_;
            std::vector<ReplicaDescription> replyRemoteReplicas_;
            ProxyActionsListTypes::Enum actionListName_;
            bool continueOnFailure_;
            ProxyErrorCode proxyErrorCode_;
        }; 
    }
}
