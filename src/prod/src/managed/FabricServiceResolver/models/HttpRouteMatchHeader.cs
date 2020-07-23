// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//  Licensed under the MIT License (MIT). See License.txt in 
//  the repo root for license information.
// ------------------------------------------------------------

namespace Gateway.Models
{
    using Newtonsoft.Json;

    public class HttpRouteMatchHeader
    {
        public HttpRouteMatchHeader()
        {
        }

        public HttpRouteMatchHeader(string name, string value = default(string), string type = default(string))
        {
            Name = name;
            Value = value;
            Type = type;
        }

        [JsonProperty(PropertyName = "name")]
        public string Name { get; set; }

        [JsonProperty(PropertyName = "value")]
        public string Value { get; set; }

        [JsonProperty(PropertyName = "type")]
        public string Type { get; set; }
    }
}
