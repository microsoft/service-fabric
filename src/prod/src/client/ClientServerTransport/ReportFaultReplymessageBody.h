// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class ReportFaultReplyMessageBody : public Serialization::FabricSerializable
    {
    public:
        ReportFaultReplyMessageBody();

        ReportFaultReplyMessageBody(
            Common::ErrorCode error);

        __declspec(property(get=get_Error)) Common::ErrorCode Error;
        Common::ErrorCode get_Error() const { return error_; }

        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);

        void FillEventData(Common::TraceEventContext & context) const;
        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        void WriteToEtw(uint16 contextSequenceId) const;

        FABRIC_FIELDS_01(error_);

    private:
        Common::ErrorCode error_;
    };
}
