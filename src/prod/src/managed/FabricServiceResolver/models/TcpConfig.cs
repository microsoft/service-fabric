// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//  Licensed under the MIT License (MIT). See License.txt in 
//  the repo root for license information.
// ------------------------------------------------------------

namespace Gateway.Models
{
    using Newtonsoft.Json;

    public class TcpConfig
    {
        public TcpConfig()
        {
        }

        public TcpConfig(string name, int port, GatewayDestination destination)
        {
            Name = name;
            Port = port;
            Destination = destination;
        }

        [JsonProperty(PropertyName = "name")]
        public string Name { get; set; }

        [JsonProperty(PropertyName = "port")]
        public int Port { get; set; }

        [JsonProperty(PropertyName = "destination")]
        public GatewayDestination Destination { get; set; }
    }
}
