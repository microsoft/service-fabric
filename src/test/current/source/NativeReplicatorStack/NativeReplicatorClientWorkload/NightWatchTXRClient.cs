// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace NativeReplicatorClientWorkload
{
    using Microsoft.ServiceFabric.Services.Client;
    using Microsoft.ServiceFabric.Services.Remoting.Client;
    using MS.Test.WinFabric.UserClients;
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Linq;
    using System.ServiceModel;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Net.Http;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Serialization;
    using Newtonsoft.Json.Converters;

    [UserClient("This client starts a v2 native replicator workload through the http channel")]
    public class NightWatchTXRClient
    {
        private FabricClient fabricClient;
        private ScriptTestClientConfig config;

        private static readonly JsonSerializerSettings SerializerSettings = new JsonSerializerSettings()
        {
            NullValueHandling = NullValueHandling.Ignore,
            Converters = new List<JsonConverter>
            {
                new StringEnumConverter { CamelCaseText = true },
            }
        };

        public NightWatchTXRClient(Dictionary<string, string> parameters, ScriptTestClientConfig config)
        {
            this.config = config;
        }

        [UserClientWorkload("Starts the test workload")]
        public async Task StartTest(
            Dictionary<string, string> parameters,
            string taskId,
            CancellationToken cancellationToken)
        {
            Console.WriteLine("starting the night watch work load");

            var clientTestParameters = ReadParameters(parameters);

            this.fabricClient = config.GatewayAddresses == null ? new FabricClient() : new FabricClient(config.ClientCredentials, config.GatewayAddresses);

            List<Task> partitionTasks = new List<Task>();

            for (int partitionKey = 0; partitionKey < clientTestParameters.NumberOfWorkloads; partitionKey++)
            {
                var partitionEndpoint = Utility.GenerateEndpoint(config.HttpApplicationGatewayAddresses[0], NightWatchTXRClientConstants.AppName, NightWatchTXRClientConstants.ServiceName, partitionKey.ToString());

                partitionTasks.Add(DoPartitionRun(partitionEndpoint, clientTestParameters, partitionKey));
            }

            await Task.WhenAll(partitionTasks.ToArray());

            Console.WriteLine("all runs on all partitions are done.");
        }

        private async Task<NightWatchTXRClientTestResult> DoPartitionRun(string endpoint, NightWatchTXRClientTestParameters clientTestParameters, int partitionKey)
        {
            var runResponse = await SendRunRequest(endpoint, clientTestParameters);

            //if the run request's response indicating any status other than running, service has failed to start the test. hence, returning the error msg.
            if (runResponse.Status != NightWatchTXRClientTestStatus.Running)
            {
                return runResponse as NightWatchTXRClientTestResult;
            }

            var result = await CheckResults(endpoint, partitionKey);

            if (result != null)
            {
                Console.WriteLine($"partition {partitionKey} test is done. results: {JsonConvert.SerializeObject(result)}");
            }
            else
            {
                Console.WriteLine($"partition {partitionKey} test result is not obtained.");
            }

            return result;
        }

        private async Task<NightWatchTXRClientTestResult> CheckResults(string address, int partitionKey)
        {
            Console.WriteLine($"partition {partitionKey} checking results");
            var payload = JsonConvert.SerializeObject(new NightWatchTXRClientRunParameters() { Action = NightWatchTXRClientTestAction.Status}, SerializerSettings);

            NightWatchTXRClientTestResult result = null;
            while (true)
            {
                Thread.Sleep(NightWatchTXRClientConstants.CheckResultInterval);

                var response = await SendRequest(address, payload);
                var responseString = await response.Content.ReadAsStringAsync();

                result = JsonConvert.DeserializeObject<NightWatchTXRClientTestResult>(responseString);

                Console.WriteLine($"partition {partitionKey} test result: {responseString}");

                //break out of check if status is finished
                if (result != null && result.Status == NightWatchTXRClientTestStatus.Finished)
                {
                    break;
                }

                //break out of check if status is anything but running
                if (result != null && result.Status != NightWatchTXRClientTestStatus.Running)
                {
                    break;
                }
            }

            return result;
        }

        private async Task<NightWatchTXRClientBaseResult> SendRunRequest(string address, NightWatchTXRClientTestParameters clientTestParameters)
        {
            var payload = JsonConvert.SerializeObject(new NightWatchTXRClientRunParameters()
            {
                NumberOfOperations = clientTestParameters.NumberOfOperations,
                MaxOutstandingOperations = clientTestParameters.MaxOutstandingOperations,
                OperationSize = clientTestParameters.OperationSize,
                Action = NightWatchTXRClientTestAction.Run
            }, new JsonSerializerSettings());

            var response = await SendRequest(address, payload);

            var responseString = await response.Content.ReadAsStringAsync();

            return JsonConvert.DeserializeObject<NightWatchTXRClientBaseResult>(responseString);
        }

        private async Task<HttpResponseMessage> SendRequest(string address, string payloadJson)
        {
            HttpClient httpClient = new HttpClient();

            httpClient.DefaultRequestHeaders.Accept.Add(new System.Net.Http.Headers.MediaTypeWithQualityHeaderValue("application/json"));

            HttpRequestMessage requestMessage = new HttpRequestMessage(HttpMethod.Post, address)
            {
                Content = new StringContent(payloadJson, Encoding.UTF8, "application/json")
            };

            return await httpClient.SendAsync(requestMessage);
        }

        [UserClientCheckProgress]
        public string CheckProgress(string taskId)
        {
            return string.Empty;
        }

        private NightWatchTXRClientTestParameters ReadParameters(Dictionary<string, string> parameters)
        {
            int numberOfWorkloads;
            int.TryParse(parameters.ContainsKey("NumberOfWorkloads") ? parameters["NumberOfWorkloads"] : "1", out numberOfWorkloads);
            Console.WriteLine($"Parameter NumberOfWorkloads: {numberOfWorkloads}");

            long numberOfOperations;
            long.TryParse(parameters.ContainsKey("NumberOfOperations") ? parameters["NumberOfOperations"] : "100000", out numberOfOperations);
            Console.WriteLine($"Parameter NumberOfOperations: {numberOfOperations}");

            long maxOutstandingOperations;
            long.TryParse(parameters.ContainsKey("MaxOutstandingOperations") ? parameters["MaxOutstandingOperations"] : "100", out maxOutstandingOperations);
            Console.WriteLine($"Parameter MaxOutstandingOperations: {maxOutstandingOperations}");

            long operationSize;
            long.TryParse(parameters.ContainsKey("OperationSize") ? parameters["OperationSize"] : "32", out operationSize);
            Console.WriteLine($"Parameter OperationSize: {operationSize}");

            return new NightWatchTXRClientTestParameters()
            {
                NumberOfWorkloads = numberOfWorkloads,
                NumberOfOperations = numberOfOperations,
                MaxOutstandingOperations = maxOutstandingOperations,
                OperationSize = operationSize
            };
        }
    }
}