// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IGatewayResourceManagerClient
    {
    public:
        virtual ~IGatewayResourceManagerClient() {};

        //
        // Gateway resource
        //
        virtual Common::AsyncOperationSPtr BeginCreateOrUpdateGatewayResource(
            std::wstring && description,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndCreateOrUpdateGatewayResource(
            Common::AsyncOperationSPtr const &,
            __out std::wstring & description) = 0;

        virtual Common::AsyncOperationSPtr BeginGetGatewayResourceList(
            ServiceModel::ModelV2::GatewayResourceQueryDescription && gatewayQueryDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndGetGatewayResourceList(
            Common::AsyncOperationSPtr const &,
            std::vector<std::wstring> & list,
            ServiceModel::PagingStatusUPtr & pagingStatus) = 0;

        virtual Common::AsyncOperationSPtr BeginDeleteGatewayResource(
            std::wstring const & resourceName,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndDeleteGatewayResource(
            Common::AsyncOperationSPtr const &) = 0;

    };
}
