// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ReplicaHealthStateChunkList
        : public HealthStateChunkList
    {
        DENY_COPY(ReplicaHealthStateChunkList)

    public:
        ReplicaHealthStateChunkList();

        ReplicaHealthStateChunkList(
            ULONG totalCount, 
            std::vector<ReplicaHealthStateChunk> && items);

        ReplicaHealthStateChunkList(ReplicaHealthStateChunkList && other) = default;
        ReplicaHealthStateChunkList & operator = (ReplicaHealthStateChunkList && other) = default;

        virtual ~ReplicaHealthStateChunkList();

        __declspec(property(get = get_Items)) std::vector<ReplicaHealthStateChunk> const & Items;
        std::vector<ReplicaHealthStateChunk> const & get_Items() const { return items_; }

        void Add(ReplicaHealthStateChunk && item) { items_.push_back(std::move(item)); }

        __declspec(property(get = get_Count)) size_t Count;
        size_t get_Count() const { return items_.size(); }

        Common::ErrorCode FromPublicApi(FABRIC_REPLICA_HEALTH_STATE_CHUNK_LIST const * publicReplicaHealthStateChunkList);

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_REPLICA_HEALTH_STATE_CHUNK_LIST & publicReplicaHealthStateChunkList) const;

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
        std::vector<ReplicaHealthStateChunk> items_;
    };
}

