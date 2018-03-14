// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    //
    // Contains the fields necessary to construct a FailoverUnit and its Replicas.
    //
    class FailoverUnitInfo : public Serialization::FabricSerializable
    {
    DEFAULT_COPY_CONSTRUCTOR(FailoverUnitInfo)
    public:
        FailoverUnitInfo();

        FailoverUnitInfo(
            Reliability::ServiceDescription const & serviceDescription,
            Reliability::FailoverUnitDescription const & failoverUnitDescription,
            Reliability::Epoch const & icEpoch,
            std::vector<Reliability::ReplicaInfo> && replicas);

        FailoverUnitInfo(
            Reliability::ServiceDescription const & serviceDescription,
            Reliability::FailoverUnitDescription const & failoverUnitDescription,
            Reliability::Epoch const & icEpoch,
            bool isReportFromPrimary,
            bool isLocalReplicaDeleted,
            std::vector<Reliability::ReplicaInfo> && replicas);

        FailoverUnitInfo(FailoverUnitInfo && other);

        FailoverUnitInfo & operator = (FailoverUnitInfo && other);

        __declspec (property(get=get_ServiceDescription, put=set_ServiceDescription)) Reliability::ServiceDescription & ServiceDescription;
        Reliability::ServiceDescription const& get_ServiceDescription() const { return serviceDescription_; }
        void set_ServiceDescription(Reliability::ServiceDescription const& value) { serviceDescription_ = value; }
        
        __declspec (property(get=get_FailoverUnitDescription)) Reliability::FailoverUnitDescription const & FailoverUnitDescription;
        Reliability::FailoverUnitDescription const& get_FailoverUnitDescription() const { return failoverUnitDescription_; }

        __declspec(property(get=get_FailoverUnitId)) Reliability::FailoverUnitId const FailoverUnitId;
        Reliability::FailoverUnitId const get_FailoverUnitId() const { return failoverUnitDescription_.FailoverUnitId; }

        __declspec (property(get=get_ReplicaCount)) size_t ReplicaCount;
        size_t get_ReplicaCount() const { return replicas_.size(); }

        // Note: it is the responsibility of the caller to ensure ReplicaCount > 0 before accessing this property
        __declspec (property(get=get_LocalReplicaInfo)) Reliability::ReplicaInfo const & LocalReplicaInfo;
        Reliability::ReplicaInfo const & get_LocalReplicaInfo() const { return replicas_[0]; }

        __declspec (property(get=get_LocalReplica)) Reliability::ReplicaDescription const & LocalReplica;
        Reliability::ReplicaDescription const & get_LocalReplica() const { return LocalReplicaInfo.Description; }

        __declspec (property(get=get_Replicas)) std::vector<Reliability::ReplicaInfo> const & Replicas;
        std::vector<Reliability::ReplicaInfo> const& get_Replicas() const { return replicas_; }

        __declspec (property(get=get_CCEpoch)) Epoch const & CCEpoch;
        Epoch const& get_CCEpoch() const { return failoverUnitDescription_.CurrentConfigurationEpoch; }

        __declspec (property(get=get_ICEpoch)) Epoch const & ICEpoch;
        Epoch const& get_ICEpoch() const { return icEpoch_; }

        __declspec (property(get=get_PCEpoch)) Epoch const & PCEpoch;
        Epoch const& get_PCEpoch() const { return failoverUnitDescription_.PreviousConfigurationEpoch; }

        __declspec (property(get=get_IsReportFromPrimary)) bool IsReportFromPrimary;
        bool get_IsReportFromPrimary() const { return isReportFromPrimary_; }

        __declspec(property(get = get_IsLocalReplicaDeleted)) bool IsLocalReplicaDeleted;
        bool get_IsLocalReplicaDeleted() const { return isLocalReplicaDeleted_; }

        void AssertInvariants() const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
        void WriteToEtw(uint16 contextSequenceId) const;

        FABRIC_FIELDS_06(serviceDescription_, failoverUnitDescription_, icEpoch_, isReportFromPrimary_, replicas_, isLocalReplicaDeleted_);

    private:
        bool IsDroppedOrIdleReplica() const;

        Reliability::ServiceDescription serviceDescription_;
        Reliability::FailoverUnitDescription failoverUnitDescription_;
        Reliability::Epoch icEpoch_;
        bool isReportFromPrimary_;
        std::vector<Reliability::ReplicaInfo> replicas_;

        // used by RA to indicate to FM that the replica on the RA is deleted
        // This is just a notification (which FM should ack)
        // But FM should not use this for rebuild
        bool isLocalReplicaDeleted_;
    };
}

DEFINE_USER_ARRAY_UTILITY(Reliability::FailoverUnitInfo);
