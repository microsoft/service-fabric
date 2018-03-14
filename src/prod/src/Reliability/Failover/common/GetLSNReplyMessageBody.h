// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class GetLSNReplyMessageBody : public ReplicaReplyMessageBody
    {
    public:

        GetLSNReplyMessageBody() : ReplicaReplyMessageBody()
        {}

        GetLSNReplyMessageBody(
            Reliability::FailoverUnitDescription const & fudesc,
            Reliability::ReplicaDescription const & replica,
            Reliability::ReplicaDeactivationInfo const & replicaDeactivationInfo,
            Common::ErrorCode const & errorCode)
            : ReplicaReplyMessageBody(fudesc, replica, errorCode),
            deactivationInfo_(replicaDeactivationInfo)
        {}

        __declspec(property(get = get_DeactivationInfo)) Reliability::ReplicaDeactivationInfo const & DeactivationInfo;
        Reliability::ReplicaDeactivationInfo const & get_DeactivationInfo() const { return deactivationInfo_; } 

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;
        void WriteToEtw(uint16 contextSequenceId) const;

        FABRIC_FIELDS_01(deactivationInfo_);

    private:
        Reliability::ReplicaDeactivationInfo deactivationInfo_;        
    };
}
