// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Serialization;
using namespace ServiceModel;

StringLiteral const TraceSource("ListPager");

template <class T>
ListPager<T>::ListPager()
    : maxAllowedSize_(static_cast<size_t>(
        static_cast<double>(ServiceModelConfig::GetConfig().MaxMessageSize) *
        ServiceModelConfig::GetConfig().MessageContentBufferRatio *
        ServiceModelConfig::GetConfig().QueryPagerContentRatio))
    , currentSize_(0)
    , entries_()
    , pagingStatus_()
    , maxResults_(QueryPagingDescription::MaxResultsDefaultValue)
{
}

template <class T>
ListPager<T>::~ListPager()
{
}

// If the maxResults value is invalid, then we will simply ignore the value
template <class T>
void ListPager<T>::SetMaxResults(int64 maxResults)
{
    // Don't allow setting maxResults to 0
    if (maxResults <= 0)
    {
        if (maxResults < 0)
        {
            Trace.WriteWarning(
                TraceSource,
                "{0}: Attempt to set max results ignored because value is negative.");
        }

        return;
    }
    maxResults_ = maxResults;
}

template<class T>
void ServiceModel::ListPager<T>::OverrideContinuationTokenManual(__in wstring & continuationToken)
{
    if (pagingStatus_ != nullptr)
    {
        pagingStatus_->ContinuationToken = move(continuationToken);
    }
    else
    {
        pagingStatus_ = make_unique<PagingStatus>(move(continuationToken));
    }
}

template <class T>
ErrorCode ListPager<T>::TryAdd(T && entry)
{
    size_t size = entry.EstimateSize();
    size_t estimatedSize = currentSize_ + size;

    // Check for both the size that the list can be (bytes) and the max number of entries it can hold (according to user specification)
    //
    // maxResults is a value that is set by the user, which specifies how many entries can be in a list
    // Unlike maxAllowedSize, this deals with the number of entries rather than the size in bytes
    // maxResults always respects maxAllowedSize. For example, if there are 10 possible results,
    // and a maxResults of 8, and a maxAllowedSize that only allows the first two entries, then only the
    // first two will be returned.
    //
    // maxResults_ needs to be compared as uint64 rather than int64 because otherwise there is a unsigned/signed mismatch causing a build error.
    if (estimatedSize >= maxAllowedSize_ || entries_.size() >= static_cast<uint64>(maxResults_))
    {
        if (entries_.empty())
        {
            // The one entry is larger than message size, return error
            // TODO: this should return something other than InvalidArg, so the calling query can give users a more useful error (ie. increase max message size)
            // And rather than returning an empty list after looping through every item, fail the query if this is the case.
            auto error = ErrorCode(
                ErrorCodeValue::MessageTooLarge,
                wformatString("{0} {1} {2}", StringResource::Get(IDS_COMMON_ENTRY_TOO_LARGE), estimatedSize, maxAllowedSize_));
            Trace.WriteInfo(TraceSource, "One entry is larger than the allowed max message size: {0}, {1}", error, error.Message);
            return error;
        }

        // TODO: Make sure the continuation token fits
        // Possibilities: define MaxContinuationToken size
        // OR when adding an item leave room for twice its size
        // OR remove last item and replace it with the continuation token
        // For simplicity, consider it fits by using the specified ratio out of max message size
        wstring continuationToken = entries_.back().CreateContinuationToken();
        pagingStatus_ = make_unique<PagingStatus>(move(continuationToken));

        // Return error to let caller know that no more items can fit
        // This error should not be returned as part of the query result, it should be considered success
        if (estimatedSize >= maxAllowedSize_)
        {
            auto error = ErrorCode(
                ErrorCodeValue::EntryTooLarge,
                wformatString("{0} {1} {2}", StringResource::Get(IDS_COMMON_ENTRY_TOO_LARGE), estimatedSize, maxAllowedSize_));
            Trace.WriteNoise(TraceSource, "Hit message size limit when adding {0}: {1}, {2}", pagingStatus_->ContinuationToken, error, error.Message);
            return error;
        }

        // Return error to let the caller know that the user specified maxResults value has been reached.
        // This should be treated the same way as EntryTooLarge
        if (entries_.size() >= static_cast<uint64>(maxResults_))
        {
            auto error = ErrorCode(
                ErrorCodeValue::MaxResultsReached,
                wformatString("{0} {1}", StringResource::Get(IDS_COMMON_Max_Results_Reached), maxResults_));
            Trace.WriteNoise(TraceSource, "Max of {0} entries reached for paged list. Received error {1} {2}", maxResults_, error, error.Message);
            return error;
        }
    }

    entries_.push_back(move(entry));
    currentSize_ = estimatedSize;
    return ErrorCode::Success();
}

