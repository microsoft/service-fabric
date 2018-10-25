// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class QueryResult
        : public ClientServerMessageBody
    {
        DENY_COPY(QueryResult)

    public:
        // For serializer
        QueryResult();

        QueryResult(Common::ErrorCode const & queryProcessingError);
        QueryResult(Common::ErrorCode && queryProcessingError);
        QueryResult(Common::ErrorCodeValue::Enum queryProcessingError);

        QueryResult(QueryResult && other);
        QueryResult & operator = (QueryResult && other);

        ~QueryResult();

        //
        // QueryResultList
        //

        template <class TServiceModel>
        QueryResult(std::vector<TServiceModel> && queryResultList)
            : resultKind_(FABRIC_QUERY_RESULT_KIND_LIST)
            , resultWrapper_()
            , queryProcessingError_(Common::ErrorCodeValue::Success)
            , pagingStatus_()
        {
            static_assert(QueryResultListWrapper<TServiceModel>::IsSupportedQueryResultItem, "Type TServiceModel is not supported. Use DEFINE_QUERY_RESULT_LIST_ITEM to add it to supported types");
            resultWrapper_ = QueryResultListWrapper<TServiceModel>::CreateListWrapper(move(queryResultList));
        }

        template <class TServiceModel>
        QueryResult(
            std::vector<TServiceModel> && queryResultList,
            ServiceModel::PagingStatusUPtr && pagingStatus)
            : resultKind_(FABRIC_QUERY_RESULT_KIND_LIST)
            , resultWrapper_()
            , queryProcessingError_(Common::ErrorCodeValue::Success)
            , pagingStatus_(std::move(pagingStatus))
        {
            static_assert(QueryResultListWrapper<TServiceModel>::IsSupportedQueryResultItem, "Type TServiceModel is not supported. Use DEFINE_QUERY_RESULT_LIST_ITEM to add it to supported types");
            resultWrapper_ = QueryResultListWrapper<TServiceModel>::CreateListWrapper(move(queryResultList));
        }

        template <class TServiceModel>
        QueryResult(ListPager<TServiceModel> && pager)
            : resultKind_(FABRIC_QUERY_RESULT_KIND_LIST)
            , resultWrapper_()
            , queryProcessingError_(Common::ErrorCodeValue::Success)
            , pagingStatus_()
        {
            static_assert(QueryResultListWrapper<TServiceModel>::IsSupportedQueryResultItem, "Type TServiceModel is not supported. Use DEFINE_QUERY_RESULT_LIST_ITEM to add it to supported types");
            resultWrapper_ = QueryResultListWrapper<TServiceModel>::CreateListWrapper(move(pager.TakeEntries()));
            pagingStatus_ = pager.TakePagingStatus();
        }

        bool HasList() { return (resultWrapper_ != nullptr); }

        //
        // QueryResultItem
        //

        template <class TServiceModel>
        QueryResult(std::unique_ptr<TServiceModel> && queryResultItem)
            : resultKind_(FABRIC_QUERY_RESULT_KIND_ITEM)
            , resultWrapper_()
            , queryProcessingError_(Common::ErrorCode::Success())
            , pagingStatus_()
        {
            static_assert(QueryResultListWrapper<TServiceModel>::IsSupportedQueryResultItem, "Type TServiceModel is not supported. Use DEFINE_QUERY_RESULT_LIST_ITEM to add it to supported types");
            resultWrapper_ = QueryResultListWrapper<TServiceModel>::CreateItemWrapper(move(queryResultItem));
        }

        template<class TServiceModel>
        Common::ErrorCode MoveList(__out std::vector<TServiceModel> & itemList)
        {
            if (!queryProcessingError_.IsSuccess())
            {
                return queryProcessingError_;
            }

            ASSERT_IF(resultWrapper_ == nullptr, "Item list is already moved or not initialized");
            QueryResultListWrapper<TServiceModel>* castedResultWrapper = nullptr;
            auto error = CastWrapper(castedResultWrapper);
            if(!error.IsSuccess()) { return error;}
            itemList = move(castedResultWrapper->ItemList);

            // As the actual data stored in listItemWrapper is moved, we can set it to nullptr.
            // We check it at the beginning of this method to avoid double move.
            resultWrapper_ = nullptr;
            return Common::ErrorCode::Success();
        }

        template<class TServiceModel>
        Common::ErrorCode GetList(__out std::vector<TServiceModel> & itemList)
        {
            if (!queryProcessingError_.IsSuccess())
            {
                return queryProcessingError_;
            }

            ASSERT_IF(resultWrapper_ == nullptr, "Item list is already moved or not initialized");
            QueryResultListWrapper<TServiceModel>* castedResultWrapper = nullptr;
            auto error = CastWrapper(castedResultWrapper);
            if(!error.IsSuccess()) { return error;}

            itemList = castedResultWrapper->ItemList;
            return Common::ErrorCode::Success();
        }

        template<class TServiceModel>
        Common::ErrorCode MoveItem(__out TServiceModel & item)
        {
            if (!queryProcessingError_.IsSuccess())
            {
                return queryProcessingError_;
            }

            ASSERT_IF(resultWrapper_ == nullptr, "Item is already moved or not initialized");
            QueryResultListWrapper<TServiceModel>* castedResultWrapper = nullptr;
            auto error = CastWrapper(castedResultWrapper);
            if(!error.IsSuccess()) { return error;}
            item = std::move(castedResultWrapper->Item);

            // As the actual data stored in listItemWrapper is moved, we can set it to nullptr.
            // We check it at the beginning of this method to avoid double move.
            resultWrapper_ = nullptr;
            return Common::ErrorCode::Success();
        }

        __declspec(property(get=get_QueryProcessingError)) Common::ErrorCode const & QueryProcessingError;
        Common::ErrorCode const & get_QueryProcessingError() const { return queryProcessingError_; }

        __declspec(property(get=get_ResultKind)) FABRIC_QUERY_RESULT_KIND ResultKind;
        FABRIC_QUERY_RESULT_KIND get_ResultKind() const { return resultKind_; }

        PagingStatusUPtr MovePagingStatus() { return std::move(pagingStatus_); }

        QueryResultListWrapperBase const & Test_GetResultWrapperBase() const
        {
            return *resultWrapper_;
        }

        void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_QUERY_RESULT & queryResult);

        FABRIC_FIELDS_04(resultKind_, resultWrapper_, queryProcessingError_, pagingStatus_)

    private:

        template<class TServiceModel>
        Common::ErrorCode CastWrapper(__out QueryResultListWrapper<TServiceModel>* & queryResultWrapper)
        {
            queryResultWrapper = dynamic_cast<QueryResultListWrapper<TServiceModel>*>(resultWrapper_.get());

            if (queryResultWrapper == nullptr)
            {
                // If it has invalid kind, it indicates a serialization error.
                // otherwise there is a configuration issue where the caller is expecting a different type that what has been
                // set by the query handler.
                if (resultWrapper_->ResultKind == QueryResultHelpers::Invalid)
                {
                    return Common::ErrorCodeValue::SerializationError;
                }
                else
                {
                    return Common::ErrorCodeValue::InvalidConfiguration;
                }
            }

            return Common::ErrorCode::Success();
        }

        FABRIC_QUERY_RESULT_KIND resultKind_;
        std::unique_ptr<QueryResultListWrapperBase> resultWrapper_;
        Common::ErrorCode queryProcessingError_;
        PagingStatusUPtr pagingStatus_;
    };
}
