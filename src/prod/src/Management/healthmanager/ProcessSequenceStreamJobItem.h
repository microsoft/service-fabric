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
        template <class TEntityId>
        class ProcessSequenceStreamJobItem
            : public IHealthJobItem
        {
            DENY_COPY(ProcessSequenceStreamJobItem);
            
        public:
             ProcessSequenceStreamJobItem(
                 __in HealthEntityCache<TEntityId> & healthEntityCache,
                 SequenceStreamRequestContext && context);

             virtual HealthJobItemKind::Enum get_Type() const override { return HealthJobItemKind::ProcessSequenceStream; }

             virtual ~ProcessSequenceStreamJobItem();

             virtual std::wstring const & get_JobId() const override { return HealthEntityKind::GetEntityTypeString(this->EntityKind); }
             
             virtual ServiceModel::Priority::Enum get_JobPriority() const override { return context_.Priority; }

             virtual Store::ReplicaActivityId const & get_ReplicaActivityId() const override { return context_.ReplicaActivityId; }

        protected:

            // Actual processing of the task, that starts work on the entity
            virtual void ProcessInternal(IHealthJobItemSPtr const & thisSPtr) override;

            // Called after the work is completed, on the same scheduling thread.
            virtual void OnComplete(Common::ErrorCode const & error) override;

            // Called to finish async work
            virtual void FinishAsyncWork() override;

        private:
            SequenceStreamRequestContext context_;
            HealthEntityCache<TEntityId> & healthEntityCache_;
        };
    }
}
