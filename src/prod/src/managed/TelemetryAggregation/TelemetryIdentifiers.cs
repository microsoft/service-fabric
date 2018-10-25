// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace TelemetryAggregation
{
    using Newtonsoft.Json;

    [JsonObject(MemberSerialization.Fields)]
    public class TelemetryIdentifiers
    {
        public TelemetryIdentifiers(string clusterId, string tenantId, string nodeName)
        {
            this.ClusterId = clusterId;
            this.TenantId = tenantId;
            this.NodeNameHash = ((uint)nodeName.GetHashCode()).ToString();
        }

        [JsonConstructor]
        private TelemetryIdentifiers()
        {
        }

        public string ClusterId { get; }

        public string TenantId { get; }

        public string NodeNameHash { get; }

        public override string ToString()
        {
            return string.Format("ClusterId: {0}, TenantId: {1}, NodeNameHash: {2}", this.ClusterId, this.TenantId, this.NodeNameHash);
        }
    }
}
