// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.GatewayResourceManager.Service.Models
{
    using System.Collections.Generic;
    using System.Linq;
    using Newtonsoft.Json;

    public class GatewayResourceProperties
    {
        public GatewayResourceProperties()
        {
        }

        public GatewayResourceProperties(NetworkRef sourceNetwork, NetworkRef destinationNetwork, string description = default(string), IList<TcpConfig> tcp = default(IList<TcpConfig>), IList<HttpConfig> http = default(IList<HttpConfig>), string status = default(string), string statusDetails = default(string))
        {
            this.Description = description;
            this.SourceNetwork = sourceNetwork;
            this.DestinationNetwork = destinationNetwork;
            this.Tcp = tcp;
            this.Http = http;
            this.Status = status;
            this.StatusDetails = statusDetails;
        }

        [JsonProperty(PropertyName = "description")]
        public string Description { get; set; }

        [JsonProperty(PropertyName = "sourceNetwork")]
        public NetworkRef SourceNetwork { get; set; }

        [JsonProperty(PropertyName = "destinationNetwork")]
        public NetworkRef DestinationNetwork { get; set; }

        [JsonProperty(PropertyName = "tcp", NullValueHandling = NullValueHandling.Ignore)]
        public IList<TcpConfig> Tcp { get; set; }

        [JsonProperty(PropertyName = "http", NullValueHandling = NullValueHandling.Ignore)]
        public IList<HttpConfig> Http { get; set; }

        [JsonProperty(PropertyName = "status")]
        public string Status { get; set; }

        [JsonProperty(PropertyName = "statusDetails", NullValueHandling = NullValueHandling.Ignore)]
        public string StatusDetails { get; set; }

        public static bool operator ==(GatewayResourceProperties ref1, GatewayResourceProperties ref2)
        {
            return EqualityComparer<GatewayResourceProperties>.Default.Equals(ref1, ref2);
        }

        public static bool operator !=(GatewayResourceProperties ref1, GatewayResourceProperties ref2)
        {
            return !(ref1 == ref2);
        }

        public override bool Equals(object obj)
        {
            var properties = obj as GatewayResourceProperties;
            var isEqual = properties != null &&
                   this.Description == properties.Description &&
                   EqualityComparer<NetworkRef>.Default.Equals(this.SourceNetwork, properties.SourceNetwork) &&
                   EqualityComparer<NetworkRef>.Default.Equals(this.DestinationNetwork, properties.DestinationNetwork);
            if (!isEqual)
            {
                return isEqual;
            }

            isEqual = (this.Tcp == null && properties.Tcp == null) ||
                (this.Tcp != null && properties.Tcp != null &&
                    this.Tcp.Count == properties.Tcp.Count &&
                    this.Tcp.SequenceEqual(properties.Tcp));
            if (!isEqual)
            {
                return isEqual;
            }

            return (this.Http == null && properties.Http == null) ||
                (this.Http != null && properties.Http != null &&
                    this.Http.Count == properties.Http.Count &&
                    this.Http.SequenceEqual(properties.Http));
        }

        public override int GetHashCode()
        {
            var hashCode = -11266605;
            hashCode = (hashCode * -1521134295) + EqualityComparer<string>.Default.GetHashCode(this.Description);
            hashCode = (hashCode * -1521134295) + EqualityComparer<NetworkRef>.Default.GetHashCode(this.SourceNetwork);
            hashCode = (hashCode * -1521134295) + EqualityComparer<NetworkRef>.Default.GetHashCode(this.DestinationNetwork);
            hashCode = (hashCode * -1521134295) + EqualityComparer<IList<TcpConfig>>.Default.GetHashCode(this.Tcp);
            hashCode = (hashCode * -1521134295) + EqualityComparer<IList<HttpConfig>>.Default.GetHashCode(this.Http);
            return hashCode;
        }
    }
}
