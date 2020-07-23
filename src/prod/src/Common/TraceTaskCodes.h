// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    namespace TraceTaskCodes
    {
        //
        // WARNING: Addition of new TraceTaskCode requires addition of 
        // event source in Trace.cpp:AddEventSource.
        //
        enum Enum : USHORT
        {
            // common
            Common = 1,
            Config = 2,
            Timer = 3,
            AsyncExclusiveLock = 4,
            PerfMonitor = 5,
            General = 10,
            FabricGateway = 11,
            Java = 12,
            Managed = 13,

            // transport
            Transport = 16,
            IPC = 17,
            UnreliableTransport = 18,

            // lease
            LeaseAgent = 19,

            // reliable messaging
            ReliableMessagingSession = 25,
            ReliableMessagingStream = 26,

            // Federation
            P2P = 48,
            RoutingTable = 49,
            Token = 50,
            VoteProxy = 51,
            VoteManager = 52,
            Bootstrap = 53,
            Join = 54,
            JoinLock = 55,
            Gap = 56,
            Lease = 57,
            Arbitration = 58,
            Routing = 59,
            Broadcast = 60,
            SiteNode = 61,
            Multicast = 62,

            // Failover
            Reliability = 64,
            Replication = 65,
            OperationQueue = 66,
            Api = 67,
            CRM = 68, //Public Name for PLB -- Cluster Resource Manager
            MCRM = 69, //Public Name for PLBMaster -- Master Cluster Resource Manager 
            FM = 72,
            RA = 73,
            RAP = 74,
            FailoverUnit = 75,
            FMM = 76,
            PLB = 77,
            PLBM = 78,

            // Store
            RocksDbStore = 79,
            EseStore = 80,
            ReplicatedStore = 81,
            ReplicatedStoreStateMachine = 82,
            SqlStore = 83,
            KeyValueStore=84,

            // Naming
            NM = 85,
            NamingStoreService = 86,
            NamingGateway = 87,
            NamingCommon = 88,
            HealthClient = 89,

            // Hosting
            Hosting = 90,
            FabricWorkerProcess = 91,
            ClrRuntimeHost = 92,
            FabricTypeHost = 93,

            // FabricNode
            FabricNode = 100,

            // Cluster Manager
            ClusterManagerTransport = 105,

            // Fault Analysis Service
            FaultAnalysisService = 106,

            // Upgrade Orchestration Service
            UpgradeOrchestrationService= 107,

            // Backup Restore Service
            BackupRestoreService = 108,

            //  Gateway Resource Manager service
            GatewayResourceManager = 109,

            // Management
            ServiceModel = 110,
            ImageStore = 111,
            NativeImageStoreClient = 112,
            ClusterManager = 115,
            RepairPolicyEngineService = 116,
            InfrastructureService = 117,
            RepairService = 118,
            RepairManager = 119,

            // Lease API
            LeaseLayer = 120,

            // Central Secret Service
            CentralSecretService = 125,

            // Local Secret Service
            LocalSecretService = 126,

            // Resource Manager
            ResourceManager = 127,
            
            // ServiceGroup
            ServiceGroupCommon = 130,
            ServiceGroupStateful = 131,
            ServiceGroupStateless = 132,
            ServiceGroupStatefulMember = 133,
            ServiceGroupStatelessMember = 134,
            ServiceGroupReplication = 135,

            // BwTree
            BwTree = 170,
            BwTreeVolatileStorage = 171,
            BwTreeStableStorage = 172,

            //ServiceCommunication
            FabricTransport =173,

            // TestabilitySubComponent
            TestabilityComponent = 174,

            // Data Stack
            RCR = 189, // ReliableCollectionRuntime (Interop)
            LR = 190, // Logging replicator
            SM = 191, // Dynamic state manager
            RStore = 192, // don't use TStore as it is already used in managed
            TR = 193, // Transactional Replicator
            LogicalLog = 194,

            // Ktl
            Ktl = 195,
            KtlLoggerNode = 196,
                  
            // Test Session
            TestSession = 200,

            // HttpApplicationGateway
            HttpApplicationGateway = 201,

            // DNS Service
            DNS = 209,
            
            // BackupRestore
            BA = 204,
            BAP = 205,

            // ResourceMonitor
            ResourceMonitor = 208,

            // Setup
            FabricSetup = 210,

            // Query
            Query = 211,

            // Health
            HealthManager = 212,
            HealthManagerCommon = 213,

            // SystemServices
            SystemServices = 214,

            //FabricActivator
            FabricActivator = 215,

            //HttpGateway
            HttpGateway = 216,

            //Client
            Client = 217,

            // FileStoreService
            FileStoreService = 218,

            //TokenValidationService
            TokenValidationService = 219,

            AccessControl = 220,

            FileTransfer = 221,

            FabricInstallerService = 222,

            EntreeServiceProxy = 223,

            //!!!
            //!!!
            //!!! DO NOT ADD MORE TASK CODES AFTER 223. TASKCODES IN MANAGED CODE START FROM 224 ONWARDS. SO
            //!!! FOR NEW TASKCODES, PLEASE USE AN UNASSIGNED NUMBER BETWEEN THE RANGE 1-223.
            //!!!
            //!!!
            MaxTask = 256
        };
    }
}
