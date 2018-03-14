// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FileStoreService
    {
        class ListRequest : public ServiceModel::ClientServerMessageBody
        {
            DENY_COPY(ListRequest)

        public:
            ListRequest();

            ListRequest(std::wstring const & storeRelativePath);

            ListRequest(
                std::wstring const & storeRelativePath,
                BOOLEAN const & shouldIncludeDetails,
                BOOLEAN const & isRecursive);

            ListRequest(
                std::wstring const & storeRelativePath,
                BOOLEAN const & shouldIncludeDetails,
                BOOLEAN const & isRecursive,
                std::wstring const & continuationToken,
                bool isPaging);

            __declspec(property(get = get_StoreRelativePath)) std::wstring const & StoreRelativePath;
            inline std::wstring const & get_StoreRelativePath() const { return storeRelativePath_; }

           
            __declspec(property(get = get_ShouldIncludeDetails)) BOOLEAN const & ShouldIncludeDetails;
            inline BOOLEAN const & get_ShouldIncludeDetails() const { return shouldIncludeDetails_; }

            __declspec(property(get = get_IsRecursive)) BOOLEAN const & IsRecursive;
            inline BOOLEAN const & get_IsRecursive() const { return isRecursive_; }

            __declspec(property(get = get_ContinuationToken)) std::wstring const & ContinuationToken;
            inline std::wstring const & get_ContinuationToken() const { return continuationToken_; }

            _declspec(property(get = get_IsPaging)) bool IsPaging;
            inline bool get_IsPaging() const { return isPaging_; }

            FABRIC_FIELDS_05(storeRelativePath_, shouldIncludeDetails_, isRecursive_, continuationToken_, isPaging_);

        private:
            std::wstring storeRelativePath_;
            BOOLEAN shouldIncludeDetails_;
            BOOLEAN isRecursive_;
            std::wstring continuationToken_;
            bool isPaging_;
        };
    }
}
