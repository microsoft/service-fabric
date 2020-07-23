// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//  Licensed under the MIT License (MIT). See License.txt in 
//  the repo root for license information.
// ------------------------------------------------------------

using System.Collections.Generic;
using Newtonsoft.Json;

namespace sfintegration.envoymodels
{
    class EnvoyTCPRouteConfig
    {
        public EnvoyTCPRouteConfig(string cluster)
        {
            routes = new List<EnvoyTCPRoute>
            {
                new EnvoyTCPRoute(cluster)
            };
        }
        [JsonProperty]
        List<EnvoyTCPRoute> routes;
    }
}
