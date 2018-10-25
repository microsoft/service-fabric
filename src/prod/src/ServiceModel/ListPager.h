// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    // Class that keeps a list of items that can fit the desired max size.
    // The items must implement IPageContinuationToken interface.
    // There is no check for sorted items, the caller must make sure the items are added in sorted fashion.
    // The expectation is that the items are sorted ASC and the continuation token represents the largest ID in the vector.
    template <class T>
    class ListPager
    {
        DENY_COPY(ListPager)

    public:
        ListPager();
        ~ListPager();

        __declspec(property(get = get_Entries)) std::vector<T> const & Entries;
        std::vector<T> const & get_Entries() const { return entries_; }

        __declspec(property(get = get_ContinuationToken)) std::wstring const & ContinuationToken;
        std::wstring const & get_ContinuationToken() const { return pagingStatus_ ? pagingStatus_->ContinuationToken : *Constants::EmptyString; }

        std::vector<T> TakeEntries() { return std::move(entries_); }

        void SetPagingStatus(PagingStatusUPtr && pagingStatus) { pagingStatus_ = std::move(pagingStatus); }
        PagingStatusUPtr TakePagingStatus() { return std::move(pagingStatus_); }

        // If the maxResults value is invalid, then we will simply ignore the value
        // Value is invalid if it is zero or negative.
        void SetMaxResults(int64 maxResults);
        void OverrideContinuationTokenManual(__in std::wstring & continuationToken);

        Common::ErrorCode TryAdd(T && entry);

        void TraceErrorFromTryAdd(Common::ErrorCode const & error, Common::StringLiteral const & traceComponent, wstring const & traceId, wstring const & queryName);

        // Returns true if error is benign (expected as part of normal operations, such as max results reached)
        bool IsBenignError(Common::ErrorCode const & error);

    private:
        size_t maxAllowedSize_;
        size_t currentSize_;
        std::vector<T> entries_;
        PagingStatusUPtr pagingStatus_;
        int64 maxResults_;
    };

    template <class TKey, class TEntry>
    class MergedQueryListPager
    {
        DENY_COPY(MergedQueryListPager)

    public:
        MergedQueryListPager();
        ~MergedQueryListPager();

        __declspec(property(get = get_MergedContinuationTokenData)) std::unique_ptr<TKey> const & MergedContinuationTokenData;
        std::unique_ptr<TKey> const & get_MergedContinuationTokenData() const { return mergedContinuationTokenData_; }

        __declspec(property(get = get_ContinuationToken)) std::wstring const & ContinuationToken;
        std::wstring const & get_ContinuationToken() const { return pager_.ContinuationToken; }

        Common::ErrorCode MergePagingStatus(
            Common::ActivityId const & activityId,
            std::string const & subQuery,
            PagingStatusUPtr && pagingStatus);

        bool RespectsContinuationToken(TKey const & data) const;

        void Add(TKey && key, TEntry && entry);
        void Add(TKey const & key, TEntry && entry);

        ListPager<TEntry> && TakePager(Common::ActivityId const & activityId);

        void UpdateHealthStates(
            Common::ActivityId const & activityId,
            std::map<TKey, FABRIC_HEALTH_STATE> const & healthStates);

    private:
        ListPager<TEntry> pager_;
        std::unique_ptr<TKey> mergedContinuationTokenData_;
        std::map<TKey, TEntry> sortedResults_;
    };
}
