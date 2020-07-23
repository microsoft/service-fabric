// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//  Licensed under the MIT License (MIT). See License.txt in 
//  the repo root for license information.
// ------------------------------------------------------------

using System.Collections.Generic;
using Newtonsoft.Json;
using webapi;

namespace sfintegration.envoymodels
{
    class EnvoyListener
    {
        public EnvoyListener(string name, string address, string stat_prefix, List<ListenerFilterConfig> clusters)
        {
            this.name = name;
            this.address = address;
            this.filters = new List<EnvoyListenerFilter>();
            foreach (var cluster in clusters)
            {
                if (cluster.Type == ListenerFilterConfig.ListenerFilterType.Tcp)
                {
                    this.filters.Add(new EnvoyTCPListenerFilter(stat_prefix, cluster.ClusterName));
                }
                else
                {
                    this.filters.Add(new EnvoyHTTPListenerFilter(stat_prefix, cluster.ClusterName));
                }
            }
        }
        [JsonProperty]
        public string name;

        [JsonProperty]
        public string address;

        [JsonProperty]
        public List<EnvoyListenerFilter> filters;
    }

}
