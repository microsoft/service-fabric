// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.GatewayResourceManager.Service.Models
{
    using System.Collections.Generic;
    using Newtonsoft.Json;

    public class TcpConfig
    {
        public TcpConfig()
        {
        }

        public TcpConfig(string name, int port, GatewayDestination destination)
        {
            this.Name = name;
            this.Port = port;
            this.Destination = destination;
        }

        [JsonProperty(PropertyName = "name")]
        public string Name { get; set; }

        [JsonProperty(PropertyName = "port")]
        public int Port { get; set; }

        [JsonProperty(PropertyName = "destination")]
        public GatewayDestination Destination { get; set; }

        public static bool operator ==(TcpConfig config1, TcpConfig config2)
        {
            return EqualityComparer<TcpConfig>.Default.Equals(config1, config2);
        }

        public static bool operator !=(TcpConfig config1, TcpConfig config2)
        {
            return !(config1 == config2);
        }

        public override bool Equals(object obj)
        {
            var config = obj as TcpConfig;
            return config != null &&
                   this.Name == config.Name &&
                   this.Port == config.Port &&
                   EqualityComparer<GatewayDestination>.Default.Equals(this.Destination, config.Destination);
        }

        public override int GetHashCode()
        {
            var hashCode = -805492357;
            hashCode = (hashCode * -1521134295) + EqualityComparer<string>.Default.GetHashCode(this.Name);
            hashCode = (hashCode * -1521134295) + this.Port.GetHashCode();
            hashCode = (hashCode * -1521134295) + EqualityComparer<GatewayDestination>.Default.GetHashCode(this.Destination);
            return hashCode;
        }
    }
}
