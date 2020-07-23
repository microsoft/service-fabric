// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//  Licensed under the MIT License (MIT). See License.txt in 
//  the repo root for license information.
// ------------------------------------------------------------

using Newtonsoft.Json;

namespace sfintegration.envoymodels
{
    class EnvoyTCPListenerFilter : EnvoyListenerFilter
    {
        public EnvoyTCPListenerFilter(string stat_prefix, string cluster)
            : base("tcp_proxy")
        {
            config = new EnvoyTCPFilterConfig(stat_prefix, cluster);
        }
        [JsonProperty]
        public EnvoyTCPFilterConfig config;
    }
}
