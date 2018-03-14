// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IInfrastructureServiceClient
    {
    public:
        virtual ~IInfrastructureServiceClient() {};

        virtual Common::AsyncOperationSPtr BeginInvokeInfrastructureCommand(
            bool isAdminOperation,
            Common::NamingUri const & serviceName,
            std::wstring const & command,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::ErrorCode EndInvokeInfrastructureCommand(
            Common::AsyncOperationSPtr const &,
            std::wstring &) = 0;
    };
}
