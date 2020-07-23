// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//  Licensed under the MIT License (MIT). See License.txt in 
//  the repo root for license information.
// ------------------------------------------------------------

namespace Gateway.Models
{
    using Newtonsoft.Json;

    public class HttpRouteMatchPath
    {
        public HttpRouteMatchPath()
        {
        }

        public HttpRouteMatchPath(string value, string rewrite = default(string))
        {
            Value = value;
            Rewrite = rewrite;
        }

        static HttpRouteMatchPath()
        {
            Type = "prefix";
        }

        [JsonProperty(PropertyName = "value")]
        public string Value { get; set; }

        [JsonProperty(PropertyName = "rewrite")]
        public string Rewrite { get; set; }

        [JsonProperty(PropertyName = "type")]
        public static string Type { get; private set; }
    }
}
