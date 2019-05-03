// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace NativeReplicatorHTTPClient
{
    using MS.Test.WinFabric.UserClients;
    using System;
    using System.Collections.Generic;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Diagnostics;
    using MCF.TestableService;

    [UserClient("This client starts a v2 native replicator workload through the http channel")]
    public class HTTPClient
    {
        Program program;
        private ScriptTestClientConfig config;
        string ServiceName { get; set; }
        readonly TimeSpan DefaultConnectionTimeout = TimeSpan.FromSeconds(10);

        public HTTPClient(
            Dictionary<string, string> parameters, 
            ScriptTestClientConfig config)
        {
            TraceEventType traceDisplayLevel = TraceEventType.Error;

            if (parameters.ContainsKey("serviceName"))
            {
                this.ServiceName = parameters["serviceName"];
            }

            if (parameters.ContainsKey("traceDisplayLevel"))
            {
                string displayLevel = parameters["traceDisplayLevel"];

                if (!Enum.TryParse<TraceEventType>(displayLevel, out traceDisplayLevel))
                {
                    Console.WriteLine("Unknown traceDisplayLevel {0}. Use Critical, Error, Warnoing, Information or Verbose. Default to Error.", displayLevel);
                }
            }

            Tracing.InitializeTracing(config.TestCaseDataRoot, traceDisplayLevel);

            this.config = config;
        }

        [UserClientWorkload("Starts the test workload")]
        public async Task StartTest(
            Dictionary<string, string> parameters,
            string taskId,
            CancellationToken cancellationToken)
        {
            Dictionary<string, string> results;
            string serviceName = this.ServiceName;
            string namedPartitionKey = "Default";
            long int64PartitionKey = 1;
            TimeSpan sendTimeout = TimeSpan.FromMinutes(1);
            int retryCount = 64;
            bool isDemo = false;
            int loopCount = 20;
            WorkloadIdentifier workloadId = WorkloadIdentifier.LegacyWorkload;
            bool runForever = false;
            bool useTestability = true;
            bool retryCall = true;
            ServicePartitionType partitionType = ServicePartitionType.Singleton;
            bool isPAAS = false;
            ClusterEnvironment clusterEnvironment = ClusterEnvironment.Windows;

            if (parameters.ContainsKey("workloadId"))
            {
                workloadId = (WorkloadIdentifier)Enum.Parse(typeof(WorkloadIdentifier), parameters["workloadId"], true);
            }

            if (parameters.ContainsKey("serviceName"))
            {
                serviceName = parameters["serviceName"];
            }

            if (parameters.ContainsKey("runForever"))
            {
                runForever = bool.Parse(parameters["runForever"]);
            }

            if (parameters.ContainsKey("retryCall"))
            {
                retryCall = bool.Parse(parameters["retryCall"]);
            }

            if (parameters.ContainsKey("isDemo"))
            {
                isDemo = bool.Parse(parameters["isDemo"]);
            }

            if (parameters.ContainsKey("loopCount"))
            {
                loopCount = int.Parse(parameters["loopCount"]);
            }

            if (parameters.ContainsKey("retryCount"))
            {
                retryCount = int.Parse(parameters["retryCount"]);
            }

            if (parameters.ContainsKey("sendTimeout"))
            {
                sendTimeout = TimeSpan.FromMinutes(int.Parse(parameters["sendTimeout"]));
            }

            if (parameters.ContainsKey("useTestability"))
            {
                useTestability = bool.Parse(parameters["useTestability"]);
            }

            if (parameters.ContainsKey("namedPartitionKey"))
            {
                if (partitionType != ServicePartitionType.Singleton)
                {
                    throw new InvalidOperationException("Ambiguous partition type - Either namedPartitionKey or int64PartitionKey should be specified");
                }

                namedPartitionKey = parameters["namedPartitionKey"];
                partitionType = ServicePartitionType.Named;
            }

            if (parameters.ContainsKey("int64PartitionKey"))
            {
                if (partitionType != ServicePartitionType.Singleton)
                {
                    throw new InvalidOperationException("Ambiguous partition type - Either namedPartitionKey or int64PartitionKey should be specified");
                }

                int64PartitionKey = long.Parse(parameters["int64PartitionKey"]);
                partitionType = ServicePartitionType.Uniform;
            }

            // If 'PAAS' is set, that means the service is running on PAAS cluster, which also means the client 
            // cannot talk directly to the service because Azure forbids that, it needs to use the ReverseProxy 
            // service to communicate with the service.
            if (parameters.ContainsKey("PAAS"))
            {
                isPAAS = bool.Parse(parameters["PAAS"]);
            }

            if ((workloadId == WorkloadIdentifier.LegacyWorkload) && runForever)
            {
                Console.WriteLine("WARNING: runForever parameter ignored when workloadId is 0");
            }

            if (parameters.ContainsKey("clusterEnvironment"))
            {
                Enum.TryParse(parameters["clusterEnvironment"], out clusterEnvironment);
            }

            Console.WriteLine("Cluster is: " + clusterEnvironment);

            Console.WriteLine("Starting Workload");
            //ConsoleWriteDictionary(parameters);

            Console.WriteLine("PartitionType: {0}", partitionType.ToString());
            //ConsoleWriteGatewayAddresses();

            program = new Program();

            Console.WriteLine("GatewayAddress: {0}", config.GatewayAddresses);
            results = await program.MainWorkload(
                taskId, 
                serviceName, 
                config.GatewayAddresses,
                config.HttpApplicationGatewayAddresses, 
                config.ViaForwardingAddress,
                config.ClientCredentials,
                workloadId,
                parameters,
                partitionType, 
                namedPartitionKey, 
                int64PartitionKey, 
                sendTimeout, 
                retryCall, 
                retryCount, 
                isDemo, 
                useTestability, 
                loopCount,
                runForever,
                isPAAS,
                clusterEnvironment,
                cancellationToken);

            Console.WriteLine("Workload {0} Results", workloadId);
            //ConsoleWriteDictionary(results);
        }

        [UserClientCheckProgress]
        public string CheckProgress(string taskId)
        {
            return string.Empty;
        }

        public enum ClusterEnvironment
        {
            Linux,
            Windows,
        }
    }
}