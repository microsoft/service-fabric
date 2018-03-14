// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Upgrade
        {
            class FabricUpgrade : public IUpgrade
            {
                DENY_COPY(FabricUpgrade);

            public:
                FabricUpgrade(
                    std::wstring const & activityId, 
                    Common::FabricCodeVersion const & nodeVersion,
                    ServiceModel::FabricUpgradeSpecification const & upgradeSpecification, 
                    bool isNodeUp, 
                    ReconfigurationAgent & ra);

                __declspec(property(get=get_UpgradeSpecification)) ServiceModel::FabricUpgradeSpecification const & UpgradeSpecification;
                ServiceModel::FabricUpgradeSpecification const & get_UpgradeSpecification() const { return upgradeSpecification_; }

                void SendReply();

                UpgradeStateName::Enum GetStartState(RollbackSnapshotUPtr && rollbackSnapshot);

                UpgradeStateDescription const & GetStateDescription(UpgradeStateName::Enum state) const;

                UpgradeStateName::Enum EnterState(UpgradeStateName::Enum state, AsyncStateActionCompleteCallback asyncCallback);

                Common::AsyncOperationSPtr EnterAsyncOperationState(
                    UpgradeStateName::Enum state,
                    Common::AsyncCallback const & asyncCallback);

                UpgradeStateName::Enum ExitAsyncOperationState(
                    UpgradeStateName::Enum state,
                    Common::AsyncOperationSPtr const & asyncOp);

                std::wstring const & GetActivityId() const
                {
                    return activityId_;
                }

                void WriteToEtw(uint16 contextSequenceId) const;
                void WriteTo(Common::TextWriter &, Common::FormatOptions const &) const;

                RollbackSnapshotUPtr CreateRollbackSnapshot(UpgradeStateName::Enum state) const;

            private:
                UpgradeStateName::Enum Rollback(AsyncStateActionCompleteCallback callback);
                Common::AsyncOperationSPtr Download(Common::AsyncCallback const & callback);
                UpgradeStateName::Enum EndDownload(Common::AsyncOperationSPtr const & op);
                Common::AsyncOperationSPtr Validate(Common::AsyncCallback const & callback);
                UpgradeStateName::Enum EndValidate(Common::AsyncOperationSPtr const & op);
                
                Common::AsyncOperationSPtr CloseReplicas(Common::AsyncCallback const & callback);
                UpgradeStateName::Enum EndCloseReplicas(Common::AsyncOperationSPtr const & op);

                Common::AsyncOperationSPtr Upgrade(Common::AsyncCallback const & callback);
                UpgradeStateName::Enum EndUpgrade(Common::AsyncOperationSPtr const & op);

                UpgradeStateName::Enum UpdateEntitiesOnCodeUpgrade(AsyncStateActionCompleteCallback callback);

                bool RollbackFTProcessor(Infrastructure::HandlerParameters & handlerParameters, JobItemContextBase & context) const;
                void OnRollbackCompleted(Infrastructure::MultipleEntityWork const & work, AsyncStateActionCompleteCallback callback);

                bool SendCloseMessageFTProcessor(Infrastructure::HandlerParameters & handlerParameters, JobItemContextBase & context) const;
                void OnCloseMessagesSent(Infrastructure::MultipleEntityWork const & work, AsyncStateActionCompleteCallback callback);

                bool UpdateEntitiesOnCodeUpgradeProcessor(Infrastructure::HandlerParameters & handlerParameters, JobItemContextBase & context) const;
                void OnUpdateEntitiesOnCodeUpgradeCompleted(Infrastructure::MultipleEntityWork const & work, AsyncStateActionCompleteCallback callback);

                bool const isNodeUp_;
                Common::FabricCodeVersion const nodeVersion_;
                ServiceModel::FabricUpgradeSpecification const upgradeSpecification_;
                std::wstring const activityId_;

                bool shouldRestartReplica_;
            };
        }
    }
}

