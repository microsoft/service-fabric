// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//  Licensed under the MIT License (MIT). See License.txt in 
//  the repo root for license information.
// ------------------------------------------------------------

namespace Gateway.Models
{
    using Newtonsoft.Json;
    using System.Collections.Generic;

    public class HttpHostNameConfig
    {
        public HttpHostNameConfig()
        {
        }

        public HttpHostNameConfig(IList<HttpRouteConfig> routes, string name = default(string))
        {
            Name = name;
            Routes = routes;
        }

        [JsonProperty(PropertyName = "name")]
        public string Name { get; set; }

        [JsonProperty(PropertyName = "routes")]
        public IList<HttpRouteConfig> Routes { get; set; }
    }
}
