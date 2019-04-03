// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.GatewayResourceManager.Service.Models
{
    using System.Collections.Generic;
    using Newtonsoft.Json;

    public class HttpRouteConfig
    {
        public HttpRouteConfig()
        {
        }

        public HttpRouteConfig(HttpRouteMatchRule match, GatewayDestination destination, string name = default(string))
        {
            this.Name = name;
            this.Match = match;
            this.Destination = destination;
        }

        [JsonProperty(PropertyName = "name")]
        public string Name { get; set; }

        [JsonProperty(PropertyName = "match")]
        public HttpRouteMatchRule Match { get; set; }

        [JsonProperty(PropertyName = "destination")]
        public GatewayDestination Destination { get; set; }

        public static bool operator ==(HttpRouteConfig config1, HttpRouteConfig config2)
        {
            return EqualityComparer<HttpRouteConfig>.Default.Equals(config1, config2);
        }

        public static bool operator !=(HttpRouteConfig config1, HttpRouteConfig config2)
        {
            return !(config1 == config2);
        }

        public override bool Equals(object obj)
        {
            var config = obj as HttpRouteConfig;
            return config != null &&
                   this.Name == config.Name &&
                   EqualityComparer<HttpRouteMatchRule>.Default.Equals(this.Match, config.Match) &&
                   EqualityComparer<GatewayDestination>.Default.Equals(this.Destination, config.Destination);
        }

        public override int GetHashCode()
        {
            var hashCode = -934086293;
            hashCode = (hashCode * -1521134295) + EqualityComparer<string>.Default.GetHashCode(this.Name);
            hashCode = (hashCode * -1521134295) + EqualityComparer<HttpRouteMatchRule>.Default.GetHashCode(this.Match);
            hashCode = (hashCode * -1521134295) + EqualityComparer<GatewayDestination>.Default.GetHashCode(this.Destination);
            return hashCode;
        }
    }
}
