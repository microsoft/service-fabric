// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//  Licensed under the MIT License (MIT). See License.txt in 
//  the repo root for license information.
// ------------------------------------------------------------

using System.Collections.Generic;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace sfintegration.envoymodels
{
    public class EnvoyRoute
    {
        public EnvoyRoute(string cluster, string prefix, string prefix_rewrite, List<JObject> headers, int timeout_ms = 10000)
        {
            this.cluster = cluster;
            this.prefix = prefix;
            this.prefix_rewrite = prefix_rewrite;
            if (!prefix.EndsWith("/"))
            {
                this.prefix += "/";
            }
            if (!prefix_rewrite.EndsWith("/"))
            {
                this.prefix_rewrite += "/";
            }
            this.headers = headers;
            this.timeout_ms = timeout_ms;
        }

        [JsonProperty]
        public string cluster;

        [JsonProperty]
        public string prefix;

        [JsonProperty]
        public string prefix_rewrite;

        [JsonProperty]
        public List<JObject> headers;
        public bool ShouldSerializeheaders()
        {
            return headers != null && headers.Count != 0;
        }

        [JsonProperty]
        public int timeout_ms;

        [JsonProperty]
        public EnvoyRetryPolicy retry_policy = EnvoyRetryPolicy.defaultValue;
    }
}
