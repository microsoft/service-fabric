// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class DeployedApplicationHealthStateChunkList
        : public HealthStateChunkList
    {
        DENY_COPY(DeployedApplicationHealthStateChunkList)

    public:
        DeployedApplicationHealthStateChunkList();

        DeployedApplicationHealthStateChunkList(
            ULONG totalCount, 
            std::vector<DeployedApplicationHealthStateChunk> && items);

        DeployedApplicationHealthStateChunkList(DeployedApplicationHealthStateChunkList && other) = default;
        DeployedApplicationHealthStateChunkList & operator = (DeployedApplicationHealthStateChunkList && other) = default;

        virtual ~DeployedApplicationHealthStateChunkList();

        __declspec(property(get = get_Items)) std::vector<DeployedApplicationHealthStateChunk> const & Items;
        std::vector<DeployedApplicationHealthStateChunk> const & get_Items() const { return items_; }

        __declspec(property(get = get_Count)) size_t Count;
        size_t get_Count() const { return items_.size(); }

        void Add(DeployedApplicationHealthStateChunk && item) { items_.push_back(std::move(item)); }

        Common::ErrorCode FromPublicApi(FABRIC_DEPLOYED_APPLICATION_HEALTH_STATE_CHUNK_LIST const * publicDeployedApplicationHealthStateChunkList);

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_DEPLOYED_APPLICATION_HEALTH_STATE_CHUNK_LIST & publicDeployedApplicationHealthStateChunkList) const;

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
        std::vector<DeployedApplicationHealthStateChunk> items_;
    };
}

