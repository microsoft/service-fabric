// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Query
{
    Common::StringLiteral const AggregateHealthParallelQueryTraceType("AggregateHealthParallelQuery");

    template<class TEntityResult, class TEntityKey>
    class AggregateHealthParallelQuerySpecificationBase
        : public ParallelQuerySpecification
        , public Common::TextTraceComponent<Common::TraceTaskCodes::Query>
    {
    public:

        AggregateHealthParallelQuerySpecificationBase(
            Query::QueryNames::Enum queryName,
            Query::QueryAddressGenerator const & queryAddress)
            : ParallelQuerySpecification(queryName, queryAddress)
         {
         }

         AggregateHealthParallelQuerySpecificationBase(
            Query::QueryNames::Enum queryName,
            Query::QueryAddressGenerator const & queryAddress,
            QueryArgument const & argument0)
            : ParallelQuerySpecification(queryName, queryAddress, argument0)
         {
         }


         AggregateHealthParallelQuerySpecificationBase(
            Query::QueryNames::Enum queryName,
            Query::QueryAddressGenerator const & queryAddress,
            QueryArgument const & argument0,
            QueryArgument const & argument1)
            : ParallelQuerySpecification(queryName, queryAddress, argument0, argument1)
         {
         }

         AggregateHealthParallelQuerySpecificationBase(
            Query::QueryNames::Enum queryName,
            Query::QueryAddressGenerator const & queryAddress,
            QueryArgument const & argument0,
            QueryArgument const & argument1,
            QueryArgument const & argument2)
            : ParallelQuerySpecification(queryName, queryAddress, argument0, argument1, argument2)
         {
         }

         AggregateHealthParallelQuerySpecificationBase(
            Query::QueryNames::Enum queryName,
            Query::QueryAddressGenerator const & queryAddress,
            QueryArgument const & argument0,
            QueryArgument const & argument1,
            QueryArgument const & argument2,
            QueryArgument const & argument3)
            : ParallelQuerySpecification(queryName, queryAddress, argument0, argument1, argument2, argument3)
         {
         }

         AggregateHealthParallelQuerySpecificationBase(
             Query::QueryNames::Enum queryName,
             Query::QueryAddressGenerator const & queryAddress,
             QueryArgument const & argument0,
             QueryArgument const & argument1,
             QueryArgument const & argument2,
             QueryArgument const & argument3,
             QueryArgument const & argument4)
             : ParallelQuerySpecification(queryName, queryAddress, argument0, argument1, argument2, argument3, argument4)
         {
         }

         AggregateHealthParallelQuerySpecificationBase(
             Query::QueryNames::Enum queryName,
             Query::QueryAddressGenerator const & queryAddress,
             QueryArgument const & argument0,
             QueryArgument const & argument1,
             QueryArgument const & argument2,
             QueryArgument const & argument3,
             QueryArgument const & argument4,
             QueryArgument const & argument5)
             : ParallelQuerySpecification(queryName, queryAddress, argument0, argument1, argument2, argument3, argument4, argument5)
         {
         }

         // Return:
         //     This just merges based on key
         // Parameter queryResults:
         //     For each parallel query, an entry is placed inside this map
         // Remarks:
         //     First adds all the results
         //     Then it sees what the min continuation token is
         //     Add the things that respect the continuation token and try to add health to it.
        virtual Common::ErrorCode OnParallelQueryExecutionComplete(
            Common::ActivityId const & activityId,
            std::map<Query::QuerySpecificationSPtr, ServiceModel::QueryResult> & queryResults,
            __out Transport::MessageUPtr & replyMessage)
        {
            std::vector<TEntityResult> entityQueryResult;
            std::map<TEntityKey, FABRIC_HEALTH_STATE> healthStateMap;
            ServiceModel::MergedQueryListPager<TEntityKey, TEntityResult> resultsPager;

            // For each parallel query, merge together the results.
            for(auto itResult = queryResults.begin(); itResult != queryResults.end(); ++itResult)
            {
                Common::ErrorCode error;
                if (IsEntityInformationQuery(itResult->first) )
                {
                    error = MergeEntityInformationQueryResults(entityQueryResult, itResult->second);
                    if (!error.IsSuccess())
                    {
                        WriteInfo(
                            AggregateHealthParallelQueryTraceType,
                            "{0}: Error '{1}' while merging entity info from sub-query {2}",
                            activityId,
                            error,
                            itResult->first->AddressString);
                        return error;
                    }
                }
                else
                {
                    error = this->AddEntityKeyFromHealthResult(itResult->second, healthStateMap);
                    if (!error.IsSuccess())
                    {
                        WriteInfo(
                            AggregateHealthParallelQueryTraceType,
                            "{0}: Error '{1}' while obtaining result from health sub-query {2} during merge",
                            activityId,
                            error,
                            itResult->first->AddressString);
                        continue;
                    }
                }

                if (error.IsSuccess())
                {
                    // Best effort update min continuation token
                    // On error, use the results from the inner query as if they are all there
                    resultsPager.MergePagingStatus(
                        activityId,
                        itResult->first->AddressString,
                        itResult->second.MovePagingStatus()).ReadValue();
                }
            }

            WriteInfo(
                AggregateHealthParallelQueryTraceType,
                "{0}: entity query returned {1} results, HM {2}",
                activityId,
                entityQueryResult.size(),
                healthStateMap.size());

            // The continuation token was correctly set; now we can add all items that respect it
            for (auto & itItem : entityQueryResult)
            {
                TEntityKey key = GetEntityKeyFromEntityResult(itItem);
                resultsPager.Add(std::move(key), std::move(itItem));
            }

            resultsPager.UpdateHealthStates(activityId, healthStateMap);

            replyMessage = Common::make_unique<Transport::Message>(ServiceModel::QueryResult(std::move(resultsPager.TakePager(activityId))));
            return Common::ErrorCode::Success();
        }

        //
        // Use a smaller timeout value for the aggregated health query to the HM so we can return at least the data without health
        // in case health manager is unresponsive.
        //
        virtual
        Common::TimeSpan GetParallelQueryExecutionTimeout(QuerySpecificationSPtr const& querySpecification, Common::TimeSpan const& totalRemainingTime)
        {
            if (IsEntityInformationQuery(querySpecification))
            {
                return totalRemainingTime;
            }
            else
            {
                return QueryConfig::GetConfig().GetAggregatedHealthQueryTimeout(totalRemainingTime);
            }
        }

    protected:
        virtual Common::ErrorCode MergeEntityInformationQueryResults(
            __inout std::vector<TEntityResult> & mergedQueryResult,
            __in ServiceModel::QueryResult & results)
        {
            if (mergedQueryResult.size() == 0)
            {
                auto error = results.MoveList(mergedQueryResult);
                if (!error.IsSuccess())
                {
                    return error;
                }
            }
            else
            {
                std::vector<TEntityResult> entryQueryResult;
                auto error = results.MoveList(entryQueryResult);
                if (!error.IsSuccess())
                {
                    return error;
                }

                mergedQueryResult.insert(
                    mergedQueryResult.end(),
                    std::make_move_iterator(entryQueryResult.begin()),
                    std::make_move_iterator(entryQueryResult.end()));
            }

            return Common::ErrorCode::Success();
        }

        virtual bool IsEntityInformationQuery(Query::QuerySpecificationSPtr const & querySpecification) = 0;
        virtual Common::ErrorCode AddEntityKeyFromHealthResult(__in ServiceModel::QueryResult & healthQueryResult, __inout std::map<TEntityKey, FABRIC_HEALTH_STATE> & healthStateMap) = 0;
        virtual TEntityKey GetEntityKeyFromEntityResult(TEntityResult const & entityInformation) = 0;
    };
}
