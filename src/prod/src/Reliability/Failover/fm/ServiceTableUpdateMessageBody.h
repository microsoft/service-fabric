// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class ServiceTableUpdateMessageBody : public Serialization::FabricSerializable
    {
    public:
        ServiceTableUpdateMessageBody()
        {
        }

        ServiceTableUpdateMessageBody(
            std::vector<ServiceTableEntry> && serviceTableEntries,
            GenerationNumber const & generation,
            Common::VersionRangeCollection && versionRangeCollection,
            int64 endVersion,
            bool isFromFMM)
            :serviceTableEntries_(std::move(serviceTableEntries)),
            generation_(generation),
            versionRangeCollection_(std::move(versionRangeCollection)),
            endVersion_(endVersion),
            isFromFMM_(isFromFMM)
        {
        }

        __declspec (property(get=get_Generation)) GenerationNumber const & Generation;
        GenerationNumber const & get_Generation() const { return generation_; }

        __declspec (property(get=get_ServiceTableEntries)) std::vector<ServiceTableEntry> const& ServiceTableEntries;
        std::vector<ServiceTableEntry> const& get_ServiceTableEntries() const { return serviceTableEntries_; }

        __declspec (property(get=get_VersionRangeCollection)) Common::VersionRangeCollection const& VersionRangeCollection;
        Common::VersionRangeCollection const& get_VersionRangeCollection() const { return versionRangeCollection_; }

        __declspec (property(get=get_EndVersion)) int64 EndVersion;
        int64 get_EndVersion() const { return endVersion_; }

        __declspec (property(get=get_IsFromFMM)) bool IsFromFMM;
        bool get_IsFromFMM() const { return isFromFMM_; }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
        {
            w.Write(
                "Generation={0}, Entries={1}, VersionRanges={2}, EndVersion={3}, IsFromFMM={4}",
                generation_, serviceTableEntries_.size(), versionRangeCollection_, endVersion_, isFromFMM_);
        }

        void WriteToEtw(uint16 contextSequenceId) const;

        FABRIC_FIELDS_05(serviceTableEntries_, generation_, versionRangeCollection_, endVersion_, isFromFMM_);

    private:
        std::vector<ServiceTableEntry> serviceTableEntries_;
        GenerationNumber generation_;
        Common::VersionRangeCollection versionRangeCollection_;
        int64 endVersion_;
        bool isFromFMM_;
    };
}
