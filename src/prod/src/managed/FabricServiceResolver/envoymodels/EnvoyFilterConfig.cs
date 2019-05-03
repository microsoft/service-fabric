// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//  Licensed under the MIT License (MIT). See License.txt in 
//  the repo root for license information.
// ------------------------------------------------------------

using Newtonsoft.Json;
using System.Collections.Generic;

namespace sfintegration.envoymodels
{
    class EnvoyFilterConfig
    {
        public EnvoyFilterConfig(string stat_prefix)
        {
            this.stat_prefix = stat_prefix;
            this.access_log = new List<EnvoyAccessLogConfig>();
            this.access_log.Add(new EnvoyAccessLogConfig());
        }

        [JsonProperty]
        public string stat_prefix;

        [JsonProperty]
        public List<EnvoyAccessLogConfig> access_log;
    }
}
