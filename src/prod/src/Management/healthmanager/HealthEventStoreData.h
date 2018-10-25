// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class HealthEventStoreData : public Store::StoreData
        {
            DENY_COPY(HealthEventStoreData)

        public:
            HealthEventStoreData();

            // First event, no history transition
            HealthEventStoreData(
                ServiceModel::HealthInformation const & healthInfo,
                ServiceModel::Priority::Enum priority,
                Store::ReplicaActivityId const & activityId);

            HealthEventStoreData(HealthEventStoreData && other);
            HealthEventStoreData & operator = (HealthEventStoreData && other);
            
            virtual ~HealthEventStoreData();
            
            __declspec (property(get=get_SourceId)) std::wstring const & SourceId;
            std::wstring const & get_SourceId() const { return sourceId_; }

            __declspec (property(get=get_Property)) std::wstring const & Property;
            std::wstring const & get_Property() const { return property_; }

            __declspec (property(get=get_State, put=set_State)) FABRIC_HEALTH_STATE State;
            FABRIC_HEALTH_STATE get_State() const { return state_; }
            void set_State(FABRIC_HEALTH_STATE value) { state_ = value; }
            
            __declspec (property(get=get_LastModifiedUtc)) Common::DateTime LastModifiedUtc;
            Common::DateTime get_LastModifiedUtc() const;

            __declspec (property(get=get_SourceUtcTimestamp)) Common::DateTime SourceUtcTimestamp;
            Common::DateTime get_SourceUtcTimestamp() const;
            
            __declspec (property(get=get_TimeToLive)) Common::TimeSpan TimeToLive;
            Common::TimeSpan get_TimeToLive() const { return timeToLive_; }

            __declspec (property(get=get_Description)) std::wstring const & Description;
            std::wstring const & get_Description() const { return description_; }
            
            __declspec (property(get=get_PersistedReportSequenceNumber)) int64 PersistedReportSequenceNumber;
            int64 get_PersistedReportSequenceNumber() const { return reportSequenceNumber_; }
            
            __declspec (property(get=get_ReportSequenceNumber)) int64 ReportSequenceNumber;
            int64 get_ReportSequenceNumber() const;

            __declspec (property(get=get_RemoveWhenExpired)) bool RemoveWhenExpired;
            bool get_RemoveWhenExpired() const { return removeWhenExpired_; }

            // Returns already computed value of isExpired. Note that UpdateExpired should have been called previously
            // at the time the check was needed (since it depends on DateTime::Now).
            __declspec (property(get=get_IsExpired)) bool IsExpired;
            bool get_IsExpired() const { return cachedIsExpired_; }

            __declspec (property(get=get_IsPendingUpdateToStore, put=set_IsPendingUpdateToStore)) bool IsPendingUpdateToStore;
            bool get_IsPendingUpdateToStore() const { return isPendingUpdateToStore_; }
            void set_IsPendingUpdateToStore(bool value) { isPendingUpdateToStore_ = value; }

            __declspec (property(get = get_IsInStore, put = set_IsInStore)) bool IsInStore;
            bool get_IsInStore() const { return isInStore_; }
            void set_IsInStore(bool value) { isInStore_ = value; }

            __declspec (property(get=get_IsSystemReport)) bool IsSystemReport;
            bool get_IsSystemReport() const;

            __declspec (property(get=get_IsStatePropertyError)) bool IsStatePropertyError;
            bool get_IsStatePropertyError() const;

            __declspec(property(get=get_Priority)) ServiceModel::Priority::Enum Priority;
            ServiceModel::Priority::Enum get_Priority() const;

            // Used by automatic cleanup. Remove user events that can't affect health as mitigation for the situation when an entity has accumulated more events than desired.
            bool DoesNotImpactHealth() const;

            // Create new event based on diff to be inserted into store
            virtual void GetDiffEvent(Store::ReplicaActivityId const & replicaActivityId, __inout HealthEventStoreDataUPtr & newEvent) const = 0;

            // Gets the evaluated health state based on the policy
            FABRIC_HEALTH_STATE GetEvaluatedHealthState(bool considerWarningAsError) const;

            // Checks whether cached expired value (computed previously on UpdateExpired) is different than persisted isExpired flag.
            bool NeedsToBePersistedToStore() const { return cachedIsExpired_ != persistedIsExpired_; }

            // Checks whether the event is transient and expired.
            // Uses previous cached value of expired, set by UpdateExpired.
            bool IsTransientExpiredEvent() const { return removeWhenExpired_ && cachedIsExpired_; }

            // After inconsistency between diff and current have been persisted to disk, move diff into current
            // Must be protected with writer lock
            void MoveDiffToCurrent(Store::ReplicaActivityId const & replicaActivityId);

            // Update event with new event and maintain the state transition history
            void Update(HealthEventStoreData && other);
                        
            virtual void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;
            virtual void WriteToEtw(uint16 contextSequenceId) const;

            bool CanUpdateEvent(
                FABRIC_SEQUENCE_NUMBER reportSequenceNumber) const;

            bool HasSameFields(ServiceModel::HealthReport const & report) const;

            bool TryUpdateDiff(ServiceModel::HealthReport const & report);

            ServiceModel::HealthEvent GenerateEvent() const;
                        
            static std::wstring GeneratePrefix(std::wstring const & entityId);

            bool UpdateExpired();

            // After load, if an event is not expired, ignore the persisted last modified time and 
            // use load time
            void UpdateOnLoadFromStore(FABRIC_HEALTH_REPORT_KIND healthInfoKind);

        protected:
            HealthEventStoreData(
                HealthEventStoreData const & previousValue,
                Store::ReplicaActivityId const & activityId);

        private:
            std::wstring GetTransitionHistory() const;

        protected:
            // Maintains a diff from a base health event.
            class HealthEventDiff
            {
                DENY_COPY(HealthEventDiff)
            public:
                HealthEventDiff(Common::DateTime const sourceUtcTimestamp, FABRIC_SEQUENCE_NUMBER reportSequenceNumber);
                ~HealthEventDiff();

                __declspec (property(get = get_LastModifiedUtc)) Common::DateTime LastModifiedUtc;
                Common::DateTime get_LastModifiedUtc() const { return lastModifiedUtc_; }

                __declspec (property(get = get_SourceUtcTimestamp)) Common::DateTime SourceUtcTimestamp;
                Common::DateTime get_SourceUtcTimestamp() const { return sourceUtcTimestamp_; }

                __declspec (property(get = get_ReportSequenceNumber)) int64 ReportSequenceNumber;
                int64 get_ReportSequenceNumber() const { return reportSequenceNumber_; }

                void Update(ServiceModel::HealthReport const & report);

            private:
                // Time stamp when the health manager received the event DIFF and updated it.
                // On load from store, represents the load time.
                Common::DateTime lastModifiedUtc_;

                // Source time stamp when the event is generated at the source (the health client time stamp)
                Common::DateTime sourceUtcTimestamp_;

                // Increasing sequence number used to order events received from the same source instance of the entity (optional parameter)
                FABRIC_SEQUENCE_NUMBER reportSequenceNumber_;
            };

            using HealthEventDiffUPtr = std::unique_ptr<HealthEventDiff>;

        protected:
            
            // Unique identifier of the reporting component.
            // Text provided by the reporting component to identify itself
            std::wstring sourceId_;

            // User defined categorization of the health event
            std::wstring property_;

            // Health state of the component
            FABRIC_HEALTH_STATE state_;

            // Timestamp when the health manager received the event and updated it
            Common::DateTime lastModifiedUtc_;

            // Source timestamp when the event is generated at the source (the health client time stamp)
            Common::DateTime sourceUtcTimestamp_;
            
            // The time the health event is considered valid.
            Common::TimeSpan timeToLive_;

            // Detail description of the event
            std::wstring description_;
                        
            // Increasing sequence number used to order events received from the same source instance of the entity (optional parameter)
            FABRIC_SEQUENCE_NUMBER reportSequenceNumber_;

            // Shows whether the report should be ignored after expiration
            bool removeWhenExpired_;
              
            // Saved to store to maintain expired flag after HM primary change. 
            // Updated on cleanup timer.
            bool persistedIsExpired_;

            //
            // COMPUTED FIELDS, not saved to store
            //

            // Keep a cached value of isExpired so the caller can access it with reader lock only.
            volatile bool cachedIsExpired_;

            // Used for transient events, to remember if they were removed from store
            bool isPendingUpdateToStore_;

            // Fields that show history of the event
            Common::DateTime lastOkTransitionAt_;
            Common::DateTime lastWarningTransitionAt_;
            Common::DateTime lastErrorTransitionAt_;

            bool isInStore_;

            // Diff to the latest event received but not saved to store.
            HealthEventDiffUPtr diff_;

            // Caches priority to avoid string comparisons
            mutable ServiceModel::Priority::Enum priority_;
        };
    }
}
