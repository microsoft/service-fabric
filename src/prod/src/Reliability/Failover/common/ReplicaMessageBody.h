// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    // Contains information about the replica that is being affected by the message.
    class ReplicaMessageBody : public Serialization::FabricSerializable
    {
    public:

        ReplicaMessageBody()
        {}

        ReplicaMessageBody(
            Reliability::FailoverUnitDescription const & fudesc,
            Reliability::ReplicaDescription const & replica,
            Reliability::ServiceDescription const & service)
            : fudesc_(fudesc),
              replica_(replica),
              service_(service)
        {}

        ReplicaMessageBody(
            Reliability::FailoverUnitDescription const & fudesc,
            Reliability::ReplicaDescription const & replica,
            Reliability::ServiceDescription const & service,
            ServiceModel::ServicePackageVersionInstance const & version)
            : fudesc_(fudesc),
              replica_(replica),
              service_(service, version)
        {
            replica_.PackageVersionInstance = version;
        }

        __declspec (property(get=get_FailoverUnitDescription)) Reliability::FailoverUnitDescription const & FailoverUnitDescription;
        Reliability::FailoverUnitDescription const & get_FailoverUnitDescription() const { return fudesc_; }

        __declspec (property(get=get_ReplicaDescription)) Reliability::ReplicaDescription const & ReplicaDescription;
        Reliability::ReplicaDescription const & get_ReplicaDescription() const { return replica_; }

        __declspec (property(get=get_ServiceDescription)) Reliability::ServiceDescription const & ServiceDescription;
        Reliability::ServiceDescription const & get_ServiceDescription() const { return service_; }

        virtual void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;
        virtual void WriteToEtw(uint16 contextSequenceId) const;

        FABRIC_FIELDS_03(fudesc_, replica_, service_);

    protected:
        Reliability::FailoverUnitDescription fudesc_;
        Reliability::ReplicaDescription replica_;
        Reliability::ServiceDescription service_;
    };
}
