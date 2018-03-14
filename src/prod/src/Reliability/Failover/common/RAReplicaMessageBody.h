// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class RAReplicaMessageBody : public ReplicaMessageBody
    {
    public:
        RAReplicaMessageBody() : ReplicaMessageBody()
        {}

        RAReplicaMessageBody(
            Reliability::FailoverUnitDescription const & fuDesc,
            Reliability::ReplicaDescription const & replica,
            Reliability::ServiceDescription const & service,
            Reliability::ReplicaDeactivationInfo const & deactivationInfo)
            : ReplicaMessageBody(fuDesc, replica, service),
            deactivationInfo_(deactivationInfo)
        {
        }

        __declspec(property(get = get_DeactivationInfo)) Reliability::ReplicaDeactivationInfo const & DeactivationInfo;
        Reliability::ReplicaDeactivationInfo const & get_DeactivationInfo() const { return deactivationInfo_; } 

        void WriteToEtw(uint16 contextSequenceId) const;
        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

        FABRIC_FIELDS_01(deactivationInfo_);
    private:
        Reliability::ReplicaDeactivationInfo deactivationInfo_;
    };
}

