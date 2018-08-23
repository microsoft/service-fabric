// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class PartitionHealthStateChunkList
        : public HealthStateChunkList
    {
        DENY_COPY(PartitionHealthStateChunkList)

    public:
        PartitionHealthStateChunkList();

        PartitionHealthStateChunkList(
            ULONG totalCount, 
            std::vector<PartitionHealthStateChunk> && items);

        PartitionHealthStateChunkList(PartitionHealthStateChunkList && other) = default;
        PartitionHealthStateChunkList & operator = (PartitionHealthStateChunkList && other) = default;

        virtual ~PartitionHealthStateChunkList();

        __declspec(property(get = get_Items)) std::vector<PartitionHealthStateChunk> const & Items;
        std::vector<PartitionHealthStateChunk> const & get_Items() const { return items_; }

        __declspec(property(get = get_Count)) size_t Count;
        size_t get_Count() const { return items_.size(); }

        void Add(PartitionHealthStateChunk && item) { items_.push_back(std::move(item)); }

        Common::ErrorCode FromPublicApi(FABRIC_PARTITION_HEALTH_STATE_CHUNK_LIST const * publicPartitionHealthStateChunkList);

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_PARTITION_HEALTH_STATE_CHUNK_LIST & publicPartitionHealthStateChunkList) const;

        FABRIC_FIELDS_01(items_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_CHAIN()
            SERIALIZABLE_PROPERTY(Constants::Items, items_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_CHAIN()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(items_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        std::vector<PartitionHealthStateChunk> items_;
    };
}

