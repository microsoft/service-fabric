// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.GatewayResourceManager.Service.Models
{
    using System.Collections.Generic;
    using Newtonsoft.Json;

    public class HttpRouteMatchPath
    {
        public HttpRouteMatchPath()
        {
        }

        public HttpRouteMatchPath(string value, string rewrite = default(string), string type = default(string))
        {
            this.Value = value;
            this.Rewrite = rewrite;
            this.Type = type;
        }

        [JsonProperty(PropertyName = "value")]
        public string Value { get; set; }

        [JsonProperty(PropertyName = "rewrite")]
        public string Rewrite { get; set; }

        [JsonProperty(PropertyName = "type")]
        public string Type { get; set; }

        public static bool operator ==(HttpRouteMatchPath path1, HttpRouteMatchPath path2)
        {
            return EqualityComparer<HttpRouteMatchPath>.Default.Equals(path1, path2);
        }

        public static bool operator !=(HttpRouteMatchPath path1, HttpRouteMatchPath path2)
        {
            return !(path1 == path2);
        }

        public override bool Equals(object obj)
        {
            var path = obj as HttpRouteMatchPath;
            return path != null &&
                   this.Value == path.Value &&
                   this.Rewrite == path.Rewrite &&
                   this.Type == path.Type;
        }

        public override int GetHashCode()
        {
            var hashCode = -671315378;
            hashCode = (hashCode * -1521134295) + EqualityComparer<string>.Default.GetHashCode(this.Value);
            hashCode = (hashCode * -1521134295) + EqualityComparer<string>.Default.GetHashCode(this.Rewrite);
            hashCode = (hashCode * -1521134295) + EqualityComparer<string>.Default.GetHashCode(this.Type);
            return hashCode;
        }
    }
}
