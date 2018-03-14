// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{

    #define DEFINE_QUERY_RESULT_LIST_ITEM(TYPE_SERVICEMODEL, SERVICE_MODEL_ENUM, TYPE_IDL, IDL_ENUM)    \
    DEFINE_USER_ARRAY_UTILITY(TYPE_SERVICEMODEL)                                                        \
    template<> class ServiceModel::QueryResultListWrapper<TYPE_SERVICEMODEL> : public QueryResultListWrapperBase      \
    {                                                                                                   \
    public:                                                                                             \
        QueryResultListWrapper() : QueryResultListWrapperBase(SERVICE_MODEL_ENUM), serviceModelTypeList_(), serviceModelTypeItem_() { } \
                                                                                                        \
        explicit QueryResultListWrapper(__in std::vector<TYPE_SERVICEMODEL> && itemList): QueryResultListWrapperBase(SERVICE_MODEL_ENUM), serviceModelTypeList_(move(itemList)), serviceModelTypeItem_() { } \
                                                                                                        \
        explicit QueryResultListWrapper(__in std::unique_ptr<TYPE_SERVICEMODEL> && item): QueryResultListWrapperBase(SERVICE_MODEL_ENUM), serviceModelTypeList_(), serviceModelTypeItem_(move(item)) { } \
                                                                                                        \
        QueryResultListWrapper const & operator = (QueryResultListWrapper && other)                     \
        {                                                                                               \
            if (&other != this)                                                                         \
            {                                                                                           \
                serviceModelTypeList_ = move(other.serviceModelTypeList_);                              \
                serviceModelTypeItem_ = move(other.serviceModelTypeItem_);                              \
            }                                                                                           \
                                                                                                        \
            return *this;                                                                               \
        }                                                                                               \
                                                                                                        \
        static std::unique_ptr<QueryResultListWrapper<TYPE_SERVICEMODEL>> CreateListWrapper(__in std::vector<TYPE_SERVICEMODEL> && itemList) \
        {                                                                                               \
            return Common::make_unique<QueryResultListWrapper>(move(itemList));                         \
        }                                                                                               \
                                                                                                        \
        static std::unique_ptr<QueryResultListWrapper<TYPE_SERVICEMODEL>> CreateItemWrapper(__in std::unique_ptr<TYPE_SERVICEMODEL> && item) \
        {                                                                                               \
            return Common::make_unique<QueryResultListWrapper>(move(item));                             \
        }                                                                                               \
                                                                                                        \
        virtual void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_QUERY_RESULT_LIST & list) \
        {                                                                                               \
            list.Count = static_cast<ULONG>(serviceModelTypeList_.size());                              \
            auto resultArray = heap.AddArray<TYPE_IDL>(serviceModelTypeList_.size());                   \
            list.Kind = IDL_ENUM;                                                                       \
            int resultArrayIndex = 0;                                                                   \
            for (auto itResultItem = serviceModelTypeList_.begin(); itResultItem != serviceModelTypeList_.end(); ++itResultItem)  \
            {                                                                                           \
                itResultItem->ToPublicApi(heap, resultArray[resultArrayIndex++]);                       \
            }                                                                                           \
                                                                                                        \
            list.Items = resultArray.GetRawArray();                                                     \
        }                                                                                               \
                                                                                                        \
        virtual void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_QUERY_RESULT_ITEM & queryResultItem) \
        {                                                                                               \
            auto resultItem = heap.AddItem<TYPE_IDL>();                                                 \
            queryResultItem.Kind = IDL_ENUM;                                                            \
            serviceModelTypeItem_->ToPublicApi(heap, *resultItem);                                      \
            queryResultItem.Item = resultItem.GetRawPointer();                                          \
        }                                                                                               \
                                                                                                        \
        __declspec(property(get=get_ItemList)) std::vector<TYPE_SERVICEMODEL> & ItemList;               \
        std::vector<TYPE_SERVICEMODEL> & get_ItemList() { return serviceModelTypeList_; }               \
                                                                                                        \
        __declspec(property(get=get_Item)) TYPE_SERVICEMODEL & Item;                                    \
        TYPE_SERVICEMODEL & get_Item() { return *serviceModelTypeItem_; }                               \
                                                                                                        \
        static const bool IsSupportedQueryResultItem = true;                                            \
                                                                                                        \
        FABRIC_FIELDS_02(serviceModelTypeList_, serviceModelTypeItem_)                                  \
                                                                                                        \
    private:                                                                                            \
        std::vector<TYPE_SERVICEMODEL> serviceModelTypeList_;                                           \
        std::unique_ptr<TYPE_SERVICEMODEL> serviceModelTypeItem_;                                       \
    };                                                                                                  \
    template<> class ServiceModel::QueryResultSerializationTypeActivator<SERVICE_MODEL_ENUM>            \
    {                                                                                                   \
    public:                                                                                             \
        static QueryResultListWrapperBase * CreateNew()                                                 \
        {                                                                                               \
            return new QueryResultListWrapper<TYPE_SERVICEMODEL>();                                     \
        }                                                                                               \
    };


    #define DEFINE_QUERY_RESULT_INTERNAL_LIST_ITEM(TYPE_SERVICEMODEL, SERVICE_MODEL_ENUM)               \
    DEFINE_USER_ARRAY_UTILITY(TYPE_SERVICEMODEL)                                                        \
    namespace ServiceModel                                                                              \
    {                                                                                                   \
    template<> class QueryResultListWrapper<TYPE_SERVICEMODEL> : public QueryResultListWrapperBase      \
    {                                                                                                   \
    public:                                                                                             \
        QueryResultListWrapper() : QueryResultListWrapperBase(SERVICE_MODEL_ENUM), serviceModelTypeList_(), serviceModelTypeItem_() { } \
                                                                                                        \
        explicit QueryResultListWrapper(__in std::vector<TYPE_SERVICEMODEL> && itemList): QueryResultListWrapperBase(SERVICE_MODEL_ENUM), serviceModelTypeList_(move(itemList)), serviceModelTypeItem_() { } \
                                                                                                        \
        explicit QueryResultListWrapper(__in std::unique_ptr<TYPE_SERVICEMODEL> && item): QueryResultListWrapperBase(SERVICE_MODEL_ENUM), serviceModelTypeList_(), serviceModelTypeItem_(move(item)) { } \
                                                                                                        \
        QueryResultListWrapper const & operator = (QueryResultListWrapper && other)                     \
        {                                                                                               \
            if (&other != this)                                                                         \
            {                                                                                           \
                serviceModelTypeList_ = move(other.serviceModelTypeList_);                              \
                serviceModelTypeItem_ = move(other.serviceModelTypeItem_);                              \
            }                                                                                           \
                                                                                                        \
            return *this;                                                                               \
        }                                                                                               \
                                                                                                        \
        static std::unique_ptr<QueryResultListWrapper<TYPE_SERVICEMODEL>> CreateListWrapper(__in std::vector<TYPE_SERVICEMODEL> && itemList) \
        {                                                                                               \
            return Common::make_unique<QueryResultListWrapper>(move(itemList));                         \
        }                                                                                               \
                                                                                                        \
        static std::unique_ptr<QueryResultListWrapper<TYPE_SERVICEMODEL>> CreateItemWrapper(__in std::unique_ptr<TYPE_SERVICEMODEL> && item) \
        {                                                                                               \
            return Common::make_unique<QueryResultListWrapper>(move(item));                             \
        }                                                                                               \
                                                                                                        \
        __declspec(property(get=get_ItemList)) std::vector<TYPE_SERVICEMODEL> & ItemList;               \
        std::vector<TYPE_SERVICEMODEL> & get_ItemList() { return serviceModelTypeList_; }               \
                                                                                                        \
        __declspec(property(get=get_Item)) TYPE_SERVICEMODEL & Item;                                    \
        TYPE_SERVICEMODEL & get_Item() { return *serviceModelTypeItem_; }                               \
                                                                                                        \
        static const bool IsSupportedQueryResultItem = true;                                            \
                                                                                                        \
        FABRIC_FIELDS_02(serviceModelTypeList_, serviceModelTypeItem_)                                  \
                                                                                                        \
    private:                                                                                            \
        std::vector<TYPE_SERVICEMODEL> serviceModelTypeList_;                                           \
        std::unique_ptr<TYPE_SERVICEMODEL> serviceModelTypeItem_;                                       \
    };                                                                                                  \
    template<> class QueryResultSerializationTypeActivator<SERVICE_MODEL_ENUM>                          \
    {                                                                                                   \
    public:                                                                                             \
        static QueryResultListWrapperBase * CreateNew()                                                 \
        {                                                                                               \
            return new QueryResultListWrapper<TYPE_SERVICEMODEL>();                                     \
        }                                                                                               \
    };                                                                                                  \
    }

    #define DECLARE_QUERY_RESULT_TYPE_ACTIVATOR_FACTORY(TYPE_FACTORY)                                   \
    class TYPE_FACTORY##QueryResultFactory                                                              \
    {                                                                                                   \
    public:                                                                                             \
        static Serialization::FabricSerializable * CreateNew();                                         \
    };

    #define DEFINE_QUERY_RESULT_TYPE_ACTIVATOR_FACTORY(TYPE_SERVICEMODEL, TYPE_FACTORY)                 \
    Serialization::FabricSerializable * TYPE_FACTORY##QueryResultFactory::CreateNew()     \
    {                                                                                                   \
        return new QueryResultListWrapper<TYPE_SERVICEMODEL>();                                         \
    }


    // The macros defined here saves writing little bit of code when a new type is added for QueryResultList
    // The method that we are implementing is called by serializer for activating the type.
    // We use it to construct the actual type instead of the base class
    #define BEGIN_QUERY_RESULT_ITEM_ENUM()                                                              \
    Serialization::FabricSerializable* QueryResultHelpers::CreateNew(QueryResultHelpers::Enum resultKind)                                       \
    {                                                                                                   \
        switch(resultKind)                                                                              \
        {                                                                                               \


            #define QUERY_RESULT_ITEM_ENUM_KIND(SERVICE_MODEL_ENUM_KIND)                                \
            case SERVICE_MODEL_ENUM_KIND:                                                               \
            {                                                                                           \
                return dynamic_cast<Serialization::FabricSerializable *>(QueryResultSerializationTypeActivator<SERVICE_MODEL_ENUM_KIND>::CreateNew()); \
            }                                                                                           \

            #define QUERY_RESULT_ITEM_ENUM_KIND_WITH_FACTORY(SERVICE_MODEL_ENUM_KIND, TYPE_SERVICEMODEL) \
            case SERVICE_MODEL_ENUM_KIND:                                                               \
            {                                                                                           \
                return (TYPE_SERVICEMODEL##QueryResultFactory::CreateNew());                            \
            }                                                                                           \

            #define END_QUERY_RESULT_ITEM_ENUM()                                                        \
            default:                                                                                    \
            {                                                                                           \
                return dynamic_cast<Serialization::FabricSerializable *>(new QueryResultListWrapperBase(QueryResultHelpers::Invalid)); \
            }                                                                                           \
        }                                                                                               \
    }


#define QUERY_JSON_LIST( LIST_TYPE, ENTRY_TYPE ) \
    struct LIST_TYPE : public Common::IFabricJsonSerializable \
    { \
        DENY_COPY(LIST_TYPE) \
    public: \
        LIST_TYPE() : Items(), ContinuationToken() {} \
        std::vector<ENTRY_TYPE> Items; \
        std::wstring ContinuationToken; \
        BEGIN_JSON_SERIALIZABLE_PROPERTIES() \
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::ContinuationToken, ContinuationToken) \
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::Items, Items) \
        END_JSON_SERIALIZABLE_PROPERTIES() \
    };
}
