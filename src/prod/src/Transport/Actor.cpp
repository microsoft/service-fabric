// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Transport
{
    namespace Actor
    {
        void WriteToTextWriter(__in Common::TextWriter & w, Enum const & enumValue)
        {
            switch (enumValue)
            {
            case Empty: w << "Empty"; return;

            // Transport
            case Transport: w << "Transport"; return;
            case Ipc: w << "Ipc"; return;
            case TransportSendTarget: w << "TransportSendTarget"; return;
            case Actor::SecurityContext: w << "SecurityContext"; return;

            // Federation
            case Federation: w << "Federation"; return;
            case Routing: w << "Routing"; return;

            // Cluster Manager
            case CM: w << "CM"; return;

            // Naming
            case NamingGateway: w << "NamingGateway"; return;
            case NamingStoreService: w << "NS"; return;

            // Hosting
            case Hosting: w << "Hosting"; return;
            case ApplicationHostManager: w << "ApplicationHostManager"; return;
            case ApplicationHost: w << "ApplicationHost"; return;
            case FabricRuntimeManager: w << "FabricRuntimeManager"; return;
                                       
            // Health Manager
            case HM: w << "HM"; return;

            // Failover
            case FMM: w << "FMM"; return;
            case FM: w << "FM"; return;
            case RA: w << "RA"; return;
            case RS: w << "RS"; return;
            case ServiceResolver: w << "ServiceResolver"; return;

            case ServiceRoutingAgent: w << "ServiceRoutingAgent"; return;
            case IS: w << "IS"; return;
            case Tvs: w << "TokenValidationService"; return;

            // Repair Manager
            case RM: w << "RM"; return;
            
            // Fault Analysis Service
            case FAS: w << "FAS"; return;

            // Upgrade Orchestration Service
            case UOS: w << "UOS"; return;

            //Fabric Activator
            case FabricActivator: w << "FabricActivator"; return;
            case HostedServiceActivator: w << "HostedServiceActivator"; return;
            case FabricActivatorClient: w << "FabricActivatorClient"; return;

            //Fabric Transfer
            case FileSender: w << "FileSender"; return;
            case FileReceiver: w << "FileReceiver"; return;    
            case FileTransferClient: w << "FileTransferClient"; return;
            case FileTransferGateway: w << "FileTransferGateway"; return;

            case EntreeServiceProxy: w << "EntreeServiceProxy"; return;
            case EntreeServiceTransport: w << "EntreeServiceTransport"; return;

            case NM: w << "NM"; return;
            case DirectMessagingAgent: w << "DirectMessagingAgent"; return;

            //FabricServiceCommunication
            case ServiceCommunicationActor:w << "DirectMessagingAgent"; return;

            // BackupRestore service.
            case BA: w << "BA"; return;
            case BRS: w << "BRS"; return;
            case BAP: w << "BAP"; return;
            
            case ContainerActivatorService: w << "ContainerActivatorService"; return;
            case ContainerActivatorServiceClient: w << "ContainerActivatorServiceClient"; return;

            // Central Secret Service
            case CSS: w << "CSS"; return;

            case GatewayResourceManager: w << "GatewayResourceManager"; return;

            // NetworkInventoryService
            case NetworkInventoryService: w << "NetworkInventoryService"; return;
            
            // NetworkInventoryAgent
            case NetworkInventoryAgent: w << "NetworkInventoryAgent"; return;

            // Test
            case WindowsFabricTestApi: w << "WindowsFabricTestApi"; return;
            case GenericTestActor: w << "GenericTestActor"; return;
            case GenericTestActor2: w << "GenericTestActor2"; return;
            case DistributedSession: w << "DistributedSession"; return;
            case IpcTestActor1: w << "IpcTestActor1"; return;
            case IpcTestActor2: w << "IpcTestActor2"; return;

            case ResourceMonitor: w << "ResourceMonitor"; return;

            default: w << "Actor(" << static_cast<int>(enumValue) << ')'; return;
            }
        }

        ENUM_STRUCTURED_TRACE( Actor, FirstValidEnum, LastValidEnum )
    }
}