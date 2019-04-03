// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class IPAMWindows :
        public IPAM
    {
        DENY_COPY(IPAMWindows)

    public:

        // IPAMWindows: Construct an instance that has a windows http client injected.
        // This is used with a mock http client for unit tests.
        // The arguments are:
        //  client: injected windows http client
        //  root: parent object for this instance
        //
        IPAMWindows(
            HttpClient::IHttpClientSPtr &client,
            Common::ComponentRootSPtr const & root);

        IPAMWindows(
            Common::ComponentRootSPtr const & root);

        virtual ~IPAMWindows();

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
        // constructor and never modified.
        HttpClient::IHttpClientSPtr client_;
    };
}
