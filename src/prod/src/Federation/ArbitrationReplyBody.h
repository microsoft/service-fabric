// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class ArbitrationReplyBody : public Serialization::FabricSerializable
    {
    public:
        static int const Extended;
        static int const Strong;
        static int const Continuous;
        static int const Delayed;

        ArbitrationReplyBody();

        ArbitrationReplyBody(Common::TimeSpan subjectTTL, bool subjectReported);

        ArbitrationReplyBody(Common::TimeSpan subjectTTL, Common::TimeSpan monitorTTL, bool subjectReported, int flags);

        __declspec(property(get = get_SubjectTTL, put = set_SubjectTTL)) Common::TimeSpan SubjectTTL;
        Common::TimeSpan get_SubjectTTL() const { return subjectTTL_; }
        void set_SubjectTTL(Common::TimeSpan value) { subjectTTL_ = value; }

        __declspec(property(get = get_MonitorTTL, put = set_MonitorTTL)) Common::TimeSpan MonitorTTL;
        Common::TimeSpan get_MonitorTTL() const { return monitorTTL_; }
        void set_MonitorTTL(Common::TimeSpan value) { monitorTTL_ = value; }

        __declspec(property(get = get_SubjectReported, put = set_SubjectReported)) bool SubjectReported;
        bool get_SubjectReported() const { return subjectReported_; }
        void set_SubjectReported(bool result) { subjectReported_ = result; }

        __declspec(property(get = get_Flags, put = set_Flags)) int Flags;
        int get_Flags() const { return flags_; }
        void set_Flags(int result) { flags_ = result; }

        void Normalize();

        bool IsExtended() const;
        bool IsStrong() const;
        bool IsContinuous() const;
        bool IsDelayed() const;

        int CompareTo(ArbitrationReplyBody const & other) const;

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_04(subjectTTL_, subjectReported_, monitorTTL_, flags_);

    private:
        Common::TimeSpan subjectTTL_;
        bool subjectReported_;
        Common::TimeSpan monitorTTL_;
        int flags_;
    };

    class DelayedArbitrationReplyBody : public Serialization::FabricSerializable
    {
    public:
        DelayedArbitrationReplyBody();

        DelayedArbitrationReplyBody(
            LeaseWrapper::LeaseAgentInstance const & monitor,
            LeaseWrapper::LeaseAgentInstance const & subject,
            int64 monitorLeaseInstance,
            int64 subjectLeaseInstance,
            ArbitrationDecision::Enum decision,
            Common::TimeSpan subjectTTL,
            bool subjectReported,
            int flags);

        DelayedArbitrationReplyBody(
            ArbitrationRequestBody const & request,
            ArbitrationDecision::Enum decision,
            Common::TimeSpan subjectTTL,
            bool subjectReported,
            int flags);

        __declspec(property(get = get_Monitor)) LeaseWrapper::LeaseAgentInstance const & Monitor;
        __declspec(property(get = get_Subject)) LeaseWrapper::LeaseAgentInstance const & Subject;
        __declspec(property(get = get_MonitorLeaseInstance)) int64 MonitorLeaseInstance;
        __declspec(property(get = get_SubjectLeaseInstance)) int64 SubjectLeaseInstance;
        __declspec(property(get = get_Decision)) ArbitrationDecision::Enum Decision;
        __declspec(property(get = get_SubjectTTL, put = set_SubjectTTL)) Common::TimeSpan SubjectTTL;
        __declspec(property(get = get_SubjectReported)) bool SubjectReported;
        __declspec(property(get = get_Flags, put = set_Flags)) int Flags;

        LeaseWrapper::LeaseAgentInstance const & get_Monitor() const { return monitor_; }
        LeaseWrapper::LeaseAgentInstance const & get_Subject() const { return subject_; }
        int64 get_MonitorLeaseInstance() const { return monitorLeaseInstance_; }
        int64 get_SubjectLeaseInstance() const { return subjectLeaseInstance_; }
        ArbitrationDecision::Enum get_Decision() const { return decision_; }
        Common::TimeSpan get_SubjectTTL() const { return subjectTTL_; }
        bool get_SubjectReported() const { return subjectReported_; }
        int get_Flags() const { return flags_; }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_08(monitor_, subject_, monitorLeaseInstance_, subjectLeaseInstance_, decision_, subjectTTL_, subjectReported_, flags_);

    private:
        LeaseWrapper::LeaseAgentInstance monitor_;
        LeaseWrapper::LeaseAgentInstance subject_;
        int64 monitorLeaseInstance_;
        int64 subjectLeaseInstance_;
        ArbitrationDecision::Enum decision_;
        Common::TimeSpan subjectTTL_;
        bool subjectReported_;
        int flags_;
    };
}
