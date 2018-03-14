// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        // Contains actions against a replica and information about the replica that is being affected by the message.
        class ProxyReplyMessageBody : public Serialization::FabricSerializable
        {
        public:
            ProxyReplyMessageBody()
            {}

            ProxyReplyMessageBody(
                Reliability::FailoverUnitDescription const & fudesc,
                ReplicaDescription const & localReplica,
                std::vector<Reliability::ReplicaDescription> && remoteReplicas,
                ProxyErrorCode && proxyErrorCode,
                ProxyMessageFlags::Enum flags = ProxyMessageFlags::None) : 
                fudesc_(fudesc),
                localReplica_(localReplica),
                remoteReplicas_(std::move(remoteReplicas)), 
                proxyErrorCode_(std::move(proxyErrorCode)),
                flags_(flags)
            {   
                proxyErrorCode_.ReadValue(); // Move constructor for error code does not transfer isRead
            }

            __declspec (property(get=get_FailoverUnitDescription)) Reliability::FailoverUnitDescription const & FailoverUnitDescription;
            Reliability::FailoverUnitDescription const & get_FailoverUnitDescription() const { return fudesc_; }

            __declspec (property(get=get_LocalReplicaDescription)) Reliability::ReplicaDescription const & LocalReplicaDescription;
            Reliability::ReplicaDescription const & get_LocalReplicaDescription() const { return localReplica_; }

            __declspec (property(get=get_RemoteReplicaDescription)) Reliability::ReplicaDescription const & RemoteReplicaDescription;
            Reliability::ReplicaDescription const & get_RemoteReplicaDescription() const {
                ASSERT_IFNOT(remoteReplicas_.size() == 1, "Cannot use single replica descriptor accessor when there is a list of descriptors.");

                return remoteReplicas_[0];
            }

            __declspec (property(get=get_RemoteReplicaDescriptions)) std::vector<Reliability::ReplicaDescription> const & RemoteReplicaDescriptions;
            std::vector<Reliability::ReplicaDescription> const & get_RemoteReplicaDescriptions() const { return remoteReplicas_; }

            __declspec(property(get = get_ErrorCode)) ProxyErrorCode const & ErrorCode;
            ProxyErrorCode const & get_ErrorCode() const { return proxyErrorCode_; }

            __declspec (property(get=get_Flags)) ProxyMessageFlags::Enum const & Flags;
            ProxyMessageFlags::Enum  const & get_Flags() const { return flags_; }

            void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;
            void WriteToEtw(uint16 contextSequenceId) const;

            FABRIC_FIELDS_05(fudesc_, localReplica_, remoteReplicas_, proxyErrorCode_, flags_);

        private:
            ProxyErrorCode proxyErrorCode_;
            Reliability::FailoverUnitDescription fudesc_;
            Reliability::ReplicaDescription localReplica_;
            std::vector<Reliability::ReplicaDescription> remoteReplicas_;
            ProxyMessageFlags::Enum flags_;
        };
    }
}
