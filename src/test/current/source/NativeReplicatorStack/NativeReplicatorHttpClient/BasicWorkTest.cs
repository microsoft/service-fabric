// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace NativeReplicatorHTTPClient
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Fabric;
    using System.Fabric.Description;
    using System.Linq;
    using System.ServiceModel;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Net.Http;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Serialization;
    using Newtonsoft.Json.Converters;
    using System.Fabric.Query;

    using MCF.TestableService;
    using Microsoft.ServiceFabric.Services.Communication;

    /// <summary>
    /// Test Manager
    /// </summary>
    public class BasicWorkTest
    {
        /// <summary>
        /// Back of interval in milliseconds.
        /// </summary>
        private const int BackOffTimeInMs = 1024;

        /// <summary>
        /// Number of re-resolve to complete the operation before giving up.
        /// </summary>
        private int retryCount = 128;

        /// <summary>
        /// FabricName of the service.
        /// </summary>
        private Uri serviceName;

        private string FQDN;

        /// <summary>
        /// Partition identifier used for logging
        /// </summary>
        private string partitionIdentifier;

        /// <summary>
        /// Partitioning kind of the service.
        /// </summary>
        private ServicePartitionKind servicePartitionKind;

        /// <summary>
        /// Relevant partition key.
        /// </summary>
        private string serviceNamedPartitionKey;

        /// <summary>
        /// Relevant partition key.
        /// </summary>
        private long serviceIntPartitionKey;

        /// <summary>
        /// Description of the service.
        /// </summary>
        private StatefulServiceDescription statefulServiceDescription;

        /// <summary>
        /// This is used for re-resolving
        /// </summary>
        private ResolvedServicePartition lastResolvedServicePartition = null;

        /// <summary>
        /// This is used for re-resolving
        /// </summary>
        private string lastResolvedPrimaryAddress = null;

        /// <summary>
        /// FabricClient used to communicate with the WinFabric cluster.
        /// </summary>
        private FabricClient fabricClient;

        /// <summary>
        /// Random number generator.
        /// </summary>
        private Random random;

        /// <summary>
        /// Send timeout.
        /// </summary>
        private TimeSpan sendTimeout;

        /// <summary>
        /// Whether to retry call or not
        /// </summary>
        private bool retryCall;

        /// <summary>
        /// Demo mode.
        /// </summary>
        private bool isDemoMode;

        /// <summary>
        /// Use testability features in the product.
        /// </summary>
        private bool useTestability;

        /// <summary>
        /// True if the service is running in PAAS cluster
        /// </summary>
        private bool isPAAS;

        /// <summary>
        /// Exception code if Service throw excepction.
        /// </summary>
        private long statusCode;

        /// <summary>
        /// The router address.
        /// </summary>
        private Uri routerAddress;

        private string[] HttpGatewayAddresses;

        private HTTPClient.ClusterEnvironment clusterEnv;

        internal void Initialize(
            Uri serviceName,
            string[] gatewayAddresses,
            string[] HttpGatewayAddresses,
            Uri routerAddress,
            SecurityCredentials credentials,
            TimeSpan sendTimeout,
            int retryCount,
            bool isDemo,
            bool useTestability,
            bool isPAAS,
            bool retryCall)
        {
            // Common
            this.isDemoMode = isDemo;
            this.serviceName = serviceName;

            // the servicename format is fabric://MyApp/Service, here we only need MyApp/Service
            // for the FQDN(Full Qualified Domain Name) used by HttpApplicatonGateway 
            string servicename = serviceName.ToString();
            this.FQDN = servicename.Substring(servicename.IndexOf('/') + 1);

            this.random = new Random();
            this.sendTimeout = sendTimeout;
            this.retryCall = retryCall;
            this.retryCount = retryCount;
            this.routerAddress = routerAddress;
            this.useTestability = useTestability;
            this.isPAAS = isPAAS;
            this.HttpGatewayAddresses = HttpGatewayAddresses;

            if (gatewayAddresses == null)
            {
                this.fabricClient = new FabricClient();
            }
            else if (isPAAS)
            {
                this.fabricClient = new FabricClient(credentials, gatewayAddresses);
            }
            else
            {
                this.fabricClient = new FabricClient(gatewayAddresses);
            }
        }

        /// <summary>
        /// Initializes a new instance of the TestManager class.
        /// </summary>
        /// <param name="serviceName">Name of the service.</param>
        /// <param name="gatewayAddresses">Client connection addresses..</param>
        /// <param name="routerAddress">Address of the router.</param>
        /// <param name="sendTimeout">Send timeout.</param>
        /// <param name="retryCount">Number of retries before failing.</param>
        /// <param name="isDemo">No failover mode.</param>
        /// <param name="useTestability">Whether to use testability hooks when inducing faults.</param>
        public BasicWorkTest(
            Uri serviceName,
            string[] gatewayAddresses,
            string[] HttpGatewayAddresses,
            Uri routerAddress,
            SecurityCredentials credentials,
            TimeSpan sendTimeout,
            int retryCount,
            bool isDemo,
            bool useTestability,
            bool isPAAS,
            HTTPClient.ClusterEnvironment clusterEnvironment,
            bool retryCall)
        {

            this.Initialize(
                serviceName,
                gatewayAddresses,
                HttpGatewayAddresses,
                routerAddress,
                credentials,
                sendTimeout,
                retryCount,
                isDemo,
                useTestability,
                isPAAS,
                retryCall);

            // Partitioning specific.
            this.servicePartitionKind = ServicePartitionKind.Singleton;

            this.serviceIntPartitionKey = long.MinValue;
            this.serviceNamedPartitionKey = null;
            this.partitionIdentifier = "Singleton";
            this.clusterEnv = clusterEnvironment;
        }

        /// <summary>
        /// Initializes a new instance of the TestManager class.
        /// </summary>
        /// <param name="serviceName">Name of the service.</param>
        /// <param name="longPartitionKey">Partition key.</param>
        /// <param name="gatewayAddresses">Client connection addresses..</param>
        /// <param name="routerAddress">Address of the router.</param>
        /// <param name="sendTimeout">Send timeout.</param>
        /// <param name="retryCount">Number of retries before failing.</param>
        /// <param name="isDemo">No failover mode.</param>
        /// <param name="useTestability">Whether to use testability hooks when inducing faults.</param>
        public BasicWorkTest(
            Uri serviceName,
            long longPartitionKey,
            string[] gatewayAddresses,
            string[] HttpGatewayAddresses,
            Uri routerAddress,
            SecurityCredentials credentials,
            TimeSpan sendTimeout,
            int retryCount,
            bool isDemo,
            bool useTestability,
            bool isPAAS,
            HTTPClient.ClusterEnvironment clusterEnvironment,
            bool retryCall)
        {
            this.Initialize(
                serviceName,
                gatewayAddresses,
                HttpGatewayAddresses,
                routerAddress,
                credentials,
                sendTimeout,
                retryCount,
                isDemo,
                useTestability,
                isPAAS,
                retryCall);

            // Partitioning specific.
            this.servicePartitionKind = ServicePartitionKind.Int64Range;

            this.serviceIntPartitionKey = longPartitionKey;
            this.serviceNamedPartitionKey = null;
            this.partitionIdentifier = String.Format("Key:{0}", longPartitionKey.ToString());
            this.clusterEnv = clusterEnvironment;
        }

        /// <summary>
        /// Initializes a new instance of the TestManager class.
        /// </summary>
        /// <param name="serviceName">Name of the service.</param>
        /// <param name="namedPartitionKey">Partition key.</param>
        /// <param name="gatewayAddresses">Client connection addresses..</param>
        /// <param name="routerAddress">Address of the router.</param>
        /// <param name="sendTimeout">Send timeout.</param>
        /// <param name="retryCount">Number of retries before failing.</param>
        /// <param name="isDemo">No failover mode.</param>
        /// <param name="useTestability">Whether to use testability hooks when inducing faults.</param>
        public BasicWorkTest(
            Uri serviceName,
            string namedPartitionKey,
            string[] gatewayAddresses,
            string[] HttpGatewayAddresses,
            Uri routerAddress,
            SecurityCredentials credentials,
            TimeSpan sendTimeout,
            int retryCount,
            bool isDemo,
            bool useTestability,
            bool isPAAS,
            HTTPClient.ClusterEnvironment clusterEnvironment,
            bool retryCall)
        {
            // Common
            this.Initialize(
                serviceName,
                gatewayAddresses,
                HttpGatewayAddresses,
                routerAddress,
                credentials,
                sendTimeout,
                retryCount,
                isDemo,
                useTestability,
                isPAAS,
                retryCall);

            // Partitioning specific.
            this.servicePartitionKind = ServicePartitionKind.Named;

            this.serviceIntPartitionKey = long.MinValue;
            this.serviceNamedPartitionKey = namedPartitionKey;
            this.partitionIdentifier = namedPartitionKey;
            this.clusterEnv = clusterEnvironment;
        }

        /// <summary>
        /// Types of Workload
        /// </summary>
        private enum WorkLoadType
        {
            /// <summary>
            /// Invalid work load type.
            /// </summary>
            Invalid,

            /// <summary>
            /// Basic work load type.
            /// </summary>
            Basic,

            /// <summary>
            /// Primary failover work load type.
            /// </summary>
            PrimaryFailover,

            /// <summary>
            /// Random failover work load type.
            /// </summary>
            RandomFailover,

            /// <summary>
            /// Failover Stress work load type.
            /// </summary>
            FailoverStress
        }

        /// <summary>
        /// Indicates the kind of data loss test mode
        /// </summary>
        [Flags]
        private enum DataLossTestMode
        {
            /// <summary>
            /// The service is NOT expected to lose any data
            /// </summary>
            None = 0x1 << 1,

            /// <summary>
            /// The service is expected to lose all the data
            /// </summary>
            Full = 0x1 << 2
        }

        /// <summary>
        /// Initializes the test harness.
        /// </summary>
        /// <returns>Task that represents the asynchronous initialization.</returns>
        public async Task InitializeAsync()
        {
            ServiceDescription serviceDescription;

            for (int i = 0; i < this.retryCount; i++)
            {
                try
                {
                    serviceDescription = await this.fabricClient.ServiceManager.GetServiceDescriptionAsync(this.serviceName).ConfigureAwait(false);

                    this.statefulServiceDescription = serviceDescription as StatefulServiceDescription;

                    MCFUtils.FailClientAssert(this.statefulServiceDescription != null, "Only Stateful Services are supported");

                    return;
                }
                catch (FabricTransientException ex)
                {
                    if (i == this.retryCount - 1)
                    {
                        Tracing.TraceEvent(TraceEventType.Critical, 0, partitionIdentifier + ": " + "GetServiceDescriptionAsync({0} retry count exceeded", this.serviceName);
                        MCFUtils.FailClientAssert(i == this.retryCount - 1, "Retry count exceeded");
                    }

                    Tracing.TraceEvent(TraceEventType.Verbose, 0, partitionIdentifier + ": " + "GetServiceDescription: FabricTransientException {0}", ex.ToString());
                }
                catch (Exception exception)
                {
                    Tracing.TraceEvent(TraceEventType.Critical, 0, partitionIdentifier + ": " + "GetServiceDescription: Exception {0}", exception.ToString());
                    throw exception;
                }

                await Task.Delay(BasicWorkTest.BackOffTimeInMs);
            }
        }

        /// <summary>Run a specific workload</summary>
        /// <returns>Task that represents the running tests.</returns>
        public async Task<Dictionary<string, string>> RunSpecificWorkloadAsync(
            WorkloadIdentifier workloadId,
            bool runForever,
            Dictionary<string, string> parameters,
            TimeSpan callTimeout,
            bool retryCall,
            CancellationToken cancellationToken)
        {
            Dictionary<string, string> results = new Dictionary<string, string>();

            try
            {
                int iteration = 0;

                do
                {
                    if (cancellationToken.IsCancellationRequested)
                    {
                        Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "Workload {0} cancelled", workloadId);
                        break;
                    }

                    results = await this.DoBunchOfWorkAsync(workloadId, parameters, 1, 1, callTimeout, retryCall);

                    if (runForever)
                    {
                        await this.TestDataConsistencyAsync();
                        iteration++;
                        if (iteration % 50 == 0)
                        {
                            Tracing.TraceEvent(TraceEventType.Verbose, 0, partitionIdentifier + ": " + "Run Forever Iteration: {0}", iteration);
                        }
                    }
                }
                while (runForever);
            }
            catch (Exception e)
            {
                // All exceptions must be handled in the tests.
                Tracing.TraceEvent(TraceEventType.Critical, 0, partitionIdentifier + ": " + "MCFClient: RunSpecificWorkload: Unhandled exception {0}", e.ToString());
                throw e;
            }

            return results;
        }

        /// <summary>
        /// Runs the basic set of tests.
        /// </summary>
        /// <returns>Task that represents the running tests.</returns>
        public async Task RunAsync(
            int loopCount,
            CancellationToken cancellationToken,
            Dictionary<string, string> parameters)
        {
            List<string> scenariosToRun = parameters.ContainsKey("scenario") ?
                new List<string>(parameters["scenario"].Split(';')) :
                new List<string> { "all" };

            bool isVolatileService = !(parameters.ContainsKey("persisted") ? bool.Parse(parameters["persisted"]) : true);

            try
            {
                if (this.isDemoMode)
                {
                    await this.RunDemoWorkloadAsync(cancellationToken);
                    Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "Test {0} cancelled", "DemoWorkload");
                }
                else
                {
                    if (scenariosToRun.Contains("all") ||
                        scenariosToRun.Contains("basic"))
                    {
                        // Start with a sanity test - no fail overs induced
                        await this.BasicWorkloadWithNoFailover(loopCount, parameters, cancellationToken);
                        if (cancellationToken.IsCancellationRequested)
                        {
                            Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "Test {0} cancelled", "BasicWorkloadWithNoFailover");
                            return;
                        }
                    }

                    if (scenariosToRun.Contains("all") ||
                        scenariosToRun.Contains("datalossnone") ||
                        scenariosToRun.Contains("datalossfull"))
                    {
                        if (isVolatileService)
                        {
                            Tracing.TraceEvent(TraceEventType.Error, 0, "{0}: Test {1} not supported for volatile services", partitionIdentifier, "TestDataLoss");
                            return;
                        }

                        DataLossTestMode testMode = scenariosToRun.Contains("all") ? (DataLossTestMode.Full | DataLossTestMode.None) :
                                                    scenariosToRun.Contains("datalossnone") ? DataLossTestMode.None :
                                                    DataLossTestMode.Full;

                        await this.TestDataLoss(loopCount, testMode, parameters, cancellationToken);
                        if (cancellationToken.IsCancellationRequested)
                        {
                            Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "Test {0} cancelled", "TestDataLoss");
                            return;
                        }
                    }

                    if (scenariosToRun.Contains("all") ||
                        scenariosToRun.Contains("restartpartition"))
                    {
                        if (isVolatileService)
                        {
                            Tracing.TraceEvent(TraceEventType.Error, 0, "{0}: Test {1} not supported for volatile services", partitionIdentifier, "TestDataLoss");
                            return;
                        }

                        // Putting the new test to the top for now.
                        await this.RunPartitionWideRestartScenarioAsync(loopCount, parameters, cancellationToken);
                        if (cancellationToken.IsCancellationRequested)
                        {
                            Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "Test {0} cancelled", "RunPartitionWideRestartScenario");
                            return;
                        }
                    }

                    if (scenariosToRun.Contains("all") ||
                        scenariosToRun.Contains("primaryfailover"))
                    {
                        // Fail over tests without losing quorum
                        await this.BasicWorkloadWithPrimaryFailover(loopCount, parameters, cancellationToken);
                        if (cancellationToken.IsCancellationRequested)
                        {
                            Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "Test {0} cancelled", "BasicWorkloadWithPrimaryFailover");
                            return;
                        }
                    }

                    if (scenariosToRun.Contains("all") ||
                        scenariosToRun.Contains("randomfailover"))
                    {
                        await this.BasicWorkloadWithRandomFailover(loopCount, parameters, cancellationToken);
                        if (cancellationToken.IsCancellationRequested)
                        {
                            Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "Test {0} cancelled", "BasicWorkloadWithRandomFailover");
                            return;
                        }
                    }

                    if (scenariosToRun.Contains("all") ||
                        scenariosToRun.Contains("failoverstress"))
                    {
                        await this.BasicWorkloadWithFailoverStress(loopCount, parameters, cancellationToken);
                        if (cancellationToken.IsCancellationRequested)
                        {
                            Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "Test {0} cancelled", "BasicWorkloadWithFailoverStress");
                            return;
                        }
                    }

                    if (scenariosToRun.Contains("all") ||
                        scenariosToRun.Contains("quorumloss"))
                    {
                        if (isVolatileService)
                        {
                            Tracing.TraceEvent(TraceEventType.Error, 0, "{0}: Test {1} not supported for volatile services", partitionIdentifier, "TestDataLoss");
                            return;
                        }

                        // Fail more than one replica.
                        await this.BasicQuorumLoss(loopCount, parameters, cancellationToken);
                        if (cancellationToken.IsCancellationRequested)
                        {
                            Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "Test {0} cancelled", "BasicQuorumLoss");
                            return;
                        }
                    }
                }
            }
            catch (Exception e)
            {
                // All exceptions must be handled in the tests.
                Tracing.TraceEvent(TraceEventType.Critical, 0, partitionIdentifier + ": " + "MCFClient: RunAsync: Unhandled exception {0}", e.ToString());
                throw e;
            }
        }

        /// <summary>
        /// Run specific transient failure workload
        /// </summary>
        /// <returns>Task that represents the running tests.</returns>
        public async Task RunTransientFaultWorkloadAsync(
            int loopCount,
            CancellationToken cancellationToken)
        {
            try
            {
                Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "Test {0}, Iteration {1}", "RunTransientFailoverWOrkload", 1);

                await this.RunPartitionWideTransientFaultScenarioAsync(loopCount, new Dictionary<string, string>(), cancellationToken);
            }
            catch (Exception e)
            {
                // All exceptions must be handled in the tests.
                Tracing.TraceEvent(TraceEventType.Critical, 0, partitionIdentifier + ": " + "MCFClient: RunTransientFaultWorkloadAsync: Unhandled exception {0}", e.ToString());
                throw e;
            }
        }

        /// <summary>
        /// Runs demo workload.
        /// </summary>
        /// <returns>Task representing the asynchronous operation.</returns>
        private async Task RunDemoWorkloadAsync(CancellationToken cancellationToken)
        {
            Console.WriteLine("****************************************************");
            Console.WriteLine();
            Console.WriteLine("Demo Workload");
            Console.WriteLine("Summary:");
            Console.WriteLine("Infinite random workloads with no failover or checks.");
            Console.WriteLine("Total Number of Iterations: Infinite");
            Console.WriteLine();
            Console.WriteLine("****************************************************");

            int totalNumberOfOperations = 10;
            int numberOfOperationPerBatch = 10;

            int i = 0;

            while (true)
            {
                Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "Test {0}, Iteration {1}", "DemoWorkload", i++);

                if (cancellationToken.IsCancellationRequested)
                {
                    break;
                }

                try
                {
                    await this.DoBunchOfWorkAsync(totalNumberOfOperations, numberOfOperationPerBatch, new Dictionary<string, string>());
                }
                catch (Exception e)
                {
                    Console.WriteLine(partitionIdentifier + ": " + e.Message);

                    Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "Test {0}, Exception {1}", "DemoWorkload", e.ToString());
                }
            }
        }

        /// <summary>
        /// Runs the basic test with no fail overs.
        /// All this tests is whether after bunch of work the replicas are at a consistent state.
        /// </summary>
        /// <param name="numberOfIterations">Number of iterations.</param>
        /// <returns>Task representing the asynchronous operation.</returns>
        private async Task BasicWorkloadWithNoFailover(int numberOfIterations, Dictionary<string, string> parameters, CancellationToken cancellationToken)
        {
            Console.WriteLine("****************************************************");
            Console.WriteLine();
            Console.WriteLine("SANITY & PERFORMANCE TEST: NO FAILOVER");
            Console.WriteLine("Summary:");
            Console.WriteLine("Repeatedly, do multiple operations and then check data consistency");
            Console.WriteLine("Total Number of Iterations: {0}", numberOfIterations);
            Console.WriteLine("Date: {0}", DateTime.UtcNow);
            Console.WriteLine();
            Console.WriteLine("****************************************************");

            double operationsCompleted = 0;
            double seconds = 0;
            int totalNumberOfOperations = 50;
            int numberOfOperationPerBatch = 50;

            for (int i = 0; i < numberOfIterations; i++)
            {
                if (cancellationToken.IsCancellationRequested)
                {
                    break;
                }

                Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "Test {0}, Iteration {1}", "BasicWorkloadWithNoFailover", i);

                DateTime startTime = DateTime.UtcNow;

                await this.DoBunchOfWorkAsync(totalNumberOfOperations, numberOfOperationPerBatch, parameters);

                DateTime endTime = DateTime.UtcNow;

                operationsCompleted += totalNumberOfOperations;

                seconds += (endTime - startTime).Seconds;

                await this.TestDataConsistencyAsync();
            }

            Console.WriteLine("Performance Results:");
            Console.WriteLine(partitionIdentifier + ": " + "Completed {0} operations in {1} seconds = {2} ops/second", operationsCompleted, seconds, operationsCompleted / seconds);

            Console.WriteLine();
        }

        /// <summary>
        /// This should also probabilistically test onDataLoss.
        /// </summary>
        /// <param name="numberOfIterations">Number of iterations.</param>
        /// <returns>Task representing the asynchronous operation.</returns>
        private async Task BasicQuorumLoss(int numberOfIterations, Dictionary<string, string> parameters, CancellationToken cancellationToken)
        {
            Console.WriteLine("****************************************************");
            Console.WriteLine();
            Console.WriteLine("QUORUM LOSS TEST");
            Console.WriteLine("Summary:");
            Console.WriteLine("Repeatedly, kill enough replicas to lose quorum, do multiple operations and then check data consistency");
            Console.WriteLine("Number of iterations:\t{0}", numberOfIterations);
            Console.WriteLine("Date: {0}", DateTime.UtcNow);
            Console.WriteLine();
            Console.WriteLine("****************************************************");

            int numberOfOperationPerIteration = 20;
            int numberOfOperationPerBatch = 10;

            Stopwatch stopwatch = new Stopwatch();

            for (int i = 0; i < numberOfIterations; i++)
            {
                Console.WriteLine("{0}: Iteration: {1}", DateTime.UtcNow, i);
                Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "Test {0}, Iteration {1}", "BasicQuorumLoss", i);

                if (cancellationToken.IsCancellationRequested)
                {
                    break;
                }

                stopwatch.Restart();

                await this.WaitForReplicaSetSizeToBeUpAsync(this.statefulServiceDescription.TargetReplicaSetSize);

                if (this.random.Next() % 2 == 0)
                {
                    await this.InduceFaultViaFabricAsync(FabricFaultType.QuorumLoss, isVolatileService: false);
                }
                else
                {
                    await this.InduceFaultViaFabricAsync(FabricFaultType.RestartNode, isVolatileService: false);
                }

                await this.DoBunchOfWorkAsync(numberOfOperationPerIteration, numberOfOperationPerBatch, parameters);

                await this.TestDataConsistencyAsync();
                Console.WriteLine("{0}: Iteration: {1} completed in {2} ms", DateTime.UtcNow, i, stopwatch.ElapsedMilliseconds);
            }

            Console.WriteLine();
        }

        /// <summary>
        /// This should also probabilistically test onDataLoss.
        /// </summary>
        /// <param name="numberOfIterations">Number of iterations.</param>
        /// <returns>Task representing the asynchronous operation.</returns>
        private async Task TestDataLoss(int numberOfIterations, DataLossTestMode testMode, Dictionary<string, string> parameters, CancellationToken cancellationToken)
        {
            Console.WriteLine("****************************************************");
            Console.WriteLine();
            Console.WriteLine("DATA LOSS TEST");
            Console.WriteLine("Summary:");
            Console.WriteLine("Repeatedly, kill enough replicas to lose quorum and primary, call RecoverPartitions then check data consistency");
            Console.WriteLine("Number of iterations:\t{0}", numberOfIterations);
            Console.WriteLine("Test Mode:\t{0}", testMode);
            Console.WriteLine("Date: {0}", DateTime.UtcNow);
            Console.WriteLine();
            Console.WriteLine("****************************************************");

            TestableServiceProgress lastProgress = TestableServiceProgress.Zero;

            for (int i = 0; i < numberOfIterations; i++)
            {
                Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "Test {0}, Iteration {1}", "TestSuspectedDataLoss", i);

                if (cancellationToken.IsCancellationRequested)
                {
                    break;
                }

                await this.WaitForReplicaSetSizeToBeUpAsync(this.statefulServiceDescription.TargetReplicaSetSize);

                // Run the workload and expect full data loss only if the user has specifically asked for the scenario
                // OR
                // he has not specified this scenario and this is an even iteration (so that there is a mix of both if he is not specified)
                bool expectFullDataLoss =
                    testMode.HasFlag(DataLossTestMode.Full) && testMode.HasFlag(DataLossTestMode.None) ?
                    i % 2 == 0 :
                    testMode.HasFlag(DataLossTestMode.Full);

                if (expectFullDataLoss)
                {
                    await this.DoBunchOfWorkAsync(30, 10, parameters);
                }

                TestableServiceProgress afterWorkProgress = await this.TestDataConsistencyAsync() as TestableServiceProgress;

                MCFUtils.FailClientAssert(afterWorkProgress.CompareTo(lastProgress) >= 0, partitionIdentifier + ": " + "Progress went backwards after calling do work (prior to inducing failures)");

                if (afterWorkProgress.CompareTo(lastProgress) == 0)
                {
                    Trace.TraceWarning(partitionIdentifier + ": " + "DoWork did not result in any progress");
                }

                if (expectFullDataLoss)
                {
                    await this.InduceFaultViaFabricAsync(FabricFaultType.FullDataLoss, isVolatileService: false);
                }
                else
                {
                    await this.InduceFaultViaFabricAsync(FabricFaultType.PartialDataLoss, isVolatileService: false);
                }

                // Allow the failover subsystem to detect the quorum loss
                await Task.Delay(30000);

                await this.RecoverPartitions();

                TestableServiceProgress newProgress = await this.TestDataConsistencyAsync() as TestableServiceProgress;

                // Only if full dataloss is not expected can we ensure that progress does not go behind.
                if (!expectFullDataLoss)
                {
                    MCFUtils.FailClientAssert(newProgress.CompareTo(afterWorkProgress) >= 0, string.Format(partitionIdentifier + ": " + "Data loss has occurred. Progress went backwards after doing work. Before: {0}, After: {1}", afterWorkProgress, newProgress));
                }

                lastProgress = newProgress;
            }

            Console.WriteLine();
        }

        private async Task RecoverPartitions()
        {
            int remainingRetries = this.retryCount;

            while (remainingRetries > 0)
            {
                try
                {
                    // TODO: is lastResolvedServicePartition ever null or incorrect at this point ?
                    await this.fabricClient.ClusterManager.RecoverPartitionAsync(this.lastResolvedServicePartition.Info.Id);
                    break;
                }
                catch (Exception ex)
                {
                    if (!this.IsExpectedException(ex, FabricErrorCode.CommunicationError))
                    {
                        throw;
                    }
                }
            }
        }

        /// <summary>
        /// Failover Stress Test.
        /// </summary>
        /// <param name="numberOfIterations">Number of iterations.</param>
        /// <returns>Task representing the asynchronous operation.</returns>
        private async Task BasicWorkloadWithFailoverStress(int numberOfIterations, Dictionary<string, string> parameters, CancellationToken cancellationToken)
        {
            Console.WriteLine("****************************************************");
            Console.WriteLine();
            Console.WriteLine("PRIMARY FAILOVER STRESS TEST");
            Console.WriteLine("Summary:");
            Console.WriteLine("Do multiple operations, and then, repeatedly, kill primary and then check data consistency");
            Console.WriteLine("Number of iterations:\t{0}", numberOfIterations);
            Console.WriteLine("Date: {0}", DateTime.UtcNow);
            Console.WriteLine();
            Console.WriteLine("****************************************************");

            int totalNumberOfOperations = 20;
            int numberOfOperationPerBatch = 10;

            await this.DoBunchOfWorkAsync(totalNumberOfOperations, numberOfOperationPerBatch, parameters);

            for (int i = 0; i < numberOfIterations; i++)
            {
                Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "Test {0}, Iteration {1}", "BasicWorkloadWithFailoverStress", i);

                if (cancellationToken.IsCancellationRequested)
                {
                    break;
                }

                bool isVolatileService = !(parameters.ContainsKey("persisted") ? bool.Parse(parameters["persisted"]) : true);
                await this.InduceFaultViaFabricAsync(FabricFaultType.PrimaryRandom, isVolatileService);

                await this.TestDataConsistencyAsync();
            }
        }

        /// <summary>
        /// Primary Failover Test.
        /// </summary>
        /// <param name="numberOfIterations">Number of iterations.</param>
        /// <returns>Task representing the asynchronous operation.</returns>
        private async Task BasicWorkloadWithPrimaryFailover(int numberOfIterations, Dictionary<string, string> parameters, CancellationToken cancellationToken)
        {
            Console.WriteLine("****************************************************");
            Console.WriteLine();
            Console.WriteLine("PRIMARY FAILOVER TEST");
            Console.WriteLine("Summary:");
            Console.WriteLine("Repeatedly, do multiple operations, kill primary, do more operations and then check data consistency");
            Console.WriteLine("Number of iterations:\t{0}", numberOfIterations);
            Console.WriteLine("Date: {0}", DateTime.UtcNow);
            Console.WriteLine();
            Console.WriteLine("****************************************************");

            int totalNumberOfOperations = 20;
            int numberOfOperationPerBatch = 10;

            for (int i = 0; i < numberOfIterations; i++)
            {
                Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "Test {0}, Iteration {1}", "BasicWorkloadWithPrimaryFailover", i);

                if (cancellationToken.IsCancellationRequested)
                {
                    break;
                }

                await this.DoBunchOfWorkAsync(totalNumberOfOperations, numberOfOperationPerBatch, parameters);

                bool isVolatileService = !(parameters.ContainsKey("persisted") ? bool.Parse(parameters["persisted"]) : true);
                await this.InduceFaultViaFabricAsync(FabricFaultType.PrimaryRandom, isVolatileService);

                await this.DoBunchOfWorkAsync(totalNumberOfOperations, numberOfOperationPerBatch, parameters);

                await this.TestDataConsistencyAsync();
            }

            Console.WriteLine();
        }

        /// <summary>
        /// Random Failover test.
        /// </summary>
        /// <param name="numberOfIterations">Number of iterations.</param>
        /// <returns>Task representing the asynchronous operation.</returns>
        private async Task BasicWorkloadWithRandomFailover(int numberOfIterations, Dictionary<string, string> parameters, CancellationToken cancellationToken)
        {
            Console.WriteLine("****************************************************");
            Console.WriteLine();
            Console.WriteLine("RANDOM FAILOVER TEST");
            Console.WriteLine("Summary:");
            Console.WriteLine("Repeatedly, do multiple operations, kill random replica, do more operations and then check data consistency");
            Console.WriteLine("Number of iterations:\t{0}", numberOfIterations);
            Console.WriteLine("Date: {0}", DateTime.UtcNow);
            Console.WriteLine();
            Console.WriteLine("****************************************************");

            int numberOfOperationPerIteration = 20;
            int numberOfOperationPerBatch = 10;

            for (int i = 0; i < numberOfIterations; i++)
            {
                Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "Test {0}, Iteration {1}", "BasicWorkloadWithRandomFailover", i);

                if (cancellationToken.IsCancellationRequested)
                {
                    break;
                }

                await this.DoBunchOfWorkAsync(numberOfOperationPerIteration, numberOfOperationPerBatch, parameters);

                // Kills a random replica
                bool isVolatileService = !(parameters.ContainsKey("persisted") ? bool.Parse(parameters["persisted"]) : true);
                await this.InduceFaultViaFabricAsync(FabricFaultType.SecondaryRandom, isVolatileService);

                await this.DoBunchOfWorkAsync(numberOfOperationPerIteration, numberOfOperationPerBatch, parameters);

                await this.TestDataConsistencyAsync();
            }

            Console.WriteLine();
        }

        /// <summary>
        /// Run Cluster Restart Scenario
        /// </summary>
        /// <param name="numberOfIterations">Number of iterations</param>
        /// <returns>Task representing the asynchronous operation.</returns>
        private async Task RunPartitionWideRestartScenarioAsync(int numberOfIterations, Dictionary<string, string> parameters, CancellationToken cancellationToken)
        {
            Console.WriteLine("****************************************************");
            Console.WriteLine();
            Console.WriteLine("FAULT THE ENTIRE PARTITION TEST");
            Console.WriteLine("Summary:");
            Console.WriteLine("Repeatedly, do multiple operations, kill all replicas, do more operations and then check data consistency");
            Console.WriteLine("Number of iterations:\t{0}", numberOfIterations);
            Console.WriteLine("Date: {0}", DateTime.UtcNow);
            Console.WriteLine();
            Console.WriteLine("****************************************************");

            int numberOfOperationPerIteration = 20;
            int numberOfOperationPerBatch = 10;

            IComparable lastProgress = TestableServiceProgress.Zero;

            for (int i = 0; i < numberOfIterations; i++)
            {
                Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "Test {0}, Iteration {1}", "PartitionWideRestartScenerio", i);

                if (cancellationToken.IsCancellationRequested)
                {
                    break;
                }

                await this.WaitForReplicaSetSizeToBeUpAsync(this.statefulServiceDescription.TargetReplicaSetSize);

                await this.DoBunchOfWorkAsync(numberOfOperationPerIteration, numberOfOperationPerBatch, parameters);

                await this.KillReplicasAsync(FabricFaultType.RestartReplica);

                await this.DoBunchOfWorkAsync(numberOfOperationPerIteration, numberOfOperationPerBatch, parameters);

                IComparable newProgress = await this.TestDataConsistencyAsync();

                TestableServiceProgress tmpOld = lastProgress as TestableServiceProgress;
                TestableServiceProgress tmpNew = newProgress as TestableServiceProgress;

                if (tmpOld != null && tmpNew != null)
                {
                    if (TestableServiceProgress.Zero.CompareTo(tmpOld) != 0)
                    {
                        MCFUtils.FailClientAssert(tmpNew.Epoch.DataLossNumber == tmpOld.Epoch.DataLossNumber, partitionIdentifier + ": " + "Data loss number must not have increased.");
                    }
                }

                MCFUtils.FailClientAssert(newProgress.CompareTo(lastProgress) > -1, partitionIdentifier + ": " + "New Progress should be at least higher than the old progress.");

                lastProgress = newProgress;
            }
        }

        private async Task RunPartitionWideTransientFaultScenarioAsync(int numberOfIterations, Dictionary<string, string> parameters, CancellationToken cancellationToken)
        {
            Console.WriteLine("****************************************************");
            Console.WriteLine();
            Console.WriteLine("TRANSIENT FAULT THE ENTIRE PARTITION TEST");
            Console.WriteLine("Summary:");
            Console.WriteLine("Do one unti of work and then repeatedly, transient fault all replicas");
            Console.WriteLine("Number of iterations:\t{0}", numberOfIterations);
            Console.WriteLine();
            Console.WriteLine("****************************************************");

            int numberOfOperationPerIteration = 20;
            int numberOfOperationPerBatch = 10;

            IComparable lastProgress = TestableServiceProgress.Zero;

            await this.WaitForReplicaSetSizeToBeUpAsync(this.statefulServiceDescription.TargetReplicaSetSize);
            await this.DoBunchOfWorkAsync(numberOfOperationPerIteration, numberOfOperationPerBatch, parameters);

            for (int i = 0; i < numberOfIterations; i++)
            {
                Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "Test {0}, Iteration {1}", "PartitionWideTransientFaultRestartScenerio", i);

                if (cancellationToken.IsCancellationRequested)
                {
                    break;
                }

                await this.WaitForReplicaSetSizeToBeUpAsync(this.statefulServiceDescription.TargetReplicaSetSize);

                Console.WriteLine("Iteration {0}", i);
                await this.InduceFaultViaFabricAsync(FabricFaultType.RestartPartition, isVolatileService: false);
            }
        }

        #region Private Helper methods

        private async Task WaitForPrimaryChange()
        {
            int retry = this.retryCount;
            ResolvedResult result = await this.ResolveAsync();
            while ((result == null || result.PrimaryAddress == this.lastResolvedPrimaryAddress) && retry > 0)
            {
                await Task.Delay(1000);
                result = await this.ResolveAsync();
                retry--;
            }
            return;
        }
        /// <summary>
        /// Helper method that wraps around resolve to hide different resolve overloads for different partition types..
        /// Resolves the service using the current RSP.
        /// Assigns the result to the current RSP.
        /// </summary>
        /// <param name="mustHavePrimary">Whether the resolved service must include a primary.</param>
        /// <remarks>Note: Resolve seems to take less than 20 milliseconds in one-box sceneries</remarks>
        /// <returns>Task representing the asynchronous operation.</returns>
        private async Task<ResolvedResult> ResolveAsync()
        {
            ResolvedResult result = new ResolvedResult();
            Stopwatch resolveTimer = Stopwatch.StartNew();
            int remainingRetries = this.retryCount;

            while (true)
            {
                try
                {
                    switch (this.servicePartitionKind)
                    {
                        case ServicePartitionKind.Singleton:
                            result.Partition = await this.fabricClient.ServiceManager.ResolveServicePartitionAsync(this.serviceName, this.lastResolvedServicePartition);
                            break;

                        case ServicePartitionKind.Named:
                            result.Partition = await this.fabricClient.ServiceManager.ResolveServicePartitionAsync(this.serviceName, this.serviceNamedPartitionKey, this.lastResolvedServicePartition);
                            break;

                        case ServicePartitionKind.Int64Range:
                            result.Partition = await this.fabricClient.ServiceManager.ResolveServicePartitionAsync(this.serviceName, this.serviceIntPartitionKey, this.lastResolvedServicePartition);
                            break;

                        default:
                            throw new InvalidOperationException(partitionIdentifier + ": " + "Unsupported Partition Kind: " + this.servicePartitionKind.ToString());
                    }

                    if (result.Partition == null)
                    {
                        Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "ResolveAsync: ResolveServicePartitionAsync returned null");
                    }
                    else
                    {
                        this.lastResolvedServicePartition = result.Partition;

                        result.Endpoints = new List<string>();
                        IList<ResolvedServiceEndpoint> endpointList = new List<ResolvedServiceEndpoint>(result.Partition.Endpoints);
                        for (int i = 0; i < endpointList.Count; i++)
                        {
                            ResolvedServiceEndpoint endpoint = endpointList[i];
                            string resolvedEndpointAddress = null;
                            bool isPrimary = endpoint.Role == ServiceEndpointRole.StatefulPrimary;


                            EndpointAddress endpointAddress;

                            try
                            {
                                endpointAddress = new EndpointAddress(endpoint.Address);
                            }
                            catch (Exception ex)
                            {
                                Tracing.TraceEvent(TraceEventType.Information, 0, "Resolved service endpoint is unable to be converted directly to an EndpointAddress.  Exception: {0}", ex.ToString());

                                ServiceEndpointCollection coll;
                                bool res;

                                res = ServiceEndpointCollection.TryParseEndpointsString(endpoint.Address, out coll);
                                MCFUtils.FailClientAssert(res, "Failed to parse endpoints string.  String: " + endpoint.Address);

                                string address;
                                res = coll.TryGetFirstEndpointAddress(out address);
                                MCFUtils.FailClientAssert(res, "Failed to get first endpoint from endpoints string.  String: " + endpoint.Address);

                                try
                                {
                                    endpointAddress = new EndpointAddress(address);
                                }
                                catch (Exception ex1)
                                {
                                    Tracing.TraceEvent(TraceEventType.Error, 0, "Failed to convert endpoint address to an EndpointAddress.  Exception: {0}", ex1.ToString());
                                    throw;
                                }
                            }

                            if (isPrimary)
                            {
                                this.lastResolvedPrimaryAddress = endpointAddress.ToString();
                                result.PrimaryAddress = endpointAddress.ToString();
                            }

                            if (isPAAS)
                            {
                                switch (this.servicePartitionKind)
                                {
                                    case ServicePartitionKind.Singleton:
                                        resolvedEndpointAddress = Utilities.GenerateEndpoint(
                                            this.clusterEnv,
                                            HttpGatewayAddresses[0],
                                            this.FQDN,
                                            isPrimary);
                                        break;
                                    case ServicePartitionKind.Named:
                                        resolvedEndpointAddress = Utilities.GenerateEndpoint(
                                            this.clusterEnv,
                                            HttpGatewayAddresses[0],
                                            this.FQDN,
                                            this.serviceNamedPartitionKey,
                                            "Named",
                                            isPrimary);
                                        break;
                                    case ServicePartitionKind.Int64Range:
                                        resolvedEndpointAddress = Utilities.GenerateEndpoint(
                                            this.clusterEnv,
                                            HttpGatewayAddresses[0],
                                            this.FQDN,
                                            this.serviceIntPartitionKey.ToString(),
                                            "Int64Range",
                                            isPrimary);
                                        break;
                                }

                            }
                            else
                            {
                                resolvedEndpointAddress = endpointAddress.ToString();
                            }

                            result.Endpoints.Add(resolvedEndpointAddress);
                            if (isPrimary)
                            {
                                result.PrimaryEndpoint = resolvedEndpointAddress;
                            }
                        }

                        if (result.PrimaryEndpoint != null)
                        {
                            return result;
                        }
                    }
                }
                catch (Exception e)
                {
                    if (this.IsExpectedException(e, FabricErrorCode.CommunicationError))
                    {
                        if (remainingRetries <= 0)
                        {
                            string message = string.Format(partitionIdentifier + ": " + "ResolveAsync: Failed all retries. Last exception: {0}; {1}; {2}", e.GetType(), e.Message, e.StackTrace);
                            Tracing.TraceEvent(TraceEventType.Critical, 0, message);
                            Console.WriteLine(message);
                            return null;
                        }

                        Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "ResolveAsync: Resolve failed with exception: {0}; {1}; {2}", e.GetType(), e.Message, e.StackTrace);
                    }
                    else
                    {
                        Tracing.TraceEvent(TraceEventType.Critical, 0, partitionIdentifier + ": " + "ResolveAsync: Unexpected exception: {0}; {1}; {2}", e.GetType(), e.Message, e.StackTrace);
                        return null;
                    }
                }

                await Task.Delay(BackOffTimeInMs);
                remainingRetries--;
            }
        }

        private bool IsExpectedException(Exception e, params FabricErrorCode[] errorCodes)
        {
            if (e is AggregateException)
            {
                foreach (Exception innerEx in ((AggregateException)e).InnerExceptions)
                {
                    if (!this.IsExpectedException(innerEx, errorCodes))
                    {
                        return false;
                    }
                }
            }

            if (e is OperationCanceledException || e is TimeoutException || e is FabricTransientException || e is EndpointNotFoundException)
            {
                return true;
            }

            if (e is FabricException)
            {
                FabricErrorCode errorCode = ((FabricException)e).ErrorCode;

                if (errorCodes.Contains(errorCode))
                {
                    return true;
                }
            }

            return false;
        }

        /// <summary>
        /// Calculates the number of replicas that would have to be killed to cause quorum loss.
        /// </summary>
        /// <returns>Number of replicas to kill.</returns>
        private int CalculateNumberOfReplicasToKillForQuorumLoss()
        {
            if (this.statefulServiceDescription.TargetReplicaSetSize == this.statefulServiceDescription.MinReplicaSetSize)
            {
                return (this.statefulServiceDescription.TargetReplicaSetSize / 2) + 1;
            }
            else
            {
                return this.statefulServiceDescription.TargetReplicaSetSize - this.statefulServiceDescription.MinReplicaSetSize + 1;
            }
        }

        private async Task KillReplicasAsync(FabricFaultType fault)
        {
            for (int i = 0; i < this.retryCount; ++i)
            {
                ResolvedResult resolvedResult = await this.ResolveAsync();

                if (resolvedResult.Partition.Endpoints.Count == this.statefulServiceDescription.TargetReplicaSetSize)
                {
                    PartitionSelector randomPartitionSelector = PartitionSelector.RandomOf(this.serviceName);
                    IList<ReplicaSelector> replicas = new List<ReplicaSelector>();
                    IList<Task> killJobList = new List<Task>();

                    // Get Replica list from FabricClient.QueryManager
                    ServiceReplicaList replicaList = await this.fabricClient.QueryManager.GetReplicaListAsync(resolvedResult.Partition.Info.Id);
                    if (replicaList == null)
                    {
                        continue;
                    }

                    // Create ReplicaSelector list using each replica id
                    for (int j = 0; j < replicaList.Count(); ++j)
                    {
                        replicas.Add(ReplicaSelector.ReplicaIdOf(randomPartitionSelector, replicaList[j].Id));
                    }

                    try
                    {
                        foreach (ReplicaSelector rs in replicas)
                        {
                            killJobList.Add(this.InduceFaultViaFabricAsync(randomPartitionSelector, rs, fault, isVolatileService: false));
                        }

                        await Task.WhenAll(killJobList);
                        await this.WaitForPrimaryChange();
                        return;
                    }
                    catch (Exception e)
                    {
                        if (this.IsExpectedException(e))
                        {
                            Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "{0}; {1}; {2}", e.GetType(), e.Message, e.StackTrace);

                            // Resolve and retry.
                            continue;
                        }
                        else
                        {
                            Tracing.TraceEvent(TraceEventType.Critical, 0, partitionIdentifier + ": " + "{0}; {1}; {2}", e.GetType(), e.Message, e.StackTrace);

                            MCFUtils.FailClientAssert(false, partitionIdentifier + ": " + "Unexpected Exception: " + e.Message);
                            throw;
                        }
                    }
                }

                // Wait for the target replica set to be up.
                await Task.Delay(BasicWorkTest.BackOffTimeInMs);
            }

            string message = string.Format(partitionIdentifier + ": " + "After {0} retries, the index could not be killed", this.retryCount);
            MCFUtils.FailClientAssert(false, message);
            Console.WriteLine(message);
        }

        private async Task<FabricFaultType> InduceFaultViaFabricAsync(
            PartitionSelector partitionSelector,
            ReplicaSelector replicaSelector,
            FabricFaultType faultType,
            bool isVolatileService)
        {
            Console.WriteLine("\tInduce failure via Fabric Testability: {0}", faultType);

            FabricFaultType[] validVolatileFaults = new FabricFaultType[]
            {
                    FabricFaultType.RemoveReplica, FabricFaultType.MoveSecondary, FabricFaultType.RestartNode, FabricFaultType.KillServiceHost,
                    FabricFaultType.RemovePrimary, FabricFaultType.MovePrimary
            };

            switch (faultType)
            {
                //
                // random faults(permanent/transient) on random single replica
                // Faults on single replica are Enum 8 to Enum.MAX
                //
                case FabricFaultType.Random:
                    if (isVolatileService)
                    {
                        faultType = validVolatileFaults[random.Next(validVolatileFaults.Length)];
                    }
                    else
                    {
                        faultType = (FabricFaultType)this.random.Next(8, Enum.GetValues(typeof(FabricFaultType)).Cast<int>().Max() + 1);
                    }
                    return await this.InduceFaultViaFabricAsync(partitionSelector, replicaSelector, faultType, isVolatileService);

                //
                // random faults(permanent/transient) on secondary
                // Faults on secondary are Enum 8 to Enum 12
                //
                //
                case FabricFaultType.SecondaryRandom:
                    if (isVolatileService)
                    {
                        faultType = validVolatileFaults[this.random.Next(0, 3 + 1)];
                    }
                    else
                    {
                        faultType = (FabricFaultType)this.random.Next(8, 12 + 1);
                    }
                    replicaSelector = ReplicaSelector.RandomSecondaryOf(partitionSelector);
                    return await this.InduceFaultViaFabricAsync(partitionSelector, replicaSelector, faultType, isVolatileService);

                //
                // random faults(permanent/transient) on primary
                // Faults on primary are Enum 13 to Enum.MAX
                //
                case FabricFaultType.PrimaryRandom:
                    if (isVolatileService)
                    {
                        faultType = validVolatileFaults[this.random.Next(4, validVolatileFaults.Length)];
                    }
                    else
                    {
                        faultType = (FabricFaultType)this.random.Next(13, Enum.GetValues(typeof(FabricFaultType)).Cast<int>().Max() + 1);
                    }
                    replicaSelector = ReplicaSelector.PrimaryOf(partitionSelector);
                    return await this.InduceFaultViaFabricAsync(partitionSelector, replicaSelector, faultType, isVolatileService);

                //
                // faults on multiple replicas
                //
                case FabricFaultType.QuorumLoss:
                    await this.HandleExceptionsAsync(async () =>
                    {
                        Guid id = Guid.NewGuid();
                        await fabricClient.TestManager.StartPartitionQuorumLossAsync(id, partitionSelector, QuorumLossMode.QuorumReplicas, TimeSpan.FromSeconds(60));
                        PartitionQuorumLossProgress progress = await fabricClient.TestManager.GetPartitionQuorumLossProgressAsync(id);
                        while (progress.State == TestCommandProgressState.Running)
                        {
                            await Task.Delay(1000);
                            progress = await fabricClient.TestManager.GetPartitionQuorumLossProgressAsync(id);
                        }
                        Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "Completed Testability: InvokeQuorumLossAsync('{0}')", this.serviceName);
                    });
                    break;

                case FabricFaultType.PartialDataLoss:
                    await this.HandleExceptionsAsync(async () =>
                    {
                        Guid id = Guid.NewGuid();
                        await fabricClient.TestManager.StartPartitionDataLossAsync(id, partitionSelector, DataLossMode.PartialDataLoss);
                        PartitionDataLossProgress progress = await fabricClient.TestManager.GetPartitionDataLossProgressAsync(id);
                        while (progress.State == TestCommandProgressState.Running)
                        {
                            await Task.Delay(1000);
                            progress = await fabricClient.TestManager.GetPartitionDataLossProgressAsync(id);
                        }
                        Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "Completed Testability: PartialDataLoss('{0}')", this.serviceName);
                    });
                    break;

                case FabricFaultType.FullDataLoss:
                    await this.HandleExceptionsAsync(async () =>
                    {
                        Guid id = Guid.NewGuid();
                        await fabricClient.TestManager.StartPartitionDataLossAsync(id, partitionSelector, DataLossMode.FullDataLoss);
                        PartitionDataLossProgress progress = await fabricClient.TestManager.GetPartitionDataLossProgressAsync(id);
                        while (progress.State == TestCommandProgressState.Running)
                        {
                            await Task.Delay(1000);
                            progress = await fabricClient.TestManager.GetPartitionDataLossProgressAsync(id);
                        }
                        await this.WaitForPrimaryChange();
                        Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "Completed Testability: FullDataLoss('{0}')", this.serviceName);
                    });
                    break;

                case FabricFaultType.RestartPartition:
                    await this.HandleExceptionsAsync(async () =>
                    {
                        Guid id = Guid.NewGuid();
                        await fabricClient.TestManager.StartPartitionRestartAsync(id, partitionSelector, RestartPartitionMode.AllReplicasOrInstances, CancellationToken.None);
                        PartitionRestartProgress progress = await fabricClient.TestManager.GetPartitionRestartProgressAsync(id);
                        while (progress.State == TestCommandProgressState.Running)
                        {
                            await Task.Delay(1000);
                            progress = await fabricClient.TestManager.GetPartitionRestartProgressAsync(id);
                        }
                        await this.WaitForPrimaryChange();
                        Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "Completed Testability: RestartPartitionAsync('{0}')", this.serviceName);
                    });
                    break;

                //
                // faults on random single replica
                //

                case FabricFaultType.RemoveReplica:
                    Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "Calling Testability: RemoveReplica('{0}')", this.serviceName);

                    await this.HandleExceptionsAsync(async () =>
                    {
                        await fabricClient.FaultManager.RemoveReplicaAsync(replicaSelector, CompletionMode.Verify, true/*forceRemove*/);
                    });
                    break;

                case FabricFaultType.RestartReplica:
                    Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "Calling Testability: RestartReplica('{0}')", this.serviceName);

                    await this.HandleExceptionsAsync(async () =>
                    {
                        await fabricClient.FaultManager.RestartReplicaAsync(replicaSelector, CompletionMode.Verify);
                    });
                    break;

                case FabricFaultType.MoveSecondary:
                    await this.HandleExceptionsAsync(async () =>
                    {
                        await fabricClient.FaultManager.MoveSecondaryAsync(partitionSelector);
                        Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "Completed Testability: MoveSecondary('{0}')", this.serviceName);
                    });
                    break;

                case FabricFaultType.RestartNode:
                    await this.HandleExceptionsAsync(async () =>
                    {
                        await fabricClient.FaultManager.RestartNodeAsync(replicaSelector, CompletionMode.Verify);
                        Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "Completed Testability: RestartNode('{0}')", this.serviceName);
                    });
                    break;

                case FabricFaultType.KillServiceHost:
                    /*
                    await this.HandleExceptionsAsync(async () =>
                    {
                        await fabricClient.FaultManager.RestartDeployedCodePackageAsync(this.serviceName, replicaSelector, CompletionMode.Verify);
                        Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "Completed Testability: RestartPartitionAsync('{0}')", this.serviceName);
                    });
                    */
                    break;

                //
                // faults on primary replica
                //

                case FabricFaultType.RemovePrimary:
                    Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "Calling Testability: RestartReplica('{0}')", this.serviceName);

                    replicaSelector = ReplicaSelector.PrimaryOf(partitionSelector);
                    await this.HandleExceptionsAsync(async () =>
                    {
                        await fabricClient.FaultManager.RemoveReplicaAsync(replicaSelector, CompletionMode.Verify, true/*forceRemove*/);
                        await this.WaitForPrimaryChange();
                    });
                    break;

                case FabricFaultType.RestartPrimary:
                    Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "Calling Testability: RestartPrimary('{0}')", this.serviceName);

                    replicaSelector = ReplicaSelector.PrimaryOf(partitionSelector);
                    await this.HandleExceptionsAsync(async () =>
                    {
                        await fabricClient.FaultManager.RestartReplicaAsync(replicaSelector, CompletionMode.Verify);
                        await this.WaitForPrimaryChange();
                    });
                    break;

                case FabricFaultType.MovePrimary:
                    await this.HandleExceptionsAsync(async () =>
                    {
                        await fabricClient.FaultManager.MovePrimaryAsync(partitionSelector);
                        await this.WaitForPrimaryChange();
                        Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "Completed Testability: MovePrimary('{0}')", this.serviceName);
                    });
                    break;
            }

            return faultType;
        }

        /// <summary>Induces a fault using the Windows Fabric testability capabilities</summary>
        /// <param name="faultType">Type of fault to induce</param>
        /// <returns>Task for the work of inducing the fault</returns>
        private async Task<FabricFaultType> InduceFaultViaFabricAsync(FabricFaultType faultType, bool isVolatileService)
        {
            // Randomly choose a partition
            PartitionSelector randomPartitionSelector = PartitionSelector.RandomOf(this.serviceName);
            ReplicaSelector replicaSelector = ReplicaSelector.RandomOf(randomPartitionSelector);
            return await InduceFaultViaFabricAsync(randomPartitionSelector, replicaSelector, faultType, isVolatileService);
        }

        /// <summary>
        /// Does a bunch of work handling exception and re-resolving (up to ResolveRetryCount).
        /// </summary>
        /// <param name="amountOfWork">Amount of work.</param>
        /// <param name="parallelWorkSize">Units of work to be done in parallel. </param>
        /// <param name="parameters">Parameters for controlling the work</param>
        /// <returns>Task that represents doing bunch of work.</returns>
        private async Task DoBunchOfWorkAsync(
            int amountOfWork,
            int parallelWorkSize,
            Dictionary<string, string> parameters)
        {
            await this.DoBunchOfWorkAsync(WorkloadIdentifier.LegacyWorkload, parameters, amountOfWork, parallelWorkSize, this.sendTimeout, this.retryCall);
        }

        /// <summary>
        /// Does a bunch of work handling exception and re-resolving (up to ResolveRetryCount).
        /// </summary>
        /// <param name="workloadId">Id of workload to run</param>
        /// <param name="parameters">Set of parameters that affect the workload</param>
        /// <param name="amountOfWork">Amount of work.</param>
        /// <param name="parallelWorkSize">Units of work to be done in parallel. </param>
        /// <param name="sendTimeout">Timeout waiting for call to complete</param>
        /// <returns>Task that represents doing bunch of work and a dictionary with results.</returns>
        private async Task<Dictionary<string, string>> DoBunchOfWorkAsync(
            WorkloadIdentifier workloadId,
            Dictionary<string, string> parameters,
            int amountOfWork,
            int parallelWorkSize,
            TimeSpan sendTimeout,
            bool retryCall)
        {
            Dictionary<string, string> results = new Dictionary<string, string>();
            int numberOfOperationExecuted = 0;

            Stopwatch stopwatch = new Stopwatch();

            for (int i = 0; i < this.retryCount; i++)
            {
                ResolvedResult resolvedResult = await this.ResolveAsync();
                if (resolvedResult == null)
                {
                    MCFUtils.FailClientAssert(false, "Unable to resolve service");
                }

                while (numberOfOperationExecuted < amountOfWork)
                {
                    bool succeeded = await this.HandleExceptionsAsync(async () =>
                    {
                        Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "DoBunchOfWorkAsync: Calling DoWorkAsync with {0}", parallelWorkSize);

                        stopwatch.Restart();
                        await DoWorkXAsync(resolvedResult.PrimaryEndpoint);

                        Console.WriteLine("\t{0}: DoBunchOfWork Completed. Units: {1} Time: {2} ms Status: {3}.", DateTime.UtcNow, parallelWorkSize, stopwatch.ElapsedMilliseconds, this.statusCode);

                        this.CheckStatusAndThrowOnFailure();

                        numberOfOperationExecuted += parallelWorkSize;

                        Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "DoBunchOfWorkAsync: DoWorkAsync completed");
                    }, retryCall);

                    if (!succeeded)
                    {
                        break;
                    }
                }

                if (numberOfOperationExecuted >= amountOfWork)
                {
                    return results;
                }

                await Task.Delay(BasicWorkTest.BackOffTimeInMs);
            }

            string message = string.Format(
                                "After {0} retries, enough progress was not made: {1} < {2}",
                                this.retryCount,
                                numberOfOperationExecuted,
                                amountOfWork);

            MCFUtils.FailClientAssert(numberOfOperationExecuted >= amountOfWork, message);
            return results;
        }

        /// <summary>Performs a Fabric Client action and catches and handles any exceptions appropriately</summary>
        /// <param name="action">action to perform with exception handling</param>
        /// <returns>true if successful</returns>
        private async Task<bool> HandleExceptionsAsync(Func<Task> action, bool retryCall = true)
        {
            try
            {
                await action();
                return true;
            }
            catch (FaultException<FabricObjectClosedException> exception)
            {
                Console.WriteLine("Expected FabricObjectClosedException: {0}; {1}; {2}", exception.GetType(), exception.Message, exception.StackTrace);
                Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "Expected FabricObjectClosedException (Re-resolve): {0}; {1}; {2}", exception.GetType(), exception.Message, exception.StackTrace);
                if (!retryCall)
                {
                    throw;
                }

                return false;
            }
            catch (FaultException<ObjectDisposedException> exception)
            {
                Console.WriteLine("Expected ObjectDisposedException: {0}; {1}; {2}", exception.GetType(), exception.Message, exception.StackTrace);
                Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "Expected ObjectDisposedException (Re-resolve): {0}; {1}; {2}", exception.GetType(), exception.Message, exception.StackTrace);
                if (!retryCall)
                {
                    throw;
                }

                return false;
            }
            catch (FaultException<InvalidOperationException> exception)
            {
                Console.WriteLine("Expected InvalidOperationException: {0}; {1}; {2}", exception.GetType(), exception.Message, exception.StackTrace);
                Tracing.TraceEvent(TraceEventType.Warning, 0, partitionIdentifier + ": " + "Expected InvalidOperationException: {0}; {1}; {2}", exception.GetType(), exception.Message, exception.StackTrace);
                throw;
            }
            catch (FaultException<FabricNotPrimaryException> exception)
            {
                Console.WriteLine("Expected FabricNotPrimaryException");
                Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "DoBunchOfWorkAsync: Expected FabricNotPrimaryException (Re-resolve): {0}; {1}; {2}", exception.GetType(), exception.Message, exception.StackTrace);
                if (!retryCall)
                {
                    throw;
                }

                return false;
            }
            catch (FaultException<OperationCanceledException> exception)
            {
                Console.WriteLine("Expected OperationCanceledException: {0}; {1}; {2}", exception.GetType(), exception.Message, exception.StackTrace);
                Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "DoBunchOfWorkAsync: Expected OperationCanceledException (Re-resolve): {0}; {1}; {2}", exception.GetType(), exception.Message, exception.StackTrace);
                if (!retryCall)
                {
                    throw;
                }

                return false;
            }
            catch (FaultException<Exception> exception)
            {
                Console.WriteLine("Unexpected FaultException<Exception>: {0}", exception.ToString());
                Tracing.TraceEvent(TraceEventType.Critical, 0, partitionIdentifier + ": " + "DoBunchOfWorkAsync: Unexpected FaultException<Exception>: {0}", exception.ToString());

                MCFUtils.FailClientAssert(false, partitionIdentifier + ": " + "Unexpected Exception: " + exception.Message);
                throw;
            }
            catch (FaultException exception)
            {
                Console.WriteLine("Unexpected FaultException: {0}; {1}; {2}", exception.GetType(), exception.Message, exception.StackTrace);
                Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "DoBunchOfWorkAsync: Unexpected FaultException: {0}; {1}; {2}", exception.GetType(), exception.Message, exception.StackTrace);
                if (!retryCall)
                {
                    throw;
                }

                return false;
            }
            catch (OperationCanceledException exception)
            {
                Console.WriteLine("Expected OperationCanceledException: {0}; {1}; {2}", exception.GetType(), exception.Message, exception.StackTrace);
                Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "DoBunchOfWorkAsync: Expected OperationCanceledException (Re-resolve): {0}; {1}; {2}", exception.GetType(), exception.Message, exception.StackTrace);
                if (!retryCall)
                {
                    throw;
                }

                return false;
            }
            catch (CommunicationException exception)
            {
                Console.WriteLine("Expected CommunicationException: {0}; {1}; {2}", exception.GetType(), exception.Message, exception.StackTrace);
                Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "DoBunchOfWorkAsync: Expected CommunicationException (Re-resolve): {0}; {1}; {2}", exception.GetType(), exception.Message, exception.StackTrace);
                if (!retryCall)
                {
                    throw;
                }

                return false;
            }
            catch (TimeoutException exception)
            {
                Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "DoBunchOfWorkAsync: Expected TimeoutException (Re-resolve): {0}; {1}; {2}", exception.GetType(), exception.Message, exception.StackTrace);
                if (!retryCall)
                {
                    throw;
                }

                return false;
            }
            catch (HttpRequestException exception)
            {
                Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "DoBunchOfWorkAsync: Expected HttpException (Re-resolve): {0}; {1}; {2}", exception.GetType(), exception.Message, exception.StackTrace);
                if (!retryCall)
                {
                    throw;
                }

                return false;
            }
            catch (Exception exception)
            {
                Tracing.TraceEvent(TraceEventType.Critical, 0, partitionIdentifier + ": " + "DoBunchOfWorkAsync: Unexpected Exception: {0}", exception.ToString());

                MCFUtils.FailClientAssert(false, partitionIdentifier + ": " + "Unexpected Exception: " + exception.Message);
                throw;
            }
        }

        /// <summary>
        /// Wait For Replica Set Size To Be Up.
        /// </summary>
        /// <param name="replicaSetSize">Replica set size.</param>
        /// <returns>Asynchronous task.</returns>
        private async Task WaitForReplicaSetSizeToBeUpAsync(int replicaSetSize)
        {
            int remainingRetries = this.retryCount;
            int endpointCount = 0;

            while (remainingRetries > 0)
            {
                ResolvedResult resolvedResult = await this.ResolveAsync();
                endpointCount = (resolvedResult != null) ? resolvedResult.Partition.Endpoints.Count : 0;

                if (endpointCount >= replicaSetSize)
                {
                    Tracing.TraceEvent(
                         TraceEventType.Information,
                         0,
                         partitionIdentifier + ": " + "WaitForReplicaSetSizeToBeUpAsync: targetReplicaSetSize({0}) ==  Number of Replicas {1}",
                         replicaSetSize,
                         resolvedResult.Partition.Endpoints.Count);

                    return;
                }
                else
                {
                    Tracing.TraceEvent(
                         TraceEventType.Warning,
                         0,
                         partitionIdentifier + ": " + "WaitForReplicaSetSizeToBeUpAsync: Failed targetReplicaSetSize({0}) ==  Number of Replicas {1}",
                         replicaSetSize,
                         resolvedResult.Partition.Endpoints.Count);
                }

                await Task.Delay(BasicWorkTest.BackOffTimeInMs);
                remainingRetries--;
            }

            string message = string.Format(partitionIdentifier + ": " + "After {0} retries, the replicaSetSize is still expected: {1} < {2}", this.retryCount, endpointCount, replicaSetSize);

            Tracing.TraceEvent(TraceEventType.Error, 0, "WaitForReplicaSetSizeToBeUpAsync:{0}", message);

            MCFUtils.FailClientAssert(false, message);
        }

        /// <summary>
        /// Checks data consistency between the replicas in the same partition.
        /// </summary>
        /// <returns>Asynchronous data consistency check returns a IComparable progress.</returns>
        private async Task<IComparable> TestDataConsistencyAsync()
        {
            int remainingRetries = this.retryCount;

            Stopwatch stopWatch = Stopwatch.StartNew();
            for (int iteration = 1; remainingRetries > 0; iteration++)
            {
                await Task.Delay(BasicWorkTest.BackOffTimeInMs);

                Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "TestDataConsistencyAsync iteration {0}", iteration);

                ResolvedResult resolvedResult = await this.ResolveAsync();
                if (resolvedResult == null)
                {
                    MCFUtils.FailClientAssert(false, "Unable to resolve service");
                }

                if (resolvedResult.Partition.Endpoints.Count < this.statefulServiceDescription.TargetReplicaSetSize)
                {
                    Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "TestDataConsistencyAsync: Failed Data Consistency Check: Replica Set Count {0}, Target {1}", resolvedResult.Partition.Endpoints.Count, this.statefulServiceDescription.TargetReplicaSetSize);

                    continue;
                }

                ISet<IComparable> progresses = new SortedSet<IComparable>();
                int numberOfProgressesAcquired = 0;
                for (int i = 0; i < resolvedResult.Endpoints.Count; i++)
                {
                    string endpoint = resolvedResult.Endpoints.ElementAt(i);
                    try
                    {
                        IComparable tmp = await this.GetWorkProgress(endpoint);
                        progresses.Add(tmp);
                        numberOfProgressesAcquired++;
                    }
                    catch (TimeoutException e)
                    {
                        Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "TestDataConsistencyAsync:{0}; {1}; {2}", e.GetType(), e.Message, e.StackTrace);

                        continue;
                    }
                    catch (HttpRequestException e)
                    {
                        Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "TestDataConsistencyAsync:{0}; {1}; {2}", e.GetType(), e.Message, e.StackTrace);

                        continue;
                    }
                    catch (OperationCanceledException e)
                    {
                        Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "TestDataConsistencyAsync:{0}; {1}; {2}", e.GetType(), e.Message, e.StackTrace);

                        continue;
                    }
                    catch (CommunicationException e)
                    {
                        // Handles InvalidOperationException and EndpointNotFoundException.
                        Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "TestDataConsistencyAsync:{0}; {1}; {2}", e.GetType(), e.Message, e.StackTrace);
                        if (e.InnerException != null)
                        {
                            Tracing.TraceEvent(TraceEventType.Information, 0, partitionIdentifier + ": " + "TestDataConsistencyAsync:InnerException:{0}; {1}; {2}", e.InnerException.GetType(), e.InnerException.Message, e.InnerException.StackTrace);
                        }

                        continue;
                    }
                    catch (Exception e)
                    {
                        Tracing.TraceEvent(TraceEventType.Critical, 0, partitionIdentifier + ": " + "TestDataConsistencyAsync:{0}; {1}; {2}", e.GetType(), e.Message, e.StackTrace);

                        MCFUtils.FailClientAssert(false, e.Message);
                    }
                }

                if (progresses.Count == 1 && numberOfProgressesAcquired >= this.statefulServiceDescription.TargetReplicaSetSize)
                {
                    foreach (IComparable comparableProgress in progresses)
                    {
                        Tracing.TraceEvent(
                             TraceEventType.Information,
                             0,
                             partitionIdentifier + ": " + "TestDataConsistencyAsync: Passed, # Progresses: {0}, # Replicas: {1}, Target #: {2}",
                             progresses.Count,
                             numberOfProgressesAcquired,
                             this.statefulServiceDescription.TargetReplicaSetSize);

                        Console.WriteLine(
                            "\t{0} Success: Replica Set size {1}, Progress {2}, Time: {3} ms",
                            DateTime.UtcNow,
                            resolvedResult.Partition.Endpoints.Count,
                            comparableProgress,
                            stopWatch.ElapsedMilliseconds);

                        return comparableProgress;
                    }
                }
                else
                {
                    Tracing.TraceEvent(
                         TraceEventType.Warning,
                         0,
                         partitionIdentifier + ": " + "TestDataConsistencyAsync: Failed Data Consistency Check: Number of different progresses = {0} (should be 1), Number Of Replicas = {1} (should be equal to Target), Target {2}",
                         progresses.Count,
                         numberOfProgressesAcquired,
                         this.statefulServiceDescription.TargetReplicaSetSize);

                    int index = 0;

                    foreach (IComparable comparableProgress in progresses)
                    {
                        Tracing.TraceEvent(
                             TraceEventType.Information,
                             0,
                             partitionIdentifier + ": " + "Failed Progress {0}: {1}",
                             index++,
                             comparableProgress);
                    }
                }

                remainingRetries--;
            }

            Tracing.TraceEvent(TraceEventType.Critical, 0, partitionIdentifier + ": " + "TestDataConsistencyAsync: Code should have never come here.");

            MCFUtils.FailClientAssert(false, partitionIdentifier + ": " + "TestDataConsistencyAsync: Code should never come here.");
            return null;
        }

        private async Task DoWorkXAsync(string endpoint)
        {
            await DoWork(endpoint);
            while (true)
            {
                await Task.Delay(1000 * 60);
                var response = await CheckWrokStatus(endpoint);
                if (response.WorkStatus == WorkStatus.Done)
                {
                    Console.WriteLine("Work Done. StatusCode: {0}", response.StatusCode);
                    statusCode = response.StatusCode;
                    break;
                }
                else if (response.WorkStatus == WorkStatus.NotStarted)
                {
                    Console.WriteLine("Work NotStarted. StatusCode: {0}", response.StatusCode);
                    await DoWork(endpoint);
                }
                else if (response.WorkStatus == WorkStatus.Faulted)
                {
                    // If the serivce return Faulted, should store the exception code and stop this workload.
                    Console.WriteLine("Work Faulted. StatusCode: {0}", response.StatusCode);
                    statusCode = response.StatusCode;
                    break;
                }
                else if (response.WorkStatus == WorkStatus.InProgress)
                {
                    Console.WriteLine("Work InProgress");
                }
                else if (response.WorkStatus == WorkStatus.Unknown)
                {
                    Console.WriteLine("Work Status Unknown");
                }
                else
                {
                    MCFUtils.FailClientAssert(false, "Invalid work status. Bad things happended!");
                }
            }
        }

        private async Task<CheckWorkStatusResponse> CheckWrokStatus(string endpoint)
        {
            Console.WriteLine("Sending check work status request to {0}", endpoint);
            var request = new HttpClientRequest();
            request.Operation = HttpClientAction.CheckWorkStatus;
            return await SendClientRequest<HttpClientRequest, CheckWorkStatusResponse>(endpoint, request);
        }

        private async Task DoWork(string endpoint)
        {
            Console.WriteLine("Sending schedule work request to {0}", endpoint);
            var request = new HttpClientRequest();
            request.Operation = HttpClientAction.DoWork;
            await SendClientRequest<HttpClientRequest, DoWorkResponse>(endpoint, request);
        }

        private async Task<IComparable> GetWorkProgress(string endpoint)
        {
            Console.WriteLine("Sending get work progress request to {0}", endpoint);
            var request = new HttpClientRequest();
            request.Operation = HttpClientAction.GetProgress;
            var response = await SendClientRequest<HttpClientRequest, GetProgressResponse>(endpoint, request);

            return new TestableServiceProgress(
                new Epoch(response.DataLossNumber, response.ConfigurationNumber),
                response.LastCommittedSequenceNumber,
                response.CustomProgress);
        }

        private static readonly JsonSerializerSettings SerializerSettings = new JsonSerializerSettings()
        {
            NullValueHandling = NullValueHandling.Ignore,
            Converters = new List<JsonConverter>
            {
                new StringEnumConverter { CamelCaseText = true },
            }
        };

        private async Task<ReponseType> SendClientRequest<RequestType, ReponseType>(
            string endpoint,
            RequestType request)
        {
            var payload = JsonConvert.SerializeObject(request, new JsonSerializerSettings());

            var response = await SendRequest(endpoint, payload);

            var responseString = await response.Content.ReadAsStringAsync();

            return JsonConvert.DeserializeObject<ReponseType>(responseString);
        }

        private async Task<HttpResponseMessage> SendRequest(string address, string payloadJson)
        {
            HttpClient httpClient = new HttpClient();

            httpClient.DefaultRequestHeaders.Accept.Add(new System.Net.Http.Headers.MediaTypeWithQualityHeaderValue("application/json"));

            HttpRequestMessage requestMessage = new HttpRequestMessage(HttpMethod.Post, address)
            {
                Content = new StringContent(payloadJson, Encoding.UTF8, "application/json")
            };

            var response = await httpClient.SendAsync(requestMessage);
            if (!response.IsSuccessStatusCode)
            {
                throw new CommunicationException("Failed to send HTTP request to client, responseMessage: " + response.ToString());
            }

            return response;
        }

        private void CheckStatusAndThrowOnFailure()
        {
            switch (this.statusCode)
            {
                case 0:
                    break;

                case -532472889:// exception SF_STATUS_NOT_READY from native
                case -532472890:// exception SF_STATUS_NOT_PRIMARY from native
                case -532472708:// exception SF_STATUS_NOT_READABLE from native
                    throw new FaultException<FabricNotPrimaryException>(
                        new FabricNotPrimaryException("Service throw SF_STATUS_NOT_PRIMARY exception, the corresponding error code is -532472890."));

                case -532472834:// exception SF_STATUS_OBJECT_CLOSED from native
                    throw new FaultException<FabricObjectClosedException>(
                        new FabricObjectClosedException("Service throw SF_STATUS_OBJECT_CLOSED exception, the corresponding error code is -532472834."));

                case -532479999:// exception SF_STATUS_OBJECT_DISPOSED from native
                    throw new FaultException<ObjectDisposedException>(
                        new ObjectDisposedException("Service throw SF_STATUS_OBJECT_DISPOSED exception, the corresponding error code is -532479999."));

                case -532472835:// exception SF_STATUS_INVALID_OPERATION from native
                    throw new FaultException<InvalidOperationException>(
                        new InvalidOperationException("Service throw SF_STATUS_INVALID_OPERATION exception, the corresponding error code is -532472835."));

                case -1073741536:// exception STATUS_CANCELLED from native
                    throw new FaultException<OperationCanceledException>(
                        new OperationCanceledException("Service throw STATUS_CANCELLED exception, the corresponding error code is -1073741536."));

                default:
                    throw new InvalidOperationException("Service throw UnKnown exception.");
            }
        }
        #endregion
    }

}