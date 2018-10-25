// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        // Context that keeps track of the incoming reports that are pending completion.
        // When a report is processed, it must notify the requestor.
        class ReportRequestContext
            : public RequestContext
        {
            DENY_COPY(ReportRequestContext);

        public:
            ReportRequestContext(
                Store::ReplicaActivityId const & replicaActivityId,
                Common::AsyncOperationSPtr const & owner,
                __in IReportRequestContextProcessor & requestProcessor,
                uint64 requestId,
                ServiceModel::HealthReport && reportEntry,
                FABRIC_SEQUENCE_NUMBER sequenceStreamFromLsn,
                FABRIC_SEQUENCE_NUMBER sequenceStreamUpToLsn);

            ReportRequestContext(ReportRequestContext && other);
            ReportRequestContext & operator = (ReportRequestContext && other);

            virtual ~ReportRequestContext();

            __declspec(property(get = get_RequestId)) uint64 RequestId;
            uint64 get_RequestId() const { return requestId_; }

            __declspec(property(get = get_Report)) ServiceModel::HealthReport const & Report;
            ServiceModel::HealthReport const & get_Report() const { return report_; }

            __declspec(property(get = get_PreviousState, put = set_PreviousState)) FABRIC_HEALTH_STATE const & PreviousState;
            FABRIC_HEALTH_STATE const & get_PreviousState() const { return previousState_; }
            void set_PreviousState(FABRIC_HEALTH_STATE const &  value) { previousState_ = value; }

            __declspec(property(get = get_ReportKind)) FABRIC_HEALTH_REPORT_KIND ReportKind;
            FABRIC_HEALTH_REPORT_KIND get_ReportKind() const { return report_.Kind; }

            __declspec(property(get = get_Kind)) HealthEntityKind::Enum Kind;
            HealthEntityKind::Enum get_Kind() const { return HealthEntityKind::FromHealthInformationKind(report_.Kind); }

            __declspec(property(get = get_SequenceStreamFromLsn)) FABRIC_SEQUENCE_NUMBER SequenceStreamFromLsn;
            FABRIC_SEQUENCE_NUMBER get_SequenceStreamFromLsn() const { return sequenceStreamFromLsn_; }

            __declspec(property(get = get_SequenceStreamUpToLsn)) FABRIC_SEQUENCE_NUMBER SequenceStreamUpToLsn;
            FABRIC_SEQUENCE_NUMBER get_SequenceStreamUpToLsn() const { return sequenceStreamUpToLsn_; }

            virtual ServiceModel::Priority::Enum get_Priority() const override { return report_.ReportPriority; }

            bool IsExpired() const;
            bool CheckAndSetExpired() const;

            void OnRequestCompleted() override;

            // Methods that get information specific to a report kind.
            // Note that they must be called when the KIND matches.
            template<class TEntityId>
            TEntityId GetEntityId() const;

            virtual void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;
            virtual void WriteToEtw(uint16 contextSequenceId) const;

        protected:
            virtual size_t EstimateSize() const override;

        private:
            // The reported entry that needs to be processed
            ServiceModel::HealthReport report_;

            // Keep a reference to the request context processor.
            // It is kept alive by the shared ptr above.
            IReportRequestContextProcessor & requestProcessor_;

            // The index of the report inside the vector of requests.
            // Together with the activityId, creates an unique identifier.
            uint64 requestId_;

            // The staring LSN in the current range associated with sourceId+entityType
            // If invalid, no sequence stream is defined
            FABRIC_SEQUENCE_NUMBER sequenceStreamFromLsn_;
            FABRIC_SEQUENCE_NUMBER sequenceStreamUpToLsn_;

            // Tracks what the previous state of the health report with the same property, source, and description
            // was. For example, we can write some traces out only when the health report coming in changes the state.
            FABRIC_HEALTH_STATE previousState_;
        };
    }
}
