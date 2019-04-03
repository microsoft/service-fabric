// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Fabric.InfrastructureService.Test;
using System.Fabric.Repair;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace System.Fabric.InfrastructureService.Parallel.Test
{
    [TestClass]
    public class ActionTest
    {
        private sealed class DelegatedRepairManager<T> : IRepairManager where T : IRepairTask, new()
        {
            public Func<T, Task<long>> CancelRepairTaskAsyncHandler { get; set; }
            public Func<T, Task<long>> CreateRepairTaskAsyncHandler { get; set; }
            public Func<string, RepairTaskStateFilter, string, Task<IList<IRepairTask>>> GetRepairTaskListAsyncHandler { get; set; }
            public Func<string, TimeSpan, CancellationToken, Task> RemoveNodeStateAsyncHandler { get; set; }
            public Func<T, Task<long>> UpdateRepairExecutionStateAsyncHandler { get; set; }
            public Func<T, bool?, bool?, Task<long>> UpdateRepairTaskHealthPolicyAsyncHandler { get; set; }

            public Task<long> CancelRepairTaskAsync(Guid activityId, IRepairTask repairTask)
            {
                return this.CancelRepairTaskAsyncHandler((T)repairTask);
            }

            public Task<long> CreateRepairTaskAsync(Guid activityId, IRepairTask repairTask)
            {
                return this.CreateRepairTaskAsyncHandler((T)repairTask);
            }

            public Task<IList<IRepairTask>> GetRepairTaskListAsync(Guid activityId, string taskIdFilter = null, RepairTaskStateFilter stateFilter = RepairTaskStateFilter.Default, string executorFilter = null)
            {
                return this.GetRepairTaskListAsyncHandler(taskIdFilter, stateFilter, executorFilter);
            }

            public IRepairTask NewRepairTask(string taskId, string action)
            {
                return new T();
            }

            public Task RemoveNodeStateAsync(Guid activityId, string nodeName, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return this.RemoveNodeStateAsyncHandler(nodeName, timeout, cancellationToken);
            }

            public Task<long> UpdateRepairExecutionStateAsync(Guid activityId, IRepairTask repairTask)
            {
                return this.UpdateRepairExecutionStateAsyncHandler((T)repairTask);
            }

            public Task<long> UpdateRepairTaskHealthPolicyAsync(Guid activityId, IRepairTask repairTask, bool? performPreparingHealthCheck, bool? performRestoringHealthCheck)
            {
                return this.UpdateRepairTaskHealthPolicyAsyncHandler((T)repairTask, performPreparingHealthCheck, performRestoringHealthCheck);
            }
        }

        private MockConfigStore configStore;
        private CoordinatorEnvironment env;
        private DelegatedRepairManager<MockRepairTask> rm;

        [TestInitialize]
        public void Initialize()
        {
            configStore = new MockConfigStore();
            configStore.AddKeyValue("InfrastructureService", "Azure.ProcessRemovedNodesRetryIntervalInSeconds", "1");

            env = new CoordinatorEnvironment(
                "fabric:/IS",
                new ConfigSection(Constants.TraceType, configStore, "InfrastructureService"),
                string.Empty,
                new MockInfrastructureAgentWrapper());

            rm = new DelegatedRepairManager<MockRepairTask>();
            rm.UpdateRepairExecutionStateAsyncHandler = t => Task.FromResult(0L);
            rm.RemoveNodeStateAsyncHandler = (n, timeout, token) =>
            {
                throw new InvalidOperationException();
            };
        }

        private IAction CreateMoveToRestoringAction(MockRepairTask task)
        {
            return new MoveToRestoringAction(
                env,
                rm,
                task,
                RepairTaskResult.Succeeded,
                null,
                surpriseJob: false,
                cancelRestoringHealthCheck: false,
                processRemovedNodes: true);
        }

        [TestMethod]
        public async Task NodeRemovalTest_NothingToRemove()
        {
            var impact = new NodeRepairImpactDescription();
            impact.ImpactedNodes.Add(new NodeImpact("Node.0", NodeImpactLevel.RemoveData));

            var task = new MockRepairTask("id", "action")
            {
                Impact = impact,
            };

            var action = CreateMoveToRestoringAction(task);

            await action.ExecuteAsync(Guid.Empty);
        }

        [TestMethod]
        public async Task NodeRemovalTest_UnexpectedException_OldTask()
        {
            var impact = new NodeRepairImpactDescription();
            impact.ImpactedNodes.Add(new NodeImpact("Node.0", NodeImpactLevel.RemoveNode));

            var task = new MockRepairTask("id", "action")
            {
                Impact = impact,
            };

            var action = CreateMoveToRestoringAction(task);

            task.ExecutingTimestamp = DateTime.UtcNow.AddMinutes(-60);
            await action.ExecuteAsync(Guid.Empty);
        }

        [TestMethod]
        public async Task NodeRemovalTest_UnexpectedException_RecentTask()
        {
            var impact = new NodeRepairImpactDescription();
            impact.ImpactedNodes.Add(new NodeImpact("Node.0", NodeImpactLevel.RemoveNode));

            var task = new MockRepairTask("id", "action")
            {
                Impact = impact,
            };

            var action = CreateMoveToRestoringAction(task);

            task.ExecutingTimestamp = DateTime.UtcNow.AddMinutes(-5);
            try
            {
                await action.ExecuteAsync(Guid.Empty);
                Assert.Fail();
            }
            catch (FabricException e)
            {
                Assert.AreEqual(FabricErrorCode.OperationNotComplete, e.ErrorCode);
            }
        }

        [TestMethod]
        public async Task NodeRemovalTest_IgnoreNodeNotFound()
        {
            var impact = new NodeRepairImpactDescription();
            impact.ImpactedNodes.Add(new NodeImpact("Node.0", NodeImpactLevel.RemoveNode));

            var task = new MockRepairTask("id", "action")
            {
                Impact = impact,
            };

            var action = CreateMoveToRestoringAction(task);

            rm.RemoveNodeStateAsyncHandler = (n, timeout, token) =>
            {
                throw new FabricException(FabricErrorCode.NodeNotFound);
            };
            await action.ExecuteAsync(Guid.Empty);
        }

        [TestMethod]
        public async Task NodeRemovalTest_RetryNodeIsUp()
        {
            var impact = new NodeRepairImpactDescription();
            impact.ImpactedNodes.Add(new NodeImpact("Node.0", NodeImpactLevel.RemoveNode));

            var task = new MockRepairTask("id", "action")
            {
                Impact = impact,
            };

            var action = CreateMoveToRestoringAction(task);

            // Retry on NodeIsUp
            int callCount = 0;
            int failCallCount = 2;
            rm.RemoveNodeStateAsyncHandler = (n, timeout, token) =>
            {
                ++callCount;
                if (failCallCount-- > 0)
                {
                    throw new FabricException(FabricErrorCode.NodeIsUp);
                }

                return Task.FromResult(0);
            };
            await action.ExecuteAsync(Guid.Empty);
            Assert.AreEqual(3, callCount);

            // Eventually give up
            callCount = 0;
            failCallCount = 1000;
            await action.ExecuteAsync(Guid.Empty);
            Assert.AreEqual(6, callCount);
        }

        [TestMethod]
        public async Task NodeRemovalTest_IgnoreNodeNotFound_MultipleNodes()
        {
            var impact = new NodeRepairImpactDescription();
            impact.ImpactedNodes.Add(new NodeImpact("Node.0", NodeImpactLevel.RemoveNode));
            impact.ImpactedNodes.Add(new NodeImpact("Node.1", NodeImpactLevel.RemoveNode));
            impact.ImpactedNodes.Add(new NodeImpact("Node.2", NodeImpactLevel.RemoveNode));

            var task = new MockRepairTask("id", "action")
            {
                Impact = impact,
            };

            var action = CreateMoveToRestoringAction(task);

            // Retry on NodeIsUp
            int callCount = 0;
            int failCallCount = 2;
            rm.RemoveNodeStateAsyncHandler = (n, timeout, token) =>
            {
                Constants.TraceType.WriteInfo("RemoveNodeState({0})", n);

                ++callCount;
                if (n == "Node.1")
                {
                    if (failCallCount-- > 0)
                    {
                        throw new FabricException(FabricErrorCode.NodeIsUp);
                    }
                }

                return Task.FromResult(0);
            };
            await action.ExecuteAsync(Guid.Empty);
            Assert.AreEqual(5, callCount);

            // Multiple nodes, eventually give up
            callCount = 0;
            failCallCount = 1000;
            await action.ExecuteAsync(Guid.Empty);
            Assert.AreEqual(8, callCount);
        }
    }
}