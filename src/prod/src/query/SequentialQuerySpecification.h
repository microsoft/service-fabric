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
            Query::QueryArgument const & argument2)
            : QuerySpecification(queryName, queryAddress, argument1, argument2)
        {
        }

        // Return ErrorCodeValue::EnumerationCompleted to indicate no more inner queries are present
        virtual Common::ErrorCode GetNext(
            __in Query::QuerySpecificationSPtr const & previousQuerySpecification,
            __in ServiceModel::QueryResult & previousQueryResult,
            __out Query::QuerySpecificationSPtr & nextQuery,
            __out Transport::MessageUPtr & replyMessage) = 0;
    };

    typedef std::shared_ptr<SequentialQuerySpecification> SequentialQuerySpecificationSPtr;
}
