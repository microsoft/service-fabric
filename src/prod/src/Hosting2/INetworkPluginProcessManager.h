// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    // 
    // This class manages lifetime of the specified network plugin process.
    //
    class INetworkPluginProcessManager : 
        public Common::AsyncFabricComponent
    {
        DENY_COPY(INetworkPluginProcessManager)

    public:

        INetworkPluginProcessManager() {};

        virtual ~INetworkPluginProcessManager() {};

    protected:

        // AsyncFabricComponent methods
        virtual Common::AsyncOperationSPtr OnBeginOpen(
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode OnEndOpen(
            Common::AsyncOperationSPtr const & operation) = 0;

        virtual Common::AsyncOperationSPtr OnBeginClose(
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode OnEndClose(
            Common::AsyncOperationSPtr const & operation) = 0;

        virtual void OnAbort() = 0;
    };
}