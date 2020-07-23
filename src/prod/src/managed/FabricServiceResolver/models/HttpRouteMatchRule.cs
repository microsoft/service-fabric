// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//  Licensed under the MIT License (MIT). See License.txt in 
//  the repo root for license information.
// ------------------------------------------------------------

namespace Gateway.Models
{
    using Newtonsoft.Json;
    using System.Collections.Generic;

    public class HttpRouteMatchRule
    {
        public HttpRouteMatchRule()
        {
            Path = new HttpRouteMatchPath();
        }

        public HttpRouteMatchRule(HttpRouteMatchPath path, IList<HttpRouteMatchHeader> headers = default(IList<HttpRouteMatchHeader>))
        {
            Path = path;
            Headers = headers;
        }

        [JsonProperty(PropertyName = "path")]
        public HttpRouteMatchPath Path { get; set; }

        [JsonProperty(PropertyName = "headers")]
        public IList<HttpRouteMatchHeader> Headers { get; set; }
    }
}
