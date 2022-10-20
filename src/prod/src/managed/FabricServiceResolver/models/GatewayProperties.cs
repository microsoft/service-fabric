// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//  Licensed under the MIT License (MIT). See License.txt in 
//  the repo root for license information.
// ------------------------------------------------------------

namespace Gateway.Models
{
    using Newtonsoft.Json;
    using System.Collections.Generic;

    public class GatewayResourceDescription
    {
        public GatewayResourceDescription()
        {
        }

        public GatewayResourceDescription(NetworkRef sourceNetwork, IList<TcpConfig> tcp, IList<HttpConfig> http)
        {
            SourceNetwork = sourceNetwork;
            Tcp = tcp;
            Http = http;
        }

        [JsonProperty(PropertyName = "sourceNetwork")]
        public NetworkRef SourceNetwork{ get; set; }

        [JsonProperty(PropertyName = "tcp")]
        public IList<TcpConfig> Tcp { get; set; }

        [JsonProperty(PropertyName = "http")]
        public IList<HttpConfig> Http { get; set; }
    }
}
