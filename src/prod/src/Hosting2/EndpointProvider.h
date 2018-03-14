// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class EndpointProvider : 
        public Common::RootedObject,
        public Common::FabricComponent,
        private Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(EndpointProvider)

    public:
        EndpointProvider(
            Common::ComponentRoot const & root, 
            std::wstring const & nodeId_, 
            int startPortRangeInclusive, 
            int endPortRangeInclusive);
        virtual ~EndpointProvider();

        Common::ErrorCode AddEndpoint(EndpointResourceSPtr const & endpoint);
        Common::ErrorCode RemoveEndpoint(EndpointResourceSPtr const & endpoint);

        static Common::ErrorCode GetPrincipal(
            EndpointResourceSPtr const & endpointResource, 
            __out SecurityPrincipalInformationSPtr & principal);

        static bool IsHttpEndpoint(EndpointResourceSPtr const & endpoint);

    protected:
        virtual Common::ErrorCode OnOpen();
        virtual Common::ErrorCode OnClose();
        virtual void OnAbort();

    private:

        Common::ErrorCode ClaimNextAvailablePort(UINT & claimedPort);
        void UnclaimPort(UINT const claimedPort);        
        
        static std::wstring GetHttpEndpoint(UINT port, bool isHttps = false);

    private:
        std::wstring const nodeId_;        
        int startPortRangeInclusive_;
        int endPortRangeInclusive_;
        int currentOffset_;
        int maxPortsAvailable_;
        Common::ExclusiveLock portReservationLock_;
        std::set<UINT> reservedPorts_;
    };
}
