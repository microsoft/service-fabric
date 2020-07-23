// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//  Licensed under the MIT License (MIT). See License.txt in 
//  the repo root for license information.
// ------------------------------------------------------------

using Newtonsoft.Json;

namespace sfintegration.envoymodels
{
    public class EnvoyRDSConfig
    {
        public EnvoyRDSConfig(string config_name)
        {
            route_config_name = config_name;
        }

        [JsonProperty]
        public string cluster = "rds";

        [JsonProperty]
        public string route_config_name = "gateway";

        [JsonProperty]
        public int refresh_delay_ms = 10000;
    }
}
