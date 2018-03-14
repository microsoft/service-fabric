// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class VersionedConsistencyUnitId 
        : public Serialization::FabricSerializable
        , public Common::ISizeEstimator
    {
    public:
        VersionedConsistencyUnitId()
            : version_(0)
            , cuid_()
        {
        }

        VersionedConsistencyUnitId(
            int64 version,
            ConsistencyUnitId const & cuid)
            : version_(version)
            , cuid_(cuid)
        {
        }

        __declspec (property(get=get_Version)) int64 Version;
        int64 const & get_Version() const { return version_; }

        __declspec (property(get=get_Cuid)) ConsistencyUnitId const & Cuid;
        ConsistencyUnitId const & get_Cuid() const { return cuid_; }

        FABRIC_FIELDS_02(version_, cuid_);

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(cuid_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        int64 version_;
        ConsistencyUnitId cuid_;
    };
}

DEFINE_USER_ARRAY_UTILITY(Reliability::VersionedConsistencyUnitId);
