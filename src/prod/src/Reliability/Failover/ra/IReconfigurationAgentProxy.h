// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent 
    { 
        // Portion of reconfiguration agent that resides in the service host
        class IReconfigurationAgentProxy : 
            public Common::AsyncFabricComponent
        {
            DENY_COPY(IReconfigurationAgentProxy);

        public:
            IReconfigurationAgentProxy() {}

            virtual ~IReconfigurationAgentProxy() {}
        };

        struct ReconfigurationAgentProxyConstructorParameters
        {
            Hosting2::IApplicationHost* ApplicationHost;
            Transport::IpcClient * IpcClient;
            Client::IpcHealthReportingClientSPtr IpcHealthReportingClient;
            Client::IpcHealthReportingClientSPtr ThrottledIpcHealthReportingClient;
            Reliability::ReplicationComponent::IReplicatorFactory * ReplicatorFactory;
            TxnReplicator::ITransactionalReplicatorFactory * TransactionalReplicatorFactory;
            Common::ComponentRoot const * Root;
        };

        typedef IReconfigurationAgentProxyUPtr ReconfigurationAgentProxyFactoryFunctionPtr(ReconfigurationAgentProxyConstructorParameters &);

        ReconfigurationAgentProxyFactoryFunctionPtr ReconfigurationAgentProxyFactory;
    }
}
