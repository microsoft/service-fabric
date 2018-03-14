// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class IGatewayManager
    {
    public:
        virtual bool RegisterGatewaySettingsUpdateHandler() = 0;

        virtual Common::AsyncOperationSPtr BeginActivateGateway(
            std::wstring const&,
            Common::TimeSpan const&,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndActivateGateway(
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginDeactivateGateway(
            Common::TimeSpan const&,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndDeactivateGateway(
            Common::AsyncOperationSPtr const &) = 0;

        virtual void AbortGateway() = 0;
    };

    typedef std::shared_ptr<IGatewayManager> IGatewayManagerSPtr;
}
