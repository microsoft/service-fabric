// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Autopilot.Test
{
    using Collections.Generic;
    using System.Fabric.Common.Tracing;
    using System.Fabric.InfrastructureService.Test;
    using Repair;
    using Threading;
    using Threading.Tasks;

    internal class MockRepairManager : IRepairManager
    {
        public Task<long> CancelRepairTaskAsync(Guid activityId, IRepairTask repairTask)
        {
            throw new NotImplementedException();
        }

        public Task<long> CreateRepairTaskAsync(Guid activityId, IRepairTask repairTask)
        {
            throw new NotImplementedException();
        }

        public Task<IList<IRepairTask>> GetRepairTaskListAsync(Guid activityId, string taskIdFilter = null, RepairTaskStateFilter stateFilter = RepairTaskStateFilter.Default, string executorFilter = null)
        {
            throw new NotImplementedException();
        }

        public IRepairTask NewRepairTask(string taskId, string action)
        {
            throw new NotImplementedException();
        }

        public Task RemoveNodeStateAsync(Guid activityId, string nodeName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task<long> UpdateRepairExecutionStateAsync(Guid activityId, IRepairTask repairTask)
        {
            throw new NotImplementedException();
        }

        public Task<long> UpdateRepairTaskHealthPolicyAsync(Guid activityId, IRepairTask repairTask, bool? performPreparingHealthCheck, bool? performRestoringHealthCheck)
        {
            throw new NotImplementedException();
        }
    }

    public static class Program
    {
        public static int Main(string[] args)
        {
            //TraceConfig.InitializeFromConfigStore();

            var coordinator = new AutopilotInfrastructureCoordinator(
                new CoordinatorEnvironment(
                    "fabric:/MyService",
                    new ConfigSection(new TraceType(Constants.TraceTypeName), new MockConfigStore(), "MySectionName"),
                    string.Empty),
                new AutopilotDMClient(), // TODO mock
                new MockRepairManager(),
                new MockHealthClient(),
                Guid.NewGuid(),
                42);

            CancellationTokenSource cts = new CancellationTokenSource(TimeSpan.FromSeconds(5));

            Task runAsyncTask = coordinator.RunAsync(0, cts.Token);
            runAsyncTask.Wait();

            return 0;
        }
    }
}