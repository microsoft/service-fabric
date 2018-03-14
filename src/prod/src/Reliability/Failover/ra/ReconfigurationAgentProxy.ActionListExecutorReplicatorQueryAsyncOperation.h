// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent 
    {
        class ReconfigurationAgentProxy::ActionListExecutorReplicatorQueryAsyncOperation : public ReconfigurationAgentProxy::ActionListExecutorAsyncOperation
        {
            DENY_COPY(ActionListExecutorReplicatorQueryAsyncOperation);

        public:
            ActionListExecutorReplicatorQueryAsyncOperation(
                ReconfigurationAgentProxy & owner,
                FailoverUnitProxyContext<ProxyRequestMessageBody> && context,
                ProxyActionsListTypes::Enum actionsList,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & parent,
                ServiceModel::DeployedServiceReplicaDetailQueryResult && fupQueryResult,
                bool continueOnFailure = false) :
                ActionListExecutorAsyncOperation(
                    owner,
                    std::move(context),
                    actionsList,
                    callback,
                    parent,
                    continueOnFailure),
                replicatorQueryResult_(),
                fupQueryResult_(std::move(fupQueryResult))
            {
                ASSERT_IF(fupQueryResult_.IsInvalid, "FUP Query result must be valid");
            }

            virtual ~ActionListExecutorReplicatorQueryAsyncOperation() {}


            static Common::ErrorCode End(
                Common::AsyncOperationSPtr const & asyncOperation,
                __out FailoverUnitProxyContext<ProxyRequestMessageBody>* & context,
                __out ProxyActionsListTypes::Enum & actionsList,
                __out ServiceModel::ReplicatorStatusQueryResultSPtr & replicatorQueryResult,
                __out ServiceModel::DeployedServiceReplicaDetailQueryResult &fupQueryResult);

        protected:
            virtual ProxyErrorCode DoReplicatorGetQuery(Common::AsyncOperationSPtr const & thisSPtr, __out bool & isCompletedSynchronously);

        private:
            bool isFUPQueryResultMoved_;
            ServiceModel::ReplicatorStatusQueryResultSPtr replicatorQueryResult_;
            ServiceModel::DeployedServiceReplicaDetailQueryResult fupQueryResult_;
        }; 
    }
}
