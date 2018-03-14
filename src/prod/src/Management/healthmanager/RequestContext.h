// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        // Context that keeps track of the incoming requests that are pending completion.
        // Its main purpose is to keep track of the owner and notify when the processing completes.
        class RequestContext 
            : public Store::ReplicaActivityTraceComponent<Common::TraceTaskCodes::HealthManager>
        {
            DENY_COPY(RequestContext);
        public:
            RequestContext(RequestContext && other);
            RequestContext & operator = (RequestContext && other);

            virtual ~RequestContext();

            __declspec(property(get = get_Owner)) Common::AsyncOperationSPtr const & Owner;
            Common::AsyncOperationSPtr const & get_Owner() const { return owner_; }

            __declspec(property(get=get_OwnerRemainingTime)) Common::TimeSpan OwnerRemainingTime;
            Common::TimeSpan get_OwnerRemainingTime() const;
       
            __declspec(property(get=get_StartTime)) int64 StartTime;
            int64 get_StartTime() const { return startTime_; }

            __declspec(property(get = get_Priority)) ServiceModel::Priority::Enum Priority;
            virtual ServiceModel::Priority::Enum get_Priority() const = 0;

            __declspec(property(get = get_Error)) Common::ErrorCode const & Error;
            Common::ErrorCode const & get_Error() const { return error_; }

            __declspec(property(get = get_IsAcceptedForProcessing, put = set_IsAcceptedForProcessing)) int64 IsAcceptedForProcessing;
            int64 get_IsAcceptedForProcessing() const { return isAccepted_; }
            void set_IsAcceptedForProcessing(bool value) const { isAccepted_ = value; }

            void SetError(Common::ErrorCode const & value) const;
            void SetError(Common::ErrorCode && value) const;
            bool IsErrorSet() const { return error_.ReadValue() != Common::ErrorCodeValue::NotReady; }

            uint64 GetEstimatedSize() const;

            virtual void OnRequestCompleted() = 0;

        protected:
            RequestContext(
                Store::ReplicaActivityId const & replicaActivityId,
                Common::AsyncOperationSPtr const & owner);
            
            virtual size_t EstimateSize() const = 0;

        private:
            // Keep the async operation alive throughout the request processing.
            // This will ensure that the parent of the replicated store is kept alive.
            Common::AsyncOperationSPtr owner_;

            // Cache the timed owner to get the remaining time without having to cast
            Common::TimedAsyncOperation * timedOwner_;

            // The time at which the operation was started. Used to compute the total duration
            // when the request is completed.
            int64 startTime_;

            // The error set after processing the context.
            mutable Common::ErrorCode error_;

            mutable uint64 estimatedSize_;
            mutable bool isAccepted_;
        };
    }
}
