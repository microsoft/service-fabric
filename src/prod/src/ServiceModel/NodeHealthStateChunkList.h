// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class NodeHealthStateChunkList
        : public HealthStateChunkList
    {
        DENY_COPY(NodeHealthStateChunkList)

    public:
        NodeHealthStateChunkList();

        NodeHealthStateChunkList(
            ULONG totalCount, 
            std::vector<NodeHealthStateChunk> && items);

        NodeHealthStateChunkList(NodeHealthStateChunkList && other) = default;
        NodeHealthStateChunkList & operator = (NodeHealthStateChunkList && other) = default;

        virtual ~NodeHealthStateChunkList();

        __declspec(property(get = get_Items)) std::vector<NodeHealthStateChunk> const & Items;
        std::vector<NodeHealthStateChunk> const & get_Items() const { return items_; }

        __declspec(property(get = get_Count)) size_t Count;
        size_t get_Count() const { return items_.size(); }

        void Add(NodeHealthStateChunk && item) { items_.push_back(std::move(item)); }

        Common::ErrorCode FromPublicApi(FABRIC_NODE_HEALTH_STATE_CHUNK_LIST const * publicNodeHealthStateChunkList);

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap,
            __inout FABRIC_NODE_HEALTH_STATE_CHUNK_LIST & publicNodeHealthStateChunkList) const;

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
        std::vector<NodeHealthStateChunk> items_;
    };
}

