// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class ServiceTableRequestMessageBody : public Serialization::FabricSerializable
    {
    public:
        ServiceTableRequestMessageBody()
        {
        }

        ServiceTableRequestMessageBody(
            GenerationNumber const & generationNumber,
            Common::VersionRangeCollection && versionRangeCollection,
            std::vector<ConsistencyUnitId> && consistencyUnitIds)
            : generationNumber_(generationNumber),
              versionRangeCollection_(std::move(versionRangeCollection)),
              consistencyUnitIds_(std::move(consistencyUnitIds))
        {
        }

        __declspec(property(get=get_generationNumber)) GenerationNumber const & Generation;
        GenerationNumber const & get_generationNumber() const { return generationNumber_; }

        __declspec (property(get=get_VersionRangeCollection)) Common::VersionRangeCollection const& VersionRangeCollection;
        Common::VersionRangeCollection const& get_VersionRangeCollection() const { return versionRangeCollection_; }

        __declspec (property(get=get_ConsistencyUnitIds)) std::vector<ConsistencyUnitId> const& ConsistencyUnitIds;
        std::vector<ConsistencyUnitId> const& get_ConsistencyUnitIds() const { return consistencyUnitIds_; }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
        {
            w.WriteLine("{0} {1}", generationNumber_, versionRangeCollection_);

            for (ConsistencyUnitId const& consistencyUnitId : consistencyUnitIds_)
            {
                w.WriteLine(consistencyUnitId);
            }
        }

        FABRIC_FIELDS_03(generationNumber_, versionRangeCollection_, consistencyUnitIds_);

    private:
        Reliability::GenerationNumber generationNumber_; 
        Common::VersionRangeCollection versionRangeCollection_;
        std::vector<ConsistencyUnitId> consistencyUnitIds_;
    };
}