template<class T>
void ListPager<T>::TraceErrorFromTryAdd(Common::ErrorCode const & error, Common::StringLiteral const & traceComponent, wstring const & traceId, wstring const & queryName)
{
    // We shouldn't need to trace the query arguments because if there was anything wrong with them, it should have failed
    // long ago or in other places. The error will not be from here.
    // TODO: the tracing isn't from the component itself anymore. Does that matter?
    if (error.IsError(ErrorCodeValue::EntryTooLarge) || error.IsError(ErrorCodeValue::MaxResultsReached))
    {
        Trace.WriteInfo(
            traceComponent,
            "{0}: TryAdd for query {1} returned an error expected as part of normal operation of this query. The error is {2} {3}.",
            traceId,
            queryName,
            error,
            error.Message);
    }
    else if (!error.IsSuccess()) // unexpected error
    {
        Trace.WriteInfo(
            traceComponent,
            "{0}: failed to add entry for query {1} to the pager: error = {2} {3}",
            traceId,
            queryName,
            error,
            error.Message);
    }
}

template<class T>
bool ListPager<T>::IsBenignError(Common::ErrorCode const & error)
{
    return (error.IsError(ErrorCodeValue::EntryTooLarge) || error.IsError(ErrorCodeValue::MaxResultsReached));
}

namespace ServiceModel {
    template class ListPager<NodeAggregatedHealthState>;
    template class ListPager<ApplicationAggregatedHealthState>;
    template class ListPager<ServiceAggregatedHealthState>;
    template class ListPager<PartitionAggregatedHealthState>;
    template class ListPager<ReplicaAggregatedHealthState>;
    template class ListPager<DeployedServicePackageAggregatedHealthState>;
    template class ListPager<DeployedApplicationAggregatedHealthState>;

    template class ListPager<NodeQueryResult>;
    template class ListPager<ApplicationQueryResult>;
    template class ListPager<ComposeDeploymentStatusQueryResult>;
    template class ListPager<ReplicaResourceQueryResult>;
    template class ListPager<ServiceQueryResult>;

    template class ListPager<ServicePartitionQueryResult>;
    template class ListPager<ServiceReplicaQueryResult>;

    template class ListPager<ApplicationTypeQueryResult>;
    template class ListPager<DeployedApplicationQueryResult>;
    template class ListPager<ReplicasByServiceQueryResult>;
    template class ListPager<ModelV2::ApplicationDescriptionQueryResult>;
    template class ListPager<ModelV2::ContainerServiceQueryResult>;
    template class ListPager<ApplicationUnhealthyEvaluation>;
    template class ListPager<ModelV2::VolumeQueryResult>;
}

template<class TKey, class TEntry>
MergedQueryListPager<TKey, TEntry>::MergedQueryListPager()
    : pager_()
    , mergedContinuationTokenData_()
    , sortedResults_()
{
}

template<class TKey, class TEntry>
MergedQueryListPager<TKey, TEntry>::~MergedQueryListPager()
{
}

template<class TKey, class TEntry>
bool MergedQueryListPager<TKey, TEntry>::RespectsContinuationToken(TKey const & data) const
{
    return !mergedContinuationTokenData_ || (data <= *mergedContinuationTokenData_);
}

