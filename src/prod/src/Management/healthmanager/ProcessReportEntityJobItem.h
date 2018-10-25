// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        // Job item that processes a context
        class ProcessReportEntityJobItem
            : public EntityJobItem
        {
            DENY_COPY(ProcessReportEntityJobItem);

        public:
            ProcessReportEntityJobItem(
                std::shared_ptr<HealthEntity> const & entity,
                ReportRequestContext && context);

            virtual HealthJobItemKind::Enum get_Type() const override { return HealthJobItemKind::ProcessReport; }

            virtual ~ProcessReportEntityJobItem();

            virtual Store::ReplicaActivityId const & get_ReplicaActivityId() const override;

            virtual ServiceModel::Priority::Enum get_JobPriority() const override;

            virtual bool CanCombine(IHealthJobItemSPtr const & other) const override;

            virtual void Append(IHealthJobItemSPtr && other) override;

            std::vector<ReportRequestContext> TakeContexts() { return std::move(contexts_); }

            std::vector<ReportRequestContext> & GetContextsReference() { return contexts_; }

        protected:
            // Actual processing of the task, that starts work on the entity
            virtual void ProcessInternal(IHealthJobItemSPtr const & thisSPtr);

            // Called after the work is completed, on the same scheduling thread.
            virtual void OnComplete(Common::ErrorCode const &) override;

            // Called to finish async work
            virtual void FinishAsyncWork();

        private:
            std::vector<ReportRequestContext> contexts_;
            bool isDeleteRequest_;
        };
    }
}
