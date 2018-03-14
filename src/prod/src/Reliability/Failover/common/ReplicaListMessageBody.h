// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class ReplicaListMessageBody : public Serialization::FabricSerializable
    {
    public:

        ReplicaListMessageBody() 
        {
        }

        ReplicaListMessageBody(std::map<FailoverUnitId, ReplicaDescription> && replicas)
            : replicas_(std::move(replicas))
        {
        }

        __declspec (property(get=get_Replicas)) std::map<FailoverUnitId, ReplicaDescription> const& Replicas;
        std::map<FailoverUnitId, ReplicaDescription> const& get_Replicas() const { return replicas_; }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
        {
            for (auto const& pair : replicas_)
            {
                w.Write("{0}: {1}", pair.first, pair.second);
            }
        }

        void WriteToEtw(uint16 contextSequenceId) const;

        FABRIC_FIELDS_01(replicas_);

    private:
        std::map<FailoverUnitId, ReplicaDescription> replicas_;
    };
}

DEFINE_USER_MAP_UTILITY(Reliability::FailoverUnitId, Reliability::ReplicaDescription);
