//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class GetChaosEventsDescription
            : public Serialization::FabricSerializable
            , public Common::IFabricJsonSerializable
        {

        public:
            GetChaosEventsDescription();
            GetChaosEventsDescription(
                std::shared_ptr<ChaosEventsFilter> &,
                std::shared_ptr<ServiceModel::QueryPagingDescription> &,
                std::wstring &);

            GetChaosEventsDescription(GetChaosEventsDescription const &) = default;
            GetChaosEventsDescription & operator = (GetChaosEventsDescription const &) = default;

            GetChaosEventsDescription(GetChaosEventsDescription &&) = default;
            GetChaosEventsDescription & operator = (GetChaosEventsDescription &&) = default;

            __declspec(property(get = get_Filter)) std::shared_ptr<ChaosEventsFilter> const & FilterSPtr;
            std::shared_ptr<ChaosEventsFilter> const & get_Filter() const { return filterSPtr_; }

            __declspec(property(get = get_QueryPagingDescription, put = set_QueryPagingDescription)) std::shared_ptr<ServiceModel::QueryPagingDescription> const & PagingDescription;
            std::shared_ptr<ServiceModel::QueryPagingDescription> const & get_QueryPagingDescription() const { return queryPagingDescriptionSPtr_; }

            __declspec(property(get = get_ClientType)) std::wstring ClientType;
            std::wstring get_ClientType() const { return clientType_; }

            Common::ErrorCode FromPublicApi(FABRIC_CHAOS_EVENTS_SEGMENT_DESCRIPTION const &);
            void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_CHAOS_EVENTS_SEGMENT_DESCRIPTION &) const;

            FABRIC_FIELDS_03(
                filterSPtr_,
                queryPagingDescriptionSPtr_,
                clientType_);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ChaosEventsFilter, filterSPtr_);
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::QueryPagingDescription, queryPagingDescriptionSPtr_);
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ClientType, clientType_)
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:
            std::shared_ptr<ChaosEventsFilter> filterSPtr_;
            std::shared_ptr<ServiceModel::QueryPagingDescription> queryPagingDescriptionSPtr_;
            std::wstring clientType_;
        };
    }
}
