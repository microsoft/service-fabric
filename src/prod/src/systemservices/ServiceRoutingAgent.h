// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace SystemServices
{
    class ServiceRoutingAgent;
    typedef std::unique_ptr<ServiceRoutingAgent> ServiceRoutingAgentUPtr;

    class ServiceRoutingAgent : public Common::FabricComponent
    {
    public:

        ~ServiceRoutingAgent();

        static ServiceRoutingAgentUPtr Create(
            __in Transport::IpcServer &,
            __in Federation::FederationSubsystem &, 
            __in Hosting2::IHostingSubsystem &, 
            __in Naming::IGateway &,
            Common::ComponentRoot const &);

    protected:
        Common::ErrorCode OnOpen();
        Common::ErrorCode OnClose();
        void OnAbort();

    private:
        ServiceRoutingAgent(
            __in Transport::IpcServer &,
            __in Federation::FederationSubsystem &, 
            __in Hosting2::IHostingSubsystem &, 
            __in Naming::IGateway &,
            Common::ComponentRoot const &);

        class Impl;
        std::unique_ptr<ServiceRoutingAgent::Impl> implUPtr_;
    };
}
