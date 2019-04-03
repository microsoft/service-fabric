// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.GatewayResourceManager.Service
{
    using System;
    using System.Collections.Generic;
    using System.Collections.Specialized;
    using System.Diagnostics;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.GatewayResourceManager.Service.Models;
    using System.Fabric.Query;
    using System.IO;
    using System.Net;
    using System.Net.Sockets;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Data;
    using Microsoft.ServiceFabric.Data.Collections;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Linq;

    public class Manager
    {
        private const int DefaultDelayBeforeRetryInSeconds = 1;
        private const int DefaultRetryCount = 60;

        private static readonly string CollectionName = "GatewayConfigs";
        private static readonly string ProxyAppTypeForGateway = "GatewayProxyApplicationType";
        private static readonly string GatewayProxyServiceName = "/GatewayProxyService";
        private static readonly TimeSpan DefaultOperationTimeout = TimeSpan.FromMinutes(1.0);
        private static readonly Random Random = new Random();

        // For NAT networks, host ports specified by user is mapped to ports inside the container
        // starting from 56789. Proxy should listen on these ports in the container.
        private static readonly int BaseNATPort = 56789;
        private static readonly object LockObject = new object();

        private static FabricClient client;
        private static GRMSettings grmSettings;
        private static Dictionary<string, DateTime> lastProxyStartTimes = new Dictionary<string, DateTime>();

        private readonly IReliableStateManager stateManager;
        private readonly ServiceContext serviceContext;
        private GatewayConfig config;
        private GatewayTraceConfig gatewayTraceConfig;

        static Manager()
        {
            client = new FabricClient();
            grmSettings = new GRMSettings();
            grmSettings.LoadSettings(NativeConfigStore.FabricGetConfigStore(null));
        }

        public Manager(string gatewayName, IReliableStateManager stateManager, ServiceContext context)
        {
            this.config = new GatewayConfig();
            if (string.IsNullOrWhiteSpace(gatewayName))
            {
                gatewayName = "<ALL>";
            }

            this.config.GatewayName = gatewayName;
            this.stateManager = stateManager;
            this.serviceContext = context;

            this.gatewayTraceConfig = new GatewayTraceConfig(gatewayName);
        }

        public async Task<Tuple<int, GatewayResourceDescription>> UpgradeOrAddResource(
            GatewayResourceDescription resourceDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ApplicationStatus state = ApplicationStatus.Invalid;

            this.gatewayTraceConfig.Operation = "UpgradeOrAddResource";
            this.gatewayTraceConfig.ResourceDescription = resourceDescription;
            TraceMessage(System.Diagnostics.TraceLevel.Info, this.gatewayTraceConfig);

            Stopwatch watch = new Stopwatch();
            watch.Start();

            this.config.ResourceDescription = resourceDescription;
            try
            {
                var configDictionary = await ExecuteFabricActionWithRetryAsync(
                    () => this.stateManager.GetOrAddAsync<IReliableDictionary<string, GatewayConfig>>(CollectionName),
                        timeout - watch.Elapsed,
                        cancellationToken,
                        this.gatewayTraceConfig);

                while (true)
                {
                    try
                    {
                        using (ITransaction tx = this.stateManager.CreateTransaction())
                        {
                            try
                            {
                                var savedConfig = await ExecuteFabricActionWithRetryAsync(
                                    () => configDictionary.TryGetValueAsync(tx, this.config.GatewayName),
                                    timeout - watch.Elapsed,
                                    cancellationToken,
                                    this.gatewayTraceConfig);
                                if (savedConfig.HasValue)
                                {
                                    this.gatewayTraceConfig.Message = "Found existing gateway resource";
                                    TraceMessage(System.Diagnostics.TraceLevel.Info, this.gatewayTraceConfig);

                                    var proxyStatus = await this.CheckProxyApplicationStatus(timeout - watch.Elapsed, cancellationToken);
                                    if (proxyStatus.Item1 == ApplicationStatus.Deleting)
                                    {
                                        this.config.ResourceDescription.Properties.Status = ApplicationStatus.Deleting.ToString();
                                        this.config.ResourceDescription.Properties.StatusDetails =
                                            "Conflicting operation in progress. Unable to update gateway resource.";

                                        this.gatewayTraceConfig.Message = new
                                        {
                                            Status = this.config.ResourceDescription.Properties.Status.ToString(),
                                            this.config.ResourceDescription.Properties.StatusDetails
                                        };
                                        TraceMessage(System.Diagnostics.TraceLevel.Warning, this.gatewayTraceConfig);

                                        return new Tuple<int, GatewayResourceDescription>(409, this.config.ResourceDescription);
                                    }

                                    if (savedConfig.Value.ResourceDescription.Properties.Equals(this.config.ResourceDescription.Properties))
                                    {
                                        if (proxyStatus.Item1 == ApplicationStatus.Ready)
                                        {
                                            this.config.ResourceDescription.Properties.Status = ApplicationStatus.Ready.ToString();

                                            this.gatewayTraceConfig.Message = new
                                            {
                                                Status = this.config.ResourceDescription.Properties.Status.ToString(),
                                                StatusDetails = "Current gateway resource config same as requested gateway config resource"
                                            };
                                            TraceMessage(System.Diagnostics.TraceLevel.Info, this.gatewayTraceConfig);

                                            return new Tuple<int, GatewayResourceDescription>(201, this.config.ResourceDescription);
                                        }

                                        if (proxyStatus.Item1 == ApplicationStatus.Creating ||
                                            proxyStatus.Item1 == ApplicationStatus.Upgrading)
                                        {
                                            this.config.ResourceDescription.Properties.Status = proxyStatus.Item1.ToString();

                                            this.gatewayTraceConfig.Message = new
                                            {
                                                Status = this.config.ResourceDescription.Properties.Status.ToString(),
                                                StatusDetails = "Current gateway resource config same as requested gateway config resource"
                                            };
                                            TraceMessage(System.Diagnostics.TraceLevel.Info, this.gatewayTraceConfig);

                                            return new Tuple<int, GatewayResourceDescription>(202, this.config.ResourceDescription);
                                        }
                                    }

                                    state = ApplicationStatus.Upgrading;
                                }
                                else
                                {
                                    state = ApplicationStatus.Creating;
                                }

                                this.gatewayTraceConfig.Message = "Storing Config in Collection";
                                TraceMessage(System.Diagnostics.TraceLevel.Info, this.gatewayTraceConfig);

                                await ExecuteFabricActionWithRetryAsync(
                                    () => configDictionary.AddOrUpdateAsync(tx, this.config.GatewayName, this.config, (key, oldvalue) => this.config),
                                    timeout - watch.Elapsed,
                                    cancellationToken,
                                    this.gatewayTraceConfig);
                                    
                                this.gatewayTraceConfig.Message = "Stored Config in Collection";
                                TraceMessage(System.Diagnostics.TraceLevel.Info, this.gatewayTraceConfig);
                            }
                            finally
                            {
                                await tx.CommitAsync();
                            }

                            break;
                        }
                    }
                    catch (InvalidOperationException e)
                    {
                        TraceExceptionMessage(System.Diagnostics.TraceLevel.Warning, this.gatewayTraceConfig, e);
                    }

                    cancellationToken.ThrowIfCancellationRequested();
                    await Task.Delay(TimeSpan.FromSeconds(AddRandomSaltToDelayInSeconds(DefaultDelayBeforeRetryInSeconds)));

                    if (watch.Elapsed > timeout)
                    {
                        this.gatewayTraceConfig.Message = "Unable to save resource config";
                        TraceMessage(System.Diagnostics.TraceLevel.Error, this.gatewayTraceConfig);

                        throw new TimeoutException();
                    }
                }
            }
            catch (Exception e)
            {
                this.config.ResourceDescription.Properties.StatusDetails = e.Message;
                this.config.ResourceDescription.Properties.Status = ApplicationStatus.Failed.ToString();

                this.gatewayTraceConfig.Message = new
                {
                    Status = this.config.ResourceDescription.Properties.Status.ToString(),
                };
                TraceExceptionMessage(System.Diagnostics.TraceLevel.Error, this.gatewayTraceConfig, e);

                return new Tuple<int, GatewayResourceDescription>(500, this.config.ResourceDescription);
            }

#pragma warning disable CS4014
            Task.Run(() => this.UpgradeProxyApplication());
#pragma warning restore CS4014

            this.config.ResourceDescription.Properties.Status = state.ToString();

            this.gatewayTraceConfig.Message = new
            {
                Status = this.config.ResourceDescription.Properties.Status.ToString(),
            };
            TraceMessage(System.Diagnostics.TraceLevel.Info, this.gatewayTraceConfig);

            return new Tuple<int, GatewayResourceDescription>(202, this.config.ResourceDescription);
        }

        public async Task<Tuple<int, GatewayResourceDescription>> DeleteResource(
            TimeSpan timeout, CancellationToken cancellationToken)
        {
            this.gatewayTraceConfig.Operation = "DeleteResource";
            TraceMessage(System.Diagnostics.TraceLevel.Info, this.gatewayTraceConfig);

            Stopwatch watch = new Stopwatch();
            watch.Start();

            try
            {
                var configDictionary = await ExecuteFabricActionWithRetryAsync(
                    () => this.stateManager.GetOrAddAsync<IReliableDictionary<string, GatewayConfig>>(CollectionName),
                    timeout - watch.Elapsed,
                    cancellationToken,
                    this.gatewayTraceConfig);

                while (true)
                {
                    try
                    {
                        using (ITransaction tx = this.stateManager.CreateTransaction())
                        {
                            try
                            {
                                var savedConfig = await ExecuteFabricActionWithRetryAsync(
                                    () => configDictionary.TryGetValueAsync(tx, this.config.GatewayName),
                                    timeout - watch.Elapsed,
                                    cancellationToken,
                                    this.gatewayTraceConfig);
                                if (!savedConfig.HasValue)
                                {
                                    return new Tuple<int, GatewayResourceDescription>(404, null);
                                }
                            }
                            finally
                            {
                                await tx.CommitAsync();
                            }

                            break;
                        }
                    }
                    catch (InvalidOperationException e)
                    {
                        TraceExceptionMessage(System.Diagnostics.TraceLevel.Warning, this.gatewayTraceConfig, e);
                    }

                    cancellationToken.ThrowIfCancellationRequested();
                    await Task.Delay(TimeSpan.FromSeconds(AddRandomSaltToDelayInSeconds(DefaultDelayBeforeRetryInSeconds)));

                    if (watch.Elapsed > timeout)
                    {
                        this.gatewayTraceConfig.Message = "Unable to retrive resource config";
                        TraceMessage(System.Diagnostics.TraceLevel.Error, this.gatewayTraceConfig);

                        throw new TimeoutException();
                    }
                }

                var proxyStatus = await this.CheckProxyApplicationStatus(timeout, cancellationToken);
                if (proxyStatus.Item1 == ApplicationStatus.Deleting)
                {
                    return new Tuple<int, GatewayResourceDescription>(202, null);
                }
            }
            catch (Exception e)
            {
                TraceExceptionMessage(System.Diagnostics.TraceLevel.Error, this.gatewayTraceConfig, e);
                return new Tuple<int, GatewayResourceDescription>(500, null);
            }

#pragma warning disable CS4014
            Task.Run(() => this.StopProxyAndRemoveGateway());
#pragma warning restore CS4014

            this.gatewayTraceConfig.Message = new
            {
                Status = ApplicationStatus.Deleting.ToString(),
            };
            TraceMessage(System.Diagnostics.TraceLevel.Info, this.gatewayTraceConfig);

            return new Tuple<int, GatewayResourceDescription>(202, null);
        }

        public async Task<Tuple<int, List<GatewayResourceDescription>>> GetConfigForGateway(
            TimeSpan timeout, CancellationToken cancellationToken)
        {
            this.gatewayTraceConfig.Operation = "GetConfigForGateway";
            TraceMessage(System.Diagnostics.TraceLevel.Info, this.gatewayTraceConfig);

            Stopwatch watch = new Stopwatch();
            watch.Start();
            var result = new List<GatewayResourceDescription>();

            try
            {
                var configDictionary = await ExecuteFabricActionWithRetryAsync(
                    () => this.stateManager.GetOrAddAsync<IReliableDictionary<string, GatewayConfig>>(CollectionName),
                    timeout - watch.Elapsed,
                    cancellationToken,
                    this.gatewayTraceConfig);

                while (true)
                {
                    try
                    {
                        using (ITransaction tx = this.stateManager.CreateTransaction())
                        {
                            try
                            {
                                var savedConfig = await ExecuteFabricActionWithRetryAsync(
                                    () => configDictionary.TryGetValueAsync(tx, this.config.GatewayName),
                                    timeout - watch.Elapsed,
                                    cancellationToken,
                                    this.gatewayTraceConfig);
                                if (!savedConfig.HasValue)
                                {
                                    return new Tuple<int, List<GatewayResourceDescription>>(404, null);
                                }

                                result.Add(savedConfig.Value.ResourceDescription);
                            }
                            finally
                            {
                                await tx.CommitAsync();
                            }

                            break;
                        }
                    }
                    catch (InvalidOperationException e)
                    {
                        TraceExceptionMessage(System.Diagnostics.TraceLevel.Warning, this.gatewayTraceConfig, e);
                    }

                    cancellationToken.ThrowIfCancellationRequested();
                    await Task.Delay(TimeSpan.FromSeconds(AddRandomSaltToDelayInSeconds(DefaultDelayBeforeRetryInSeconds)));

                    if (watch.Elapsed > timeout)
                    {
                        this.gatewayTraceConfig.Message = "Unable to retrive resource config";
                        TraceMessage(System.Diagnostics.TraceLevel.Error, this.gatewayTraceConfig);

                        throw new TimeoutException();
                    }
                }
            }
            catch (Exception e)
            {
                TraceExceptionMessage(System.Diagnostics.TraceLevel.Error, this.gatewayTraceConfig, e);
                return new Tuple<int, List<GatewayResourceDescription>>(500, null);
            }

            foreach (var description in result)
            {
                var proxyStatus = await CheckProxyApplicationStatus(description.Name, timeout - watch.Elapsed, cancellationToken, this.gatewayTraceConfig);
                description.Properties.Status = proxyStatus.Item1.ToString();
                description.Properties.StatusDetails = proxyStatus.Item2;
            }

            this.gatewayTraceConfig.Message = result;
            TraceMessage(System.Diagnostics.TraceLevel.Info, this.gatewayTraceConfig);

            return new Tuple<int, List<GatewayResourceDescription>>(200, result);
        }

        public async Task<Tuple<int, List<GatewayResourceDescription>>> GetConfigs(
            TimeSpan timeout, CancellationToken cancellationToken)
        {
            this.gatewayTraceConfig.Operation = "GetConfigs";
            TraceMessage(System.Diagnostics.TraceLevel.Info, this.gatewayTraceConfig);

            Stopwatch watch = new Stopwatch();
            watch.Start();
            var result = new List<GatewayResourceDescription>();

            try
            {
                var configDictionary = await ExecuteFabricActionWithRetryAsync(
                    () => this.stateManager.GetOrAddAsync<IReliableDictionary<string, GatewayConfig>>(CollectionName),
                    timeout - watch.Elapsed,
                    cancellationToken,
                    this.gatewayTraceConfig);

                while (true)
                {
                    try
                    {
                        using (ITransaction tx = this.stateManager.CreateTransaction())
                        {
                            try
                            {
                                var list = await ExecuteFabricActionWithRetryAsync(
                                    () => configDictionary.CreateEnumerableAsync(tx),
                                    timeout - watch.Elapsed,
                                    cancellationToken,
                                    this.gatewayTraceConfig);
                                var enumerator = list.GetAsyncEnumerator();
                                while (await enumerator.MoveNextAsync(cancellationToken))
                                {
                                    result.Add(enumerator.Current.Value.ResourceDescription);
                                }
                            }
                            finally
                            {
                                await tx.CommitAsync();
                            }

                            break;
                        }
                    }
                    catch (InvalidOperationException e)
                    {
                        TraceExceptionMessage(System.Diagnostics.TraceLevel.Warning, this.gatewayTraceConfig, e);
                    }

                    cancellationToken.ThrowIfCancellationRequested();
                    await Task.Delay(TimeSpan.FromSeconds(AddRandomSaltToDelayInSeconds(DefaultDelayBeforeRetryInSeconds)));

                    if (watch.Elapsed > timeout)
                    {
                        this.gatewayTraceConfig.Message = "Unable to retrive resource config";
                        TraceMessage(System.Diagnostics.TraceLevel.Error, this.gatewayTraceConfig);

                        throw new TimeoutException();
                    }
                }
            }
            catch (Exception e)
            {
                TraceExceptionMessage(System.Diagnostics.TraceLevel.Error, this.gatewayTraceConfig, e);

                GatewayResourceDescription description = new GatewayResourceDescription();
                description.Properties.Status = ApplicationStatus.Failed.ToString();
                description.Properties.StatusDetails = e.Message;
                return new Tuple<int, List<GatewayResourceDescription>>(500, null);
            }

            foreach (var description in result)
            {
                var proxyStatus = await CheckProxyApplicationStatus(description.Name, timeout - watch.Elapsed, cancellationToken, this.gatewayTraceConfig);
                description.Properties.Status = proxyStatus.Item1.ToString();
                description.Properties.StatusDetails = proxyStatus.Item2;
            }

            this.gatewayTraceConfig.Message = result;
            TraceMessage(System.Diagnostics.TraceLevel.Info, this.gatewayTraceConfig);

            return new Tuple<int, List<GatewayResourceDescription>>(200, result);
        }

        private static Uri EnvoyAppNameForGateway(string gatewayName)
        {
            return new Uri("fabric:/GatewayProxyFor_" + gatewayName);
        }

        private static string GetApplicationPackageFolder()
        {
            string applicationPackageFolder = string.Empty;
            bool isOpen = grmSettings.IPProviderEnabled;
            bool isIsolated = grmSettings.IsolatedNetworkSetup;
            bool isNatIpProvider = grmSettings.LocalNatIPProviderEnabled;

#if DotNetCoreClrLinux
            if (isIsolated)
            {
                // Proxy connected to two networks
                applicationPackageFolder = isOpen? "OpenIL" : "NATIL";
            }
            else
            {
                // Proxy connected to one network
                applicationPackageFolder = isOpen ? "OpenL" : "NATL";
            }
#else
            if (isIsolated)
            {
                // Proxy connected to two networks
                applicationPackageFolder = isOpen ? "OpenI" : "NATI";
            }
            else
            {
                if (isNatIpProvider)
                {
                    applicationPackageFolder = "Open";
                }
                else
                {
                    // Proxy connected to one network
                    applicationPackageFolder = isOpen ? "Open" : "NAT";
                }
            }

#endif
            return applicationPackageFolder;
        }

        private static void TraceRetryMessage(GatewayTraceConfig traceConfig, System.Diagnostics.TraceLevel traceLevel, string retryBehavior, TimeSpan timeRemaining, Exception e)
        {
            traceConfig.Message = new
            {
                RetryBehavior = retryBehavior,
                TimeRemaining = timeRemaining,
            };

            TraceExceptionMessage(traceLevel, traceConfig, e);
        }

        private static int AddRandomSaltToDelayInSeconds(int delay)
        {
            return delay + Random.Next(2);
        }

        private static async Task ExecuteFabricActionWithRetryAsync(Func<Task> function, GatewayTraceConfig traceConfig)
        {
            await ExecuteFabricActionWithRetryAsync(function, DefaultOperationTimeout, CancellationToken.None, traceConfig);
        }

        private static async Task ExecuteFabricActionWithRetryAsync(
            Func<Task> function,
            TimeSpan operationTimeout,
            CancellationToken cancellationToken,
            GatewayTraceConfig gatewayTraceConfig,
            int delayRetryForSeconds = DefaultDelayBeforeRetryInSeconds)
        {
            var traceConfig = gatewayTraceConfig.ShallowCopy();
            traceConfig.Api = function.GetType().ToString();

            Stopwatch watch = new Stopwatch();
            watch.Start();
            while (true)
            {
                cancellationToken.ThrowIfCancellationRequested();

                try
                {
                    await function().ConfigureAwait(false);
                    return;
                }
                catch (FabricTransientException e)
                {
                    TraceRetryMessage(traceConfig, System.Diagnostics.TraceLevel.Info, "Retrying operation", operationTimeout - watch.Elapsed, e);
                    await Task.Delay(TimeSpan.FromSeconds(AddRandomSaltToDelayInSeconds(delayRetryForSeconds)), cancellationToken);
                }
                catch (TimeoutException e)
                {
                    if (watch.Elapsed + TimeSpan.FromSeconds(AddRandomSaltToDelayInSeconds(delayRetryForSeconds)) > operationTimeout)
                    {
                        TraceRetryMessage(traceConfig, System.Diagnostics.TraceLevel.Error, "Operation timed out", TimeSpan.Zero, e);
                        throw;
                    }

                    TraceRetryMessage(traceConfig, System.Diagnostics.TraceLevel.Info, "Retrying operation", operationTimeout - watch.Elapsed, e);
                    await Task.Delay(TimeSpan.FromSeconds(AddRandomSaltToDelayInSeconds(delayRetryForSeconds)), cancellationToken);
                }
            }
        }

        private static async Task<T> ExecuteFabricActionWithRetryAsync<T>(Func<Task<T>> function, GatewayTraceConfig traceConfig)
        {
            return await ExecuteFabricActionWithRetryAsync<T>(function, DefaultOperationTimeout, CancellationToken.None, traceConfig);
        }

        private static async Task<T> ExecuteFabricActionWithRetryAsync<T>(
            Func<Task<T>> function,
            TimeSpan operationTimeout,
            CancellationToken cancellationToken,
            GatewayTraceConfig gatewayTraceConfig,
            int delayRetryForSeconds = DefaultDelayBeforeRetryInSeconds)
        {
            var traceConfig = gatewayTraceConfig.ShallowCopy();
            traceConfig.Api = function.GetType().ToString();

            Stopwatch watch = new Stopwatch();
            watch.Start();
            while (true)
            {
                cancellationToken.ThrowIfCancellationRequested();

                try
                {
                    return await function().ConfigureAwait(false);
                }
                catch (FabricTransientException e)
                {
                    TraceRetryMessage(traceConfig, System.Diagnostics.TraceLevel.Info, "Retrying operation", operationTimeout - watch.Elapsed, e);
                    await Task.Delay(TimeSpan.FromSeconds(AddRandomSaltToDelayInSeconds(delayRetryForSeconds)), cancellationToken);
                }
                catch (TimeoutException e)
                {
                    if (watch.Elapsed + TimeSpan.FromSeconds(AddRandomSaltToDelayInSeconds(delayRetryForSeconds)) > operationTimeout)
                    {
                        TraceRetryMessage(traceConfig, System.Diagnostics.TraceLevel.Error, "Operation timed out", TimeSpan.Zero, e);
                        throw;
                    }

                    TraceRetryMessage(traceConfig, System.Diagnostics.TraceLevel.Info, "Retrying operation", operationTimeout - watch.Elapsed, e);
                    await Task.Delay(TimeSpan.FromSeconds(AddRandomSaltToDelayInSeconds(delayRetryForSeconds)), cancellationToken);
                }
            }
        }

        private static async Task<Tuple<Query.ApplicationTypeStatus, string>> CheckProxyApplicationRegistrationStatus(GatewayTraceConfig gatewayTraceConfig)
        {
            var traceConfig = gatewayTraceConfig.ShallowCopy();
            traceConfig.Task = "CheckProxyApplicationRegistrationStatus";

            try
            {
                var appliationTypes = await ExecuteFabricActionWithRetryAsync(
                    () => client.QueryManager.GetApplicationTypeListAsync(ProxyAppTypeForGateway),
                    traceConfig);

                foreach (var applicationType in appliationTypes)
                {
                    if (applicationType.ApplicationTypeName == ProxyAppTypeForGateway)
                    {
                        traceConfig.Message = new
                        {
                            Status = applicationType.Status.ToString(),
                            applicationType.StatusDetails
                        };
                        if (applicationType.Status == ApplicationTypeStatus.Failed)
                        {
                            TraceMessage(System.Diagnostics.TraceLevel.Error, traceConfig);
                        }
                        else
                        {
                            TraceMessage(System.Diagnostics.TraceLevel.Info, traceConfig);
                        }

                        return new Tuple<ApplicationTypeStatus, string>(applicationType.Status, applicationType.StatusDetails);
                    }
                }

                traceConfig.Message = "Proxy application type not registered";
                TraceMessage(System.Diagnostics.TraceLevel.Warning, traceConfig);

                return new Tuple<ApplicationTypeStatus, string>(Query.ApplicationTypeStatus.Invalid, "Proxy application type not registered");
            }
            catch (Exception e)
            {
                TraceExceptionMessage(System.Diagnostics.TraceLevel.Error, traceConfig, e);
            }

            return new Tuple<ApplicationTypeStatus, string>(Query.ApplicationTypeStatus.Invalid, "Proxy application type not registered");
        }

        private static async Task<Tuple<ApplicationStatus, string>> CheckProxyApplicationStatus(
            string gatewayName,
            TimeSpan timeout,
            CancellationToken cancellationToken,
            GatewayTraceConfig gatewayTraceConfig)
        {
            Stopwatch watch = new Stopwatch();
            watch.Start();

            var traceConfig = gatewayTraceConfig.ShallowCopy();
            traceConfig.Task = "CheckProxyApplicationStatus";

            var applicationUri = EnvoyAppNameForGateway(gatewayName);
            try
            {
                var applications = await ExecuteFabricActionWithRetryAsync(
                    () => client.QueryManager.GetApplicationListAsync(applicationUri, timeout - watch.Elapsed, cancellationToken),
                    traceConfig);
                for (int i = 0; i < applications.Count; i++)
                {
                    if (applications[i].ApplicationName.AbsoluteUri == applicationUri.AbsoluteUri)
                    {
                        if (applications[i].ApplicationStatus == ApplicationStatus.Failed)
                        {
                            traceConfig.Message = new
                            {
                                Status = applications[i].ApplicationStatus.ToString(),
                                applications[i].HealthState,
                            };
                            TraceMessage(System.Diagnostics.TraceLevel.Error, traceConfig);

                            return new Tuple<ApplicationStatus, string>(applications[i].ApplicationStatus, "Failed to deploy Gateway Proxy");
                        }

                        if (applications[i].ApplicationStatus != ApplicationStatus.Ready)
                        {
                            traceConfig.Message = new
                            {
                                Status = applications[i].ApplicationStatus.ToString(),
                                applications[i].HealthState,
                            };
                            TraceMessage(System.Diagnostics.TraceLevel.Info, traceConfig);

                            return new Tuple<ApplicationStatus, string>(applications[i].ApplicationStatus, null);
                        }

                        try
                        {
                            //// Check if at least one of the replicas is in Ready state
                            ServicePartitionList partitions = await ExecuteFabricActionWithRetryAsync(
                                () => client.QueryManager.GetPartitionListAsync(new Uri(applicationUri + GatewayProxyServiceName), null, timeout - watch.Elapsed, cancellationToken),
                                traceConfig);

                            foreach (var partition in partitions)
                            {
                                var replicas = await ExecuteFabricActionWithRetryAsync(
                                    () => client.QueryManager.GetReplicaListAsync(partition.PartitionId(), 0, ServiceReplicaStatusFilter.Ready, timeout - watch.Elapsed, cancellationToken),
                                    traceConfig);
                                if (replicas.Count != 0)
                                {
                                    foreach (var replica in replicas)
                                    {
                                        traceConfig.Message = new
                                        {
                                            ReplicaID = replica.Id,
                                            replica.ReplicaStatus,
                                            replica.NodeName,
                                            replica.ReplicaAddress,
                                            replica.HealthState
                                        };
                                        TraceMessage(System.Diagnostics.TraceLevel.Info, traceConfig);

                                        var replicaEndpoints = JObject.Parse(replica.ReplicaAddress);
                                        var replicaAddresses = replicaEndpoints["Endpoints"].Value<JObject>();
                                        foreach (var address in replicaAddresses)
                                        {
                                            var parts = ((string)address.Value).Split(':');

                                            Socket s = new Socket(
                                                AddressFamily.InterNetwork,
                                                SocketType.Stream,
                                                ProtocolType.Tcp);
                                            try
                                            {
                                                IAsyncResult result = s.BeginConnect(parts[0], int.Parse(parts[1]), null, null);
                                                result.AsyncWaitHandle.WaitOne(timeout - watch.Elapsed, true);

                                                if (s.Connected)
                                                {
                                                    traceConfig.Message = new
                                                    {
                                                        address,
                                                        connectionStatus = "Connected"
                                                    };
                                                    TraceMessage(System.Diagnostics.TraceLevel.Info, traceConfig);

                                                    s.Close();
                                                    s = null;

                                                    traceConfig.Message = new
                                                    {
                                                        Status = ApplicationStatus.Ready.ToString(),
                                                        applications[i].HealthState,
                                                    };
                                                    TraceMessage(System.Diagnostics.TraceLevel.Info, traceConfig);
                                                    return new Tuple<ApplicationStatus, string>(ApplicationStatus.Ready, null);
                                                }
                                            }
                                            catch (Exception)
                                            {
                                            }

                                            s.Close();
                                            s = null;
                                        }
                                    }
                                }
                            }
                        }
                        catch (Exception)
                        {
                            //// ok to continue. Application is in Ready state. It will take a while for service
                            //// and replicas to start.
                        }

                        lock (LockObject)
                        {
                            DateTime lastProxyStartTime = DateTime.MinValue;
                            var hasKey = lastProxyStartTimes.TryGetValue(gatewayName, out lastProxyStartTime);

                            if (!hasKey)
                            {
                                traceConfig.Message = new
                                {
                                    Status = ApplicationStatus.Invalid.ToString(),
                                    StatusDetail = string.Format("Proxy Endpoint(s) not available after {0} minutes", grmSettings.ProxyCreateToReadyTimeoutInMinutes.TotalMinutes)
                                };

                                return new Tuple<ApplicationStatus, string>(ApplicationStatus.Invalid, null);
                            }

                            //// Application is ready but there is not Endpoint available for ProxyCreateToReadyTimeoutInMinutes
                            //// Call it failed!
                            if (DateTime.Now - lastProxyStartTime > grmSettings.ProxyCreateToReadyTimeoutInMinutes)
                            {
                                lastProxyStartTime = DateTime.MinValue;
                                traceConfig.Message = new
                                {
                                    Status = ApplicationStatus.Failed.ToString(),
                                    StatusDetail = string.Format("Proxy Endpoint(s) not available after {0} minutes", grmSettings.ProxyCreateToReadyTimeoutInMinutes.TotalMinutes)
                                };
                                TraceMessage(System.Diagnostics.TraceLevel.Info, traceConfig);

                                return new Tuple<ApplicationStatus, string>(ApplicationStatus.Failed, null);
                            }
                        }

                        traceConfig.Message = new
                        {
                            Status = ApplicationStatus.Creating.ToString(),
                            applications[i].HealthState,
                        };
                        TraceMessage(System.Diagnostics.TraceLevel.Info, traceConfig);

                        return new Tuple<ApplicationStatus, string>(ApplicationStatus.Creating, null);
                    }
                }

                //// Did not find the application. Check App type Registration status.
                traceConfig.Message = "Proxy Application not found. Checking ApplicationType registration";
                TraceMessage(System.Diagnostics.TraceLevel.Info, traceConfig);

                var registrtionStatus = await CheckProxyApplicationRegistrationStatus(traceConfig);
                if (registrtionStatus.Item1 == ApplicationTypeStatus.Failed)
                {
                    return new Tuple<ApplicationStatus, string>(ApplicationStatus.Failed, registrtionStatus.Item2);
                }

                if (registrtionStatus.Item1 == ApplicationTypeStatus.Provisioning)
                {
                    return new Tuple<ApplicationStatus, string>(ApplicationStatus.Creating, null);
                }

                if (registrtionStatus.Item1 == ApplicationTypeStatus.Unprovisioning)
                {
                    return new Tuple<ApplicationStatus, string>(ApplicationStatus.Deleting, null);
                }

                return new Tuple<ApplicationStatus, string>(ApplicationStatus.Invalid, null);
            }
            catch (Exception e)
            {
                TraceExceptionMessage(System.Diagnostics.TraceLevel.Error, traceConfig, e);
                return new Tuple<ApplicationStatus, string>(ApplicationStatus.Invalid, null);
            }
        }

        private static string PrepareApplicationPackageFolder(ServiceContext serviceContext)
        {
            var dataPkg = serviceContext.CodePackageActivationContext.GetDataPackageObject("Data").Path;
            var dataRoot = Path.Combine(dataPkg, GetApplicationPackageFolder());

#if !DotNetCoreClrLinux
            // On Windows, FabricServiceResolver will run on the host, so extract it's CodePackage.
            // On Linux, FabricServiceResolver is currently in the proxy container itself.
            var nupkg = Path.Combine(dataPkg, "Microsoft.ServiceFabric.FabricServiceResolver.nupkg");
            var codePkgDestDir = Path.Combine(dataRoot, "Resolver");
            System.IO.Compression.ZipFile.ExtractToDirectory(nupkg, codePkgDestDir);
#endif

            return dataRoot;
        }

        private static async Task<bool> RegisterProxyApplicationType(ServiceContext serviceContext, GatewayTraceConfig gatewayTraceConfig)
        {
            var traceConfig = gatewayTraceConfig.ShallowCopy();
            traceConfig.Task = "RegisterProxyApplicationType";

            var registrationStatus = await CheckProxyApplicationRegistrationStatus(traceConfig);
            if (registrationStatus.Item1 == Query.ApplicationTypeStatus.Available)
            {
                return true;
            }

            if (registrationStatus.Item1 == Query.ApplicationTypeStatus.Failed)
            {
                return false;
            }

            traceConfig.Message = "Registering Application Type for Proxy";
            TraceMessage(System.Diagnostics.TraceLevel.Info, traceConfig);

            try
            {
                string imagestoreconnection = grmSettings.ImageStoreConnectionString;
                var dataRoot = PrepareApplicationPackageFolder(serviceContext);

                traceConfig.Message = new
                {
                    DataRoot = dataRoot
                };
                TraceMessage(System.Diagnostics.TraceLevel.Info, traceConfig);

                client.ApplicationManager.CopyApplicationPackage(
                    imagestoreconnection,
                    dataRoot,
                    "GatewayProxyApplicationPackage");

                await ExecuteFabricActionWithRetryAsync(
                    () => client.ApplicationManager.ProvisionApplicationAsync("GatewayProxyApplicationPackage"),
                    traceConfig);

                var retryCount = DefaultRetryCount;
                registrationStatus = new Tuple<ApplicationTypeStatus, string>(ApplicationTypeStatus.Invalid, null);
                while (retryCount-- != 0 && registrationStatus.Item1 != Query.ApplicationTypeStatus.Available)
                {
                    registrationStatus = await CheckProxyApplicationRegistrationStatus(traceConfig);
                    if (registrationStatus.Item1 == Query.ApplicationTypeStatus.Provisioning)
                    {
                        traceConfig.Message = new
                        {
                            Status = registrationStatus.Item1.ToString(),
                            StatusDetail = "Proxy Application Type not yet provisioned",
                            RetryRemainingCount = retryCount
                        };
                        TraceMessage(System.Diagnostics.TraceLevel.Info, traceConfig);

                        await Task.Delay(TimeSpan.FromSeconds(AddRandomSaltToDelayInSeconds(DefaultDelayBeforeRetryInSeconds)));
                    }
                    else if (registrationStatus.Item1 != Query.ApplicationTypeStatus.Available)
                    {
                        break;
                    }
                    else
                    {
                        traceConfig.Message = "Application Type for Proxy sucessfully registered";
                        TraceMessage(System.Diagnostics.TraceLevel.Info, traceConfig);
                        return true;
                    }
                }

                if (retryCount == 0 && registrationStatus.Item1 != Query.ApplicationTypeStatus.Available)
                {
                    traceConfig.Message = new
                    {
                        Status = registrationStatus.Item1.ToString(),
                        StatusDetail = "Registration of Application Type for Proxy failed",
                        RetryRemainingCount = retryCount
                    };
                    TraceMessage(System.Diagnostics.TraceLevel.Error, traceConfig);
                }
            }
            catch (FabricElementAlreadyExistsException)
            {
                traceConfig.Message = "Application Type for Proxy sucessfully registered";
                TraceMessage(System.Diagnostics.TraceLevel.Info, traceConfig);
                return true;
            }
            catch (Exception e)
            {
                traceConfig.Message = "Registration of Application Type for Proxy failed";
                TraceExceptionMessage(System.Diagnostics.TraceLevel.Error, traceConfig, e);
            }

            return false;
        }

        private static void TraceExceptionMessage(System.Diagnostics.TraceLevel traceLevel, GatewayTraceConfig traceConfig, Exception e)
        {
            StringBuilder traceTypeBuilder = new StringBuilder(traceConfig.Operation);
            if (!string.IsNullOrWhiteSpace(traceConfig.Task))
            {
                traceTypeBuilder.Append(".");
                traceTypeBuilder.Append(traceConfig.Task);
            }

            if (!string.IsNullOrWhiteSpace(traceConfig.Api))
            {
                traceTypeBuilder.Append(".");
                traceTypeBuilder.Append(traceConfig.Api);
            }

            switch (traceLevel)
            {
                default:
                    GatewayResourceManagerTrace.TraceSource.WriteExceptionAsInfo(
                        traceTypeBuilder.ToString(),
                        e,
                        JsonConvert.SerializeObject(traceConfig, Formatting.Indented));
                    break;

                case System.Diagnostics.TraceLevel.Error:
                    GatewayResourceManagerTrace.TraceSource.WriteExceptionAsError(
                        traceTypeBuilder.ToString(),
                        e,
                        JsonConvert.SerializeObject(traceConfig, Formatting.Indented));
                    break;

                case System.Diagnostics.TraceLevel.Warning:
                    GatewayResourceManagerTrace.TraceSource.WriteExceptionAsWarning(
                        traceTypeBuilder.ToString(),
                        e,
                        JsonConvert.SerializeObject(traceConfig, Formatting.Indented));
                    break;
            }
        }

        private static void TraceMessage(System.Diagnostics.TraceLevel traceLevel, GatewayTraceConfig traceConfig)
        {
            StringBuilder traceTypeBuilder = new StringBuilder(traceConfig.Operation);
            if (!string.IsNullOrWhiteSpace(traceConfig.Task))
            {
                traceTypeBuilder.Append(".");
                traceTypeBuilder.Append(traceConfig.Task);
            }

            if (!string.IsNullOrWhiteSpace(traceConfig.Api))
            {
                traceTypeBuilder.Append(".");
                traceTypeBuilder.Append(traceConfig.Api);
            }

            switch (traceLevel)
            {
                default:
                    GatewayResourceManagerTrace.TraceSource.WriteInfo(
                        traceTypeBuilder.ToString(),
                        JsonConvert.SerializeObject(traceConfig, Formatting.Indented));
                    break;

                case System.Diagnostics.TraceLevel.Error:
                    GatewayResourceManagerTrace.TraceSource.WriteError(
                        traceTypeBuilder.ToString(),
                        JsonConvert.SerializeObject(traceConfig, Formatting.Indented));
                    break;

                case System.Diagnostics.TraceLevel.Warning:
                    GatewayResourceManagerTrace.TraceSource.WriteWarning(
                        traceTypeBuilder.ToString(),
                        JsonConvert.SerializeObject(traceConfig, Formatting.Indented));
                    break;
            }
        }

        // Name to use for Gateway Application.
        private Uri ProxyApplicationNameForGateway()
        {
            return EnvoyAppNameForGateway(this.config.GatewayName);
        }

        private string GetDestinationNetwork()
        {
            return this.config.ResourceDescription.Properties.DestinationNetwork.Name;
        }

        private string GetSourceNetwork()
        {
            return this.config.ResourceDescription.Properties.SourceNetwork.Name;
        }

        private NameValueCollection GetAppParametersForOpen()
        {
            var parameters = new NameValueCollection();

            parameters.Add("Gateway_Listen_Network", "Open");
            parameters.Add("externalNetwork", "Open");

            int port = 80;
            if (this.config.ResourceDescription.Properties.Tcp != null &&
                this.config.ResourceDescription.Properties.Tcp.Count != 0)
            {
                port = this.config.ResourceDescription.Properties.Tcp[0].Port;
            }
            else if (this.config.ResourceDescription.Properties.Http != null &&
                this.config.ResourceDescription.Properties.Http.Count != 0)
            {
                port = this.config.ResourceDescription.Properties.Http[0].Port;
            }

            parameters.Add("Gateway_Proxy_Endpoint_Port", port.ToString());
            parameters.Add("Gateway_Proxy_Instance_Count", grmSettings.ProxyReplicaCount);

            return parameters;
        }

        private NameValueCollection GetAppParametersAndAdjustPortsForNAT()
        {
            var parameters = new NameValueCollection();

            parameters.Add("Gateway_Listen_Network", "Other");
            parameters.Add("externalNetwork", "Other");

            List<int> ports = new List<int>();
            int index = 0;
            if (this.config.ResourceDescription.Properties.Tcp != null &&
                this.config.ResourceDescription.Properties.Tcp.Count != 0)
            {
                foreach (var tcpConfig in this.config.ResourceDescription.Properties.Tcp)
                {
                    parameters.Add("HostPort" + index.ToString(), tcpConfig.Port.ToString());
                    ports.Add(tcpConfig.Port);
                    tcpConfig.Port = BaseNATPort + index;
                    index++;
                }
            }

            if (this.config.ResourceDescription.Properties.Http != null &&
                this.config.ResourceDescription.Properties.Http.Count != 0)
            {
                foreach (var httpConfig in this.config.ResourceDescription.Properties.Http)
                {
                    parameters.Add("HostPort" + index.ToString(), httpConfig.Port.ToString());
                    ports.Add(httpConfig.Port);
                    httpConfig.Port = BaseNATPort + index;
                    index++;
                }
            }

            parameters.Add("Gateway_Config", JsonConvert.SerializeObject(this.config.ResourceDescription.Properties));

            index = 0;
            if (this.config.ResourceDescription.Properties.Tcp != null &&
                this.config.ResourceDescription.Properties.Tcp.Count != 0)
            {
                foreach (var tcpConfig in this.config.ResourceDescription.Properties.Tcp)
                {
                    tcpConfig.Port = ports[index];
                    index++;
                }
            }

            if (this.config.ResourceDescription.Properties.Http != null &&
                this.config.ResourceDescription.Properties.Http.Count != 0)
            {
                foreach (var httpConfig in this.config.ResourceDescription.Properties.Http)
                {
                    httpConfig.Port = ports[index];
                    index++;
                }
            }

            parameters.Add("Gateway_Proxy_Instance_Count", grmSettings.ProxyReplicaCount);

            return parameters;
        }

        private NameValueCollection GetAppParametersForIsolated()
        {
            var parameters = new NameValueCollection();
            parameters.Add("internalNetwork", this.GetDestinationNetwork());
            return parameters;
        }

        private NameValueCollection GetAppParameters()
        {
            var parameters = new NameValueCollection();

            // Common parameters
#if DotNetCoreClrLinux
            parameters.Add("Gateway_Proxy_Container_Name", grmSettings.LinuxProxyImageName);
#else
            parameters.Add("Gateway_Proxy_Container_Name", grmSettings.WindowsProxyImageName);
#endif
            parameters.Add("Gateway_Proxy_CPUCores", grmSettings.ProxyCPUCores);
            parameters.Add("SF_ClientCertIssuerThumbprints", grmSettings.ClientCertThumbprints);
            parameters.Add("SF_ClusterCertIssuerThumbprints", grmSettings.ClusterCertThumbprints);

#if DotNetCoreClrLinux
            string certPath = "/var/lib/sfcerts";
            if (!Directory.Exists(certPath))
            {
                Directory.CreateDirectory(certPath);
            }

            parameters.Add("SF_CertsFolder", certPath);
            if (grmSettings.ContainerNetworkSetup)
            {
                parameters.Add("Gateway_Proxy_Isolation", "process");
            }
#endif

            if (grmSettings.IPProviderEnabled || grmSettings.LocalNatIPProviderEnabled)
            {
                parameters.Add("Gateway_Config", JsonConvert.SerializeObject(this.config.ResourceDescription.Properties));
                parameters.Add(this.GetAppParametersForOpen());
            }
            else
            {
                parameters.Add(this.GetAppParametersAndAdjustPortsForNAT());
            }

            if (grmSettings.IsolatedNetworkSetup && this.GetDestinationNetwork() != this.GetSourceNetwork() && !grmSettings.LocalNatIPProviderEnabled)
            {
                parameters.Add(this.GetAppParametersForIsolated());
            }

            return parameters;
        }

        private async Task<Tuple<ApplicationStatus, string>> CheckProxyApplicationStatus(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return await CheckProxyApplicationStatus(this.config.GatewayName, timeout, cancellationToken, this.gatewayTraceConfig);
        }

        private async Task<bool> StartProxyApplicationForGateway()
        {
            var traceConfig = this.gatewayTraceConfig.ShallowCopy();
            traceConfig.Task = "StartProxyApplicationForGateway";

            var applicationTypeRegistered = await RegisterProxyApplicationType(this.serviceContext, traceConfig);
            if (!applicationTypeRegistered)
            {
                traceConfig.Message = new
                {
                    StatusDetails = "Could not start proxy application",
                    AnalysisInformation = new
                    {
                        Information = "Search for traces with following information to identify cause",
                        type = string.Format(
                            "{0}.{1}.RegisterProxyApplicationType || {0}.{1}.CheckProxyApplicationRegistrationStatus",
                            FabricGatewayResourceManager.GatewayResourceManagerTraceType,
                            traceConfig.Operation),
                        traceConfig.CorrelationId
                    }
                };
                TraceMessage(System.Diagnostics.TraceLevel.Error, traceConfig);

                return false;
            }

            //// Start app
            traceConfig.Message = "Starting Proxy Application";
            TraceMessage(System.Diagnostics.TraceLevel.Info, traceConfig);
            try
            {
                System.Fabric.Description.ApplicationDescription description =
                   new System.Fabric.Description.ApplicationDescription(
                       this.ProxyApplicationNameForGateway(),
                        ProxyAppTypeForGateway,
                        "1.0",
                        this.GetAppParameters());
                await ExecuteFabricActionWithRetryAsync(
                    () => client.ApplicationManager.CreateApplicationAsync(description),
                    traceConfig);

                lock (LockObject)
                {
                    var lastProxyStartTime = DateTime.Now;
                    lastProxyStartTimes[this.config.GatewayName] = lastProxyStartTime;
                }
            }
            catch (FabricElementAlreadyExistsException)
            {
            }
            catch (Exception e)
            {
                traceConfig.Message = "Proxy Application Failed";
                TraceExceptionMessage(System.Diagnostics.TraceLevel.Error, traceConfig, e);
                return false;
            }

            traceConfig.Message = "Started Proxy Application";
            TraceMessage(System.Diagnostics.TraceLevel.Info, traceConfig);
            return true;
        }

        private async Task<bool> StopProxyApplicationForGateway()
        {
            var traceConfig = this.gatewayTraceConfig.ShallowCopy();
            traceConfig.Task = "StopProxyApplicationForGateway";
            traceConfig.Message = "Stopping Proxy Application";
            TraceMessage(System.Diagnostics.TraceLevel.Info, traceConfig);

            System.Fabric.Description.DeleteApplicationDescription description =
               new System.Fabric.Description.DeleteApplicationDescription(
                   this.ProxyApplicationNameForGateway());
            description.ForceDelete = true;

            try
            {
                await ExecuteFabricActionWithRetryAsync(
                    () => client.ApplicationManager.DeleteApplicationAsync(description),
                    traceConfig);
            }
            catch (FabricElementNotFoundException)
            {
            }
            catch (Exception e)
            {
                TraceExceptionMessage(System.Diagnostics.TraceLevel.Error, traceConfig, e);
                return false;
            }

            lock (LockObject)
            {
                lastProxyStartTimes.Remove(this.config.GatewayName);
            }

            var retryCount = DefaultRetryCount;
            var proxyApplicationStatus = await this.CheckProxyApplicationStatus(DefaultOperationTimeout, CancellationToken.None);
            while (retryCount-- != 0 && proxyApplicationStatus.Item1 != ApplicationStatus.Invalid)
            {
                traceConfig.Message = new
                {
                    Status = proxyApplicationStatus.Item1.ToString(),
                    StatusDetail = "Proxy Application not yet stopped",
                    RetryRemainingCount = retryCount
                };
                TraceMessage(System.Diagnostics.TraceLevel.Info, traceConfig);

                await Task.Delay(TimeSpan.FromSeconds(AddRandomSaltToDelayInSeconds(DefaultDelayBeforeRetryInSeconds)));
                proxyApplicationStatus = await this.CheckProxyApplicationStatus(DefaultOperationTimeout, CancellationToken.None);
            }

            if (retryCount == 0 && proxyApplicationStatus.Item1 != ApplicationStatus.Invalid)
            {
                traceConfig.Message = new
                {
                    Status = proxyApplicationStatus.Item1.ToString(),
                    StatusDetail = "Proxy Application not yet stopped",
                    RetryRemainingCount = 0
                };
                TraceMessage(System.Diagnostics.TraceLevel.Error, traceConfig);
                return false;
            }

            traceConfig.Message = "Stopped Proxy Application";
            TraceMessage(System.Diagnostics.TraceLevel.Info, traceConfig);
            return true;
        }

        private async void UpgradeProxyApplication()
        {
            var traceConfig = this.gatewayTraceConfig.ShallowCopy();
            traceConfig.Task = "UpgradeProxyApplication";
            traceConfig.Message = string.Format(
                "Upgrading Proxy Application. See messages of type {0}.{1}.UpgradeProxyApplication || {0}.{1}.StartProxyApplicationForGateway || {0}.{1}.StopProxyApplicationForGateway",
                FabricGatewayResourceManager.GatewayResourceManagerTraceType,
                traceConfig.Operation);
            TraceMessage(System.Diagnostics.TraceLevel.Info, traceConfig);

            if (!await this.StopProxyApplicationForGateway())
            {
                traceConfig.Message = "Unable to stop proxy application. Gateway in invalid state";
                TraceMessage(System.Diagnostics.TraceLevel.Error, traceConfig);
                return;
            }

            await this.StartProxyApplicationForGateway();
        }

        private async void StopProxyAndRemoveGateway()
        {
            var traceConfig = this.gatewayTraceConfig.ShallowCopy();
            traceConfig.Task = "StopProxyAndRemoveGateway";

            if (!await this.StopProxyApplicationForGateway())
            {
                traceConfig.Message = "Unable to stop proxy application. Not removing gateway resource config";
                TraceMessage(System.Diagnostics.TraceLevel.Error, traceConfig);
                return;
            }

            try
            {
                var configDictionary = await ExecuteFabricActionWithRetryAsync(
                    () => this.stateManager.GetOrAddAsync<IReliableDictionary<string, GatewayConfig>>(CollectionName),
                    traceConfig);

                var retryCount = DefaultRetryCount;
                while (retryCount-- != 0)
                {
                    using (ITransaction tx = this.stateManager.CreateTransaction())
                    {
                        try
                        {
                            traceConfig.Message = "Removing Config from Collection";
                            TraceMessage(System.Diagnostics.TraceLevel.Info, traceConfig);

                            var config = await ExecuteFabricActionWithRetryAsync(
                                () => configDictionary.TryRemoveAsync(tx, this.config.GatewayName),
                                traceConfig);

                            traceConfig.Message = "Removed Config from Collection";
                            if (config.HasValue)
                            {
                                traceConfig.ResourceDescription = config.Value.ResourceDescription;
                            }

                            TraceMessage(System.Diagnostics.TraceLevel.Info, traceConfig);

                            break;
                        }
                        catch (InvalidOperationException e)
                        {
                            TraceExceptionMessage(System.Diagnostics.TraceLevel.Warning, traceConfig, e);
                        }
                        finally
                        {
                            await tx.CommitAsync();
                        }
                    }

                    await Task.Delay(TimeSpan.FromSeconds(AddRandomSaltToDelayInSeconds(DefaultDelayBeforeRetryInSeconds)));
                }

                if (retryCount == 0)
                {
                    traceConfig.Message = "Unable to remove resource config";
                    TraceMessage(System.Diagnostics.TraceLevel.Error, traceConfig);
                }
            }
            catch (Exception e)
            {
                TraceExceptionMessage(System.Diagnostics.TraceLevel.Error, traceConfig, e);
            }
        }

        public class GatewayConfig
        {
            public GatewayConfig()
            {
                this.ResourceDescription = null;
            }

            public string GatewayName { get; set; }

            public GatewayResourceDescription ResourceDescription { get; set; }
        }

        public class GatewayTraceConfig
        {
            public GatewayTraceConfig(string gatewayName)
            {
                this.CorrelationId = Guid.NewGuid().ToString();
                this.GatewayName = gatewayName;
            }

            [JsonProperty]
            public string CorrelationId { get; }

            [JsonProperty]
            public string GatewayName { get; }

            [JsonProperty]
            public object Message { get; set; }

            [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
            public GatewayResourceDescription ResourceDescription { get; set; }

            [JsonIgnore]
            public string Operation { get; set; }

            [JsonIgnore]
            public string Task { get; set; }

            [JsonIgnore]
            public string Api { get; set; }

            public GatewayTraceConfig ShallowCopy()
            {
                return (GatewayTraceConfig)this.MemberwiseClone();
            }
        }
    }
}
