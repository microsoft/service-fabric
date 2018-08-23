// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class RolloutContext : public Store::StoreData
        {
        public:
            explicit RolloutContext(RolloutContextType::Enum);

            RolloutContext(
                RolloutContext const &);

            RolloutContext(
                RolloutContext &&);

            RolloutContext & operator=(
                RolloutContext &&);

            RolloutContext(
                RolloutContextType::Enum,
                Common::ComponentRoot const &,
                ClientRequestSPtr const &);

            RolloutContext(
                RolloutContextType::Enum,
                Store::ReplicaActivityId const &);

            __declspec (property(get=get_Status, put=set_Status)) RolloutStatus::Enum Status;
            __declspec (property(get=get_ContextType)) RolloutContextType::Enum ContextType;
            __declspec (property(get=get_IsComplete)) bool IsComplete;
            __declspec (property(get=get_IsPending)) bool IsPending;
            __declspec (property(get=get_IsDeleting)) bool IsDeleting;
            __declspec (property(get=get_IsFailed)) bool IsFailed;
            __declspec (property(get=get_IsUnknown)) bool IsUnknown;
            __declspec (property(get=get_IsConvertToForceDelete)) bool IsConvertToForceDelete;
            __declspec (property(get=get_Error)) Common::ErrorCode const & Error;
            __declspec (property(get=get_MapKey)) std::wstring const & MapKey;
            __declspec (property(get=get_ClientRequest, put=put_ClientRequest)) ClientRequestSPtr const ClientRequest;
            __declspec (property(get=get_OperationRetryStopwatch)) Common::Stopwatch & OperationRetryStopwatch;

            RolloutStatus::Enum get_Status() const { return status_; }
            RolloutContextType::Enum get_ContextType() const { return contextType_; }
            bool get_IsComplete() const { return (status_ == RolloutStatus::Completed); }
            bool get_IsPending() const { return (status_ == RolloutStatus::Pending); }
            bool get_IsDeleting() const { return (status_ == RolloutStatus::DeletePending); }
            bool get_IsFailed() const { return (status_ == RolloutStatus::Failed); }
            bool get_IsUnknown() const { return (status_ == RolloutStatus::Unknown); }
            virtual bool get_IsConvertToForceDelete() const { return false; }
            Common::ErrorCode const & get_Error() const { return error_; }
            std::wstring const & get_MapKey() const;
            ClientRequestSPtr const get_ClientRequest() const { return clientRequest_; }
            void put_ClientRequest(ClientRequestSPtr value) { clientRequest_ = value; }
            Common::Stopwatch & get_OperationRetryStopwatch() { return operationRetryStopwatch_; }

            virtual ~RolloutContext() { }
            virtual std::wstring const & get_Type() const = 0;
            virtual void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
            void StartOperationRetryStopwatch() { this->OperationRetryStopwatch.Start(); }
            virtual bool ShouldTerminateProcessing() { return false; }

            void ReInitializeContext(Common::ComponentRoot const &, ClientRequestSPtr const & = ClientRequestSPtr());

            __declspec (property(get=get_OperationTimeout)) Common::TimeSpan OperationTimeout;
            Common::TimeSpan get_OperationTimeout() const;
            bool TrySetOperationTimeout(Common::TimeSpan const);

            __declspec (property(get=get_CommunicationTimeout)) Common::TimeSpan CommunicationTimeout;
            Common::TimeSpan get_CommunicationTimeout() const;

            Common::ErrorCode InsertCompletedIfNotFound(Store::StoreTransaction const &);
            Common::ErrorCode UpdateStatus(Store::StoreTransaction const &, RolloutStatus::Enum status);
            Common::ErrorCode Refresh(Store::StoreTransaction const &);
            Common::ErrorCode Complete(Store::StoreTransaction const &);
            virtual Common::ErrorCode Fail(Store::StoreTransaction const &);
            virtual void OnFailRolloutContext() = 0;

            void CompleteClientRequest();
            void CompleteClientRequest(Common::ErrorCode const &);

            void SetRetryTimer(Common::ThreadpoolCallback const &, Common::TimeSpan const);

            void SetCompletionError(Common::ErrorCode const &);
            void IncrementTimeoutCount() { ++timeoutCount_; }
            size_t GetTimeoutCount() const { return timeoutCount_; }

            void ExternalFail() { isExternallyFailed_ = true; }
            bool IsExternallyFailed() { return isExternallyFailed_; }

            virtual Common::ErrorCode SwitchReplaceToCreate(Store::StoreTransaction const & storeTx);
            bool ShouldKeepInQueue() { return shouldKeepInQueue_; }

            FABRIC_FIELDS_02(status_, timeout_);

        protected:
            Common::ComponentRootSPtr replicaSPtr_;

        private:
            void set_Status(RolloutStatus::Enum status) { status_ = status; }

            RolloutStatus::Enum status_;
            RolloutContextType::Enum contextType_;

            mutable std::wstring cachedMapKey_;
            ClientRequestSPtr clientRequest_;
            Common::TimerSPtr retryTimer_;
            Common::ExclusiveLock timerLock_;
            mutable Common::TimeSpan timeout_;
            Common::ErrorCode error_;

            // Allows forcefully failing a context to drive
            // processing to failure.
            //
            bool isExternallyFailed_;

            // true - keep this RolloutContext in manager's queue even operation success
            // It is reserved to handle "Replacement" requests
            bool shouldKeepInQueue_;

            Common::Stopwatch operationRetryStopwatch_;
            size_t timeoutCount_;
        };
    }
}