// Set the paging status of pager_ based on new incoming data.
// pager_ is a member variable of the MergedQueryListPager class and is a ListPager object.
// On parallel query calls, there can be multiple continuation tokens returned from the multiple queries.
// We want to take the smallest continuation token, to ensure that we are returning the most comprehensive data.
// If one of the queries returns nothing, then we simply ignore that.
template<class TKey, class TEntry>
Common::ErrorCode MergedQueryListPager<TKey, TEntry>::MergePagingStatus(
    Common::ActivityId const & activityId,
    std::string const & subQuery,
    PagingStatusUPtr && pagingStatus)
{
    if (pagingStatus && !pagingStatus->ContinuationToken.empty())
    {
        TKey data;
        auto error = ServiceModel::PagingStatus::GetContinuationTokenData<TKey>(pagingStatus->ContinuationToken, data);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(
                TraceSource,
                "{0}: Error '{1}' {2} while parsing continuation token from sub-query '{3}'",
                activityId,
                error,
                error.Message,
                subQuery);
            return error;
        }

        if (!mergedContinuationTokenData_)
        {
            mergedContinuationTokenData_ = Common::make_unique<TKey>(move(data));
            pager_.SetPagingStatus(move(pagingStatus));
        }
        else if (data < *mergedContinuationTokenData_)
        {
            *mergedContinuationTokenData_ = move(data);
            // Set the continuation token to the min continuation token returned by the sub-queries
            pager_.SetPagingStatus(move(pagingStatus));
        }
    }

    return Common::ErrorCode::Success();
}

template<class TKey, class TEntry>
void MergedQueryListPager<TKey, TEntry>::Add(TKey && key, TEntry && entry)
{
    if (RespectsContinuationToken(key))
    {
        sortedResults_.insert(std::pair<TKey &&, TEntry &&>(std::move(key), std::move(entry)));
    }
}

template<class TKey, class TEntry>
void MergedQueryListPager<TKey, TEntry>::Add(TKey const & key, TEntry && entry)
{
    if (RespectsContinuationToken(key))
    {
        sortedResults_.insert(std::pair<TKey, TEntry &&>(key, std::move(entry)));
    }
}

template<class TKey, class TEntry>
void MergedQueryListPager<TKey, TEntry>::UpdateHealthStates(
    Common::ActivityId const & activityId,
    std::map<TKey, FABRIC_HEALTH_STATE> const & healthStates)
{
    for (auto & itSortedResultItem : sortedResults_)
    {
        auto healthInfo = healthStates.find(itSortedResultItem.first);
        if (healthInfo != healthStates.end())
        {
            itSortedResultItem.second.HealthState = healthInfo->second;
        }
        else
        {
            Trace.WriteInfo(
                TraceSource,
                "{0}: {1}: no health info",
                activityId,
                itSortedResultItem.first);
        }
    }
}

// Get a ListPager item from a MergedQueryListPager
template<class TKey, class TEntry>
ListPager<TEntry> && MergedQueryListPager<TKey, TEntry>::TakePager(Common::ActivityId const & activityId)
{
    for (auto & itSortedResultItem : sortedResults_)
    {
        auto mergeError = pager_.TryAdd(move(itSortedResultItem.second));
        if (mergeError.IsError(ErrorCodeValue::EntryTooLarge))
        {
            Trace.WriteInfo(
                TraceSource,
                "{0}: TryAdd {1}: {2}, {3}",
                activityId,
                itSortedResultItem.first,
                mergeError,
                mergeError.Message);
            break;
        }
    }

    return move(pager_);
}

//
// Template specializations
//
namespace ServiceModel {
    template class MergedQueryListPager<Federation::NodeId, NodeQueryResult>;
    template class MergedQueryListPager<std::wstring, ApplicationQueryResult>;
    template class MergedQueryListPager<std::wstring, DeployedApplicationQueryResult>;
    template class MergedQueryListPager<std::wstring, ServiceQueryResult>;
    template class MergedQueryListPager<Common::Guid, ServicePartitionQueryResult>;
    template class MergedQueryListPager<FABRIC_REPLICA_ID, ServiceReplicaQueryResult>;
    template class MergedQueryListPager<std::wstring, ModelV2::ApplicationDescriptionQueryResult>;
    template class MergedQueryListPager<std::wstring, ModelV2::ContainerServiceQueryResult>;
}

