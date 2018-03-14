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
            class ApplicationUpgrade : public IUpgrade
            {
                DENY_COPY(ApplicationUpgrade);
            public:
                ApplicationUpgrade(std::wstring && activityId, ServiceModel::ApplicationUpgradeSpecification const & upgradeSpec, ReconfigurationAgent & ra);

                __declspec(property(get=get_UpgradeSpecification)) ServiceModel::ApplicationUpgradeSpecification const & UpgradeSpecification;
                ServiceModel::ApplicationUpgradeSpecification const & get_UpgradeSpecification() const { return upgradeSpec_; }

                UpgradeStateName::Enum GetStartState(RollbackSnapshotUPtr && rollbackSnapshot);

                UpgradeStateDescription const & GetStateDescription(UpgradeStateName::Enum state) const ;

                UpgradeStateName::Enum EnterState(UpgradeStateName::Enum state, AsyncStateActionCompleteCallback asyncCallback);

                Common::AsyncOperationSPtr EnterAsyncOperationState(
                    UpgradeStateName::Enum state,
                    Common::AsyncCallback const & asyncCallback);

                UpgradeStateName::Enum ExitAsyncOperationState(
                    UpgradeStateName::Enum state,
                    Common::AsyncOperationSPtr const & asyncOp);

                void SendReply();
            
                std::wstring const & GetActivityId() const
                {
                    return activityId_;
                }
            
                void WriteToEtw(uint16 contextSequenceId) const;
                void WriteTo(Common::TextWriter &, Common::FormatOptions const &) const;

                RollbackSnapshotUPtr CreateRollbackSnapshot(UpgradeStateName::Enum state) const;

            private:
                UpgradeStateName::Enum EnumerateFTs(AsyncStateActionCompleteCallback asyncCallback);
                Common::AsyncOperationSPtr Download(Common::AsyncCallback const & callback);
                UpgradeStateName::Enum EndDownload(Common::AsyncOperationSPtr const &);
                UpgradeStateName::Enum Analyze();
                UpgradeStateName::Enum UpdateVersionsAndCloseIfNeeded(AsyncStateActionCompleteCallback callback);
                UpgradeStateName::Enum CloseCompletionCheck(AsyncStateActionCompleteCallback callback);
                UpgradeStateName::Enum ReplicaDownCompletionCheck(AsyncStateActionCompleteCallback callback);
                Common::AsyncOperationSPtr Upgrade(Common::AsyncCallback const & callback);
                UpgradeStateName::Enum EndUpgrade(Common::AsyncOperationSPtr const &);
                UpgradeStateName::Enum Finalize(AsyncStateActionCompleteCallback callback);

                Common::AsyncOperationSPtr WaitForCloseCompletion(Common::AsyncCallback const & callback);
                UpgradeStateName::Enum EndWaitForCloseCompletion(Common::AsyncOperationSPtr const &);

                Common::AsyncOperationSPtr WaitForDropCompletion(Common::AsyncCallback const & callback);
                UpgradeStateName::Enum EndWaitForDropCompletion(Common::AsyncOperationSPtr const &);

                // Methods related to ft job item processing
                bool EnumerateFTProcessor(Infrastructure::HandlerParameters & handlerParameters, ApplicationUpgradeEnumerateFTsJobItemContext & context) const;
                void OnEnumerationComplete(Infrastructure::MultipleEntityWork const & work, AsyncStateActionCompleteCallback callback);

                bool UpdateVersionAndCloseIfNeeded(Infrastructure::HandlerParameters & handlerParameters, ApplicationUpgradeUpdateVersionJobItemContext & context) const;
                void OnVersionsUpdated(Infrastructure::MultipleEntityWork const & work, AsyncStateActionCompleteCallback callback);

                bool ReplicaDownCompletionCheckProcessor(Infrastructure::HandlerParameters & handlerParameters, ApplicationUpgradeReplicaDownCompletionCheckJobItemContext & context) const;
                void OnReplicaDownCompletionCheckComplete(Infrastructure::MultipleEntityWork const & work, AsyncStateActionCompleteCallback callback);

                bool FinalizeUpgradeFTProcessor(Infrastructure::HandlerParameters & handlerParameters, JobItemContextBase & context) const;
                void OnUpgradeFinalized(Infrastructure::MultipleEntityWork const & work, AsyncStateActionCompleteCallback callback);

                void AddJobItemsForReplicaDownCompletionCheckWork(Infrastructure::MultipleEntityWorkSPtr const & work, Infrastructure::EntityEntryBaseList const & fts, Infrastructure::RAJobQueueManager::EntryJobItemList & list);

                MultipleReplicaCloseCompletionCheckAsyncOperation::Parameters CreateParameters();

                Hosting2::ApplicationDownloadSpecification CreateFilteredDownloadSpecification() const;
                ServiceModel::ApplicationUpgradeSpecification CreateFilteredUpgradeSpecification() const;

                std::wstring const activityId_;
                ServiceModel::ApplicationUpgradeSpecification const upgradeSpec_;

                // This list maintains all the fts that are being affected by this upgrade
                Infrastructure::EntityEntryBaseList ftsAffectedByUpgrade_;

                // This map contains information about all the packages that all the fts affected by this upgrade belong to
                // This is the filtered list that is used to pass to hosting in the download specification
                std::map<std::wstring, ServiceModel::RolloutVersion> packagesAffectedByUpgrade_;

                // This list maintains all the FTs that must be closed
                // For Persisted these are the ones the RA will restart after upgrade completes
                Infrastructure::EntityEntryBaseList ftsClosedByUpgrade_;

                // These are the FTs for which the service types are being deleted as part of this upgrade
                // The RA will wait till these FTs are Dropped from the Node
                // The FM will send the DeleteReplica message for each of these
                Infrastructure::EntityEntryBaseList ftsToBeDeleted_;
                Hosting2::IHostingSubsystem::AffectedRuntimesSet affectedRuntimes_;          
            };
        }
    }
}
