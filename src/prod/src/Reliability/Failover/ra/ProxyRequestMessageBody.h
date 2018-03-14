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
        class ProxyRequestMessageBody : public Serialization::FabricSerializable
        {
        public:

            ProxyRequestMessageBody() 
            {
            }

            ProxyRequestMessageBody(
                std::wstring const & runtimeId,
                Reliability::FailoverUnitDescription const & fudesc,
                Reliability::ReplicaDescription const & localReplica,
                Reliability::ServiceDescription const & service,
                ProxyMessageFlags::Enum flags) 
                : runtimeId_(runtimeId),
                  fudesc_(fudesc),
                  localReplica_(localReplica),
                  remoteReplicas_(),
                  flags_(flags),
                  service_(service)
            {
            }

            ProxyRequestMessageBody(
                std::wstring const & runtimeId,
                Reliability::FailoverUnitDescription const & fudesc,
                Reliability::ReplicaDescription const & localReplica,
                ProxyMessageFlags::Enum flags) 
                : runtimeId_(runtimeId),
                  fudesc_(fudesc),
                  localReplica_(localReplica),
                  remoteReplicas_(),
                  flags_(flags),
                  service_()
            {
            }

            ProxyRequestMessageBody(
                std::wstring const & runtimeId,
                Reliability::FailoverUnitDescription const & fudesc,
                Reliability::ServiceDescription const & serviceDesc,
                Reliability::ReplicaDescription const & localReplica,
                ProxyMessageFlags::Enum flags) 
                : runtimeId_(runtimeId),
                  fudesc_(fudesc),
                  localReplica_(localReplica),
                  remoteReplicas_(),
                  flags_(flags),
                  service_(serviceDesc)
            {
            }

            ProxyRequestMessageBody(
                Reliability::FailoverUnitDescription const & fudesc,
                Reliability::ReplicaDescription const & localReplica) 
                : fudesc_(fudesc),
                  localReplica_(localReplica),
                  remoteReplicas_(),
                  flags_(),
                  service_()
            {
            }

            ProxyRequestMessageBody(
                Reliability::FailoverUnitDescription const & fudesc,
                Reliability::ReplicaDescription const & localReplica,
                std::vector<Reliability::ReplicaDescription> && remoteReplicas,
                ProxyMessageFlags::Enum flags) 
                : fudesc_(fudesc),
                  localReplica_(localReplica),
                  remoteReplicas_(std::move(remoteReplicas)),
                  flags_(flags)
            {}

            ProxyRequestMessageBody(
                Reliability::FailoverUnitDescription const & fudesc,
                Reliability::ReplicaDescription const & localReplica,
                Reliability::ReplicaDescription const & remoteReplica) 
                : fudesc_(fudesc),
                  localReplica_(localReplica),
                  remoteReplicas_(),
                  flags_()
            {
                ReplicaDescription replica(remoteReplica);

                if (remoteReplica.CurrentConfigurationRole == Reliability::ReplicaRole::Secondary)
                {
                    // Build/Remove Secondary is always build/remove idle, from the perspective of
                    // RAP.
                    replica.CurrentConfigurationRole = Reliability::ReplicaRole::Idle;
                }

                remoteReplicas_.push_back(std::move(replica));
            }

            __declspec (property(get=get_RuntimeId)) std::wstring const & RuntimeId;
            std::wstring const & get_RuntimeId() const { return runtimeId_; }

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

            __declspec (property(get=get_ServiceDescription)) Reliability::ServiceDescription const & ServiceDescription;
            Reliability::ServiceDescription const & get_ServiceDescription() const { return service_; }

            __declspec (property(get=get_Flags)) ProxyMessageFlags::Enum const & Flags;
            ProxyMessageFlags::Enum  const & get_Flags() const { return flags_; }

            void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;
            void WriteToEtw(uint16 contextSequenceId) const;

            FABRIC_FIELDS_06(runtimeId_, fudesc_, localReplica_, remoteReplicas_, service_, flags_);

            void Test_SetServiceDescription(Reliability::ServiceDescription const & service)
            {
                service_ = service;
            }

        private:
            std::wstring runtimeId_;
            Reliability::FailoverUnitDescription fudesc_;
            Reliability::ReplicaDescription localReplica_;
            std::vector<Reliability::ReplicaDescription> remoteReplicas_;
            ProxyMessageFlags::Enum flags_;
            Reliability::ServiceDescription service_;
        };
    }
}
