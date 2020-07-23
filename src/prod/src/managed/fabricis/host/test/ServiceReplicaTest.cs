// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Collections.Generic;
using System.Diagnostics;
using System.Fabric.Description;
using System.Fabric.Health;
using System.Fabric.Query;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace System.Fabric.InfrastructureService.Test
{
    [TestClass]
    public class ServiceReplicaTest
    {
        private class NullInfrastructureAgent : IInfrastructureAgentWrapper
        {
            public Task FinishInfrastructureTaskAsync(string taskId, long instanceId, TimeSpan timeout, CancellationToken cancellationToken)
            {
                throw new NotImplementedException();
            }

            public Task<InfrastructureTaskQueryResult> QueryInfrastructureTaskAsync(TimeSpan timeout, CancellationToken cancellationToken)
            {
                throw new NotImplementedException();
            }

            public string RegisterInfrastructureService(Guid partitionId, long replicaId, IInfrastructureService service)
            {
                return "Test";
            }

            public void RegisterInfrastructureServiceFactory(IStatefulServiceFactory factory)
            {
                throw new NotImplementedException();
            }

            public Task StartInfrastructureTaskAsync(InfrastructureTaskDescription description, TimeSpan timeout, CancellationToken cancellationToken)
            {
                throw new NotImplementedException();
            }

            public void UnregisterInfrastructureService(Guid partitionId, long replicaId)
            {
            }
        }

        private class RunAsyncTestCoordinator : IInfrastructureCoordinator
        {
            public Func<int, CancellationToken, Task> RunAsyncHandler { get; set; }

            public Task RunAsync(int primaryEpoch, CancellationToken token)
            {
                return this.RunAsyncHandler(primaryEpoch, token);
            }
            public Task ReportFinishTaskSuccessAsync(string taskId, long instanceId, TimeSpan timeout, CancellationToken cancellationToken)
            {
                throw new NotImplementedException();
            }

            public Task ReportStartTaskSuccessAsync(string taskId, long instanceId, TimeSpan timeout, CancellationToken cancellationToken)
            {
                throw new NotImplementedException();
            }

            public Task ReportTaskFailureAsync(string taskId, long instanceId, TimeSpan timeout, CancellationToken cancellationToken)
            {
                throw new NotImplementedException();
            }

            public Task<string> RunCommandAsync(bool isAdminCommand, string command, TimeSpan timeout, CancellationToken cancellationToken)
            {
                throw new NotImplementedException();
            }
        }

        private class MockStatefulServicePartition : IStatefulServicePartition
        {
            public ServicePartitionInformation PartitionInfo
            {
                get
                {
                    throw new NotImplementedException();
                }
            }

            public PartitionAccessStatus ReadStatus
            {
                get
                {
                    throw new NotImplementedException();
                }
            }

            public PartitionAccessStatus WriteStatus
            {
                get
                {
                    throw new NotImplementedException();
                }
            }

            public FabricReplicator CreateReplicator(IStateProvider stateProvider, ReplicatorSettings replicatorSettings)
            {
                return null;
            }

            public void ReportFault(FaultType faultType)
            {
                Trace.WriteInfo(new TraceType("Test"), "ReportFault({0})", faultType);
            }

            public void ReportLoad(IEnumerable<LoadMetric> metrics)
            {
                throw new NotImplementedException();
            }

            public void ReportMoveCost(MoveCost moveCost)
            {
                throw new NotImplementedException();
            }

            public void ReportPartitionHealth(HealthInformation healthInfo)
            {
                throw new NotImplementedException();
            }

            public void ReportPartitionHealth(HealthInformation healthInfo, HealthReportSendOptions sendOptions)
            {
                throw new NotImplementedException();
            }

            public void ReportReplicaHealth(HealthInformation healthInfo)
            {
                throw new NotImplementedException();
            }

            public void ReportReplicaHealth(HealthInformation healthInfo, HealthReportSendOptions sendOptions)
            {
                throw new NotImplementedException();
            }
        }

        [TestMethod]
        public async Task ServiceReplicaLifecycleTest()
        {
            var configStore = new MockConfigStore();
            configStore.AddKeyValue("InfrastructureService", "CrashOnRunAsyncUnexpectedCompletion", "false");

            var coordinator = new RunAsyncTestCoordinator();

            var replica = new ServiceReplica(
                new NullInfrastructureAgent(),
                coordinator,
                null,
                false,
                new ConfigSection(new TraceType("ServiceReplicaTest"), configStore, "InfrastructureService"));

            replica.Initialize(new StatefulServiceInitializationParameters()
            {
                PartitionId = Guid.NewGuid(),
                ReplicaId = Stopwatch.GetTimestamp(),
            });

            await replica.OpenAsync(ReplicaOpenMode.New, new MockStatefulServicePartition(), CancellationToken.None);

            // Good coordinator: runs until cancelled
            coordinator.RunAsyncHandler = async (epoch, token) => { await Task.Delay(Timeout.InfiniteTimeSpan, token); };

            await replica.ChangeRoleAsync(ReplicaRole.Primary, CancellationToken.None);
            await Task.Delay(500);
            await replica.ChangeRoleAsync(ReplicaRole.ActiveSecondary, CancellationToken.None);

            await replica.ChangeRoleAsync(ReplicaRole.Primary, CancellationToken.None);
            await Task.Delay(500);
            await replica.CloseAsync(CancellationToken.None);

            // Bad coordinator: returns immediately
            coordinator.RunAsyncHandler = (epoch, token) => Task.FromResult(0);

            await replica.ChangeRoleAsync(ReplicaRole.Primary, CancellationToken.None);
            await Task.Delay(500);
            await replica.ChangeRoleAsync(ReplicaRole.ActiveSecondary, CancellationToken.None);

            await replica.ChangeRoleAsync(ReplicaRole.Primary, CancellationToken.None);
            await Task.Delay(500);
            await replica.CloseAsync(CancellationToken.None);

            // Bad coordinator: synchronous unhandled exception
            configStore.AddKeyValue("InfrastructureService", "CrashOnRunAsyncUnhandledException", "false");
            coordinator.RunAsyncHandler = null;

            await replica.ChangeRoleAsync(ReplicaRole.Primary, CancellationToken.None);
            await Task.Delay(500);
            await replica.ChangeRoleAsync(ReplicaRole.ActiveSecondary, CancellationToken.None);

            await replica.ChangeRoleAsync(ReplicaRole.Primary, CancellationToken.None);
            await Task.Delay(500);
            await replica.CloseAsync(CancellationToken.None);

            // Bad coordinator: delayed (async) unhandled exception
            coordinator.RunAsyncHandler = async (epoch, token) =>
            {
                await Task.Delay(100, token);
                throw new InvalidOperationException();
            };

            await replica.ChangeRoleAsync(ReplicaRole.Primary, CancellationToken.None);
            await Task.Delay(500);
            await replica.ChangeRoleAsync(ReplicaRole.ActiveSecondary, CancellationToken.None);

            await replica.ChangeRoleAsync(ReplicaRole.Primary, CancellationToken.None);
            await Task.Delay(500);
            await replica.CloseAsync(CancellationToken.None);

            // Old code - Bad coordinator: delayed (async) unhandled exception
            configStore.AddKeyValue("InfrastructureService", "WrapRunAsync", "false");

            await replica.ChangeRoleAsync(ReplicaRole.Primary, CancellationToken.None);
            await Task.Delay(500);
            await replica.ChangeRoleAsync(ReplicaRole.ActiveSecondary, CancellationToken.None);

            await replica.ChangeRoleAsync(ReplicaRole.Primary, CancellationToken.None);
            await Task.Delay(500);
            await replica.CloseAsync(CancellationToken.None);

            // Old code - Good coordinator: runs until cancelled
            coordinator.RunAsyncHandler = async (epoch, token) => { await Task.Delay(Timeout.InfiniteTimeSpan, token); };

            await replica.ChangeRoleAsync(ReplicaRole.Primary, CancellationToken.None);
            await Task.Delay(500);
            await replica.ChangeRoleAsync(ReplicaRole.ActiveSecondary, CancellationToken.None);

            await replica.ChangeRoleAsync(ReplicaRole.Primary, CancellationToken.None);
            await Task.Delay(500);
            await replica.CloseAsync(CancellationToken.None);
        }
    }
}