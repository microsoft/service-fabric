// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.GatewayResourceManager.Service.Models
{
    using System.Collections.Generic;
    using Newtonsoft.Json;

    public class HttpRouteMatchHeader
    {
        public HttpRouteMatchHeader()
        {
        }

        public HttpRouteMatchHeader(string name, string value = default(string), string type = default(string))
        {
            this.Name = name;
            this.Value = value;
            this.Type = type;
        }

        [JsonProperty(PropertyName = "name")]
        public string Name { get; set; }

        [JsonProperty(PropertyName = "value")]
        public string Value { get; set; }

        [JsonProperty(PropertyName = "type")]
        public string Type { get; set; }

        public static bool operator ==(HttpRouteMatchHeader header1, HttpRouteMatchHeader header2)
        {
            return EqualityComparer<HttpRouteMatchHeader>.Default.Equals(header1, header2);
        }

        public static bool operator !=(HttpRouteMatchHeader header1, HttpRouteMatchHeader header2)
        {
            return !(header1 == header2);
        }

        public override bool Equals(object obj)
        {
            var header = obj as HttpRouteMatchHeader;
            return header != null &&
                   this.Name == header.Name &&
                   this.Value == header.Value &&
                   this.Type == header.Type;
        }

        public override int GetHashCode()
        {
            var hashCode = 1477810893;
            hashCode = (hashCode * -1521134295) + EqualityComparer<string>.Default.GetHashCode(this.Name);
            hashCode = (hashCode * -1521134295) + EqualityComparer<string>.Default.GetHashCode(this.Value);
            hashCode = (hashCode * -1521134295) + EqualityComparer<string>.Default.GetHashCode(this.Type);
            return hashCode;
        }
    }
}
