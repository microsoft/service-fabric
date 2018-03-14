// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Query
{
    class QueryRequestMessageBodyBase : public ServiceModel::ClientServerMessageBody
    {
    public:
        QueryRequestMessageBodyBase()
        : queryArgs_()
        {}

        QueryRequestMessageBodyBase(ServiceModel::QueryArgumentMap const & queryArgs)
            : queryArgs_(queryArgs)
        {}

        QueryRequestMessageBodyBase(ServiceModel::QueryArgumentMap && queryArgs)
        : queryArgs_(std::move(queryArgs))
        {}

        __declspec(property(get=get_QueryArgs)) ServiceModel::QueryArgumentMap & QueryArgs;
        ServiceModel::QueryArgumentMap & get_QueryArgs() { return queryArgs_; }
                
    protected:
        ServiceModel::QueryArgumentMap queryArgs_;
    };

    class QueryRequestMessageBody : public QueryRequestMessageBodyBase
    {
    public:
        QueryRequestMessageBody()
            : QueryRequestMessageBodyBase()
            , queryName_()      
        {}

        QueryRequestMessageBody(std::wstring const & queryName, ServiceModel::QueryArgumentMap const & queryArgs)
            : QueryRequestMessageBodyBase(queryArgs)
            , queryName_(queryName)          
        {}

        QueryRequestMessageBody(std::wstring && queryName, ServiceModel::QueryArgumentMap && queryArgs)
            : QueryRequestMessageBodyBase(std::move(queryArgs))
            , queryName_(std::move(queryName))            
        {}

        __declspec(property(get=get_QueryName)) std::wstring const & QueryName;
        std::wstring const & get_QueryName() { return queryName_; }

        FABRIC_FIELDS_02(queryArgs_, queryName_)

    private:
        std::wstring queryName_;        
    };

    class QueryRequestMessageBodyInternal : public QueryRequestMessageBodyBase
    {
    public:
        QueryRequestMessageBodyInternal()
            : QueryRequestMessageBodyBase()
            , queryName_()           
        {}

        QueryRequestMessageBodyInternal(Query::QueryNames::Enum queryName, Query::QueryType::Enum queryType, ServiceModel::QueryArgumentMap const & queryArgs)
            : QueryRequestMessageBodyBase(queryArgs)
            , queryName_(queryName)           
            , queryType_(queryType)
        {}

        QueryRequestMessageBodyInternal(Query::QueryNames::Enum queryName, ServiceModel::QueryArgumentMap && queryArgs)
            : QueryRequestMessageBodyBase(std::move(queryArgs))
            , queryName_(queryName)            
        {}

        __declspec(property(get=get_QueryName)) Query::QueryNames::Enum QueryName;
        Query::QueryNames::Enum get_QueryName() const { return queryName_; }

        __declspec(property(get=get_QueryType)) Query::QueryType::Enum QueryType;
        Query::QueryType::Enum get_QueryType() const { return queryType_; }

        FABRIC_FIELDS_03(queryName_, queryArgs_, queryType_)

    private:
        Query::QueryNames::Enum queryName_;        
        Query::QueryType::Enum queryType_;
    };
}
