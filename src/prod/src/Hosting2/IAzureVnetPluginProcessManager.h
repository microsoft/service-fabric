// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    // 
    // This class manages lifetime of the azure vnet plugin process.
    //
    class IAzureVnetPluginProcessManager :
       public Common::AsyncFabricComponent
    {
        DENY_COPY(IAzureVnetPluginProcessManager)

    public:

        IAzureVnetPluginProcessManager() {};

        virtual ~IAzureVnetPluginProcessManager() {};

        virtual Common::AsyncOperationSPtr OnBeginOpen(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode OnEndOpen(
            Common::AsyncOperationSPtr const & operation) = 0;

        virtual Common::AsyncOperationSPtr OnBeginClose(
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::ErrorCode OnEndClose(Common::AsyncOperationSPtr const &) = 0;

        virtual void OnAbort() = 0;
    };
}
