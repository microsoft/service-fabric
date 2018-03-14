// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    namespace SequenceStreamState
    {
        enum Enum : int
        {
            Uninitialized = 0,
            PreInitializing = 1,
            PreInitialized = 2,
            Initialized = 3,

            LAST_STATE = Initialized
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

        DECLARE_ENUM_STRUCTURED_TRACE(SequenceStreamState);
    };

    // SequenceStream; used to manage sequence streams
    class HealthReportingComponent::SequenceStream : public Common::TextTraceComponent<Common::TraceTaskCodes::HealthClient>
    {
        DENY_COPY(SequenceStream);

    public:
        SequenceStream(
            HealthReportingComponent & owner,
            Management::HealthManager::SequenceStreamId && sequenceStreamId,
            std::wstring const & traceContext);

        ~SequenceStream();

        __declspec(property(get=get_Id)) Management::HealthManager::SequenceStreamId const& Id;
        Management::HealthManager::SequenceStreamId const& get_Id() const { return sequenceStreamId_; }

        __declspec(property(get=get_IsPreInitializing)) bool const IsPreInitializing;
        bool get_IsPreInitializing() const;

        __declspec (property(get=get_Instance)) FABRIC_INSTANCE_ID Instance;
        FABRIC_INSTANCE_ID get_Instance() const { return instance_; }

        void GetContent(
            int maxNumberOfReports,
            Common::TimeSpan const & sendRetryInterval,
            __inout std::vector<ServiceModel::HealthReport> & reports,
            __inout std::vector<Management::HealthManager::SequenceStreamInformation> & sequenceStreams);

        // Used by system services, typically FM to initialize the progress information for sequence stream
        Common::ErrorCode PreInitialize(FABRIC_INSTANCE_ID instance, __out long & updateCount);
        Common::ErrorCode PostInitialize(FABRIC_SEQUENCE_NUMBER sequence, FABRIC_SEQUENCE_NUMBER invalidateSequence);
        Common::ErrorCode GetProgress(__out FABRIC_SEQUENCE_NUMBER & progress) const;
        Common::ErrorCode HealthSkipSequence(FABRIC_SEQUENCE_NUMBER sequenceToSkip);

        Common::ErrorCode UpdateProgress(FABRIC_SEQUENCE_NUMBER const& progress);

        Common::ErrorCode ProcessReport(
            ServiceModel::HealthReport && healthReport,
            bool sendImmediately,
            __out long & updateCount);
        void ProcessReportResult(Management::HealthManager::HealthReportResult const & reportResult, __out long & updateCount);
        void ProcessSequenceResult(
            Management::HealthManager::SequenceStreamResult const& sequenceResult,
            Common::ActivityId const & activityId,
            bool isGetSequenceStreamProgress);

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        void WriteToEtw(uint16 contextSequenceId) const;

    private:
        void TransitionTo(SequenceStreamState::Enum state);
        void SetAckedRange(FABRIC_SEQUENCE_NUMBER & ackedFromLsn, FABRIC_SEQUENCE_NUMBER & ackedUpToLsn);
        void SetPendingRange(FABRIC_SEQUENCE_NUMBER & pendingFromLsn, FABRIC_SEQUENCE_NUMBER & pendingUpToLsn);
        void SetProgress(FABRIC_SEQUENCE_NUMBER & progress);

    private:
        // Inner class, defined in cpp. Hold pointer to it as a private member.
        class HealthReportsMap;
        using HealthReportsMapUPtr = std::unique_ptr<HealthReportsMap>;

    private:
        SequenceStreamState::Enum state_;

        // All the health reports that need to be processed by HealthManager
        HealthReportsMapUPtr healthReports_;
        
        Common::VersionRangeCollection sequenceRanges_;
        Common::VersionRangeCollection skippedRanges_;

        FABRIC_SEQUENCE_NUMBER ackedUpToLsn_;
        FABRIC_SEQUENCE_NUMBER initialLsn_;
        FABRIC_SEQUENCE_NUMBER invalidateSequence_;

        mutable Common::RwLock lock_;
        std::wstring traceContext_;

        Management::HealthManager::SequenceStreamId sequenceStreamId_;
        FABRIC_INSTANCE_ID instance_;
        HealthReportingComponent & owner_;

        std::wstring sequenceStreamIdString_;
    };
}
