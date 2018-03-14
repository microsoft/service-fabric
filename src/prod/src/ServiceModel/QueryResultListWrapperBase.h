// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    namespace QueryResultHelpers
    {
        enum Enum;
    }

    // Base class for all the templated classes that wrap the list of the ServiceModel type that needs to be exposed
    // via query. This saves us from templating the classes such as QueryResultList and QueryResult and they can be
    // without worrying about the template argument type.
    class QueryResultListWrapperBase : public Serialization::FabricSerializable
    {    
    public:
        QueryResultListWrapperBase(QueryResultHelpers::Enum resultKind);

        static Serialization::IFabricSerializable * FabricSerializerActivator(Serialization::FabricTypeInformation typeInformation);

        virtual NTSTATUS GetTypeInformation(__out Serialization::FabricTypeInformation & typeInformation) const;        

        virtual void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_QUERY_RESULT_LIST & queryResultList);

        virtual void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_QUERY_RESULT_ITEM & queryResultItem);

        __declspec(property(get=get_ResultKind)) QueryResultHelpers::Enum ResultKind;
        QueryResultHelpers::Enum get_ResultKind() const { return resultKind_; }

    private:
        QueryResultHelpers::Enum resultKind_;        
    };

}
