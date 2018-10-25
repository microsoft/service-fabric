// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // Wrapper class to the list of items. This is templatized and we create specialized 
    // template using the macros. The macro requires the IDL type to be
    // be specified which makes it possible to call into the ToPublicAPI function of
    // each class. Macro also requires IDL enum type to be specified. We use this to 
    // construct the exact type while deserializing the wrapper using a static method
    // in QueryResultSerializationTypeActivator
    template<class TServiceModel>
    class QueryResultListWrapper : public QueryResultListWrapperBase
    {
    public:
        QueryResultListWrapper()
            : QueryResultListWrapperBase(QueryResultHelpers::Invalid) 
            , serviceModelTypeList_()          
        {            
        }

        void AddItem(TServiceModel const & queryResultItem)
        {
            serviceModelTypeList_.push_back(queryResultItem);
        }

        virtual void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_QUERY_RESULT_LIST & list)
        {
            UNREFERENCED_PARAMETER(heap);
            list.Count = 0;
            list.Items = NULL;
            list.Kind = FABRIC_QUERY_RESULT_ITEM_KIND_INVALID;                                    
        }                                                                               
                                                                                        
        static std::shared_ptr<QueryResultListWrapperBase> CreateSPtr()                 
        {                                                                               
            return std::make_shared<QueryResultListWrapperBase>(FABRIC_QUERY_RESULT_ITEM_KIND_INVALID);  
        }                            

        __declspec(property(get=get_ItemList)) std::vector<TServiceModel> const & ItemList;
        std::vector<TServiceModel> const & get_ItemList() const { return serviceModelTypeList_; }
                                                                                        
        static const bool IsSupportedQueryResultItem = false;                            

        FABRIC_FIELDS_01(serviceModelTypeList_)   

    private:
        std::vector<TServiceModel> serviceModelTypeList_;        
    };

    template<QueryResultHelpers::Enum Kind>
    class QueryResultSerializationTypeActivator
    {
    public:        
        static Serialization::IFabricSerializable * CreateNew()
        {
            return new QueryResultListWrapperBase(QueryResultHelpers::Invalid);
        }
    };	                                                                                        
}
