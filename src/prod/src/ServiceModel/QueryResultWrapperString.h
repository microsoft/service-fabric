// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    template<> class QueryResultListWrapper<std::wstring> : public QueryResultListWrapperBase    
    {                                                                                                 
    public:                                                                                           
        QueryResultListWrapper();
                                                                                                      
        explicit QueryResultListWrapper(__in std::vector<std::wstring> && itemList);
                                                                                                      
        explicit QueryResultListWrapper(__in std::unique_ptr<std::wstring> && item);
                                                                                                      
        static std::unique_ptr<QueryResultListWrapper<std::wstring>> CreateListWrapper(__in std::vector<std::wstring> && itemList);
                                                                                                      
        static std::unique_ptr<QueryResultListWrapper<std::wstring>> CreateItemWrapper(__in std::unique_ptr<std::wstring> && item);
                                                                                                      
        __declspec(property(get=get_ItemList)) std::vector<std::wstring> const & ItemList;       
        std::vector<std::wstring> const & get_ItemList() const { return serviceModelTypeList_; } 
        
        __declspec(property(get=get_Item)) std::wstring const & Item;       
        std::wstring const & get_Item() const { return serviceModelTypeItem_; } 

        static const bool IsSupportedQueryResultItem = true;                                          
                                                                                                      
        FABRIC_FIELDS_02(serviceModelTypeList_, serviceModelTypeItem_)                                
                                                                                                      
    private:                                                                                          
        std::vector<std::wstring> serviceModelTypeList_;                                         
        std::wstring serviceModelTypeItem_;                                     
    };                                                                                                
    template<> class QueryResultSerializationTypeActivator<QueryResultHelpers::String>                                  
    {                                                                                                 
    public:                                                                                           
        static QueryResultListWrapperBase* CreateNew();        
    };                                                                                                
}
