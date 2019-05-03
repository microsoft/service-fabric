// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//  Licensed under the MIT License (MIT). See License.txt in 
//  the repo root for license information.
// ------------------------------------------------------------

namespace Gateway.Models
{
    using Newtonsoft.Json;
    using System.Text;

    public partial class GatewayDestination
    {
        public GatewayDestination()
        {
        }

        public GatewayDestination(string applicationName, string serviceName, string endpointName = default(string))
        {
            ApplicationName = applicationName;
            ServiceName = serviceName;
            EndpointName = endpointName;
        }

        [JsonProperty(PropertyName = "applicationName")]
        public string ApplicationName { get; set; }

        [JsonProperty(PropertyName = "serviceName")]
        public string ServiceName { get; set; }

        [JsonProperty(PropertyName = "endpointName")]
        public string EndpointName { get; set; }

        public string EnvoyServiceName()
        {
            StringBuilder sb = new StringBuilder();
            sb.Append(ApplicationName);
            sb.Append("_");
            sb.Append(ServiceName);
            if (!string.IsNullOrEmpty(EndpointName))
            {
                sb.Append("_");
                sb.Append(EndpointName);
            }
            sb.Append("|*|-2");
            return sb.ToString();
        }
    }
}
