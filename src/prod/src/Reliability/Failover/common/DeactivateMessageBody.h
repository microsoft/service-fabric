// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class DeactivateMessageBody : public ConfigurationMessageBody
    {
    public:

        DeactivateMessageBody()
        {}

        DeactivateMessageBody(
            Reliability::FailoverUnitDescription const & fudesc,
            Reliability::ServiceDescription const & serviceDesc,
            std::vector<Reliability::ReplicaDescription> && replicas, 
            Reliability::ReplicaDeactivationInfo const & deactivationInfo,
            bool isForce = false)
            : ConfigurationMessageBody(fudesc, serviceDesc, std::move(replicas)),
              isForce_(isForce),
              deactivationInfo_(deactivationInfo)
        {
        }

        __declspec(property(get=get_IsForce)) bool IsForce;
        bool get_IsForce() const { return isForce_; }

        __declspec(property(get = get_ReplicaDeactivationInfo)) Reliability::ReplicaDeactivationInfo const & DeactivationInfo;
        Reliability::ReplicaDeactivationInfo const & get_ReplicaDeactivationInfo() const { return deactivationInfo_; } 

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;
        void WriteToEtw(uint16 contextSequenceId) const;

        FABRIC_FIELDS_02(isForce_, deactivationInfo_);

    private:

        bool isForce_;
        Reliability::ReplicaDeactivationInfo deactivationInfo_;
    };
}
