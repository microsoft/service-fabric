// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Test
{
    using System;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Threading;
    using Threading.Tasks;
    using WEX.TestExecution;

    [TestClass]
    public class DelayLoadCoordinatorTest
    {
        private static readonly TraceType traceType = new TraceType("DelayLoadCoordinatorTest");

        [TestMethod]
        public async Task BasicTestAsync()
        {
            var configStore = new MockConfigStore();
            var configSection = new ConfigSection(traceType, configStore, "Test");

            CoordinatorFactoryArgs args = new CoordinatorFactoryArgs()
            {
                Agent = new MockInfrastructureAgentWrapper(),
                ConfigSection = configSection,
                PartitionId = Guid.NewGuid(),
                ReplicaId = 42L,
                ServiceName = new Uri("fabric:/System/InfrastructureService"),
            };

            configStore.AddKeyValue(args.ConfigSectionName, "DelayLoad.RetryDelayInSeconds", "1");

            bool runAsyncCalled = false;

            MockCoordinator mockCoordinator = new MockCoordinator();

            mockCoordinator.RunAsyncHandler = (e, t) =>
            {
                runAsyncCalled = true;
                return Task.Delay(Timeout.InfiniteTimeSpan, t);
            };

            const string expectedCommandResult = "Hello";
            mockCoordinator.RunCommandAsyncHandler = (admin, cmd, t, ct) =>
            {
                return Task.FromResult(expectedCommandResult);
            };

            int failureCount = 2;
            Func<CoordinatorFactoryArgs, IInfrastructureCoordinator> factory = a =>
            {
                if (failureCount-- > 0)
                    throw new Exception("Intentional failure");

                return mockCoordinator;
            };

            var coordinator = new DelayLoadCoordinator(
                factory: factory,
                factoryArgs: args,
                healthClient: new MockHealthClient(),
                configSection: configSection);

            var cts = new CancellationTokenSource();

            Task runAsyncTask = coordinator.RunAsync(0, cts.Token);

            while (!runAsyncCalled)
            {
                await Task.Delay(100);
            }

            Verify.AreEqual(expectedCommandResult, await coordinator.RunCommandAsync(false, "", TimeSpan.FromSeconds(1), CancellationToken.None));

            cts.Cancel();

            try
            {
                await runAsyncTask;
            }
            catch (TaskCanceledException)
            {
            }
        }
    }
}