// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IGatewayResourceManager
    {
    public:
        virtual ~IGatewayResourceManager() {};

        virtual Common::AsyncOperationSPtr BeginCreateOrUpdateGatewayResource(
            std::wstring const & resourceDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;


        virtual Common::ErrorCode EndCreateOrUpdateGatewayResource(
            Common::AsyncOperationSPtr const &,
            __out IFabricStringResult ** resourceDescription) = 0;

        virtual Common::AsyncOperationSPtr BeginGetGatewayResourceList(
            std::wstring const & queryDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::ErrorCode EndGetGatewayResourceList(
            Common::AsyncOperationSPtr const &,
            __out IFabricStringListResult ** queryResult) = 0;


        virtual Common::AsyncOperationSPtr BeginDeleteGatewayResource(
            std::wstring const & resourceName,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::ErrorCode EndDeleteGatewayResource(
            Common::AsyncOperationSPtr const &) = 0;
    };
}
