// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

QueryResultListWrapperBase::QueryResultListWrapperBase(QueryResultHelpers::Enum resultKind)
    : resultKind_(resultKind)    
{
}

Serialization::IFabricSerializable * QueryResultListWrapperBase::FabricSerializerActivator(Serialization::FabricTypeInformation typeInformation)
{                                                                                                                                           
    if (typeInformation.buffer == nullptr || typeInformation.length != sizeof(QueryResultHelpers::Enum))                               
    {                                                                                                                                       
        return nullptr;                                                                                                                     
    }
                                                                                                                                                    
    QueryResultHelpers::Enum resultKind = *(reinterpret_cast<QueryResultHelpers::Enum const *>(typeInformation.buffer));
                                                                                                                                                    
    return QueryResultHelpers::CreateNew(resultKind);
}        

NTSTATUS QueryResultListWrapperBase::GetTypeInformation(__out Serialization::FabricTypeInformation & typeInformation) const
{
    typeInformation.buffer = reinterpret_cast<UCHAR const *>(&resultKind_);
    typeInformation.length = sizeof(resultKind_);   
    return STATUS_SUCCESS;
}

void QueryResultListWrapperBase::ToPublicApi(__in Common::ScopedHeap & , __out FABRIC_QUERY_RESULT_LIST & queryResultList)
{
    queryResultList.Count = 0;
    queryResultList.Items = NULL;
    queryResultList.Kind = FABRIC_QUERY_RESULT_ITEM_KIND_INVALID;
}

void QueryResultListWrapperBase::ToPublicApi(__in Common::ScopedHeap & , __out FABRIC_QUERY_RESULT_ITEM & queryResultItem)
{
    queryResultItem.Item = NULL;
    queryResultItem.Kind = FABRIC_QUERY_RESULT_ITEM_KIND_INVALID;
}
