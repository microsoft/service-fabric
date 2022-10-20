// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//  Licensed under the MIT License (MIT). See License.txt in 
//  the repo root for license information.
// ------------------------------------------------------------

using System.Collections.Generic;
using Newtonsoft.Json;

namespace sfintegration.envoymodels
{
    class EnvoyHTTPFilterConfig : EnvoyFilterConfig
    {
        public EnvoyHTTPFilterConfig(string stat_prefix, string cluster) :
            base(stat_prefix)
        {
            rds = new EnvoyRDSConfig(cluster);
            filters = new List<object>
            {
                new { type = "decoder", name = "router", config = new { } }
            };
        }
        [JsonProperty]
        public EnvoyRDSConfig rds;

        [JsonProperty]
        public string codec_type = "auto";

        [JsonProperty]
        public List<object> filters;
    }
}
