// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class UpdateServiceMessageBody : public Serialization::FabricSerializable
    {
    public:

        UpdateServiceMessageBody()
        {
        }

        UpdateServiceMessageBody(
            ServiceDescription const& serviceDescription,
            std::vector<ConsistencyUnitDescription> && addedCuids,
            std::vector<ConsistencyUnitDescription> && removedCuids)
            : serviceDescription_(serviceDescription)
            , addedCuids_(std::move(addedCuids))
            , removedCuids_(std::move(removedCuids))
        {
        }

        __declspec (property(get=get_ServiceDescription)) Reliability::ServiceDescription const& ServiceDescription;
        Reliability::ServiceDescription const& get_ServiceDescription() const { return serviceDescription_; }

        __declspec(property(get = get_AddedCuids)) std::vector<ConsistencyUnitDescription> AddedCuids;
        std::vector<ConsistencyUnitDescription> const& get_AddedCuids() const { return addedCuids_; }

        __declspec(property(get = get_RemovedCuids)) std::vector<ConsistencyUnitDescription> RemovedCuids;
        std::vector<ConsistencyUnitDescription> const& get_RemovedCuids() const { return removedCuids_; }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
        {
            w << serviceDescription_;
        }

        FABRIC_FIELDS_03(serviceDescription_, addedCuids_, removedCuids_);

    private:

        Reliability::ServiceDescription serviceDescription_;
        std::vector<ConsistencyUnitDescription> addedCuids_;
        std::vector<ConsistencyUnitDescription> removedCuids_;
    };
}
