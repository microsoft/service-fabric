// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace NativeReplicatorHTTPClient
{
    using System;
    using System.Diagnostics;
    using System.Fabric;
    using System.IO;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Collections.Generic;
    using MCF.TestableService;

    /// <summary>
    /// Test Client program
    /// </summary>
    public class Program
    {
        /// <summary>
        /// Name of the TaskId specified by Scripttest. This is a unique string used to name the 
        /// specific instance of the workload.
        /// </summary>
        private string ScriptTestTaskId;

        /// <summary>
        /// Named partition key.
        /// </summary>
        private string namedPartitionKey = "Default";

        /// <summary>
        /// Long partition key.
        /// </summary>
        private long int64PartitionKey = 1;

        /// <summary>
        /// Send Timeout
        /// </summary>
        private TimeSpan sendTimeout = TimeSpan.FromMinutes(1);

        private static string DefaultLocalGatewayAddress = @"localhost:19000";

        /// <summary>
        /// Send Timeout
        /// </summary>
        private string[] gatewayAddresses = new string[] { DefaultLocalGatewayAddress };

        /// <summary>
        /// Retry Count.
        /// </summary>
        private int retryCount = 64;

        /// <summary>
        /// Back off interval.
        /// </summary>
        private TimeSpan backOffInterval = TimeSpan.FromSeconds(1);

        /// <summary>
        /// The Main of the client.
        /// </summary>
        /// <param name="args">Arguments to the program.</param>
        public async Task<Dictionary<string, string>> MainWorkload(
            string TaskId,
            string ServiceName,
            string[] GatewayAddresses,
            string[] HttpGatewayAddresses,
            Uri RouterAddress,
            SecurityCredentials credentials,
            WorkloadIdentifier WorkloadId,
            Dictionary<string, string> Parameters,
            ServicePartitionType PartitionType,
            string NamedPartitionKey,
            long Int64PartitionKey,
            TimeSpan SendTimeout,
            bool RetryCall,
            int RetryCount,
            bool isDemo,
            bool useTestability,
            int loopCount,
            bool RunForever,
            bool isPAAS,
            HTTPClient.ClusterEnvironment clusterEnvironment,
            CancellationToken cancellationToken
            )
        {
            Dictionary<string, string> results = new Dictionary<string, string>();

            ScriptTestTaskId = TaskId;

            gatewayAddresses = GatewayAddresses;
            namedPartitionKey = NamedPartitionKey;
            int64PartitionKey = Int64PartitionKey;
            sendTimeout = SendTimeout;
            retryCount = RetryCount;

            BasicWorkTest basicWorkTest;

            Uri serviceName = new Uri(ServiceName);

            // Run Basic Workload test.
            switch (PartitionType)
            {
                case ServicePartitionType.Singleton:
                    Tracing.TraceEvent(TraceEventType.Information, 0, "Creating Test Manager: Singleton Service Name: {0}", serviceName.ToString());

                    basicWorkTest = new BasicWorkTest(
                        serviceName, 
                        gatewayAddresses, 
                        HttpGatewayAddresses, 
                        RouterAddress, 
                        credentials, 
                        sendTimeout, 
                        retryCount, 
                        isDemo, 
                        useTestability, 
                        isPAAS,
                        clusterEnvironment,
                        RetryCall);
                    break;
                case ServicePartitionType.Uniform:
                    Tracing.TraceEvent(
                        TraceEventType.Information,
                        0,
                        "Creating Test Manager: Service Name: {0}, partition key: {1}.",
                        serviceName.ToString(),
                        int64PartitionKey);

                    basicWorkTest = new BasicWorkTest(
                        serviceName, 
                        int64PartitionKey, 
                        gatewayAddresses, 
                        HttpGatewayAddresses, 
                        RouterAddress, 
                        credentials, 
                        sendTimeout, 
                        retryCount, 
                        isDemo, 
                        useTestability, 
                        isPAAS,
                        clusterEnvironment,
                        RetryCall);
                    break;
                case ServicePartitionType.Named:
                    Tracing.TraceEvent(
                        TraceEventType.Information,
                        0,
                        "Creating Test Manager: Service Name: {0}, partition key: {1}.",
                        serviceName.ToString(),
                        namedPartitionKey);

                    basicWorkTest = new BasicWorkTest(
                        serviceName, 
                        namedPartitionKey,
                        gatewayAddresses, 
                        HttpGatewayAddresses, 
                        RouterAddress, 
                        credentials, 
                        sendTimeout,
                        retryCount, 
                        isDemo, 
                        useTestability, 
                        isPAAS,
                        clusterEnvironment,
                        RetryCall);
                    break;
                default:
                    Tracing.TraceEvent(
                        TraceEventType.Critical,
                        0,
                        "Main: Unexpected type {0} when creating the test manager.",
                        PartitionType);

                    Console.Error.WriteLine("MCF.Client.MainWorkload: Internal Error: Unexpected type {0} wehen creating the test manager.",
                        PartitionType);
                    return (results);
            }

            await basicWorkTest.InitializeAsync();

            if (WorkloadId == WorkloadIdentifier.LegacyWorkload)
            {
                //
                // Workload 0 is special in that it runs the workload and failover tests
                // cases as the legacy ServiceTestClient did
                //
                await basicWorkTest.RunAsync(loopCount, cancellationToken, Parameters);
            }
            else if (WorkloadId == WorkloadIdentifier.LegacyTransientFaultWorkload)
            {
                await basicWorkTest.RunTransientFaultWorkloadAsync(loopCount, cancellationToken);
            }
            else
            {
                //
                // Other workloads are dependent upon how the service implements them
                //
                results = await basicWorkTest.RunSpecificWorkloadAsync(WorkloadId, RunForever, Parameters, sendTimeout, RetryCall, cancellationToken);
            }

            return (results);
        }
    }

    public static class Tracing
    {
        /// <summary>
        /// Trace File Name Format
        /// </summary>
        private const string TraceFileNameFormat = "TestClient.{0}-{1}-{2}-{3}.traces.trace";

        /// <summary>
        /// Trace Header Line
        /// </summary>
        private const string TraceHeaderLine = "TraceSource,TraceEventType,EventId,Message,,ProcessId,,ThreadId,DateTime,TimeStamp";

        public static TraceEventType ConsoleDisplayLevel
        {
            get;
            private set;
        }

        /// <summary>
        /// Gets the trace source.
        /// </summary>
        public static TraceSource ProgramTraceSource
        {
            get;
            private set;
        }


        /// <summary>
        /// Write a trace event
        /// </summary>
        public static void TraceEvent(
            TraceEventType eventType,
            int id,
            string format,
            params Object[] args)
        {
            string message = string.Format(format, args);
            string messageWithTime = string.Format("{0}: {1}", DateTime.UtcNow, message);

            if (eventType <= ConsoleDisplayLevel)
            {
                ConsoleColor color;

                if (eventType == TraceEventType.Critical)
                {
                    color = ConsoleColor.Red;
                }
                else if (eventType == TraceEventType.Error)
                {
                    color = ConsoleColor.DarkRed;
                }
                else if (eventType == TraceEventType.Warning)
                {
                    color = ConsoleColor.Yellow;
                }
                else if (eventType == TraceEventType.Information)
                {
                    color = ConsoleColor.Cyan;
                }
                else
                {
                    color = ConsoleColor.White;
                }

                Console.ForegroundColor = color;
                Console.WriteLine(messageWithTime);
                Console.ResetColor();
            }

            ProgramTraceSource.TraceEvent(eventType, id, messageWithTime);
        }

        /// <summary>
        /// Initialize tracing.
        /// </summary>
        public static void InitializeTracing(string TestCaseDataRoot = ".\\",
                                             TraceEventType ConsoleWriteLevel = TraceEventType.Error)
        {
            ConsoleDisplayLevel = ConsoleWriteLevel;

            string fileName = Path.Combine(TestCaseDataRoot, string.Format(TraceFileNameFormat, DateTime.Now.Year, DateTime.Now.Month, DateTime.Now.Day, Guid.NewGuid()));
            ProgramTraceSource = new TraceSource(fileName);

            FileStream traceFile = new FileStream(fileName, FileMode.Create, FileAccess.Write, FileShare.Read);

            using (StreamWriter sw = new StreamWriter(traceFile))
            {
                sw.WriteLine(TraceHeaderLine);
            }

            traceFile = new FileStream(fileName, FileMode.Append, FileAccess.Write, FileShare.Read);

            ProgramTraceSource.Switch = new SourceSwitch("mySwitch1");
            ProgramTraceSource.Switch.Level = SourceLevels.All;

            DelimitedListTraceListener dltl = new DelimitedListTraceListener(traceFile);
            dltl.Delimiter = ",";
            dltl.TraceOutputOptions = TraceOptions.ProcessId | TraceOptions.ThreadId | TraceOptions.DateTime | TraceOptions.Timestamp;

            ProgramTraceSource.Listeners.Add(dltl);

            Trace.AutoFlush = true;
        }

        /// <summary>
        /// Close tracing.
        /// </summary>
        public static void CloseTracing()
        {
            ProgramTraceSource.Close();
        }
    }
}