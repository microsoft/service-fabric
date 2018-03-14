// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class ActivateMessageBody : public ConfigurationMessageBody
    {
    public:

        ActivateMessageBody()
        {}

        ActivateMessageBody(
            Reliability::FailoverUnitDescription const & fudesc,
            Reliability::ServiceDescription const & serviceDesc,
            std::vector<Reliability::ReplicaDescription> && replicas,
            Reliability::ReplicaDeactivationInfo const & deactivationInfo)
            : ConfigurationMessageBody(fudesc, serviceDesc, std::move(replicas)),            
            deactivationInfo_(deactivationInfo)
        {
        }

        __declspec(property(get = get_ReplicaDeactivationInfo)) Reliability::ReplicaDeactivationInfo const & DeactivationInfo;
        Reliability::ReplicaDeactivationInfo const & get_ReplicaDeactivationInfo() const { return deactivationInfo_; }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;
        void WriteToEtw(uint16 contextSequenceId) const;

        FABRIC_FIELDS_01(deactivationInfo_);

    private:
        Reliability::ReplicaDeactivationInfo deactivationInfo_;
    };
}

