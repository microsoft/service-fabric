// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    // Contains the FailoverUnit description that needs to be exchanged between FM-RA, RA-RA.
    class FailoverUnitMessageBody : public Serialization::FabricSerializable
    {
    public:

        FailoverUnitMessageBody()
        {}

        FailoverUnitMessageBody(
            Reliability::FailoverUnitDescription const & fudesc)
            : fudesc_(fudesc)
        {}

        __declspec (property(get=get_FailoverUnitDescription)) Reliability::FailoverUnitDescription const & FailoverUnitDescription;
        Reliability::FailoverUnitDescription const & get_FailoverUnitDescription() const { return fudesc_; }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;
        void WriteToEtw(uint16 contextSequenceId) const;

        FABRIC_FIELDS_01(fudesc_);

    private:

        Reliability::FailoverUnitDescription fudesc_;
    };
}
