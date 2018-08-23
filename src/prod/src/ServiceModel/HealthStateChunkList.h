// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class HealthStateChunkList
        : public Common::IFabricJsonSerializable
        , public Serialization::FabricSerializable
        , public Common::ISizeEstimator
    {
        DENY_COPY(HealthStateChunkList)

    public:
        HealthStateChunkList();

        explicit HealthStateChunkList(ULONG totalCount);
        
        HealthStateChunkList(HealthStateChunkList && other) = default;
        HealthStateChunkList & operator = (HealthStateChunkList && other) = default;

        virtual ~HealthStateChunkList();

        __declspec(property(get=get_TotalCount,put=set_TotalCount)) ULONG TotalCount;
        ULONG get_TotalCount() const { return totalCount_; }
        void set_TotalCount(ULONG value) { totalCount_ = value; }

        FABRIC_FIELDS_01(totalCount_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::TotalCount, totalCount_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        BEGIN_BASE_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(totalCount_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        ULONG totalCount_;
    };
}

