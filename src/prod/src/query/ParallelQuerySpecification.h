// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Query
{
    class ParallelQuerySpecification
        : public QuerySpecification
    {
    public:
        ParallelQuerySpecification(
            Query::QueryNames::Enum queryName,
            Query::QueryAddressGenerator const & queryAddress);

        ParallelQuerySpecification(
            Query::QueryNames::Enum queryName,
            Query::QueryAddressGenerator const & queryAddress,
            QueryArgument const & argument0);

        ParallelQuerySpecification(
            Query::QueryNames::Enum queryName,
            Query::QueryAddressGenerator const & queryAddress,
            QueryArgument const & argument0,
            QueryArgument const & argument1);

        ParallelQuerySpecification(
            Query::QueryNames::Enum queryName,
            Query::QueryAddressGenerator const & queryAddress,
            QueryArgument const & argument0,
            QueryArgument const & argument1,
            QueryArgument const & argument2);

        ParallelQuerySpecification(
            Query::QueryNames::Enum queryName,
            Query::QueryAddressGenerator const & queryAddress,
            QueryArgument const & argument0,
            QueryArgument const & argument1,
            QueryArgument const & argument2,
            QueryArgument const & argument3);

        ParallelQuerySpecification(
            Query::QueryNames::Enum queryName,
            Query::QueryAddressGenerator const & queryAddress,
            QueryArgument const & argument0,
            QueryArgument const & argument1,
            QueryArgument const & argument2,
            QueryArgument const & argument3,
            QueryArgument const & argument4);

        ParallelQuerySpecification(
            Query::QueryNames::Enum queryName,
            Query::QueryAddressGenerator const & queryAddress,
            QueryArgument const & argument0,
            QueryArgument const & argument1,
            QueryArgument const & argument2,
            QueryArgument const & argument3,
            QueryArgument const & argument4,
            QueryArgument const & argument5);

         __declspec(property(get=get_ParallelQuerySpecifications)) std::vector<QuerySpecificationSPtr> & ParallelQuerySpecifications;
         std::vector<Query::QuerySpecificationSPtr> & get_ParallelQuerySpecifications() { return parallelQuerySpecifications_; }

         virtual Common::ErrorCode OnParallelQueryExecutionComplete(
             Common::ActivityId const & activityId,
             std::map<Query::QuerySpecificationSPtr, ServiceModel::QueryResult> & queryResults,
             __out Transport::MessageUPtr & replyMessage) = 0;

         // Can be overridden by the derived classes if they want to control the timeout specified for each of the parallel query.
         virtual Common::TimeSpan GetParallelQueryExecutionTimeout(Query::QuerySpecificationSPtr const&, Common::TimeSpan const& totalRemainingTime)
         {
             return totalRemainingTime;
         }

         virtual ServiceModel::QueryArgumentMap GetQueryArgumentMap(
             size_t index,
             ServiceModel::QueryArgumentMap const & baseArg);

    private:
        std::vector<Query::QuerySpecificationSPtr> parallelQuerySpecifications_;
    };

    typedef std::shared_ptr<ParallelQuerySpecification> ParallelQuerySpecificationSPtr;
}
