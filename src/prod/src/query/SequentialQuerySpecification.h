// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Query
{
    class SequentialQuerySpecification
        : public QuerySpecification
    {
    public:
        SequentialQuerySpecification(
            Query::QueryNames::Enum queryName,
            Query::QueryAddressGenerator const & queryAddress,
            Query::QueryArgument const & argument1,
            Query::QueryArgument const & argument2,
            Query::QueryArgument const & argument3)
            : QuerySpecification(queryName, queryAddress, argument1, argument2, argument3, QueryType::Sequential)
        {
        }

        SequentialQuerySpecification(
            Query::QueryNames::Enum queryName,
            Query::QueryAddressGenerator const & queryAddress,
            Query::QueryArgument const & argument1,
            Query::QueryArgument const & argument2,
            Query::QueryArgument const & argument3,
            Query::QueryArgument const & argument4)
            : QuerySpecification(queryName, queryAddress, argument1, argument2, argument3, argument4, QueryType::Sequential)
        {
        }

        SequentialQuerySpecification(
            Query::QueryNames::Enum queryName,
            Query::QueryAddressGenerator const & queryAddress,
            Query::QueryArgument const & argument1,
            Query::QueryArgument const & argument2,
            Query::QueryArgument const & argument3,
            Query::QueryArgument const & argument4,
            Query::QueryArgument const & argument5)
            : QuerySpecification(queryName, queryAddress, argument1, argument2, argument3, argument4, argument5, QueryType::Sequential)
        {
        }

        // Return ErrorCodeValue::EnumerationCompleted to indicate no more inner queries are present
        virtual Common::ErrorCode GetNext(
            __in Query::QuerySpecificationSPtr const & previousQuerySpecification,
            __in std::unordered_map<Query::QueryNames::Enum, ServiceModel::QueryResult> & queryResults,
            __inout ServiceModel::QueryArgumentMap & queryArgs, // on input this contains the original query args.
            __out Query::QuerySpecificationSPtr & nextQuery,
            __out Transport::MessageUPtr & replyMessage) = 0;

        virtual bool ShouldContinueSequentialQuery(
            Query::QuerySpecificationSPtr const & lastExecutedQuerySpecification,
            Common::ErrorCode const & error)
        {
            UNREFERENCED_PARAMETER(lastExecutedQuerySpecification);
            return error.IsSuccess();
        }
    };

    typedef std::shared_ptr<SequentialQuerySpecification> SequentialQuerySpecificationSPtr;
}
