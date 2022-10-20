// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//  Licensed under the MIT License (MIT). See License.txt in 
//  the repo root for license information.
// ------------------------------------------------------------

namespace Gateway.Models
{
    using Newtonsoft.Json;

    public class HttpRouteConfig
    {
        public HttpRouteConfig()
        {
            Match = new HttpRouteMatchRule();
        }

        public HttpRouteConfig(HttpRouteMatchRule match, GatewayDestination destination, string name = default(string))
        {
            Name = name;
            Match = match;
            Destination = destination;
        }

        [JsonProperty(PropertyName = "name")]
        public string Name { get; set; }

        [JsonProperty(PropertyName = "match")]
        public HttpRouteMatchRule Match { get; set; }

        [JsonProperty(PropertyName = "destination")]
        public GatewayDestination Destination { get; set; }
    }
}
