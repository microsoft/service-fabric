// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//  Licensed under the MIT License (MIT). See License.txt in 
//  the repo root for license information.
// ------------------------------------------------------------

using Newtonsoft.Json;

namespace sfintegration.envoymodels
{
    public class EnvoyCluster
    {
        public EnvoyCluster(string name, int timeout_ms = 10000, bool isHttps = false, EnvoyClusterSslContext context = null)
        {
            this.name = name;
            this.service_name = name;
            if (isHttps)
            {
                ssl_context = context;
            }
            connect_timeout_ms = timeout_ms;
        }

        [JsonProperty]
        public string name;

        [JsonProperty]
        public string service_name;

        [JsonProperty]
        public string type = "sds";

        [JsonProperty]
        public int connect_timeout_ms;

        [JsonProperty]
        public string lb_type = "round_robin";

        [JsonProperty]
        public EvoyOutlierDetection outlier_detection = EvoyOutlierDetection.defaultValue;

        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public EnvoyClusterSslContext ssl_context;
    }
}
