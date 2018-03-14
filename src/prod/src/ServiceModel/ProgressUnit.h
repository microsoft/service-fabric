// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class ProgressUnit
    {
        DENY_COPY(ProgressUnit)

    public:

        explicit ProgressUnit(ServiceModel::ProgressUnitType::Enum itemType);
           
        void IncrementCompletedItems(size_t value);
        void IncrementTotalItems(size_t value);
       
        __declspec(property(get = get_ItemType)) ServiceModel::ProgressUnitType::Enum ItemType;
        ServiceModel::ProgressUnitType::Enum get_ItemType() const { return itemType_; }

        __declspec(property(get = get_CompletedItems)) uint64 CompletedItems;
        uint64 get_CompletedItems() const { return completedItems_.load(); }

        __declspec(property(get = get_TotalItems)) uint64 TotalItems;
        uint64 get_TotalItems() const { return totalItems_.load(); }

        __declspec(property(get = get_LastModifiedTimeInTicks)) uint64 LastModifiedTimeInTicks;
        uint64 get_LastModifiedTimeInTicks() const { return lastModifiedTimeInTicks_.load(); }

    private:

        ServiceModel::ProgressUnitType::Enum itemType_;
        Common::atomic_uint64 completedItems_;
        Common::atomic_uint64 totalItems_;
        Common::atomic_uint64 lastModifiedTimeInTicks_;;
    };

    typedef std::shared_ptr<ProgressUnit> ProgressUnitSPtr;
}
