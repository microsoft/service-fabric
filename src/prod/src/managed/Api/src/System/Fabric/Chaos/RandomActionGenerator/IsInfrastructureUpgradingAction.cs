// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.RandomActionGenerator
{
    using System.Fabric.Chaos.Common;
    using System.Fabric.Common;
    using System.Fabric.Query;
    using System.IO;
    using System.Text;
    using System.Runtime.Serialization.Json;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Collections.Generic;

    internal sealed class IsInfrastructureUpgradingAction : FabricTestAction<bool>
    {
        private const string TraceType = "IsInfrastructureUpgradingAction";
        internal override Type ActionHandlerType
        {
            get { return typeof(IsInfrastructureUpgradingActionHandler); }
        }

        private sealed class IsInfrastructureUpgradingActionHandler : FabricTestActionHandler<IsInfrastructureUpgradingAction>
        {
            protected override async Task ExecuteActionAsync(FabricTestContext testContext, IsInfrastructureUpgradingAction action, CancellationToken cancellationToken)
            {
                TestabilityTrace.TraceSource.WriteInfo(TraceType, "Checking if InfrastructureUpgrade is going on.");

                var status = await this.GetInfrastructureServiceStatus(
                    testContext.FabricClient,
                    action.ActionTimeout,
                    action.RequestTimeout,
                    cancellationToken).ConfigureAwait(false);

                if (!status.Any())
                {
                    action.Result = false;
                }
                else
                {
                    foreach (var kvp in status)
                    {
                        var jobCollectionData = kvp.Value.Jobs;
                        if (jobCollectionData == null)
                        {
                            continue;
                        }

                        var jobDataArray = jobCollectionData.Jobs;

                        if (jobDataArray == null)
                        {
                            continue;
                        }

                        var activeJobs =
                            jobDataArray.Where(
                                j => ChaosConstants.UpgradingInfrastructureJobStatus.Contains(j.DetailedStatus));

                        if (activeJobs.Any())
                        {
                            action.Result = true;
                            return;
                        }
                    }

                    action.Result = false;
                }
            }

            private async Task<Service[]> GetInfrastructureServiceInstancesAsync(
                FabricClient fabricClient,
                TimeSpan operationTimeout,
                TimeSpan requestTimeout,
                CancellationToken cancellationToken)
            {
                var allSystemServices = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                    () => fabricClient.QueryManager.GetServiceListAsync(
                        new Uri(Constants.SystemApplicationName),
                        null,
                        requestTimeout,
                        cancellationToken),
                    operationTimeout,
                    cancellationToken).ConfigureAwait(false);

                var infraInstances = allSystemServices.Where(
                    i => i.ServiceTypeName.Equals(ChaosConstants.InfrastructureServiceType, StringComparison.InvariantCultureIgnoreCase)).ToArray();

                return infraInstances;
            }

            /// <summary>
            /// Fetches the current status of the InfraStructureService instances.
            /// </summary>
            /// <param name="fabricClient">The FabricClient to make API calls again the InfrastructureService</param>
            /// <param name="operationTimeout">Total time of all retries.</param>
            /// <param name="requestTimeout">Timeout for a single query.</param>
            /// <param name="ct">CancellationToken</param>
            /// <returns>A dictionary which provides status of all infrastructure service instances.</returns>
            public async Task<IDictionary<Uri, InfrastructureStatus>> GetInfrastructureServiceStatus(
                FabricClient fabricClient,
                TimeSpan operationTimeout,
                TimeSpan requestTimeout,
                CancellationToken ct)
            {
                var infraStructureTasks = new Dictionary<Uri, Tuple<Task<CoordinatorStateData>, Task<JobCollectionData>>>();
                foreach (Service service in await this.GetInfrastructureServiceInstancesAsync(fabricClient, operationTimeout, requestTimeout, ct).ConfigureAwait(false))
                {
                    Task<CoordinatorStateData> statusTask = FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                        () => fabricClient.InfrastructureManager.InvokeInfrastructureQueryAsync(
                            service.ServiceName,
                            ChaosConstants.InfrastructureServiceGetCurrentStateCommand,
                            requestTimeout, ct),
                        operationTimeout,
                        ct).ContinueWith(t =>
                                {
                                    if (t.IsFaulted)
                                    {
                                        TestabilityTrace.TraceSource.WriteError(TraceType, "Unable to get infrastructure service status for {0}: {1}", service.ServiceName, t.Exception);
                                        return null;
                                    }

                                    if (t.Result == null)
                                    {
                                        TestabilityTrace.TraceSource.WriteError(TraceType, "Infrastructure service status null for {0}.", service.ServiceName);
                                        return null;
                                    }

                                    using (MemoryStream stream = new MemoryStream(Encoding.UTF8.GetBytes(t.Result)))
                                    {
                                        var serializerSerial = new DataContractJsonSerializer(typeof(CoordinatorStateDataSerial));
                                        var stateSerial = (CoordinatorStateDataSerial)serializerSerial.ReadObject(stream);

                                        // comparing with !Parallel since existing clusters don't return any mode value.
                                        if (!string.Equals(stateSerial.Mode, InfrastructureServiceMode.Parallel, StringComparison.OrdinalIgnoreCase))
                                        {
                                            return (CoordinatorStateData)stateSerial;
                                        }

                                        stream.Position = 0;
                                        var serializerParallel = new DataContractJsonSerializer(typeof(CoordinatorStateDataParallel));
                                        var stateParallel = (CoordinatorStateDataParallel)serializerParallel.ReadObject(stream);

                                        return stateParallel;
                                }
                            },
                        ct);

                    Task<JobCollectionData> jobsTask = FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                        () => fabricClient.InfrastructureManager.InvokeInfrastructureQueryAsync(
                            service.ServiceName,
                            ChaosConstants.InfrastructureServiceGetJobsCommand,
                            requestTimeout,
                            ct),
                        operationTimeout,
                        ct).ContinueWith(t =>
                            {
                                if (t.IsFaulted)
                                {
                                    TestabilityTrace.TraceSource.WriteError(TraceType, "Unable to get infrastructure service jobs for {0}: {1}", service.ServiceName, t.Exception);
                                    return null;
                                }

                                if (t.Result == null)
                                {
                                    TestabilityTrace.TraceSource.WriteError(TraceType, "Infrastructure service jobs null for {0}.", service.ServiceName);
                                    return null;
                                }

                                using (MemoryStream stream = new MemoryStream(Encoding.UTF8.GetBytes(t.Result)))
                                {
                                    var serializer = new DataContractJsonSerializer(typeof(JobCollectionData));
                                    return (JobCollectionData)serializer.ReadObject(stream);
                                }
                            }, ct);

                    infraStructureTasks.Add(service.ServiceName, Tuple.Create(statusTask, jobsTask));
                }

                await Task.WhenAll(infraStructureTasks.Values.Select(v => v.Item1)).ConfigureAwait(false);
                await Task.WhenAll(infraStructureTasks.Values.Select(v => v.Item2)).ConfigureAwait(false);
                return infraStructureTasks.ToDictionary(kvp => kvp.Key, kvp => new InfrastructureStatus { CurrentState = kvp.Value.Item1.Result, Jobs = kvp.Value.Item2.Result });
            }
        }
    }
}