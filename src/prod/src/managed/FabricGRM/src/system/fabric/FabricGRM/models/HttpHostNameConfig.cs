// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.GatewayResourceManager.Service.Models
{
    using System.Collections.Generic;
    using System.Linq;
    using Newtonsoft.Json;

    public class HttpHostNameConfig
    {
        public HttpHostNameConfig()
        {
        }

        public HttpHostNameConfig(IList<HttpRouteConfig> routes, string name = default(string))
        {
            this.Name = name;
            this.Routes = routes;
        }

        [JsonProperty(PropertyName = "name")]
        public string Name { get; set; }

        [JsonProperty(PropertyName = "routes")]
        public IList<HttpRouteConfig> Routes { get; set; }

        public static bool operator ==(HttpHostNameConfig config1, HttpHostNameConfig config2)
        {
            return EqualityComparer<HttpHostNameConfig>.Default.Equals(config1, config2);
        }

        public static bool operator !=(HttpHostNameConfig config1, HttpHostNameConfig config2)
        {
            return !(config1 == config2);
        }

        public override bool Equals(object obj)
        {
            var config = obj as HttpHostNameConfig;
            var isEqual = config != null &&
                   this.Name == config.Name;
            if (!isEqual)
            {
                return isEqual;
            }

            return (this.Routes == null && config.Routes == null) ||
                (this.Routes != null && config.Routes != null &&
                this.Routes.Count == config.Routes.Count &&
                this.Routes.SequenceEqual(config.Routes));
        }

        public override int GetHashCode()
        {
            var hashCode = -1286587219;
            hashCode = (hashCode * -1521134295) + EqualityComparer<string>.Default.GetHashCode(this.Name);
            hashCode = (hashCode * -1521134295) + EqualityComparer<IList<HttpRouteConfig>>.Default.GetHashCode(this.Routes);
            return hashCode;
        }
    }
}
