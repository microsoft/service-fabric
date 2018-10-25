// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {

        class CommitJobItem
        {
            DENY_COPY(CommitJobItem);

        public:

            CommitJobItem();

            virtual ~CommitJobItem();

            virtual bool ProcessJob(FailoverManager & fm) = 0;

            virtual void SynchronizedProcess(FailoverManager & fm);

            virtual void Close(FailoverManager & fm);
        };

        typedef std::unique_ptr<CommitJobItem> CommitJobItemUPtr;

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        class FailoverUnitCommitJobItem : public CommitJobItem
        {
            DENY_COPY(FailoverUnitCommitJobItem);

        public:
            FailoverUnitCommitJobItem(
                LockedFailoverUnitPtr && failoverUnit,
                PersistenceState::Enum persistenceState,
                bool shouldUpdateLooupVersion,
                bool shouldReportHealth,
                std::vector<StateMachineActionUPtr> actions,
                bool isBackground);

            LockedFailoverUnitPtr & GetFailoverUnit()
            {
                return failoverUnit_;
            }

            void SetCommitError(Common::ErrorCode error);
            void SetCommitDuration(int64 commitDuration);

            bool ProcessJob(FailoverManager & failoverManager);
            void SynchronizedProcess(FailoverManager & failoverManager);
            void Close(FailoverManager & failoverManager);

        private:
            static bool ResumeBackgroundManager(FailoverManager & failoverManager);
            static bool ResumeProcessingQueue(FailoverManager & failoverManager);

            LockedFailoverUnitPtr failoverUnit_;

            PersistenceState::Enum persistenceState_;
            bool shouldUpdateLooupVersion_;
            bool shouldReportHealth_;

            std::vector<StateMachineActionUPtr> actions_;
            int replicaDifference_;
            Common::ErrorCode error_;
            int64 commitDuration_;
            bool isBackground_;
        };

        typedef std::unique_ptr<FailoverUnitCommitJobItem> FailoverUnitCommitJobItemUPtr;

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        class CreateServiceCommitJobItem : public CommitJobItem
        {
            DENY_COPY(CreateServiceCommitJobItem);

        public:

            CreateServiceCommitJobItem(
                Common::AsyncOperationSPtr const& operation,
                Common::ErrorCode error,
                int64 commitDuration,
                int64 plbDuration);

            bool ProcessJob(FailoverManager & fm) override;

        private:
            Common::AsyncOperationSPtr operation_;
            Common::ErrorCode error_;
            int64 commitDuration_;
            int64 plbDuration_;
        };

        typedef std::unique_ptr<CreateServiceCommitJobItem> CreateServiceCommitJobItemUPtr;

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        class RemoveServiceCommitJobItem : public CommitJobItem
        {
            DENY_COPY(RemoveServiceCommitJobItem);

        public:

            RemoveServiceCommitJobItem(
                LockedServiceInfo && lockedServiceInfo,
                int64 commitDuration);

            bool ProcessJob(FailoverManager & fm) override;

        private:

            LockedServiceInfo lockedServiceInfo_;
            int64 commitDuration_;
        };

        typedef std::unique_ptr<RemoveServiceCommitJobItem> RemoveServiceCommitJobItemUPtr;

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        class RemoveServiceTypeCommitJobItem : public CommitJobItem
        {
            DENY_COPY(RemoveServiceTypeCommitJobItem);

        public:

            RemoveServiceTypeCommitJobItem(
                LockedServiceType && lockedServiceType,
                int64 commitDuration);

            bool ProcessJob(FailoverManager & fm) override;

        private:

            LockedServiceType lockedServiceType_;
            int64 commitDuration_;
        };

        typedef std::unique_ptr<RemoveServiceTypeCommitJobItem> RemoveServiceTypeCommitJobItemUPtr;

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        class RemoveApplicationCommitJobItem : public CommitJobItem
        {
            DENY_COPY(RemoveApplicationCommitJobItem);

        public:

            RemoveApplicationCommitJobItem(
                LockedApplicationInfo && lockedApplicationInfo,
                int64 commitDuration,
                int64 plbDuration);

            bool ProcessJob(FailoverManager & fm) override;

        private:

            LockedApplicationInfo lockedApplicationInfo_;
            int64 commitDuration_;
            int64 plbDuration_;
        };

        typedef std::unique_ptr<RemoveApplicationCommitJobItem> RemoveApplicationCommitJobItemUPtr;

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        class RemoveNodeCommitJobItem : public CommitJobItem
        {
            DENY_COPY(RemoveNodeCommitJobItem);

        public:

            RemoveNodeCommitJobItem(
                LockedNodeInfo && lockedNodeInfo,
                int64 commitDuration);

            bool ProcessJob(FailoverManager & fm) override;

        private:

            LockedNodeInfo lockedNodeInfo_;
            int64 commitDuration_;
        };

        typedef std::unique_ptr<RemoveNodeCommitJobItem> RemoveNodeCommitJobItemUPtr;

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        class MarkServiceAsDeletedCommitJobItem : public CommitJobItem
        {
            DENY_COPY(MarkServiceAsDeletedCommitJobItem);

        public:
            MarkServiceAsDeletedCommitJobItem(const ServiceInfoSPtr & serviceInfo);

            bool ProcessJob(FailoverManager & fm) override;

        private:
            ServiceInfoSPtr serviceInfo_;
        };

        typedef std::unique_ptr<MarkServiceAsDeletedCommitJobItem> MarkServiceAsDeletedCommitJobItemUPtr;

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        class CommitJobQueue : public Common::JobQueue<CommitJobItemUPtr, FailoverManager>
        {
            DENY_COPY(CommitJobQueue);

        public:
            CommitJobQueue(FailoverManager & fm);
        };
    }
}
