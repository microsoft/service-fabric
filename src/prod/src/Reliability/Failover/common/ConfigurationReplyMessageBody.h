// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    // Contains reply information about the configuration of the FailoverUnit
    class ConfigurationReplyMessageBody : public Serialization::FabricSerializable
    {
    public:

        ConfigurationReplyMessageBody()
        {}

        ConfigurationReplyMessageBody(
            Reliability::FailoverUnitDescription const & fudesc,
            std::vector<Reliability::ReplicaDescription> && replicas,
            Common::ErrorCode errorCode) 
            : fudesc_(fudesc), replicas_(std::move(replicas)), errorCode_(errorCode)
        {}

        __declspec (property(get=get_FailoverUnitDescription)) Reliability::FailoverUnitDescription const & FailoverUnitDescription;
        Reliability::FailoverUnitDescription const & get_FailoverUnitDescription() const { return fudesc_; }

        __declspec (property(get=get_ReplicaDescriptions)) std::vector<Reliability::ReplicaDescription> const & ReplicaDescriptions;
        std::vector<Reliability::ReplicaDescription> const & get_ReplicaDescriptions() const { return replicas_; }

        __declspec (property(get=get_ReplicaCount)) size_t ReplicaCount;
        size_t get_ReplicaCount() const { return replicas_.size(); }

        __declspec (property(get=get_ErrorCode)) Common::ErrorCode const ErrorCode;
        Common::ErrorCode const get_ErrorCode() const { return errorCode_; }

        ReplicaDescription const* GetReplica(Federation::NodeId nodeId) const;

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;
        void WriteToEtw(uint16 contextSequenceId) const;

        FABRIC_FIELDS_03(fudesc_, replicas_, errorCode_);

    private:

        Reliability::FailoverUnitDescription fudesc_;
        std::vector<Reliability::ReplicaDescription> replicas_;
        Common::ErrorCode errorCode_;
    };
}
