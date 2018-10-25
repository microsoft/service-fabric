// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ApplicationHealthStateChunkList
        : public HealthStateChunkList
    {
        DENY_COPY(ApplicationHealthStateChunkList)

    public:
        ApplicationHealthStateChunkList();

        ApplicationHealthStateChunkList(
            ULONG totalCount, 
            std::vector<ApplicationHealthStateChunk> && items);

        ApplicationHealthStateChunkList(ApplicationHealthStateChunkList && other) = default;
        ApplicationHealthStateChunkList & operator = (ApplicationHealthStateChunkList && other) = default;

        virtual ~ApplicationHealthStateChunkList();

        __declspec(property(get = get_Items)) std::vector<ApplicationHealthStateChunk> const & Items;
        std::vector<ApplicationHealthStateChunk> const & get_Items() const { return items_; }

        __declspec(property(get = get_Count)) size_t Count;
        size_t get_Count() const { return items_.size(); }

        void Add(ApplicationHealthStateChunk && item) { items_.push_back(std::move(item)); }

        Common::ErrorCode FromPublicApi(FABRIC_APPLICATION_HEALTH_STATE_CHUNK_LIST const * publicApplicationHealthStateChunkList);

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_APPLICATION_HEALTH_STATE_CHUNK_LIST & publicApplicationHealthStateChunkList) const;

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
        std::vector<ApplicationHealthStateChunk> items_;
    };
}

