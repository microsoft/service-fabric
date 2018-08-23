// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Query
{
    class QuerySpecification;
    typedef std::shared_ptr<Query::QuerySpecification> QuerySpecificationSPtr;
    // This class acts as metadata for the query. Every new query that is created needs to
    // have an instance of specification.
    class QuerySpecification
    {
    public:

        virtual ~QuerySpecification() = default;

        QuerySpecification(
            Query::QueryNames::Enum queryName,
            Query::QueryAddressGenerator const & queryAddress,
            Query::QueryType::Enum queryType = QueryType::Simple);

        QuerySpecification(
            Query::QueryNames::Enum queryName,
            Query::QueryAddressGenerator const & queryAddress,
            Query::QueryArgument const & queryArgument0,
            Query::QueryType::Enum queryType = QueryType::Simple);

        QuerySpecification(
            Query::QueryNames::Enum queryName,
            Query::QueryAddressGenerator const & queryAddress,
            Query::QueryArgument const & queryArgument0,
            Query::QueryArgument const & queryArgument1,
            Query::QueryType::Enum queryType = QueryType::Simple);

        QuerySpecification(
            Query::QueryNames::Enum queryName,
            Query::QueryAddressGenerator const & queryAddress,
            Query::QueryArgument const & queryArgument0,
            Query::QueryArgument const & queryArgument1,
            Query::QueryArgument const & queryArgument2,
            Query::QueryType::Enum queryType = QueryType::Simple);

        QuerySpecification(
            Query::QueryNames::Enum queryName,
            Query::QueryAddressGenerator const & queryAddress,
            Query::QueryArgument const & queryArgument0,
            Query::QueryArgument const & queryArgument1,
            Query::QueryArgument const & queryArgument2,
            Query::QueryArgument const & queryArgument3,
            Query::QueryType::Enum queryType = QueryType::Simple);

        QuerySpecification(
            Query::QueryNames::Enum queryName,
            Query::QueryAddressGenerator const & queryAddress,
            Query::QueryArgument const & queryArgument0,
            Query::QueryArgument const & queryArgument1,
            Query::QueryArgument const & queryArgument2,
            Query::QueryArgument const & queryArgument3,
            Query::QueryArgument const & queryArgument4,
            Query::QueryType::Enum queryType = QueryType::Simple);

        QuerySpecification(
            Query::QueryNames::Enum queryName,
            Query::QueryAddressGenerator const & queryAddress,
            Query::QueryArgument const & queryArgument0,
            Query::QueryArgument const & queryArgument1,
            Query::QueryArgument const & queryArgument2,
            Query::QueryArgument const & queryArgument3,
            Query::QueryArgument const & queryArgument4,
            Query::QueryArgument const & queryArgument5,
            Query::QueryType::Enum queryType = QueryType::Simple);

        QuerySpecification(
            Query::QueryNames::Enum queryName,
            Query::QueryAddressGenerator const & queryAddress,
            Query::QueryArgument const & queryArgument0,
            Query::QueryArgument const & queryArgument1,
            Query::QueryArgument const & queryArgument2,
            Query::QueryArgument const & queryArgument3,
            Query::QueryArgument const & queryArgument4,
            Query::QueryArgument const & queryArgument5,
            Query::QueryArgument const & queryArgument6,
            Query::QueryType::Enum queryType = QueryType::Simple);
            
        QuerySpecification(
            Query::QueryNames::Enum queryName,
            Query::QueryAddressGenerator const & queryAddress,
            Query::QueryArgument const & queryArgument0,
            Query::QueryArgument const & queryArgument1,
            Query::QueryArgument const & queryArgument2,
            Query::QueryArgument const & queryArgument3,
            Query::QueryArgument const & queryArgument4,
            Query::QueryArgument const & queryArgument5,
            Query::QueryArgument const & queryArgument6,
            Query::QueryArgument const & queryArgument7,
            Query::QueryType::Enum queryType = QueryType::Simple);
        
        QuerySpecification(
            Query::QueryNames::Enum queryName,
            Query::QueryAddressGenerator const & queryAddress,
            Query::QueryArgument const & queryArgument0,
            Query::QueryArgument const & queryArgument1,
            Query::QueryArgument const & queryArgument2,
            Query::QueryArgument const & queryArgument3,
            Query::QueryArgument const & queryArgument4,
            Query::QueryArgument const & queryArgument5,
            Query::QueryArgument const & queryArgument6,
            Query::QueryArgument const & queryArgument7,
            Query::QueryArgument const & queryArgument8,
            Query::QueryType::Enum queryType = QueryType::Simple);

        static std::vector<QuerySpecificationSPtr> CreateSpecifications();

        static std::wstring GetSpecificationId(
            QueryNames::Enum queryName,
            ServiceModel::QueryArgumentMap const & queryArgs);

        Common::ErrorCode HasRequiredArguments(ServiceModel::QueryArgumentMap const & queryArgs);
        Common::ErrorCode GenerateAddress(Common::ActivityId const & activityId, ServiceModel::QueryArgumentMap const & queryArgs, _Out_ std::wstring & address);

        __declspec(property(get=get_Arguments)) std::vector<Query::QueryArgument> const & SupportedArguments;
        std::vector<Query::QueryArgument> const & get_Arguments() const { return arguments_; }

        __declspec(property(get=get_QueryName)) Query::QueryNames::Enum QueryName;
        Query::QueryNames::Enum get_QueryName() const { return queryName_; };

        __declspec(property(get=get_QueryId, put=put_QueryId)) std::wstring QuerySpecificationId;
        std::wstring get_QueryId() const { return querySpecificationId_; };
        void put_QueryId(std::wstring const & value) { querySpecificationId_ = value; }

        __declspec(property(get=get_QueryType)) Query::QueryType::Enum QueryType;
        Query::QueryType::Enum get_QueryType() const { return queryType_; }

        __declspec(property(get=get_AddressString)) std::string const & AddressString;
        std::string const & get_AddressString() const { return queryAddressGenerator_.AddressString; }

    protected:
        // Query can extend the QuerySpecification and change the address generator based upon the arguments
        Query::QueryAddressGenerator queryAddressGenerator_;

        //
        // QuerySpecificationId uniquely identifies a specification.
        // If the query name is not unique, it is a combination of the Name and
        // target Address(component servicing the query)
        //
        std::wstring querySpecificationId_;
        #define MAKE_QUERY_SPEC_ID(EnumName, Address) EnumName + L"@" + Address

    private:
        void InitializeArgumentSets();

        Query::QueryNames::Enum queryName_;
        std::vector<Query::QueryArgument> arguments_;
        Query::QueryType::Enum queryType_;
        std::vector<std::wstring> argumentSets_;
    };
}
