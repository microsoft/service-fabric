// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class ReplicaInfo : public Serialization::FabricSerializable
    {
    DEFAULT_COPY_CONSTRUCTOR(ReplicaInfo)
    public:
        ReplicaInfo()
        {
        }

        ReplicaInfo(
            ReplicaDescription const & replicaDescription,
            ReplicaRole::Enum icRole)
            : replicaDescription_(replicaDescription),
              icRole_(icRole)
        {
        }

        ReplicaInfo(
            ReplicaDescription && replicaDescription,
            ReplicaRole::Enum icRole)
            : replicaDescription_(std::move(replicaDescription)),
              icRole_(icRole)
        {
        }

        ReplicaInfo(ReplicaInfo && other)
            : replicaDescription_(std::move(other.replicaDescription_)),
              icRole_(other.icRole_)
        {
        }

        ReplicaInfo & operator = (ReplicaInfo && other)
        {
            if (this != &other)
            {
                replicaDescription_ = std::move(other.replicaDescription_);
                icRole_ = other.icRole_;
            }

            return *this;
        }

        __declspec(property(get=get_Description)) ReplicaDescription const & Description;
        ReplicaDescription const & get_Description() const { return replicaDescription_; }

        __declspec(property(get=get_ICRole)) ReplicaRole::Enum ICRole;
        ReplicaRole::Enum get_ICRole() const { return icRole_; }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
        {
            w.Write("{0}/{1}/{2} {3:s}",
                replicaDescription_.PreviousConfigurationRole,
                icRole_,
                replicaDescription_.CurrentConfigurationRole,
                replicaDescription_);
        }

        void WriteToEtw(uint16 contextSequenceId) const;

        FABRIC_FIELDS_02(replicaDescription_, icRole_);

    private:
        ReplicaDescription replicaDescription_;
        ReplicaRole::Enum icRole_;
    };
}

DEFINE_USER_ARRAY_UTILITY(Reliability::ReplicaInfo);
