// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    //
    // This class represents the range of lookup version for the service location transfer.
    //
    class VersionRange 
        : public Serialization::FabricSerializable
        , public Common::ISizeEstimator
    {
    public:
        VersionRange()
        {
        }

        VersionRange(int64 startVersion)
            : startVersion_(startVersion), endVersion_(startVersion + 1)
        {
        }

        VersionRange(int64 startVersion, int64 endVersion)
            : startVersion_(startVersion), endVersion_(endVersion)
        {
            ASSERT_IF(startVersion > endVersion, "StartVersion cannot be greater than EndVersion.");
        }

        // The 'inclusive' start of the version range.
        __declspec (property(get=get_StartVersion)) int64 StartVersion;
        int64 get_StartVersion() const { return startVersion_; }

        // The 'exclusive' end of the version range.
        __declspec (property(get=get_EndVersion)) int64 EndVersion;
        int64 get_EndVersion() const { return endVersion_; }

        __declspec (property(get=get_RangeSize)) int64 RangeSize;
        int64 get_RangeSize() const { return (endVersion_ - startVersion_); }

        void SetStartVersion(int64 value) { startVersion_ = value; }
        void SetEndVersion(int64 value) { endVersion_ = value; }

        bool Contains(int64 version) const
        {
            return (version >= startVersion_ && version < endVersion_);
        }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;
        void WriteToEtw(uint16 contextSequenceId) const;

        FABRIC_FIELDS_02(startVersion_, endVersion_);

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        int64 startVersion_;
        int64 endVersion_;
    };
}

DEFINE_USER_ARRAY_UTILITY(Common::VersionRange);
