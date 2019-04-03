// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class ComProxyGatewayResourceManager :
        public ComProxySystemServiceBase<IFabricGatewayResourceManager>,
        public IGatewayResourceManager
    {
        DENY_COPY(ComProxyGatewayResourceManager);

    public:
        ComProxyGatewayResourceManager(Common::ComPointer<IFabricGatewayResourceManager> const & comImpl);
        virtual ~ComProxyGatewayResourceManager();

        virtual Common::AsyncOperationSPtr BeginCreateOrUpdateGatewayResource(
            std::wstring const & resourceDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);
        virtual Common::ErrorCode EndCreateOrUpdateGatewayResource(
            Common::AsyncOperationSPtr const &,
            __out IFabricStringResult ** resourceDescription);

        virtual Common::AsyncOperationSPtr BeginGetGatewayResourceList(
            std::wstring const & queryDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);
        virtual Common::ErrorCode EndGetGatewayResourceList(
            Common::AsyncOperationSPtr const &,
            IFabricStringListResult ** queryResult);

        virtual Common::AsyncOperationSPtr BeginDeleteGatewayResource(
            std::wstring const & resourceName,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);
        virtual Common::ErrorCode EndDeleteGatewayResource(
            Common::AsyncOperationSPtr const &);

    private:
        class ProcessQueryAsyncOperation;
        class CreateGatewayResourceAsyncOperation;
        class DeleteGatewayResourceAsyncOperation;
    };
}
