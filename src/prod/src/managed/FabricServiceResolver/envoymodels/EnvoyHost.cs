// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//  Licensed under the MIT License (MIT). See License.txt in 
//  the repo root for license information.
// ------------------------------------------------------------

using Newtonsoft.Json;
using webapi;

namespace sfintegration.envoymodels
{
    public class EnvoyHost
    {
        public EnvoyHost(string ip_address, int port)
        {
            this.ip_address = EnvoyDefaults.LocalHostFixup(ip_address);
            this.port = port;
        }

        [JsonProperty]
        public string ip_address;

        [JsonProperty]
        public int port;

        [JsonProperty]
        public EnvoyHostTags tags = EnvoyHostTags.defaultValue;
    }
}
