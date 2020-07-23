// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//  Licensed under the MIT License (MIT). See License.txt in 
//  the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections.Generic;
using Newtonsoft.Json;

namespace sfintegration.envoymodels
{
    public class EnvoyVirtualHost
    {
        public EnvoyVirtualHost(string name, List<string> domains, List<EnvoyRoute> routes)
        {
            this.name = name;
            this.domains = domains;
            this.routes = routes;
        }

        [JsonProperty]
        public string name;

        [JsonProperty]
        public List<string> domains;

        [JsonProperty]
        public List<EnvoyRoute> routes;
    }
}
