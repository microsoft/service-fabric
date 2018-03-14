// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    // Contains reply information about the replica that is being affected by the message.
    class ReplicaReplyMessageBody : public Serialization::FabricSerializable
    {
    public:

        ReplicaReplyMessageBody()
        {}

        ReplicaReplyMessageBody(
            Reliability::FailoverUnitDescription const & fudesc,
            Reliability::ReplicaDescription const & replica,
            Common::ErrorCode const & errorCode) 
            : fudesc_(fudesc), replica_(replica), errorCode_(errorCode)
        {}

        __declspec (property(get=get_FailoverUnitDescription)) Reliability::FailoverUnitDescription const & FailoverUnitDescription;
        Reliability::FailoverUnitDescription const & get_FailoverUnitDescription() const { return fudesc_; }

        __declspec (property(get=get_ReplicaDescription)) Reliability::ReplicaDescription const & ReplicaDescription;
        Reliability::ReplicaDescription const & get_ReplicaDescription() const { return replica_; }

        __declspec (property(get=get_ErrorCode)) Common::ErrorCode const ErrorCode;
        Common::ErrorCode const get_ErrorCode() const { return errorCode_; }

        virtual void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;
        virtual void WriteToEtw(uint16 contextSequenceId) const;

        FABRIC_FIELDS_03(fudesc_, replica_, errorCode_);

    protected:

        Reliability::FailoverUnitDescription fudesc_;
        Reliability::ReplicaDescription replica_;
        Common::ErrorCode errorCode_;
    };
}
