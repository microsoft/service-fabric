// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class ArbitrationRequestBody : public Serialization::FabricSerializable
    {
    public:
        ArbitrationRequestBody();

        ArbitrationRequestBody(
            LeaseWrapper::LeaseAgentInstance const & monitor,
            LeaseWrapper::LeaseAgentInstance const & subject,
            Common::TimeSpan subjectTTL,
            Common::TimeSpan historyNeeded,
            int64 monitorLeaseInstance,
            int64 subjectLeaseInstance,
            int64 extraData,
            ArbitrationType::Enum type);

        __declspec(property(get = get_Monitor)) LeaseWrapper::LeaseAgentInstance const & Monitor;
        __declspec(property(get = get_Subject)) LeaseWrapper::LeaseAgentInstance const & Subject;
        __declspec(property(get = get_SubjectTTL)) Common::TimeSpan SubjectTTL;
        __declspec(property(get = get_HistoryNeeded)) Common::TimeSpan HistoryNeeded;
        __declspec(property(get = get_MonitorLeaseInstance)) int64 MonitorLeaseInstance;
        __declspec(property(get = get_SubjectLeaseInstance)) int64 SubjectLeaseInstance;
        __declspec(property(get = get_Type)) ArbitrationType::Enum Type;
        __declspec(property(get = get_ImplicitArbitration1)) int ImplicitArbitration1;
        __declspec(property(get = get_ImplicitArbitration2)) int ImplicitArbitration2;

        inline LeaseWrapper::LeaseAgentInstance const & get_Monitor() const { return monitor_; }
        inline LeaseWrapper::LeaseAgentInstance const & get_Subject() const { return subject_; }
        inline Common::TimeSpan get_SubjectTTL() const { return subjectTTL_; }
        inline Common::TimeSpan get_HistoryNeeded() const { return historyNeeded_; }
        inline int64 get_MonitorLeaseInstance() const { return monitorLeaseInstance_; }
        inline int64 get_SubjectLeaseInstance() const { return subjectLeaseInstance_; }
        inline ArbitrationType::Enum get_Type() const { return type_; }
        inline int get_ImplicitArbitration1() const { return (extraData_ & 0x0f); }
        inline int get_ImplicitArbitration2() const { return ((extraData_ >> 4) & 0x0f); }

        void Normalize();

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_08(monitor_, subject_, subjectTTL_, historyNeeded_, monitorLeaseInstance_, subjectLeaseInstance_, extraData_, type_);

    private:
        LeaseWrapper::LeaseAgentInstance monitor_;
        LeaseWrapper::LeaseAgentInstance subject_;
        Common::TimeSpan subjectTTL_;
        Common::TimeSpan historyNeeded_;
        int64 monitorLeaseInstance_;
        int64 subjectLeaseInstance_;
        int64 extraData_;
        ArbitrationType::Enum type_;
    };
}
