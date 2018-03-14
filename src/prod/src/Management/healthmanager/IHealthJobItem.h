// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class HealthManagerReplica;

        // Interface implemented by a job item. Used so we can define structured traces for the derived job items
        // without exposing internals to the manifest generator.
        class IHealthJobItem
            : public std::enable_shared_from_this<IHealthJobItem>
        {
            DENY_COPY(IHealthJobItem);

        public:
            explicit IHealthJobItem(HealthEntityKind::Enum entityKind);
            virtual ~IHealthJobItem();

            __declspec(property(get=get_CanExecuteOnStoreThrottled)) bool CanExecuteOnStoreThrottled;
            bool get_CanExecuteOnStoreThrottled() const { return state_ == EntityJobItemState::AsyncPending; }

            __declspec(property(get=get_SequenceNumber, put=set_SequenceNumber)) uint64 SequenceNumber;
            uint64 get_SequenceNumber() const { return sequenceNumber_; }
            void set_SequenceNumber(uint64 value) { sequenceNumber_ = value; }
            
            __declspec(property(get=get_EntityKind)) HealthEntityKind::Enum EntityKind;
            HealthEntityKind::Enum get_EntityKind() const { return entityKind_; }
            
            __declspec(property(get=get_JobId)) std::wstring const & JobId;
            virtual std::wstring const & get_JobId() const = 0;

            __declspec(property(get=get_JobPriority)) ServiceModel::Priority::Enum JobPriority;
            virtual ServiceModel::Priority::Enum get_JobPriority() const = 0;

            __declspec(property(get=get_ReplicaActivityId)) Store::ReplicaActivityId const & ReplicaActivityId;
            virtual Store::ReplicaActivityId const & get_ReplicaActivityId() const = 0;

            // The type of the job item, returned by the derived classes
            __declspec (property(get=get_Type)) HealthJobItemKind::Enum Type;
            virtual HealthJobItemKind::Enum get_Type() const = 0;

            __declspec (property(get=get_TypeString)) std::wstring const & TypeString;
            std::wstring const & get_TypeString() const { return HealthJobItemKind::GetTypeString(this->Type); }

            bool operator > (IHealthJobItem const & other) const;

            // Called by the JobQueue on a JobQueue thread.
            // Calls specific logic in derived class based on the current state.
            bool ProcessJob(IHealthJobItemSPtr const & thisSPtr, HealthManagerReplica & root);

            // Set the error and call derived classes to do specific post-processing 
            // Called by JobQueueManager, outside its lock
            void ProcessCompleteResult(Common::ErrorCode const & error);

            // Checks whether it can combine with the provided job item
            virtual bool CanCombine(IHealthJobItemSPtr const &) const { return false; }

            // Merges the inner work of another work item into this job item
            // Users should first check that the 2 items are compatible.
            virtual void Append(IHealthJobItemSPtr && other) { Common::Assert::CodingError("Append job items not supported: {0} vs {1}", *this, *other); }

            // ------------------------------------------------
            // Called by JobQueueManager, protected by its lock
            // Change internal state to mark the progress
            // ------------------------------------------------
            void OnWorkComplete();
            void OnAsyncWorkStarted();
            void OnAsyncWorkReadyToComplete(
                Common::AsyncOperationSPtr const & operation);
            Common::AsyncOperationSPtr OnWorkCanceled();
            
            virtual void WriteTo(__in Common::TextWriter&, Common::FormatOptions const &) const;
            virtual std::wstring ToString() const;
            virtual void WriteToEtw(uint16 contextSequenceId) const;

        protected:
            __declspec(property(get=get_State)) EntityJobItemState::Enum State;
            EntityJobItemState::Enum get_State() const { return state_; }
            
            __declspec(property(get=get_PendingAsyncWork)) Common::AsyncOperationSPtr const & PendingAsyncWork;
            Common::AsyncOperationSPtr const & get_PendingAsyncWork() const { return pendingAsyncOperation_; }

            // Actual processing of the task, that starts work on the entity
            virtual void ProcessInternal(IHealthJobItemSPtr const & thisSPtr) = 0;

            // Called after the work is completed, on the same scheduling thread.
            virtual void OnComplete(Common::ErrorCode const & error) = 0;

            // Called to finish async work
            virtual void FinishAsyncWork() = 0;

        private:
            HealthEntityKind::Enum entityKind_;
            EntityJobItemState::Enum state_;
            Common::AsyncOperationSPtr pendingAsyncOperation_;
            // Used by the scheduler
            uint64 sequenceNumber_;
        };
    }
}
