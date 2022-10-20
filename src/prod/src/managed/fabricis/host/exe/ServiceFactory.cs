// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using System;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Reflection;
    using System.Text;
    using Collections.Generic;
    using Globalization;
    using Linq;
    using Net;
    using Net.NetworkInformation;
    using Runtime.ExceptionServices;

    public sealed class ServiceFactory : IStatefulServiceFactory
    {
        /// <summary>
        /// The setting to be provided in the config (e.g. cluster manifest) to load the corresponding coordinator.
        /// </summary>
        private const string CoordinatorTypeKeyName = "CoordinatorType";

        /// <summary>
        /// Assembly name for loading via reflection. Do not suffix with .dll as the load fails when using <see cref="Assembly.Load(AssemblyName)"/>.
        /// </summary>
        private const string SerialAssemblyName = "FabricIS.serial";

        /// <summary>
        /// <see cref="SerialAssemblyName"/>
        /// </summary>
        private const string ParallelAssemblyName = "FabricIS.parallel";

        private const string SerialCoordinatorFactoryTypeName = "System.Fabric.InfrastructureService.WindowsAzureInfrastructureCoordinatorFactory";
        private const string ParallelCoordinatorFactoryTypeName = "System.Fabric.InfrastructureService.Parallel.AzureParallelInfrastructureCoordinatorFactory";
        private const string ServerRestartCoordinatorFactoryTypeName = "System.Fabric.InfrastructureService.WindowsServerRestartCoordinatorFactory";

        private static readonly TraceType TraceType = new TraceType("ServiceFactory");

        private readonly Dictionary<string, Func<CoordinatorFactoryArgs, IInfrastructureCoordinator>> coordinatorFactoryMap =
            new Dictionary<string, Func<CoordinatorFactoryArgs, IInfrastructureCoordinator>>(StringComparer.OrdinalIgnoreCase);

        // Shared across replicas
        private readonly IInfrastructureAgentWrapper infrastructureServiceAgent;
        private readonly IConfigStore configStore;
        private readonly string configSectionName;
        private readonly FactoryConfigUpdateHandler configUpdateHandler;

        private ServiceFactory(
            IInfrastructureAgentWrapper infrastructureServiceAgent,
            IConfigStore configStore,
            string configSectionName,
            FactoryConfigUpdateHandler configUpdateHandler)
        {
            this.infrastructureServiceAgent = infrastructureServiceAgent;
            this.configStore = configStore.Validate("configStore");
            this.configSectionName = configSectionName;
            this.configUpdateHandler = configUpdateHandler;

            this.coordinatorFactoryMap.Add("Disabled", this.CreateCoordinatorNull);
            this.coordinatorFactoryMap.Add("Test", this.CreateCoordinatorTest);
            this.coordinatorFactoryMap.Add("ServerRestart", this.CreateCoordinatorServerRestart);
            this.coordinatorFactoryMap.Add("AzureSerial", this.CreateCoordinatorAzureSerial);
            this.coordinatorFactoryMap.Add("AzureParallel", this.CreateCoordinatorAzureParallel);
            this.coordinatorFactoryMap.Add("AzureParallelDisabled", this.CreateCoordinatorAzureParallelDisabled);
            this.coordinatorFactoryMap.Add("AzureAutodetect", this.CreateCoordinatorAzureAutodetect);
            this.coordinatorFactoryMap.Add("Dynamic", this.CreateCoordinatorDynamic);
        }

        private sealed class FactoryConfigUpdateHandler : IConfigStoreUpdateHandler
        {
            private static readonly TraceType traceType = new TraceType("FactoryConfigUpdateHandler");
            private readonly HashSet<string> registeredKeys = new HashSet<string>(StringComparer.OrdinalIgnoreCase);

            public bool CheckUpdate(string sectionName, string keyName, string value, bool isEncrypted)
            {
                // CheckUpdate is used by fabric.exe to determine if an upgrade can occur without
                // a fabric node restart.  Since the InfrastructureService runs in a separate process,
                // this check does not apply.
                return true;
            }

            public bool OnUpdate(string sectionName, string keyName)
            {
                string key = GetCompoundKey(sectionName, keyName);
                bool handled;

                lock (registeredKeys)
                {
                    handled = !registeredKeys.Contains(key);
                }

                if (!handled)
                {
                    traceType.WriteInfo("Update detected to factory configuration setting (section = {0}, key = {1})", sectionName, keyName);
                }

                return handled;
            }

            public void RegisterKey(string sectionName, string keyName)
            {
                string key = GetCompoundKey(sectionName, keyName);

                lock (registeredKeys)
                {
                    traceType.WriteInfo("Monitoring factory configuration setting (section = {0}, key = {1})", sectionName, keyName);
                    registeredKeys.Add(key);
                }
            }

            private static string GetCompoundKey(string sectionName, string keyName)
            {
                return $"{sectionName}@{keyName}";
            }
        }

        private T ReadFactoryConfig<T>(
            IConfigSection configSection,
            string keyName,
            T defaultValue = default(T))
        {
            if (this.configUpdateHandler != null)
            {
                this.configUpdateHandler.RegisterKey(configSection.Name, keyName);
            }

            return configSection.ReadConfigValue(keyName, defaultValue);
        }

        public static ServiceFactory CreateAndRegister()
        {
            try
            {
                IInfrastructureAgentWrapper agent = new InfrastructureAgentWrapper();

                var configUpdateHandler = new FactoryConfigUpdateHandler();
                NativeConfigStore configStore = NativeConfigStore.FabricGetConfigStore(configUpdateHandler);

                ServiceFactory factory = new ServiceFactory(agent, configStore, null, configUpdateHandler);

                agent.RegisterInfrastructureServiceFactory(factory);

                return factory;
            }
            catch (Exception ex)
            {
                TraceType.WriteError("Error registering infrastructure service factory. Cannot continue further. Exception: {0}", ex);
                throw;
            }
        }

        public static ServiceFactory CreateAppServiceFactory()
        {
            IInfrastructureAgentWrapper agent = new NullInfrastructureAgent();

            AppConfigStore configStore = new AppConfigStore(
                FabricRuntime.GetActivationContext(),
                "Config");

            return new ServiceFactory(agent, configStore, "InfrastructureService", null);
        }

        public IStatefulServiceReplica CreateReplica(
            string serviceTypeName,
            Uri serviceName,
            byte[] initializationData,
            Guid partitionId,
            long replicaId)
        {
            string replicaConfigSectionName = this.configSectionName;
            if (replicaConfigSectionName == null)
            {
                // When run as a system service, the configuration section name is encoded
                // in the service initialization data, because there can be multiple
                // InfrastructureService instances, each with its own config section.
                // When run as an application, a fixed configuration section name is used,
                // and passed into the factory constructor.
                replicaConfigSectionName = Encoding.Unicode.GetString(initializationData);
            }

            Trace.WriteInfo(
                TraceType,
                "CreateReplica: serviceName = {0}, partitionId = {1}, configSectionName = {2}",
                serviceName,
                partitionId,
                replicaConfigSectionName);

            try
            {
                var args = new CoordinatorFactoryArgs()
                {
                    Agent = this.infrastructureServiceAgent,
                    ConfigSection = GetConfigSection(replicaConfigSectionName),
                    PartitionId = partitionId,
                    ReplicaId = replicaId,
                    ServiceName = serviceName,
                };

                IInfrastructureCoordinator coordinator = CreateCoordinator(args);

                var replica = new ServiceReplica(
                    this.infrastructureServiceAgent,
                    coordinator,
                    GetReplicatorAddress(coordinator, args.ConfigSection),
                    UseClusterSecuritySettingsForReplicator(args.ConfigSection),
                    args.ConfigSection);

                return replica;
            }
            catch (TargetInvocationException ex)
            {
                TraceType.WriteError("Error creating infrastructure coordinator. Cannot continue further. Exception: {0}", ex);

                if (ex.InnerException != null)
                {
                    // Rethrow, preserving the original stack trace from exceptions that
                    // come from dynamic invoke (e.g. factory creation) via reflection.
                    ExceptionDispatchInfo.Capture(ex.InnerException).Throw();
                }

                throw;
            }
            catch (Exception ex)
            {
                TraceType.WriteError("Error creating infrastructure coordinator. Cannot continue further. Exception: {0}", ex);
                throw;
            }            
        }

        private string GetReplicatorAddress(IInfrastructureCoordinator coordinator, IConfigSection configSection)
        {
            string replicatorAddress = null;

            bool loopbackOnly = (coordinator is TestInfrastructureCoordinator);
            if (!loopbackOnly)
            {
                replicatorAddress = string.Format(
                    CultureInfo.InvariantCulture,
                    "{0}:{1}",
                    GetReplicatorHost(),
                    GetReplicatorPort(configSection));
            }

            return replicatorAddress;
        }

        private static string GetReplicatorHost()
        {
            string fabricNodeAddress = Environment.GetEnvironmentVariable("Fabric_NodeIPOrFQDN");
            if (!string.IsNullOrEmpty(fabricNodeAddress))
            {
                TraceType.WriteInfo("Using fabric node address for replication endpoint: {0}", fabricNodeAddress);
                return fabricNodeAddress;
            }

            TraceType.WriteInfo("Using IP address discovery for replication endpoint");
            return GetIPAddressOrHostName(true);
        }

        /// <summary>
        /// Returns the IP address or host name of this machine. IPv4 is preferred.
        /// </summary>
        /// <param name="isFabricReplicatorAddress">True, if the call is made for composing the replicator address.</param>
        /// <returns></returns>
        private static string GetIPAddressOrHostName(bool isFabricReplicatorAddress)
        {
            IPAddress ipv6Address = null;
            foreach (NetworkInterface ni in NetworkInterface.GetAllNetworkInterfaces().Where(t => (t.NetworkInterfaceType != NetworkInterfaceType.Loopback)))
            {
                foreach (UnicastIPAddressInformation ip in ni.GetIPProperties().UnicastAddresses)
                {
                    if (ip.Address.AddressFamily == System.Net.Sockets.AddressFamily.InterNetwork)
                    {
                        return ip.Address.ToString();
                    }
                    else if (ipv6Address == null && ip.Address.AddressFamily == System.Net.Sockets.AddressFamily.InterNetworkV6)
                    {
                        ipv6Address = ip.Address;
                    }
                }
            }

            if (ipv6Address != null)
            {
                if (isFabricReplicatorAddress)
                {
                    return string.Format(CultureInfo.InvariantCulture, "[{0}]", ipv6Address);
                }
                return ipv6Address.ToString();
            }
            string domainName = IPGlobalProperties.GetIPGlobalProperties().DomainName;
            string hostName = Dns.GetHostName();

            if (!hostName.EndsWith("." + domainName, StringComparison.OrdinalIgnoreCase))
            {
                hostName += "." + domainName;
            }
            return hostName;
        }

        private int GetReplicatorPort(IConfigSection configSection)
        {
            if (this.ReadFactoryConfig(configSection, "UseFabricRuntimeForReplicatorEndpoint", false))
            {
                int port = FabricRuntime.GetActivationContext().GetEndpoint("ReplicatorEndpoint").Port;
                TraceType.WriteInfo("Fabric runtime returned ReplicatorEndpoint port {0}", port);
                return port;
            }

            // Activation context and the dynamic endpoint information was not available
            // in FabricIS.exe when run as a system service.
            return 0;
        }

        private bool UseClusterSecuritySettingsForReplicator(IConfigSection configSection)
        {
            NativeConfigStore configStore = NativeConfigStore.FabricGetConfigStore();
            if (EnableEndpointV2Utility.GetValue(configStore))
            {
                TraceType.WriteInfo("Using cluster security settings for replicator because cluster setting EnableEndpointV2 is true");
                return true;
            }

            if (this.ReadFactoryConfig(configSection, "UseClusterSecuritySettingsForReplicator", false))
            {
                TraceType.WriteInfo("Using cluster security settings for replicator because UseClusterSecuritySettingsForReplicator is true");
                return true;
            }

            return false;
        }

        private IConfigSection GetConfigSection(string configSectionName)
        {
            return new ConfigSection(new TraceType("Config"), this.configStore, configSectionName);
        }

        private IInfrastructureCoordinator CreateCoordinator(CoordinatorFactoryArgs args)
        {
            IInfrastructureCoordinator coordinator;

            var configSection = args.ConfigSection;
            if (configSection.SectionExists)
            {
                string coordinatorType = this.ReadFactoryConfig(configSection, CoordinatorTypeKeyName, string.Empty);
                TraceType.WriteInfo("Coordinator type: {0}", coordinatorType ?? "<not specified>");

                Func<CoordinatorFactoryArgs, IInfrastructureCoordinator> factory;
                if (!this.coordinatorFactoryMap.TryGetValue(coordinatorType, out factory))
                {
                    // Current behavior is that coordinator defaults to AzureAutodetect when not specified in config
                    TraceType.WriteInfo("Using default coordinator type: AzureAutodetect");
                    factory = this.CreateCoordinatorAzureAutodetect;
                }

                if (this.ReadFactoryConfig(configSection, "DelayLoadCoordinator", false))
                {
                    TraceType.WriteInfo("Coordinator will be delay-loaded");
                    coordinator = new DelayLoadCoordinator(factory, args, new ServiceFabricHealthClient(), configSection);
                }
                else
                {
                    coordinator = factory(args);
                }
            }
            else
            {
                Trace.WriteWarning(TraceType, "No configuration found; infrastructure coordinator is not active");
                coordinator = this.CreateCoordinatorNull(args);
            }

            return coordinator;
        }

        IInfrastructureCoordinator CreateCoordinatorNull(CoordinatorFactoryArgs args)
        {
            return new NullCoordinator();
        }

        IInfrastructureCoordinator CreateCoordinatorTest(CoordinatorFactoryArgs args)
        {
            return new TestInfrastructureCoordinator(args.Agent, args.PartitionId);
        }

        IInfrastructureCoordinator CreateCoordinatorServerRestart(CoordinatorFactoryArgs args)
        {
            return CreateCoordinatorByReflection(
                SerialAssemblyName,
                ServerRestartCoordinatorFactoryTypeName,
                this.configStore, args.ConfigSectionName, args.PartitionId, args.ReplicaId);
        }

        IInfrastructureCoordinator CreateCoordinatorAzureSerial(CoordinatorFactoryArgs args)
        {
            return InternalCreateCoordinatorAzureSerial(args, null);
        }

        IInfrastructureCoordinator InternalCreateCoordinatorAzureSerial(CoordinatorFactoryArgs args, IAzureModeDetector modeDetector)
        {
            return CreateCoordinatorByReflection(
                SerialAssemblyName,
                SerialCoordinatorFactoryTypeName,
                this.configStore, args.ConfigSectionName, args.PartitionId, args.ReplicaId, args.Agent, modeDetector);
        }

        IInfrastructureCoordinatorFactory InternalCreateCoordinatorFactoryAzureParallel(CoordinatorFactoryArgs args)
        {
            return CreateCoordinatorFactoryByReflection(
                ParallelAssemblyName,
                ParallelCoordinatorFactoryTypeName,
                args.ServiceName, this.configStore, args.ConfigSectionName, args.PartitionId, args.ReplicaId, args.Agent);
        }

        IInfrastructureCoordinator CreateCoordinatorAzureParallel(CoordinatorFactoryArgs args)
        {
            var factory = InternalCreateCoordinatorFactoryAzureParallel(args);
            return factory.Create();
        }

        IInfrastructureCoordinator CreateCoordinatorAzureParallelDisabled(CoordinatorFactoryArgs args)
        {
            return CreateCoordinatorByReflection(
                ParallelAssemblyName,
                "System.Fabric.InfrastructureService.Parallel.AzureParallelDisabledCoordinatorFactory",
                args.ServiceName, this.configStore, args.ConfigSectionName, args.PartitionId, args.ReplicaId, args.Agent);
        }

        /// <summary>
        /// Detects the mode by checking if the parallel endpoint is publishing
        /// job information. If so, creates the parallel coordinator. If not, creates
        /// the serial coordinator but passes it a reference to the autodetector, so
        /// that the serial coordinator can exit when it detects that parallel mode has
        /// been enabled via a tenant update. That update would be invisible over the
        /// serial channel, so this is the only way the serial coordinator can detect a change.
        /// </summary>
        IInfrastructureCoordinator CreateCoordinatorAzureAutodetect(CoordinatorFactoryArgs args)
        {
            var azureParallelFactory = InternalCreateCoordinatorFactoryAzureParallel(args);

            IAzureModeDetector modeDetector = ((IAzureModeDetectorFactory)azureParallelFactory).CreateAzureModeDetector();

            AzureMode mode = modeDetector.DetectModeAsync().GetAwaiter().GetResult();
            TraceType.WriteInfo("Auto-detected Azure mode: {0}", mode);

            switch (mode)
            {
                case AzureMode.Serial:
                    return InternalCreateCoordinatorAzureSerial(args, modeDetector);

                case AzureMode.Parallel:
                    return azureParallelFactory.Create();

                default:
                    throw new InvalidOperationException("Failed to auto-detect Azure coordinator mode");
            }
        }

        IInfrastructureCoordinator CreateCoordinatorDynamic(CoordinatorFactoryArgs args)
        {
            string coordinatorAssemblyName = this.ReadFactoryConfig(
                args.ConfigSection,
                "CoordinatorAssemblyName",
                string.Empty);

            string coordinatorFactoryType = this.ReadFactoryConfig(
                args.ConfigSection,
                "CoordinatorFactoryType",
                string.Empty);

            return CreateCoordinatorByReflection(
                coordinatorAssemblyName,
                coordinatorFactoryType,
                args.ServiceName,
                this.configStore,
                args.ConfigSectionName,
                args.PartitionId,
                args.ReplicaId);
        }

        private static IInfrastructureCoordinatorFactory CreateCoordinatorFactoryByReflection(
            string assemblyName,
            string factoryTypeName,
            params object[] factoryCreateMethodArgs)
        {
            TraceType.WriteInfo(
                "Loading coordinator factory {0} from assembly {1}",
                factoryTypeName,
                assemblyName);

            // Using this particular overload of Assembly.Load only for CoreCLR compat.
            // TODO, move the serial and parallel assemblies into separate folders and use
            // AssemblyLoadContext.Default.Load* API once CoreClr Fx assemblies are part of our build system. 
            Assembly assembly = Assembly.Load(new AssemblyName(assemblyName));
            Type factoryType = assembly.GetType(factoryTypeName, true, true);
            return (IInfrastructureCoordinatorFactory)Activator.CreateInstance(factoryType, factoryCreateMethodArgs);
        }

        private static IInfrastructureCoordinator CreateCoordinatorByReflection(
            string assemblyName,
            string factoryTypeName,
            params object[] factoryCreateMethodArgs)
        {
            try
            {
                var coordinatorFactory = CreateCoordinatorFactoryByReflection(assemblyName, factoryTypeName, factoryCreateMethodArgs);
                IInfrastructureCoordinator coordinator = coordinatorFactory.Create();
                return coordinator;
            }
            catch (Exception ex)
            {
                TraceType.WriteError(
                    "Unable to create infrastructure coordinator by reflection. Assembly name: {0}, Factory type name: {1}, Error: {2}", 
                    assemblyName, 
                    factoryTypeName, 
                    ex);
                throw;
            }
        }
    }
}