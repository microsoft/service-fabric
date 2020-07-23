// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    namespace Actor
    {
        // !!! When adding a new enum value here, please also add its friendly string !!!
        // !!! name to Actor.cpp so that we can dump enum value in trace.   !!!
        enum Enum
        {
            Empty = 0,

            // Transport
            Transport = 1,

            // Federation
            Federation = 2,
            Routing = 3,

            // Cluster Manager
            CM = 4,

            // Naming
            NamingGateway = 5,
            NamingStoreService = 6,

            // Hosting
            ApplicationHostManager = 7,
            ApplicationHost = 8,
            FabricRuntimeManager = 9,

            // Failover
            FMM = 10,
            FM = 11,
            RA = 12,
            RS = 13,
            ServiceResolver = 14,

            // Hosting subsystem
            Hosting = 15,

            // HealthManager
            HM = 16,

            // Infrastructure Service
            ServiceRoutingAgent = 17,
            IS = 18,

            //Fabric Activator
            FabricActivator = 19,
            FabricActivatorClient = 20,

            // Transport
            Ipc = 21,

            // FileStoreService
            FileStoreService = 22,

            // TokenValidationService
            Tvs = 23,

            // Repair Manager
            RM = 24,

            FileSender = 25,
            FileReceiver = 26,
            FileTransferClient = 27,
            FileTransferGateway = 28,

            TransportSendTarget = 29,
            EntreeServiceProxy = 30,
            EntreeServiceTransport = 31,
            HostedServiceActivator = 32,

            NM = 33,
            DirectMessagingAgent = 34,

            SecurityContext = 35,
            ServiceCommunicationActor = 36,

            RestartManager = 37,
            RestartManagerClient = 38,

            // FaultAnalysisService
            FAS = 39,

            // TestabilityAgent
            TestabilitySubsystem = 40,

            // UpgradeOrchestrationService
            UOS = 41,

            // Backup Restore Agent.
            BA = 42,
            BRS = 43,
            BAP = 44,

            // Fabric Container Activator Service
            ContainerActivatorService = 45,
            ContainerActivatorServiceClient = 46,

            ResourceMonitor = 47,

            // Central Secret Service
            CSS = 48,

            // NetworkInventoryService
            NetworkInventoryService = 49,

            // NetworkInventoryAgent
            NetworkInventoryAgent = 50,

            // GatewayResourceManager Service
            GatewayResourceManager = 51,

            EndValidEnum = 52,

            // !!! Please add new actor values above this !!!
            FirstValidEnum = Empty,
            LastValidEnum = EndValidEnum - 1,

            // Test range
            WindowsFabricTestApi = 0xFFFF,
            GenericTestActor = 0x10000,
            GenericTestActor2 = 0x10001,
            DistributedSession = 0x10002,
            IpcTestActor1 = 0x10003,
            IpcTestActor2 = 0x10004,
        };

        void WriteToTextWriter(__in Common::TextWriter & w, Enum const & enumValue);

        DECLARE_ENUM_STRUCTURED_TRACE( Actor )
    }
}
