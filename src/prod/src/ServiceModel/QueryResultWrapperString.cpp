// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

QueryResultListWrapper<std::wstring>::QueryResultListWrapper() 
    : QueryResultListWrapperBase(QueryResultHelpers::String)
    , serviceModelTypeList_()
    , serviceModelTypeItem_() 
{ 
}
                                                                                                      
QueryResultListWrapper<std::wstring>::QueryResultListWrapper(__in std::vector<std::wstring> && itemList)
    : QueryResultListWrapperBase(QueryResultHelpers::String)
    , serviceModelTypeList_(move(itemList))
    , serviceModelTypeItem_() 
{ 
}
                                                                                                      
QueryResultListWrapper<std::wstring>::QueryResultListWrapper(__in std::unique_ptr<std::wstring> && item)
    : QueryResultListWrapperBase(QueryResultHelpers::String)
    , serviceModelTypeList_()
    , serviceModelTypeItem_(move(*item)) 
{ 
}
                                                                                                      
std::unique_ptr<QueryResultListWrapper<std::wstring>> QueryResultListWrapper<std::wstring>::CreateListWrapper(__in std::vector<std::wstring> && itemList)
{                                                                                             
    return Common::make_unique<QueryResultListWrapper>(move(itemList));                       
}                                                                                             
                                                                                                      
std::unique_ptr<QueryResultListWrapper<std::wstring>> QueryResultListWrapper<std::wstring>::CreateItemWrapper(__in std::unique_ptr<std::wstring> && item)
{                                                                                             
    return Common::make_unique<QueryResultListWrapper>(move(item));                           
}                                                                                             
                                                                                                      
QueryResultListWrapperBase* QueryResultSerializationTypeActivator<QueryResultHelpers::String>::CreateNew()                                       
{                                                                                             
    return new QueryResultListWrapper<std::wstring>();
}                                                                                             
    
