// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.GatewayResourceManager.Service.Models
{
    using System.Collections.Generic;
    using Newtonsoft.Json;

    public class GatewayDestination
    {
        public GatewayDestination()
        {
        }

        public GatewayDestination(string applicationName, string serviceName, string endpointName = default(string))
        {
            this.ApplicationName = applicationName;
            this.ServiceName = serviceName;
            this.EndpointName = endpointName;
        }

        [JsonProperty(PropertyName = "applicationName")]
        public string ApplicationName { get; set; }

        [JsonProperty(PropertyName = "serviceName")]
        public string ServiceName { get; set; }

        [JsonProperty(PropertyName = "endpointName", NullValueHandling = NullValueHandling.Ignore)]
        public string EndpointName { get; set; }

        public static bool operator ==(GatewayDestination destination1, GatewayDestination destination2)
        {
            return EqualityComparer<GatewayDestination>.Default.Equals(destination1, destination2);
        }

        public static bool operator !=(GatewayDestination destination1, GatewayDestination destination2)
        {
            return !(destination1 == destination2);
        }

        public override bool Equals(object obj)
        {
            var destination = obj as GatewayDestination;
            return destination != null &&
                   this.ApplicationName == destination.ApplicationName &&
                   this.ServiceName == destination.ServiceName &&
                   this.EndpointName == destination.EndpointName;
        }

        public override int GetHashCode()
        {
            var hashCode = -1940133836;
            hashCode = (hashCode * -1521134295) + EqualityComparer<string>.Default.GetHashCode(this.ApplicationName);
            hashCode = (hashCode * -1521134295) + EqualityComparer<string>.Default.GetHashCode(this.ServiceName);
            hashCode = (hashCode * -1521134295) + EqualityComparer<string>.Default.GetHashCode(this.EndpointName);
            return hashCode;
        }
    }
}
