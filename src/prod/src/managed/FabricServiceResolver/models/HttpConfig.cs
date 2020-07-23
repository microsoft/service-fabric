// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//  Licensed under the MIT License (MIT). See License.txt in 
//  the repo root for license information.
// ------------------------------------------------------------

namespace Gateway.Models
{
    using Newtonsoft.Json;
    using System.Collections.Generic;

    public class HttpConfig
    {
        public HttpConfig()
        {
        }

        public HttpConfig(int port, IList<HttpHostNameConfig> hosts, string name = default(string))
        {
            Name = name;
            Port = port;
            Hosts = hosts;
        }

        [JsonProperty(PropertyName = "name")]
        public string Name { get; set; }

        [JsonProperty(PropertyName = "port")]
        public int Port { get; set; }

        [JsonProperty(PropertyName = "hosts")]
        public IList<HttpHostNameConfig> Hosts { get; set; }
    }
}
