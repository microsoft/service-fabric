// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace web
{
    namespace http
    {
        namespace client
        {
             class http_client;
        }
    }
}

namespace Hosting2
{
    class IPAMLinux :
        public IPAM
    {
        DENY_COPY(IPAMLinux)

    public:

        IPAMLinux(
            Common::ComponentRootSPtr const & root);

        virtual ~IPAMLinux();

        Common::AsyncOperationSPtr BeginInitialize(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndInitialize(
            Common::AsyncOperationSPtr const & operation);

        Common::ErrorCode StartRefreshProcessing(
            Common::TimeSpan const refreshInterval,
            Common::TimeSpan const refreshTimeout,
            std::function<void(Common::DateTime)> ghostChangeCallback);

    private:

        // These are two internal helper classes used to drive the internal
        // async operations
        class RefreshPoolAsyncOperation;
        class RetrieveNmAgentDataAyncOperation;

        // InternalStartRefreshProcessing: Helper method that initiates a new
        // refresh cycle, using the parameters set in this instance.
        //
        // Returns:
        //  success, if the operation was started; failure, if not.
        //
        Common::ErrorCode InternalStartRefreshProcessing();

        // OnRefreshComplete: This method is called when a refresh cycle
        // concludes.  It is responsible for starting the next cycle.
        //
        // The arguments are:
        //	operation: The current async operation context
        //  expectedCompletedSynchronously: true, to process synchronous results; 
        //                                  false, to process asynchronous results.
        //
        void OnRefreshComplete(
            Common::AsyncOperationSPtr const & operation,
            bool expectedCompletedSynchronously);

        // Holds the client to use for http operations.  This is set on the 
        // constructor and never modified, so does not need to be protected
        // by the lock below.
        //
#if defined(PLATFORM_UNIX)
        std::unique_ptr<web::http::client::http_client> httpClient_;
#endif
    };
}
