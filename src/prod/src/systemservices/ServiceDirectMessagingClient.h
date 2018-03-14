// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace SystemServices
{
    class ServiceDirectMessagingClient;
    typedef std::shared_ptr<ServiceDirectMessagingClient> ServiceDirectMessagingClientSPtr;

    class ServiceDirectMessagingClient
    {
    public:
        static ServiceDirectMessagingClientSPtr Create(
            __in Transport::IDatagramTransport &,
            __in SystemServiceResolver &,
            Common::ComponentRoot const &);

        Common::AsyncOperationSPtr BeginResolve(
            std::wstring const & serviceName,
            Common::ActivityId const &,
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);

        Common::ErrorCode EndResolve(
            Common::AsyncOperationSPtr const &, 
            __out SystemServiceLocation & primaryLocation,
            __out Transport::ISendTarget::SPtr & primaryTarget);

    private:
        ServiceDirectMessagingClient(
            __in Transport::IDatagramTransport &,
            __in SystemServiceResolver &,
            Common::ComponentRoot const &);

        class Impl;
        std::unique_ptr<ServiceDirectMessagingClient::Impl> implUPtr_;
    };
}
