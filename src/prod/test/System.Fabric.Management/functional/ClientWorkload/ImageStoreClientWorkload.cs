// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ClientWorkload
{
    using System;
    using System.Collections.Generic;
    using System.Net;
    using System.Net.Http;
    using System.Threading;
    using System.Threading.Tasks;
    using MS.Test.WinFabric.UserClients;

    [UserClient("ImageStoreClientWorkload")]
    public class ImageStoreClientWorkload
    {
        private static readonly HttpClient Client = new HttpClient();

        private readonly Dictionary<string, string> WorkloadParameters;
        private readonly string AppUploadUriFormat = "{0}://{1}:{2}/upload";
        private readonly string AppDeleteUriFormat = "{0}://{1}:{2}/delete";
        private readonly string DefaultImageStoreServicePort = "8081";
        private readonly string DefaultAppGatewayProtocol = "http";
        private readonly int TcpKeepAliveTimeInMs = 60000; // 1 minute : Determines how often TCP sends keep-alive transmissions.
        private readonly int TcpKeepAliveIntervalInMs = 2000; // 2 sec : Determines how often TCP repeats keep-alive transmissions when no response is received.

        static ImageStoreClientWorkload()
        {
            Client.Timeout = TimeSpan.FromMinutes(10);
        }

        /// <summary>
        /// ImageStore ClientWorkload constructor.
        /// </summary>
        /// <param name="Parameters">A dictionary of name/value pairs that represent the parameters passed to the specific workload method.</param>
        /// <param name="Config">A set of script test configuration parameters</param>
        public ImageStoreClientWorkload(Dictionary<string, string> parameters, ScriptTestClientConfig config)
        {
            this.Config = config;
            this.WorkloadParameters = parameters;
        }

        private ScriptTestClientConfig Config { get; set; }

        [UserClientWorkload("Upload App workload.")]
        public async Task UploadAppWorkload(
            Dictionary<string, string> parameters,
            string taskId,
            CancellationToken cancellationToken)
        {
            await this.RunAppWorkload(this.AppUploadUriFormat, parameters, taskId, cancellationToken);
        }

        [UserClientWorkload("Delete App workload.")]
        public async Task DeleteAppWorkload(
            Dictionary<string, string> parameters,
            string taskId,
            CancellationToken cancellationToken)
        {
            await this.RunAppWorkload(this.AppDeleteUriFormat, parameters, taskId, cancellationToken);
        }

        [UserClientCheckProgress]
        public string CheckProgress(string taskId)
        {
            return string.Empty;
        }

        private async Task RunAppWorkload(
            string apiUriFormat,
            Dictionary<string, string> parameters,
            string taskId,
            CancellationToken cancellationToken)
        {
            var uriBuilder = this.GetWorkloadUri(apiUriFormat);

            await Task.Run(async () =>
            {
                // Keep sending tcp keep-alive messages for long workloads for Azure Load Balancer.
                ServicePointManager.FindServicePoint(uriBuilder.Uri).SetTcpKeepAlive(true, this.TcpKeepAliveTimeInMs, this.TcpKeepAliveIntervalInMs);

                Console.WriteLine("Request to {0} at {1}", uriBuilder.Uri, DateTime.Now.ToString("h:mm:ss tt"));
                var response = await Client.PostAsync(uriBuilder.Uri, null, cancellationToken);
                Console.WriteLine("Response : {0} {1}", response, DateTime.Now.ToString("h:mm:ss tt"));

                if (!response.IsSuccessStatusCode)
                {
                    throw new Exception("Request failed : Response body : " + await response.Content.ReadAsStringAsync());
                }
            });
        }

        private string GetParameterValue(Dictionary<string, string> parameters, string paramName)
        {
            if (parameters.ContainsKey(paramName))
            {
                return parameters[paramName];
            }
            else
            {
                throw new InvalidOperationException("Missing parameter: " + paramName);
            }
        }

        private UriBuilder GetWorkloadUri(string appUriFormat)
        {
            string imageStoreServiceAddress = string.Format(appUriFormat, this.GetAppGatewayProtocol(),
                this.GetHost(this.Config.HttpApplicationGatewayAddresses[0]), this.GetAppPort());

            Console.WriteLine("ImageStoreServiceAddress: {0}", imageStoreServiceAddress);

            UriBuilder builder = new UriBuilder(imageStoreServiceAddress);
            builder.Scheme = this.GetAppGatewayProtocol();

            return builder;
        }

        private string GetAppGatewayProtocol()
        {
            if (this.WorkloadParameters.ContainsKey("appGatewayProtocol"))
            {
                return this.WorkloadParameters["appGatewayProtocol"];
            }

            return this.DefaultAppGatewayProtocol;
        }

        private string GetAppPort()
        {
            if (this.WorkloadParameters.ContainsKey("ImageStoreServicePort"))
            {
                return this.WorkloadParameters["ImageStoreServicePort"];
            }

            return this.DefaultImageStoreServicePort;
        }

        private string GetHost(string uriStr)
        {
            if (!uriStr.Contains(Uri.SchemeDelimiter))
            {
                uriStr = string.Concat(Uri.UriSchemeHttp, Uri.SchemeDelimiter, uriStr);
            }

            return new Uri(uriStr).Host;
        }
    }
}
