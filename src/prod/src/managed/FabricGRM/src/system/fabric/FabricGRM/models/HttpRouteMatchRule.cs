// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.GatewayResourceManager.Service.Models
{
    using System.Collections.Generic;
    using System.Linq;
    using Newtonsoft.Json;

    public class HttpRouteMatchRule
    {
        public HttpRouteMatchRule()
        {
        }

        public HttpRouteMatchRule(HttpRouteMatchPath path, IList<HttpRouteMatchHeader> headers = default(IList<HttpRouteMatchHeader>))
        {
            this.Path = path;
            this.Headers = headers;
        }

        [JsonProperty(PropertyName = "path")]
        public HttpRouteMatchPath Path { get; set; }

        [JsonProperty(PropertyName = "headers")]
        public IList<HttpRouteMatchHeader> Headers { get; set; }

        public static bool operator ==(HttpRouteMatchRule rule1, HttpRouteMatchRule rule2)
        {
            return EqualityComparer<HttpRouteMatchRule>.Default.Equals(rule1, rule2);
        }

        public static bool operator !=(HttpRouteMatchRule rule1, HttpRouteMatchRule rule2)
        {
            return !(rule1 == rule2);
        }

        public override bool Equals(object obj)
        {
            var rule = obj as HttpRouteMatchRule;
            var isEqual = rule != null &&
                   EqualityComparer<HttpRouteMatchPath>.Default.Equals(this.Path, rule.Path);
            if (!isEqual)
            {
                return isEqual;
            }

            return (this.Headers == null && rule.Headers == null) ||
                (this.Headers != null && rule.Headers != null &&
                this.Headers.Count == rule.Headers.Count &&
                this.Headers.All(rule.Headers.Contains));
        }

        public override int GetHashCode()
        {
            var hashCode = 1328052559;
            hashCode = (hashCode * -1521134295) + EqualityComparer<HttpRouteMatchPath>.Default.GetHashCode(this.Path);
            hashCode = (hashCode * -1521134295) + EqualityComparer<IList<HttpRouteMatchHeader>>.Default.GetHashCode(this.Headers);
            return hashCode;
        }
    }
}
