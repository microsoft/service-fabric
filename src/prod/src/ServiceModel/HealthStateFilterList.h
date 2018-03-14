// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    template <class TServiceModelEntry, class TPublicApi, class TPublicApiList>
    class HealthStateFilterList
        : public Common::IFabricJsonSerializable
        , public Serialization::FabricSerializable
    {
        DENY_COPY(HealthStateFilterList)

    public:
        HealthStateFilterList() : items_() {}

        HealthStateFilterList(std::vector<TServiceModelEntry> && items) 
            : items_(std::move(items)) 
        {
        }

        HealthStateFilterList(HealthStateFilterList && other) 
            : items_(std::move(other.items_))
        {
        }

        HealthStateFilterList & operator = (HealthStateFilterList && other)
        {
            if (this != &other)
            {
                this->items_ = std::move(other.items_);
            }

            return *this;
        }

        ~HealthStateFilterList() {}

        __declspec(property(get = get_Items)) std::vector<TServiceModelEntry> const & Items;
        std::vector<TServiceModelEntry> const & get_Items() const { return items_; }

        void Add(TServiceModelEntry && item) { items_.push_back(std::move(item)); }

        std::wstring ToJsonString() const
        {
            std::wstring objectString;
            auto error = Common::JsonHelper::Serialize(const_cast<HealthStateFilterList&>(*this), objectString);
            if (!error.IsSuccess())
            {
                return std::wstring();
            }

            return objectString;
        }

        static Common::ErrorCode FromJsonString(std::wstring const & str, __inout HealthStateFilterList<TServiceModelEntry, TPublicApi, TPublicApiList> & filter)
        {
            return Common::JsonHelper::Deserialize(filter, str);
        }

        Common::ErrorCode FromPublicApi(__in TPublicApiList const * publicList)
        {
            return PublicApiHelper::FromPublicApiList<TServiceModelEntry, TPublicApiList>(publicList, items_);
        }

        // Used by FabricTest
        Common::ErrorCode ToPublicApi(__in Common::ScopedHeap & heap, __inout TPublicApiList & publicList) const
        {
            return PublicApiHelper::ToPublicApiList<TServiceModelEntry, TPublicApi, TPublicApiList>(heap, items_, publicList);
        }

        FABRIC_FIELDS_01(items_)

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::Items, items_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::vector<TServiceModelEntry> items_;
    };
}

