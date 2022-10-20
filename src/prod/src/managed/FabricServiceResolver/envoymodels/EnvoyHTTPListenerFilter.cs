// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//  Licensed under the MIT License (MIT). See License.txt in 
//  the repo root for license information.
// ------------------------------------------------------------

using Newtonsoft.Json;

namespace sfintegration.envoymodels
{
    class EnvoyHTTPListenerFilter : EnvoyListenerFilter
    {
        public EnvoyHTTPListenerFilter(string stat_prefix, string cluster)
            : base("http_connection_manager")
        {
            config = new EnvoyHTTPFilterConfig(stat_prefix, cluster);
        }
        [JsonProperty]
        public EnvoyHTTPFilterConfig config;
    }
}
