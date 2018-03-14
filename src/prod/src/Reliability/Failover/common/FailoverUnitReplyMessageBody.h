// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    // Contains reply information about the FailoverUnit message exchanged between FM-RA, RA-RA.
    class FailoverUnitReplyMessageBody : public Serialization::FabricSerializable
    {
    public:

        FailoverUnitReplyMessageBody()
        {}

        FailoverUnitReplyMessageBody(
            Reliability::FailoverUnitDescription const & fudesc,
            Common::ErrorCode errorCode)
            : fudesc_(fudesc), errorCode_(errorCode)
        {}

        __declspec (property(get=get_FailoverUnitDescription)) Reliability::FailoverUnitDescription const & FailoverUnitDescription;
        Reliability::FailoverUnitDescription const & get_FailoverUnitDescription() const { return fudesc_; }

        __declspec (property(get=get_ErrorCode)) Common::ErrorCode const ErrorCode;
        Common::ErrorCode const get_ErrorCode() const { return errorCode_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
        void WriteToEtw(uint16 contextSequenceId) const;

        FABRIC_FIELDS_02(fudesc_, errorCode_);

    private:

        Reliability::FailoverUnitDescription fudesc_;
        Common::ErrorCode errorCode_;
    };
}
