// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//  Licensed under the MIT License (MIT). See License.txt in 
//  the repo root for license information.
// ------------------------------------------------------------

using Newtonsoft.Json;

namespace sfintegration.envoymodels
{
    class EnvoyTCPFilterConfig : EnvoyFilterConfig
    {
        public EnvoyTCPFilterConfig(string stat_prefix, string cluster) : base(stat_prefix)
        {
            route_config = new EnvoyTCPRouteConfig(cluster);
        }
        [JsonProperty]
        public EnvoyTCPRouteConfig route_config;
    }
}
