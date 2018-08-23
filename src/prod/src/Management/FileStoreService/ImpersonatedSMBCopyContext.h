// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FileStoreService
    {
        class ImpersonatedSMBCopyContext;
        using ImpersonatedSMBCopyContextSPtr = std::shared_ptr<ImpersonatedSMBCopyContext>;

        // Function that get access tokens that are not in the map,
        // and saves them both in the tokens list and the map.
        using GetAccessTokensCallback = std::function<Common::ErrorCode(
            __inout AccessTokensCollection & tokenMap,
            __inout AccessTokensList & tokens)>;

        class ImpersonatedSMBCopyContext
            : private Common::TextTraceComponent<Common::TraceTaskCodes::FileStoreService>
        {
        public:
            static Common::ErrorCode Create(__inout ImpersonatedSMBCopyContextSPtr & smbCopyContext);

            static bool IsRetryableError(ErrorCode const & error);

            static bool IsRetryableNetworkError(ErrorCode const & error);

            ImpersonatedSMBCopyContext(
                AccessTokensCollection && accessTokens,
                GetAccessTokensCallback getAccessTokensCallback);

            Common::ErrorCode CopyFile(
                std::wstring const & sourcePath,
                std::wstring const & destinationPath);

        private:
            // Impersonate access tokens from the specified list
            Common::ErrorCode ImpersonateAndCopyFile(
                std::wstring const & sourcePath,
                std::wstring const & destinationPath,
                AccessTokensList const & accessTokens);

            // Impersonate specified access token
            Common::ErrorCode ImpersonateAndCopyFile(
                std::wstring const & sourcePath,
                std::wstring const & destinationPath,
                Common::AccessTokenSPtr const & accessToken);

            // Map with access tokens per account name
            AccessTokensCollection accessTokens_;
            // Callback to update the access tokens when the impersonation fails
            GetAccessTokensCallback getAccessTokensCallback_;
            // Last access token refresh. Needed to prevent too aggressive refresh.
            Common::StopwatchTime lastTokenRefreshTime_;
            // Lock that protects the access tokens for being modified by multiple threads serving multiple copy file operations.
            Common::RwLock lock_;
            // Trace id for the copy context.
            std::wstring traceId_;
        };
    }
}
