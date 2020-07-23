// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeService
{
    using Common;    
    using Linq;
    using System.Collections.Generic;
    using System.Fabric.WRP.Model;    
    using Threading;
    using Threading.Tasks;

    public class ResourceCoordinator : IUpgradeCoordinator
    {
        public const string TaskName = "SFRP poll";

        private static readonly TraceType TraceType = new TraceType("ResourceCoordinator");

        private readonly IConfigStore configStore;
        private readonly string configSectionName;

        private readonly IExceptionHandlingPolicy exceptionPolicy;

        private readonly IWrpPackageRetriever packageRetriever;

        private readonly IDictionary<ResourceType, IResourceCommandProcessor> commandProcessors;
                
        private readonly KeyValueStoreReplica kvsStore;        

        private Task pollingTask;        

        internal ResourceCoordinator(
            KeyValueStoreReplica kvsStore,
            IConfigStore configStore,
            string configSectionName,
            IDictionary<ResourceType, IResourceCommandProcessor> resourceCommandProcessors,
            IWrpPackageRetriever packageRetriever,
            IExceptionHandlingPolicy healthPolicy)
        {
            kvsStore.ThrowIfNull(nameof(kvsStore));
            configStore.ThrowIfNull(nameof(configStore));
            configSectionName.ThrowIfNullOrWhiteSpace(nameof(configSectionName));
            resourceCommandProcessors.ThrowIfNull(nameof(resourceCommandProcessors));
            packageRetriever.ThrowIfNull(nameof(packageRetriever));
            healthPolicy.ThrowIfNull(nameof(healthPolicy));

            this.kvsStore = kvsStore;
            this.configStore = configStore;
            this.configSectionName = configSectionName;
            this.commandProcessors = resourceCommandProcessors;
            this.packageRetriever = packageRetriever;
            this.exceptionPolicy = healthPolicy;
        }

        public string ListeningAddress
        {
            get { return string.Empty; }
        }

        public IEnumerable<Task> StartProcessing(CancellationToken token)
        {
            var tasks = new List<Task>();
            Trace.WriteInfo(TraceType, "StartProcessing called");

            ReleaseAssert.AssertIf((this.pollingTask != null && !this.pollingTask.IsCompleted),
                "StartProcessing should be called only once");

            TimeSpan pollingInterval =
                configStore.ReadTimeSpan(
                    this.configSectionName,
                    Constants.ConfigurationSection.PollIntervalInSeconds, 
                    TimeSpan.FromSeconds(60));

            this.pollingTask = this.RunPollingThreadAsync(new OperationContext(token, pollingInterval));
            tasks.Add(this.pollingTask);
            return tasks;
        }        

        public async Task RunPollingThreadAsync(IOperationContext context)
        {
            Trace.WriteInfo(TraceType, "PollingThread initialized");
                        
            var pollRequest = new UpgradeServicePollRequest();

            var result = await this.commandProcessors[ResourceType.Cluster].ProcessAsync(
                new List<IOperationDescription>() { null },
                new OperationContext(
                        context.CancellationToken,
                        context.OperationTimeout,
                        context.ContinuationToken));

            pollRequest.ClusterOperationStatus = result.Cast<ClusterOperationStatus>().FirstOrDefault();

            while (!context.CancellationToken.IsCancellationRequested)
            {
                var operationContext =
                    new OperationContext(
                        context.CancellationToken,
                        context.OperationTimeout,
                        context.ContinuationToken);
                try
                {
                    Trace.WriteInfo(TraceType, "Poll WRP - started");
                    var pollResponse = await this.packageRetriever.GetWrpResponseAsync(pollRequest, context.CancellationToken);
                    Trace.WriteInfo(TraceType, "Poll WRP - end");

                    Trace.WriteInfo(TraceType, "Handle poll response - started");
                    pollRequest = await this.ProcessWrpServicePollResponseAsync(
                        pollResponse,
                        new OperationContext(
                            operationContext.CancellationToken,
                            operationContext.GetRemainingTimeOrThrow(),
                            operationContext.ContinuationToken));
                    Trace.WriteInfo(TraceType, "Handle poll response - ended");

                    this.exceptionPolicy.ReportSuccess();                    
                }
                catch (Exception e)
                {
                    this.exceptionPolicy.ReportError(e, context.CancellationToken.IsCancellationRequested);
                }

                await Task.Delay(operationContext.GetRemainingTime(), operationContext.CancellationToken);
            }

            Trace.WriteInfo(TraceType, "PollingThread cancelled");
        }

        public async Task<UpgradeServicePollRequest> ProcessWrpServicePollResponseAsync(
            UpgradeServicePollResponse pollResponse,
            IOperationContext context)
        {
            Task<IList<IOperationStatus>> clusterOps = null;
            Task<IList<IOperationStatus>> appTypeOps = null;
            Task<IList<IOperationStatus>> appOps = null;
            Task<IList<IOperationStatus>> svcOps = null;
            
            clusterOps = this.commandProcessors[ResourceType.Cluster].ProcessAsync(
                new List<IOperationDescription>() { pollResponse.ClusterOperationDescription }, 
                context);

            appTypeOps = this.commandProcessors[ResourceType.ApplicationType].ProcessAsync(
                pollResponse.ApplicationTypeOperationDescriptions.ToList<IOperationDescription>(),
                context);

            appOps = this.commandProcessors[ResourceType.Application].ProcessAsync(
                pollResponse.ApplicationOperationDescriptions.ToList<IOperationDescription>(),
                context);

            svcOps = this.commandProcessors[ResourceType.Service].ProcessAsync(
                pollResponse.ServiceOperationDescriptions.ToList<IOperationDescription>(),
                context);

            await Task.WhenAll(clusterOps, appTypeOps, appOps, svcOps);

            return new UpgradeServicePollRequest()
            {
                ClusterOperationStatus = clusterOps?.Result.Cast<ClusterOperationStatus>().FirstOrDefault(),
                ApplicationTypeOperationStatuses = appTypeOps?.Result.Cast<ApplicationTypeVersionOperationStatus>().ToList(),
                ApplicationOperationStatuses = appOps?.Result.Cast<ApplicationOperationStatus>().ToList(),
                ServiceOperationStatuses = svcOps?.Result.Cast<ServiceOperationStatus>().ToList(),
            };
        }
    }
}
