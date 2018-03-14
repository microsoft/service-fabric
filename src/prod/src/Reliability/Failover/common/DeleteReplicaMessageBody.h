// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class DeleteReplicaMessageBody : public ReplicaMessageBody
    {
    public:

        DeleteReplicaMessageBody()
            : ReplicaMessageBody()
            , isForce_(false)
        {}

        DeleteReplicaMessageBody(
            Reliability::FailoverUnitDescription const & fudesc,
            Reliability::ReplicaDescription const & replica,
            Reliability::ServiceDescription const & service)
            : ReplicaMessageBody(fudesc, replica, service)
            , isForce_(false)
        {}

        DeleteReplicaMessageBody(
            Reliability::FailoverUnitDescription const & fudesc,
            Reliability::ReplicaDescription const & replica,
            Reliability::ServiceDescription const & service,
            bool const isForce)
            : ReplicaMessageBody(fudesc, replica, service)
            , isForce_(isForce)
        {}

        DeleteReplicaMessageBody(
            Reliability::FailoverUnitDescription const & fudesc,
            Reliability::ReplicaDescription const & replica,
            Reliability::ServiceDescription const & service,
            bool const isForce,
            ServiceModel::ServicePackageVersionInstance const & version)
            : ReplicaMessageBody(fudesc, replica, service, version)
            , isForce_(isForce)
        {}

        __declspec (property(get=get_IsForce)) bool IsForce;
        bool get_IsForce() const { return isForce_; }

        virtual void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const override;
        virtual void WriteToEtw(uint16 contextSequenceId) const override;

        FABRIC_FIELDS_01(isForce_);

    protected:
        bool isForce_;
    };
}

