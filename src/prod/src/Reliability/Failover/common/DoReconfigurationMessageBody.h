// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class DoReconfigurationMessageBody : public ConfigurationMessageBody
    {
    public:
        DoReconfigurationMessageBody() :
            phase0Duration_(Common::TimeSpan::MaxValue)
        {}

        DoReconfigurationMessageBody(
            Reliability::FailoverUnitDescription const & fudesc,
            Reliability::ServiceDescription const & serviceDesc,
            std::vector<Reliability::ReplicaDescription> && replicas) :
            ConfigurationMessageBody(fudesc, serviceDesc, std::move(replicas)),
            phase0Duration_(Common::TimeSpan::MaxValue)
        {
        }

        DoReconfigurationMessageBody(
            Reliability::FailoverUnitDescription const & fudesc,
            Reliability::ServiceDescription const & serviceDesc,
            std::vector<Reliability::ReplicaDescription> && replicas,
            Common::TimeSpan phase0Duration) : 
            ConfigurationMessageBody(fudesc, serviceDesc, std::move(replicas)),
            phase0Duration_(phase0Duration)
        {
        }

        __declspec(property(get = get_Phase0Duration)) Common::TimeSpan Phase0Duration;
        Common::TimeSpan get_Phase0Duration() const { return phase0Duration_; }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;
        void WriteToEtw(uint16 contextSequenceId) const;

        FABRIC_FIELDS_01(phase0Duration_);

    private:
        Common::TimeSpan phase0Duration_;
    };
}
