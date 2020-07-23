// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.ImageStore;
    using System.Fabric.Interop;
    using System.Fabric.Security;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.Reflection;
    using System.Runtime.InteropServices;
    using System.Security.Cryptography.X509Certificates;

    /// <summary>
    /// <para>Creates and manages Service Fabric services and other entities.</para>
    /// </summary>
    /// <remarks>
    ///     <para>It is highly recommended that you share FabricClients as much as possible.
    ///     This is because the FabricClient has multiple optimizations such as caching and batching that you would not be able to fully utilize otherwise.
    ///     </para>
    /// </remarks>
    [SuppressMessage("StyleCop.CSharp.OrderingRules", "SA1201:ElementsMustAppearInTheCorrectOrder", Justification = "Preserve order of public members from V1.")]
    [SuppressMessage("StyleCop.CSharp.OrderingRules", "SA1204:StaticElementsMustAppearBeforeInstanceElements", Justification = "Current grouping improves readability.")]
    [SuppressMessage("StyleCop.CSharp.OrderingRules", "SA1202:ElementsMustBeOrderedByAccess", Justification = "Current grouping improves readability.")]
    public sealed partial class FabricClient : IDisposable
    {
        #region Fields & Properties

        internal static readonly TimeSpan DefaultTimeout = TimeSpan.FromMinutes(1);
        private static FabricEvents.ExtensionsEvents traceSource = new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.ManagementCommon);

        private bool disposed;

        private NativeClient.IFabricClientSettings2 nativeClientSettings;
        private NativeClient.IFabricPropertyManagementClient2 nativePropertyClient;
        private NativeClient.IFabricServiceManagementClient6 nativeServiceClient;
        private NativeClient.IFabricServiceGroupManagementClient4 nativeServiceGroupClient;
        private NativeClient.IFabricApplicationManagementClient10 nativeApplicationClient;
        private NativeClient.IFabricClusterManagementClient10 nativeClusterClient;
        private NativeClient.IFabricQueryClient10 nativeQueryClient;
        private NativeClient.IFabricHealthClient4 nativeHealthClient;
        private NativeClient.IFabricInfrastructureServiceClient nativeInfraServiceClient;
        private NativeClient.IFabricTestManagementClient4 nativeTestManagementClient;
        private NativeClient.IFabricTestManagementClientInternal2 nativeTestManagementClientInternal;
        private NativeClient.IFabricFaultManagementClient nativeFaultManagementClient;
        private NativeClient.IFabricSecretStoreClient nativeSecretStoreClient;
        private NativeClient.IFabricRepairManagementClient2 nativeRepairClient;
        private NativeClient.IFabricFaultManagementClientInternal nativeFaultManagementClientInternal;
        private NativeClient.IFabricNetworkManagementClient nativeNetworkManagementClient;

        private ApplicationManagementClient applicationManager;
        private ComposeDeploymentClient composeDeploymentManager;
        private ServiceManagementClient serviceManager;
        private PropertyManagementClient propertyManager;
        private ClusterManagementClient clusterManager;
        private QueryClient queryManager;
        private HealthClient healthManager;
        private ServiceGroupManagementClient serviceGroupManager;
        private InfrastructureServiceClient infraServiceClient;
        private RepairManagementClient repairManager;
        private ImageStoreClient imageStoreClient;
        private SecretStoreClient secretStoreClient;
        private TestManagementClient testManagementClient;
        private FaultManagementClient faultManagementClient;
        private NetworkManagementClient networkManagementClient;
        private SecurityCredentials credential;
        private string[] hostEndpoints;

        private object syncLock;
        private Dictionary<string, IImageStore> imageStoreMap;
        private static readonly string ImageStoreConnectionFabricType = "fabric:";
        private static readonly TimeSpan NativeImageStoreDefaultTimeout = TimeSpan.FromMinutes(30);

        /// <summary>
        /// <para>Gets the <see cref="System.Fabric.FabricClient.PropertyManager" /> that can be used to perform operations related to names and properties.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.Fabric.FabricClient.PropertyManager" /> that can be used to perform operations related to names and properties.</para>
        /// </value>
        public PropertyManagementClient PropertyManager
        {
            get
            {
                return this.propertyManager;
            }
        }

        /// <summary>
        /// <para>Gets the <see cref="System.Fabric.FabricClient.ServiceManager" /> that can be used to perform operations related to services and service types.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.Fabric.FabricClient.ServiceManager" /> that can be used to perform operations related to services and service types.</para>
        /// </value>
        public ServiceManagementClient ServiceManager
        {
            get
            {
                return this.serviceManager;
            }
        }

        /// <summary>
        /// <para>Gets the <see cref="System.Fabric.FabricClient.ServiceGroupManager" /> that can be used to perform operations related to service groups.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.Fabric.FabricClient.ServiceGroupManager" /> that can be used to perform operations related to service groups.</para>
        /// </value>
        public ServiceGroupManagementClient ServiceGroupManager
        {
            get
            {
                return this.serviceGroupManager;
            }
        }

        /// <summary>
        /// <para>Gets the <see cref="System.Fabric.FabricClient.ApplicationManager" /> that can be used to perform operations related to applications and application types.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.Fabric.FabricClient.ApplicationManager" /> that can be used to perform operations related to applications and application types.</para>
        /// </value>
        public ApplicationManagementClient ApplicationManager
        {
            get
            {
                return this.applicationManager;
            }
        }

        /// <summary>
        /// <para>Gets the <see cref="System.Fabric.FabricClient.ComposeDeploymentClient" /> that can be used to perform operations related to compose deployment.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.Fabric.FabricClient.ComposeDeploymentManager" /> that can be used to perform operations related to compose deployment.</para>
        /// </value>
        public ComposeDeploymentClient ComposeDeploymentManager
        {
            get
            {
                return this.composeDeploymentManager;
            }
        }

        /// <summary>
        /// <para>Gets the <see cref="System.Fabric.FabricClient.ClusterManager" /> that can be used to perform operations related to the Service Fabric cluster.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.Fabric.FabricClient.ClusterManager" /> that can be used to perform operations related to the Service Fabric cluster.</para>
        /// </value>
        public ClusterManagementClient ClusterManager
        {
            get
            {
                return this.clusterManager;
            }
        }

        /// <summary>
        /// <para>Gets the <see cref="System.Fabric.FabricClient.RepairManagementClient" /> that can be used to manage repair tasks.</para>
        /// <para>This property supports the Service Fabric platform; it is not meant to be called directly from your code.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Fabric.FabricClient.RepairManagementClient" />.</para>
        /// </value>
        public RepairManagementClient RepairManager
        {
            get
            {
                return this.repairManager;
            }
        }

        /// <summary>
        /// <para>Gets the query manager that can be used to execute queries against the Service Fabric cluster.</para>
        /// </summary>
        /// <value>
        /// <para>The query manager that can be used to execute queries against the Service Fabric cluster.</para>
        /// </value>
        /// <remarks>
        /// <para>The query manager can be used to execute queries against the cluster.
        /// Most of the queries are distributed. They call multiple system components to get different information.
        /// The calls to the system components are executed in parallel and the returned results are aggregated based on a common key.
        /// For example, to get the list of nodes in the cluster, the <see cref="System.Fabric.FabricClient.QueryClient.GetNodeListAsync()"/> query goes to
        /// Failover Manager, Cluster Manager, and Health Manager system services.
        /// For this reason, some queries are expensive.
        /// </para>
        /// </remarks>
        public QueryClient QueryManager
        {
            get
            {
                return this.queryManager;
            }
        }

        /// <summary>
        /// <para>Gets the health client that can be used to perform health related operations, like report health or get entity health.</para>
        /// </summary>
        /// <value>
        /// <para>The health client that can be used to perform health related operations, like report health or get entity health.</para>
        /// </value>
        public HealthClient HealthManager
        {
            get
            {
                return this.healthManager;
            }
        }

        internal ImageStoreClient ImageStore
        {
            get
            {
                return this.imageStoreClient;
            }
        }

        internal SecretStoreClient SecretStore
        {
            get
            {
                return this.secretStoreClient;
            }
        }

        /// <summary>
        /// <para>Gets the <see cref="System.Fabric.FabricClient.InfrastructureServiceClient" /> that can be used to perform operations related to the infrastructure on which the cluster is running.</para>
        /// <para>This property supports the Service Fabric platform; it is not meant to be called directly from your code.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.Fabric.FabricClient.InfrastructureServiceClient" /> that can be used to perform operations related to the infrastructure on which the cluster is running.</para>
        /// </value>
        public InfrastructureServiceClient InfrastructureManager
        {
            get
            {
                return this.infraServiceClient;
            }
        }

        /// <summary>
        /// Gets the <see cref="System.Fabric.FabricClient.TestManagementClient"/> to perform complex actions that go through FaultAnalysisService. For example, StartPartitionDataLossAsync.
        /// This also supports APIs for validation (that do not go through FaultAnalysisService). For example, ValidateServiceAsync.
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Fabric.FabricClient.TestManagementClient" />.</para>
        /// </value>
        public TestManagementClient TestManager
        {
            get
            {
                return this.testManagementClient;
            }
        }

        /// <summary>
        /// Gets the <see cref="System.Fabric.FabricClient.FaultManagementClient"/> to induce faults. For example, RestartNodeAsync.
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Fabric.FabricClient.FaultManagementClient" />.</para>
        /// </value>
        public FaultManagementClient FaultManager
        {
            get
            {
                return this.faultManagementClient;
            }
        }

        /// <summary>
        /// Gets the <see cref="System.Fabric.FabricClient.NetworkManagementClient"/> to manage container networks.
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Fabric.FabricClient.NetworkManagementClient" />.</para>
        /// </value>
        public NetworkManagementClient NetworkManager
        {
            get
            {
                return this.networkManagementClient;
            }
        }

        /// <summary>
        /// <para>Gets the fabric client settings.</para>
        /// </summary>
        /// <value>
        /// <para>The fabric client settings.</para>
        /// </value>
        public FabricClientSettings Settings
        {
            get
            {
                this.ThrowIfDisposed();
                return Utility.WrapNativeSyncInvokeInMTA(() => this.GetFabricClientSettingsInternal(), "GetSettings");
            }
        }

        /// <summary>
        /// <para>The Service Fabric System application.</para>
        /// </summary>
        public readonly Uri FabricSystemApplication = new Uri("fabric:/System");

        internal NativeClient.IFabricPropertyManagementClient2 NativePropertyClient
        {
            get
            {
                return this.nativePropertyClient;
            }
        }

        internal SecurityCredentials Credentials
        {
            get
            {
                return this.credential;
            }
        }

        internal string[] HostEndpoints
        {
            get
            {
                return this.hostEndpoints;
            }
        }

        #endregion

        #region Constructors & Initialization

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.FabricClient" /> class. This constructor should be used by code that is running inside the cluster. It allows the <see cref="System.Fabric.FabricClient" /> instance to connect to the cluster via the local Gateway service running on the same node.</para>
        /// </summary>
        /// <remarks>
        /// <para>Since this constructor uses the local Gateway service running on
        /// the same node to connect to th cluster, your client can bypass an extra network hop.
        /// To connect to a cluster from code running outside the cluster, use a different constructor
        /// which allows you to explicitly specify the connection parameters.</para>
        /// </remarks>
        public FabricClient()
        {
            this.InitializeFabricClient(null, null, null);
        }

        /// <summary>
        /// <para>
        /// Initializes a new instance of the <see cref="System.Fabric.FabricClient" /> class with the specified fabric client role - <see cref="System.Fabric.FabricClientRole" />.
        /// </para>
        /// </summary>
        /// <param name="clientRole">
        /// <para>The fabric client role.</para>
        /// </param>
        public FabricClient(FabricClientRole clientRole)
        {
            this.InitializeFabricClient(clientRole);
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.FabricClient" /> class with the desired <see cref="System.Fabric.FabricClientSettings" />.
        /// If the <see cref="System.Fabric.FabricClient" /> is on the same cluster as the service, then use a Local <see cref="System.Fabric.FabricClient" />. Local <see cref="System.Fabric.FabricClient" /> is a feature of Service Fabric that allows the <see cref="System.Fabric.FabricClient" /> to connect to the local Gateway Service instead of choosing from a list. This way, your client can bypass an extra network hop. In case a service is resolving another service partition in the same cluster, then it is recommended that you use Local <see cref="System.Fabric.FabricClient" />, as it enables automatic load balancing and improves performance.</para>
        /// </summary>
        /// <param name="settings">
        /// <para>The fabric client settings used by the fabric client.</para>
        /// </param>
        public FabricClient(FabricClientSettings settings)
        {
            this.InitializeFabricClient(null, settings, null);
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.FabricClient" /> class with given Service Fabric Gateway addresses. These host-endpoints are list of ':' delimited strings where the first part is the ip of the cluster and the second part is the client-connection endpoint-port. </para>
        /// </summary>
        /// <param name="hostEndpoints">
        /// <para>Defines the set of Gateway addresses the <see cref="System.Fabric.FabricClient" /> can use to connect to the cluster.</para>
        /// </param>
        public FabricClient(params string[] hostEndpoints)
        {
            this.InitializeFabricClient(null, null, hostEndpoints);
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.FabricClient" /> class with given Service Fabric Gateway addresses and <see cref="System.Fabric.SecurityCredentials" />.</para>
        /// </summary>
        /// <param name="credential">
        ///  <para><see cref = "System.Fabric.SecurityCredentials" /> defines the security settings for the<see cref="System.Fabric.FabricClient" />.</para></param>
        /// <param name="hostEndpoints">
        /// <para>Defines the set of Gateway addresses the <see cref="System.Fabric.FabricClient" /> can use to connect to the cluster.</para>
        /// </param>
        public FabricClient(SecurityCredentials credential, params string[] hostEndpoints)
        {
            this.InitializeFabricClient(credential, null, hostEndpoints);
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.FabricClient" /> class with given
        /// Service Fabric Gateway addresses and the desired <see cref="System.Fabric.FabricClientSettings" />.</para>
        /// </summary>
        /// <param name="settings">
        /// <para>The fabric client settings.</para>
        /// </param>
        /// <param name="hostEndpoints">
        /// <para>Defines the set of Gateway addresses the <see cref="System.Fabric.FabricClient" /> can use to connect to the cluster.</para>
        /// </param>
        public FabricClient(FabricClientSettings settings, params string[] hostEndpoints)
        {
            this.InitializeFabricClient(null, settings, hostEndpoints);
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.FabricClient" /> class with given Service Fabric Gateway addresses, <see cref = "System.Fabric.SecurityCredentials" /> and <see cref="System.Fabric.FabricClientSettings" />.</para>
        /// </summary>
        /// <param name="credential">
        ///  <para><see cref = "System.Fabric.SecurityCredentials" /> defines the security settings for the<see cref="System.Fabric.FabricClient" />.</para></param>
        /// <param name="settings">
        /// <para>The fabric client settings.</para>
        /// </param>
        /// <param name="hostEndpoints">
        /// <para>Defines the set of Gateway addresses the <see cref="System.Fabric.FabricClient" /> can use to connect to the cluster.</para>
        /// </param>
        public FabricClient(SecurityCredentials credential, FabricClientSettings settings, params string[] hostEndpoints)
        {
            this.InitializeFabricClient(credential, settings, hostEndpoints);
        }

        /// <summary>
        /// <para>DEPRECATED. Initializes a new instance of the <see cref="System.Fabric.FabricClient" /> class with given keepAliveInterval
        /// and Service Fabric Gateway addresses (hostEndpoints).</para>
        /// </summary>
        /// <param name="keepAliveInterval">
        /// <para>Defines the periodic keep alive message interval.</para>
        /// </param>
        /// <param name="hostEndpoints">
        /// <para> Defines the set of Gateway addresses the <see cref="System.Fabric.FabricClient" /> can use to connect to the cluster.</para>
        /// </param>
        /// <remarks>
        /// <para>If there are external devices in between the connection from the client to the cluster that require periodic messages to keep the connection alive,
        /// then make sure to use the KeepAlive feature of FabricClient. During the initialization of the FabricClient, users can specify a TimeSpan keepAliveInterval.
        /// If this argument is specified, then the FabricClient will periodically ping the Service Fabric Gateway Service it is currently communicating with,
        /// as long as there is a pending operation. An example of a scenario where this feature is useful is Windows Azure. If the <see cref="System.Fabric.FabricClient" />
        /// is outside of Windows Azure and the cluster is inside of Windows Azure, then all connections will go through the Azure Load Balancer (ALB).
        /// ALB terminates connections that are idle for more than 60 seconds. Hence, in these situations, <see cref="System.Fabric.FabricClient" /> should be created with
        /// KeepAliveInterval set to &lt;59 seconds (20 -30 is recommended).</para>
        /// </remarks>
        [Obsolete("Deprecated", true)]
        public FabricClient(TimeSpan keepAliveInterval, params string[] hostEndpoints)
        {
            this.InitializeFabricClient(null, keepAliveInterval, hostEndpoints);
        }

        /// <summary>
        /// <para>DEPRECATED. Initializes a new instance of the <see cref="System.Fabric.FabricClient" /> class with given credentials, keepAliveInterval
        /// and Service Fabric Gateway addresses (hostEndpoints).</para>
        /// </summary>
        /// <param name="credential">Defines the security credentials.</param>
        /// <param name="keepAliveInterval">
        /// <para>Defines the periodic keep alive message interval.</para>
        /// </param>
        /// <param name="hostEndpoints">
        /// <para> Defines the set of Gateway addresses the <see cref="System.Fabric.FabricClient" /> can use to connect to the cluster.</para>
        /// </param>
        /// <remarks>
        /// <para>If there are external devices in between the connection from the client to the cluster that require periodic messages to keep the connection alive,
        /// then make sure to use the KeepAlive feature of FabricClient. During the initialization of the FabricClient, users can specify a TimeSpan keepAliveInterval.
        /// If this argument is specified, then the FabricClient will periodically ping the Service Fabric Gateway Service it is currently communicating with,
        /// as long as there is a pending operation. An example of a scenario where this feature is useful is Windows Azure. If the <see cref="System.Fabric.FabricClient" />
        /// is outside of Windows Azure and the cluster is inside of Windows Azure, then all connections will go through the Azure Load Balancer (ALB).
        /// ALB terminates connections that are idle for more than 60 seconds. Hence, in these situations, <see cref="System.Fabric.FabricClient" /> should be created with
        /// KeepAliveInterval set to &lt;59 seconds (20 -30 is recommended).</para>
        /// </remarks>
        [Obsolete("Deprecated", true)]
        public FabricClient(SecurityCredentials credential, TimeSpan keepAliveInterval, params string[] hostEndpoints)
        {
            this.InitializeFabricClient(credential, keepAliveInterval, hostEndpoints);
        }

        /// <summary>
        /// <para>Updates the fabric client settings.</para>
        /// </summary>
        /// <param name="settings">
        /// <para>The new fabric client settings to be used.</para>
        /// </param>
        /// <exception cref="System.ArgumentNullException">
        /// <para>The specified fabric client settings can’t be null.</para>
        /// </exception>
        /// <exception cref="System.ArgumentException">
        /// <para>
        ///     <see cref="System.Fabric.FabricClientSettings.PartitionLocationCacheLimit" /> must be positive.</para>
        /// </exception>
        /// <exception cref="System.ArgumentException">
        /// <para>
        ///     <see cref="System.Fabric.FabricClientSettings.PartitionLocationCacheBucketCount" /> must be greater or equal to zero.</para>
        /// </exception>
        /// <exception cref="System.ArgumentException">
        /// <para>
        ///     <see cref="System.Fabric.FabricClientSettings.ServiceChangePollInterval" /> must represent a positive duration.</para>
        /// </exception>
        /// <exception cref="System.ArgumentException">
        /// <para>
        ///     <see cref="System.Fabric.FabricClientSettings.ConnectionInitializationTimeout" /> must represent a positive duration.</para>
        /// </exception>
        /// <exception cref="System.ArgumentException">
        /// <para>
        ///     <see cref="System.Fabric.FabricClientSettings.KeepAliveInterval" /> must be zero or a positive duration.</para>
        /// </exception>
        /// <exception cref="System.ArgumentException">
        /// <para>
        ///     <see cref="System.Fabric.FabricClientSettings.ConnectionIdleTimeout" /> must be zero or a positive duration.</para>
        /// </exception>
        /// <exception cref="System.ArgumentException">
        /// <para>
        ///     <see cref="System.Fabric.FabricClientSettings.HealthOperationTimeout" /> must represent a positive duration.</para>
        /// </exception>
        /// <exception cref="System.ArgumentException">
        /// <para>
        ///     <see cref="System.Fabric.FabricClientSettings.HealthReportSendInterval" /> must be zero or a positive duration.</para>
        /// </exception>
        /// <exception cref="System.ArgumentException">
        /// <para>
        ///     <see cref="System.Fabric.FabricClientSettings.HealthReportRetrySendInterval" /> must represent a positive duration.</para>
        /// </exception>
        /// <exception cref="System.ArgumentException">
        /// <para>
        ///     <see cref="System.Fabric.FabricClientSettings.PartitionLocationCacheBucketCount" /> must be greater or equal to zero.</para>
        /// </exception>
        /// <exception cref="System.ArgumentException">
        /// <para>
        ///     <see cref="System.Fabric.FabricClientSettings.AuthTokenBufferSize" /> must be greater or equal to zero.</para>
        /// </exception>
        public void UpdateSettings(FabricClientSettings settings)
        {
            this.ThrowIfDisposed();
            Requires.Argument("settings", settings).NotNull();
            settings.Validate();
            Utility.WrapNativeSyncInvokeInMTA(() => this.UpdateFabricClientSettingsInternal(settings), "UpdateSettings");
        }

        /// <summary>
        /// Updates the fabric client security credentials.
        /// </summary>
        /// <param name="credentials">The new security credentials to be used.</param>
        public void UpdateSecurityCredentials(SecurityCredentials credentials)
        {
            this.ThrowIfDisposed();
            Requires.Argument("credentials", credentials).NotNull();

            //
            // Credentials used for imagestore client and the native fabric client should be the same.
            //
            lock (this.syncLock)
            {
                this.credential = credentials;
                this.imageStoreMap.Clear();
                Utility.WrapNativeSyncInvokeInMTA(() => this.SetSecurityCredentialsInternal(this.credential), "UpdateSecurityCredentials");
            }
        }

        internal void InitializeFabricClient(SecurityCredentials credentialArg, TimeSpan keepAliveInterval, string[] hostEndpointsArg)
        {
            this.syncLock = new object();
            this.imageStoreMap = new Dictionary<string, IImageStore>();
            this.credential = credentialArg;
            this.hostEndpoints = null;
            if ((hostEndpointsArg != null)  && (hostEndpointsArg.Length > 0))
            {
                this.hostEndpoints = hostEndpointsArg;
            }

            Utility.WrapNativeSyncInvokeInMTA(() => this.CreateNativeClient(this.hostEndpoints), "FabricClient.CreateNativeClient");

            if (this.credential != null)
            {
                Utility.WrapNativeSyncInvokeInMTA(() =>
                    this.SetSecurityCredentialsInternal(this.credential),
                    "FabricClient.SetSecurityCredentialsInternal");
            }

            Utility.WrapNativeSyncInvokeInMTA(
                () => this.SetKeepAliveInternal(keepAliveInterval),
                "FabricClient.SetKeepAliveInternal");

            this.CreateManagedClients();
        }

        internal void InitializeFabricClient(SecurityCredentials credentialArg, FabricClientSettings newSettings, string[] hostEndpointsArg)
        {
            this.syncLock = new object();
            this.imageStoreMap = new Dictionary<string, IImageStore>();
            this.credential = credentialArg;
            this.hostEndpoints = null;
            if ((hostEndpointsArg != null) && (hostEndpointsArg.Length > 0))
            {
                this.hostEndpoints = hostEndpointsArg;
            }

            if (newSettings != null)
            {
                newSettings.Validate();
            }

            Utility.WrapNativeSyncInvokeInMTA(
                () => this.CreateNativeClient(this.hostEndpoints),
                "FabricClient.CreateNativeClient");

            if (newSettings != null)
            {
                Utility.WrapNativeSyncInvokeInMTA(
                    () => this.UpdateFabricClientSettingsInternal(newSettings),
                    "FabricClient.UpdateFabricClientSettingsInternal");
            }

            if (this.credential != null)
            {
                Utility.WrapNativeSyncInvokeInMTA(
                    () => this.SetSecurityCredentialsInternal(this.credential),
                    "FabricClient.SetSecurityCredentialsInternal");
            }

            this.CreateManagedClients();
           
        }

        internal void InitializeFabricClient(FabricClientRole clientRole)
        {
            this.syncLock = new object();
            this.imageStoreMap = new Dictionary<string, IImageStore>();
            this.credential = null;
            this.hostEndpoints = null;

            Utility.WrapNativeSyncInvokeInMTA(() => this.CreateNativeClient(clientRole), "FabricClient.CreateNativeClient");

            this.CreateManagedClients();
        }

        internal IImageStore GetImageStore(string imageStoreConnectionString)
        {
            IImageStore imageStore;
            lock (this.syncLock)
            {
                if (!this.imageStoreMap.TryGetValue(imageStoreConnectionString, out imageStore))
                {
                    imageStore = ImageStoreFactory.CreateImageStore(imageStoreConnectionString, null, this.hostEndpoints, this.credential, null, false);
                    this.imageStoreMap.Add(imageStoreConnectionString, imageStore);
                }
            }

            return imageStore;
        }

        internal static TimeSpan GetImageStoreDefaultTimeout(string imageStoreConnectionString)
        {
            return imageStoreConnectionString.StartsWith(ImageStoreConnectionFabricType, StringComparison.OrdinalIgnoreCase) 
                ? NativeImageStoreDefaultTimeout 
                : TimeSpan.MaxValue;
        }

        private static string DecryptText(string text, StoreLocation storeLocation)
        {
            using (var pin = new PinCollection())
            {
                // Calling native API directly because we need to return string instead of SecureString for this cmdlet,
                // and we do not want to have a public managed API that returns string instead of SecureString.
                return StringResult.FromNative(NativeCommon.FabricDecryptText(
                    pin.AddBlittable(text),
                    (NativeTypes.FABRIC_X509_STORE_LOCATION)storeLocation));
            }
        }

        private void CreateManagedClients()
        {
            this.propertyManager = new PropertyManagementClient(this, this.nativePropertyClient);
            this.serviceManager = new ServiceManagementClient(this, this.nativeServiceClient);
            this.serviceGroupManager = new ServiceGroupManagementClient(this, this.nativeServiceGroupClient);
            this.applicationManager = new ApplicationManagementClient(this, this.nativeApplicationClient);
            this.composeDeploymentManager = new ComposeDeploymentClient(this, this.nativeApplicationClient);
            this.clusterManager = new ClusterManagementClient(this, this.nativeClusterClient);
            this.queryManager = new QueryClient(this, this.nativeQueryClient, this.nativeApplicationClient);
            this.healthManager = new HealthClient(this, this.nativeHealthClient);
            this.infraServiceClient = new InfrastructureServiceClient(this, this.nativeInfraServiceClient);
            this.testManagementClient = new TestManagementClient(this, this.nativeTestManagementClient, this.nativeTestManagementClientInternal);
            this.faultManagementClient = new FaultManagementClient(this, this.nativeFaultManagementClient, this.nativeFaultManagementClientInternal);
            this.repairManager = new RepairManagementClient(this, this.nativeRepairClient);            
            this.imageStoreClient = new ImageStoreClient(this);
            this.secretStoreClient = new SecretStoreClient(this, this.nativeSecretStoreClient);
            this.networkManagementClient = new NetworkManagementClient(this, this.nativeNetworkManagementClient);
        }

        private void CreateNativeClient(IEnumerable<string> connectionStringsLocal)
        {
            Guid iid = typeof(NativeClient.IFabricPropertyManagementClient2).GetTypeInfo().GUID;

            var serviceNotificationBroker = new ServiceNotificationEventHandlerBroker(this);
            var connectionBroker = new ClientConnectionEventHandlerBroker(this);

            if (connectionStringsLocal == null)
            {
                //// PInvoke call
                this.nativePropertyClient = NativeClient.FabricCreateLocalClient3(
                    serviceNotificationBroker,
                    connectionBroker,
                    ref iid);
            }
            else
            {
                using (var pin = new PinArray(connectionStringsLocal, PinBlittable.Create))
                {
                    //// PInvoke call
                    this.nativePropertyClient = NativeClient.FabricCreateClient3(
                        (ushort)pin.Count,
                        pin.AddrOfPinnedObject(),
                        serviceNotificationBroker,
                        connectionBroker,
                        ref iid);
                }
            }

            // ReSharper disable SuspiciousTypeConversion.Global
            // cast the native object to other COM interfaces supported by the object as defined in the IDL
            this.nativeClientSettings = (NativeClient.IFabricClientSettings2) this.nativePropertyClient;
            this.nativeServiceClient = (NativeClient.IFabricServiceManagementClient6)this.nativePropertyClient;
            this.nativeServiceGroupClient = (NativeClient.IFabricServiceGroupManagementClient4)this.nativePropertyClient;
            this.nativeApplicationClient = (NativeClient.IFabricApplicationManagementClient10)this.nativePropertyClient;
            this.nativeClusterClient = (NativeClient.IFabricClusterManagementClient10)this.nativePropertyClient;
            this.nativeQueryClient = (NativeClient.IFabricQueryClient10)this.nativePropertyClient;
            this.nativeHealthClient = (NativeClient.IFabricHealthClient4)this.nativePropertyClient;
            this.nativeInfraServiceClient = (NativeClient.IFabricInfrastructureServiceClient)this.nativePropertyClient;
            this.nativeTestManagementClient = (NativeClient.IFabricTestManagementClient4)this.NativePropertyClient;
            this.nativeTestManagementClientInternal = (NativeClient.IFabricTestManagementClientInternal2)this.NativePropertyClient;
            this.nativeFaultManagementClient = (NativeClient.IFabricFaultManagementClient)this.NativePropertyClient;
            this.nativeSecretStoreClient = (NativeClient.IFabricSecretStoreClient)this.nativePropertyClient;
            this.nativeRepairClient = (NativeClient.IFabricRepairManagementClient2)this.nativePropertyClient;
            this.nativeFaultManagementClientInternal = (NativeClient.IFabricFaultManagementClientInternal)this.nativePropertyClient;
            this.nativeNetworkManagementClient = (NativeClient.IFabricNetworkManagementClient)this.nativePropertyClient;

            // ReSharper enable SuspiciousTypeConversion.Global
        }

        private void CreateNativeClient(FabricClientRole clientRole)
        {
            Guid iid = typeof(NativeClient.IFabricPropertyManagementClient2).GetTypeInfo().GUID;

            var serviceNotificationBroker = new ServiceNotificationEventHandlerBroker(this);
            var connectionBroker = new ClientConnectionEventHandlerBroker(this);
            var nativeClientRole = ToNativeFabricClientRole(clientRole);

            //// PInvoke call
            this.nativePropertyClient = NativeClient.FabricCreateLocalClient4(
                serviceNotificationBroker,
                connectionBroker,
                nativeClientRole,
                ref iid);

            this.nativeClientSettings = (NativeClient.IFabricClientSettings2)this.nativePropertyClient;
            this.nativeServiceClient = (NativeClient.IFabricServiceManagementClient6)this.nativePropertyClient;
            this.nativeServiceGroupClient = (NativeClient.IFabricServiceGroupManagementClient4)this.nativePropertyClient;
            this.nativeApplicationClient = (NativeClient.IFabricApplicationManagementClient10)this.nativePropertyClient;
            this.nativeClusterClient = (NativeClient.IFabricClusterManagementClient10)this.nativePropertyClient;
            this.nativeQueryClient = (NativeClient.IFabricQueryClient10)this.nativePropertyClient;
            this.nativeHealthClient = (NativeClient.IFabricHealthClient4)this.nativePropertyClient;
            this.nativeInfraServiceClient = (NativeClient.IFabricInfrastructureServiceClient)this.nativePropertyClient;
            this.nativeTestManagementClient = (NativeClient.IFabricTestManagementClient4)this.NativePropertyClient;
            this.nativeTestManagementClientInternal = (NativeClient.IFabricTestManagementClientInternal2)this.NativePropertyClient;
            this.nativeFaultManagementClient = (NativeClient.IFabricFaultManagementClient)this.NativePropertyClient;
            this.nativeSecretStoreClient = (NativeClient.IFabricSecretStoreClient)this.nativePropertyClient;
            this.nativeRepairClient = (NativeClient.IFabricRepairManagementClient2)this.nativePropertyClient;
            this.nativeFaultManagementClientInternal = (NativeClient.IFabricFaultManagementClientInternal)this.nativePropertyClient;
            this.nativeNetworkManagementClient = (NativeClient.IFabricNetworkManagementClient)this.nativePropertyClient;
        }

        /// <summary>
        /// <para>
        /// Destructor of fabric client.
        /// </para>
        /// </summary>
        ~FabricClient()
        {
            this.Dispose(false);
        }

        /// <summary>
        /// <para>
        /// Disposes of the fabric client.
        /// </para>
        /// </summary>
        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        private void Dispose(bool disposing)
        {
            if (!this.disposed)
            {
                if (disposing)
                {
                    // Do not need to explicitly dispose other clients
                    // unless they have their own native resources
                    // not owned by FabricClient.
                }

                if (this.nativePropertyClient != null)
                {
                    Marshal.FinalReleaseComObject(this.nativePropertyClient);
                    this.nativePropertyClient = null;
                }

                this.disposed = true;
            }
        }

        private bool IsDisposed()
        {
            return this.disposed;
        }

        internal void ThrowIfDisposed()
        {
            if (this.IsDisposed())
            {
                throw new ObjectDisposedException("FabricClient");
            }
        }

#endregion

#region Service notifications

        internal void OnNotification(ServiceNotification notification)
        {
            this.serviceManager.OnServiceNotificationFilterMatched(notification);
        }

        private class ServiceNotificationEventHandlerBroker : NativeClient.IFabricServiceNotificationEventHandler
        {
            private WeakReference ownerWref;

            public ServiceNotificationEventHandlerBroker(FabricClient owner)
            {
                this.ownerWref = new WeakReference(owner);
            }

            void NativeClient.IFabricServiceNotificationEventHandler.OnNotification(
                NativeClient.IFabricServiceNotification nativeNotification)
            {
                var managedNotification = Utility.WrapNativeSyncInvokeInMTA(
                    () => { return ServiceNotification.FromNative(nativeNotification); },
                    "FabricClient.OnNotification");

                var owner = this.ownerWref.Target as FabricClient;
                if (owner != null)
                {
                    owner.OnNotification(managedNotification);
                }
            }
        }

#endregion

#region Client Connections

        /// <summary>
        /// <para>
        /// Represents the event arguments for gateway related events, like connect and disconnect.
        /// </para>
        /// </summary>
        /// <remarks>
        /// <para>Provides more information about the gateway that the client is configured with.</para>
        /// </remarks>
        public class GatewayInformationEventArgs : EventArgs
        {
            internal GatewayInformationEventArgs(GatewayInformation info)
            {
                this.GatewayInformation = info;
            }

            /// <summary>
            /// <para>
            /// Gets the gateway information.
            /// </para>
            /// </summary>
            /// <value>
            /// <para>The gateway information.</para>
            /// </value>
            public GatewayInformation GatewayInformation { get; internal set; }
        }

        /// <summary>
        /// Represents the event arguments for a claims token retrieval event
        /// </summary>
        /// <remarks>Optionally allows the application to provide a claims token for authorization.</remarks>
        public class ClaimsRetrievalEventArgs : EventArgs
        {
            internal ClaimsRetrievalEventArgs(AzureActiveDirectoryMetadata metadata)
            {
                this.AzureActiveDirectoryMetadata = metadata;
            }

            /// <summary>
            /// Gets metadata needed for acquiring a claims token from Azure Active Directory
            /// </summary>
            /// <value>The metadata object.</value>
            public AzureActiveDirectoryMetadata AzureActiveDirectoryMetadata { get; internal set; }
        }

        /// <summary>
        /// Occurs when the client is connected to gateway.
        /// </summary>
        public event EventHandler ClientConnected;

        /// <summary>
        /// Occurs when the client is disconnected from the gateway.
        /// </summary>
        public event EventHandler ClientDisconnected;

        /// <summary>
        /// Occurs when the client needs to provide a claims token for authorization with the gateway
        /// </summary>
        /// <remarks>
        /// <para>
        /// See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-connect-to-secure-cluster#connect-to-a-secure-cluster-using-the-fabricclient-apis"/> for connecting to secure cluster using Azure Active Directory authentication.
        /// </para>
        /// </remarks>
        public event ClaimsRetrievalEventHandler ClaimsRetrieval;

        /// <summary>
        /// Delegate for handling a claims token retrieval callback by registering for the ClaimsRetrieval event
        /// </summary>
        /// <param name="sender">
        /// <para>A reference to the <see cref="System.Fabric.FabricClient" /> instance on which the event is being raised.</para>
        /// </param>
        /// <param name="e">
        /// <para>The event arguments.</para>
        /// <seealso cref="System.Fabric.FabricClient.ClaimsRetrievalEventArgs" />
        /// </param>
        public delegate string ClaimsRetrievalEventHandler(object sender, ClaimsRetrievalEventArgs e);

        internal void OnClientConnected(GatewayInformation info)
        {
            var handler = this.ClientConnected;
            if (handler != null)
            {
                handler(this, new GatewayInformationEventArgs(info));
            }
        }

        internal void OnClientDisconnected(GatewayInformation info)
        {
            var handler = this.ClientDisconnected;
            if (handler != null)
            {
                handler(this, new GatewayInformationEventArgs(info));
            }
        }

        internal string OnClaimsRetrieval(AzureActiveDirectoryMetadata metadata)
        {
            var handler = this.ClaimsRetrieval;
            if (handler != null)
            {
                return handler(this, new ClaimsRetrievalEventArgs(metadata));
            }

            return string.Empty;
        }

        private class ClientConnectionEventHandlerBroker : NativeClient.IFabricClientConnectionEventHandler2
        {
            private static readonly InteropApi CallbackApi = new InteropApi {CopyExceptionDetailsToThreadErrorMessage = true};

            private WeakReference ownerWref;

            public ClientConnectionEventHandlerBroker(FabricClient owner)
            {
                this.ownerWref = new WeakReference(owner);
            }

            void NativeClient.IFabricClientConnectionEventHandler.OnConnected(
                NativeClient.IFabricGatewayInformationResult nativeInfo)
            {
                ((NativeClient.IFabricClientConnectionEventHandler2)this).OnConnected(nativeInfo);
            }

            void NativeClient.IFabricClientConnectionEventHandler.OnDisconnected(
                NativeClient.IFabricGatewayInformationResult nativeInfo)
            {
                ((NativeClient.IFabricClientConnectionEventHandler2)this).OnDisconnected(nativeInfo);
            }

            void NativeClient.IFabricClientConnectionEventHandler2.OnConnected(
                NativeClient.IFabricGatewayInformationResult nativeInfo)
            {
                try
                {
                    var managedInfo = Utility.WrapNativeSyncInvokeInMTA(
                        () => { return GatewayInformation.FromNative(nativeInfo); },
                        "FabricClient.OnConnected");

                    var owner = this.ownerWref.Target as FabricClient;
                    if (owner != null)
                    {
                        owner.OnClientConnected(managedInfo);
                    }
                }
                catch (Exception e)
                {
                    CallbackApi.HandleException(e);
                    throw;
                }
            }

            void NativeClient.IFabricClientConnectionEventHandler2.OnDisconnected(
                NativeClient.IFabricGatewayInformationResult nativeInfo)
            {
                try
                {
                    var managedInfo = Utility.WrapNativeSyncInvokeInMTA(
                        () => { return GatewayInformation.FromNative(nativeInfo); },
                        "FabricClient.OnDisconnected");

                    var owner = this.ownerWref.Target as FabricClient;
                    if (owner != null)
                    {
                        owner.OnClientDisconnected(managedInfo);
                    }
                }
                catch (Exception e)
                {
                    CallbackApi.HandleException(e);
                    throw;
                }
            }

            NativeCommon.IFabricStringResult NativeClient.IFabricClientConnectionEventHandler2.OnClaimsRetrieval(
                IntPtr nativePtr)
            {
                try
                {
                    var managedMetadata = Utility.WrapNativeSyncInvokeInMTA(
                        () => { return AzureActiveDirectoryMetadata.FromNative(nativePtr); },
                        "FabricClient.OnClaimsRetrieval");

                    var result = string.Empty;

                    if (managedMetadata != null)
                    {
                        var owner = this.ownerWref.Target as FabricClient;
                        if (owner != null)
                        {
                            result = owner.OnClaimsRetrieval(managedMetadata);
                        }
                    }

                    return new StringResult(result);
                }
                catch (Exception e)
                {
                    CallbackApi.HandleException(e);
                    throw;
                }
            }
        }

#endregion

#region Helper Methods

        internal FabricClientSettings GetFabricClientSettingsInternal()
        {
            var settingsResult = this.nativeClientSettings.GetSettings();
            return FabricClientSettings.FromNative(settingsResult);
        }

        internal void UpdateFabricClientSettingsInternal(FabricClientSettings settings)
        {
            using (var pin = new PinCollection())
            {
                this.nativeClientSettings.SetSettings(settings.ToNative(pin));
            }
        }

        internal void SetSecurityCredentialsInternal(SecurityCredentials credentials)
        {
            using (var pin = new PinCollection())
            {
                this.nativeClientSettings.SetSecurityCredentials(credentials.ToNative(pin));
            }
        }

        internal void SetKeepAliveInternal(TimeSpan keepAliveInterval)
        {
            this.nativeClientSettings.SetKeepAlive((uint)keepAliveInterval.TotalSeconds);
        }

        internal static object NormalizePartitionKey(object partitionKey)
        {
            if (partitionKey == null || partitionKey is string)
            {
                return partitionKey;
            }

            if (partitionKey is long)
            {
                return partitionKey;
            }
            if (partitionKey is int)
            {
                long castedKey = (int)partitionKey;
                return castedKey;
            }
            throw new ArgumentException(
                string.Format(
                    CultureInfo.CurrentCulture,
                    StringResources.Error_PartitionKeyNotSupported_Formatted,
                    partitionKey.GetType().Name),
                "partitionKey");
        }

        internal static NativeTypes.FABRIC_CLIENT_ROLE ToNativeFabricClientRole(FabricClientRole clientRole)
        {
            switch (clientRole)
            {
                case FabricClientRole.User:
                    return NativeTypes.FABRIC_CLIENT_ROLE.FABRIC_CLIENT_ROLE_USER;
                case FabricClientRole.Admin:
                    return NativeTypes.FABRIC_CLIENT_ROLE.FABRIC_CLIENT_ROLE_ADMIN;
                default:
                    throw new InvalidEnumArgumentException("clientRole", (int)clientRole, typeof(FabricClientRole));
            }
        }

#endregion
    }
}