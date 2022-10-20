// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.GatewayResourceManager.Service.Models
{
    using System.Collections.Generic;
    using System.Linq;
    using Newtonsoft.Json;

    public class HttpConfig
    {
        public HttpConfig()
        {
        }

        public HttpConfig(int port, IList<HttpHostNameConfig> hosts, string name = default(string))
        {
            this.Name = name;
            this.Port = port;
            this.Hosts = hosts;
        }

        [JsonProperty(PropertyName = "name")]
        public string Name { get; set; }

        [JsonProperty(PropertyName = "port")]
        public int Port { get; set; }

        [JsonProperty(PropertyName = "hosts")]
        public IList<HttpHostNameConfig> Hosts { get; set; }

        public static bool operator ==(HttpConfig config1, HttpConfig config2)
        {
            return EqualityComparer<HttpConfig>.Default.Equals(config1, config2);
        }

        public static bool operator !=(HttpConfig config1, HttpConfig config2)
        {
            return !(config1 == config2);
        }

        public override bool Equals(object obj)
        {
            var config = obj as HttpConfig;
            var isEqual = config != null &&
                   this.Name == config.Name &&
                   this.Port == config.Port;
            if (!isEqual)
            {
                return isEqual;
            }

            return (this.Hosts == null && config.Hosts == null) ||
                (this.Hosts != null && config.Hosts != null &&
                this.Hosts.Count == config.Hosts.Count &&
                this.Hosts.SequenceEqual(config.Hosts));
        }

        public override int GetHashCode()
        {
            var hashCode = 1683230672;
            hashCode = (hashCode * -1521134295) + EqualityComparer<string>.Default.GetHashCode(this.Name);
            hashCode = (hashCode * -1521134295) + this.Port.GetHashCode();
            hashCode = (hashCode * -1521134295) + EqualityComparer<IList<HttpHostNameConfig>>.Default.GetHashCode(this.Hosts);
            return hashCode;
        }
    }
}
