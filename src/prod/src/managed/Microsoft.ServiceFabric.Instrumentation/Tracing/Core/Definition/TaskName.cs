// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition
{
    /// <summary>
    /// Describe name of the Tasks SF uses. This is copied from SF Native and Managed Code.
    /// </summary>
    public enum TaskName : ushort
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
        KeyValueStore = 84,

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
        UpgradeOrchestrationService = 107,

        // Backup Restore Service
        BackupRestoreService = 108,

        // Management
        ServiceModel = 110,
        ImageStore = 111,
        NativeImageStoreClient = 112,

        // Keeping it as CM since this is string representation that is written in manifest and in azure table.
        CM = 115,
        RepairPolicyEngineService = 116,
        InfrastructureService = 117,
        RepairService = 118,
        RepairManager = 119,

        // Lease API
        LeaseLayer = 120,

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
        FabricTransport = 173,

        // TestabilitySubComponent
        TestabilityComponent = 174,

        // Data Stack
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

        // Setup
        FabricSetup = 210,

        // Query
        Query = 211,

        // Health. Keeping it as HM since this is string representation that is written in manifest and in azure table.
        HM = 212,
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

        /// <summary>
        /// General task for managed code.
        /// </summary>
        ManagedGeneral = 224,

        /// <summary>
        /// The trace code for management common library.
        /// </summary>
        ManagementCommon = 225,

        /// <summary>
        /// The trace code for the Image Store code.
        /// </summary>
        ImageStoreClient = 226,

        /// <summary>
        /// The trace code for the Fabric Host code.
        /// </summary>
        FabricHost = 227,

        /// <summary>
        /// The trace code for the Cluster Setup code.
        /// </summary>
        FabricDeployer = 228,

        /// <summary>
        /// System Traces for Fabric Deployer
        /// </summary>
        SystemFabricDeployer = 229,

        /// <summary>
        /// Used by tests which write to Fabric trace session
        /// </summary>
        Test = 230,

        /// <summary>
        /// The trace code for the Azure Log Collector code.
        /// </summary>
        AzureLogCollector = 231,

        /// <summary>
        /// The trace code for System.Fabric
        /// </summary>
        SystemFabric = 232,

        /// <summary>
        /// The trace code for ImageBuilder
        /// </summary>
        ImageBuilder = 233,

        /// <summary>
        /// The trace code for DCA
        /// </summary>
        FabricDCA = 234,

        /// <summary>
        /// The trace code for FabricHttpGateway
        /// </summary>
        FabricHttpGateway = 235,

        /// <summary>
        /// The trace code for InfrastructureService
        /// </summary>
        ManagedInfrastructureService = 236,

        /// <summary>
        /// The trace code for ManagedTokenValidationService
        /// </summary>
        ManagedTokenValidationService = 237,

        /// <summary>
        /// The trace code for DSTS Client
        /// </summary>
        DSTSClient = 238,

        /// <summary>
        /// The trace code for Monitoring Service
        /// </summary>
        FabricMonSvc = 239,

        /// <summary>
        /// Differential store
        /// </summary>
        TStore = 240,

        /// <summary>
        /// Distributed Dictionary 
        /// </summary>
        DistributedDictionary = 241,

        /// <summary>
        /// Distributed Queue 
        /// </summary>
        DistributedQueue = 242,

        /// <summary>
        /// Stream based waves
        /// </summary>
        Wave = 243,

        /// <summary>
        /// Reliable Streams
        /// </summary>
        ReliableStream = 244,

        /// <summary>
        /// Distributed Versioned Dictionary 
        /// </summary>
        DistributedVersionedDictionary = 245,

        /// <summary>
        /// Testability
        /// </summary>
        Testability = 246,

        /// <summary>
        /// RandomActionGenerator
        /// </summary>
        RandomActionGenerator = 247,

        /// <summary>
        /// The trace code for MDS agent service
        /// </summary>
        /// <remarks>
        /// This task's last EventId is 63615
        /// </remarks>
        FabricMdsAgentSvc = 248,

        /// <summary>
        /// Transactional replicator
        /// </summary>
        TReplicator = 249,

        /// <summary>
        /// Stateful service replica that is part of the transactional replicator
        /// </summary>
        TStatefulServiceReplica = 250,

        /// <summary>
        /// State Manager that is part of the transactional replicator
        /// </summary>
        TStateManager = 251,

        /// <summary>
        /// Actor framework
        /// </summary>
        ActorFramework = 252,

        /// <summary>
        /// State Manager that is part of the transactional replicator
        /// </summary>
        WRP = 253,

        /// <summary>
        /// Service Framework (fabsrv).  Shares EventId space with ApiMonitor. Ends at 65151.
        /// </summary>
        ServiceFramework = 254,

        /// <summary>
        /// Fabric Analysis Service
        /// </summary>
        ManagedFaultAnalysisService = 255,

        /// <summary>
        /// API Monitor. Shares EventId space with ServiceFramework. Starts at 65152.
        /// </summary>
        ApiMonitor = 256,

        /// <summary>
        /// ReliableConcurrentQueue
        /// </summary>
        /// <remarks>
        /// Task range is shared with FabricMdsAgentService and starts at 63616
        /// </remarks>
        ReliableConcurrentQueue = 257,

        /// <summary>
        /// UpgradeOrchestrationService
        /// </summary>
        /// <remarks>
        /// Task range starts at 65284
        /// </remarks>
        ManagedUpgradeOrchestrationService = 258,

        /// <summary>
        /// BackupRestoreService
        /// </summary>
        ManagedBackupRestoreService = 259,

        /// <summary>
        /// All valid task id must be below this number. Increase when adding tasks. 
        /// Max value is 2**16.
        /// </summary>
        Max = 260,
    };
}