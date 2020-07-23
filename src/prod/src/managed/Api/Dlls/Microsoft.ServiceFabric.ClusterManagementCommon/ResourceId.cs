// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System;
    using System.Fabric.Common;
    using Newtonsoft.Json;

    [JsonConverter(typeof(ResourceIdConverter))]
    public class ResourceId
    {
        public ResourceId()
        {
        }

        public ResourceId(string subscriptionId, string resourceGroupName, string resourceName)
        {
            subscriptionId.MustNotBeNullOrWhiteSpace("subscriptionId");
            resourceGroupName.MustNotBeNullOrWhiteSpace("resourceGroupName");
            resourceName.MustNotBeNullOrWhiteSpace("resourceName");

            this.SubscriptionId = subscriptionId;
            this.ResourceGroupName = resourceGroupName;
            this.Name = resourceName;
        }

        public string SubscriptionId { get; set; }

        public string ResourceGroupName { get; set; }

        public string Name { get; set; }

        public static string GetParitalId(string subscriptionId)
        {
            return string.Format(
                       "/subscriptions/{0}",
                       subscriptionId);
        }

        public static string GetParitalId(string subscriptionId, string resourceGroupName)
        {
            return string.Format(
                       "/subscriptions/{0}/resourcegroups/{1}",
                       subscriptionId,
                       resourceGroupName.ToUpperInvariant());
        }

        public static bool TryParse(string resourceIdString, out ResourceId resourceId)
        {
            resourceId = null;

            resourceIdString.MustNotBeNull("resourceId");

            resourceIdString = resourceIdString.Trim('/');

            var tokens = resourceIdString.Split(new[] { '/' }, StringSplitOptions.None);

            if (tokens.Length != 8)
            {
                // There should be 8 tokens in ResourceId
                return false;
            }

            if (!string.Equals(tokens[0], "subscriptions", StringComparison.InvariantCultureIgnoreCase))
            {
                return false;
            }

            if (!string.Equals(tokens[2], "resourcegroups", StringComparison.InvariantCultureIgnoreCase))
            {
                return false;
            }

            if (!string.Equals(tokens[4], "providers", StringComparison.InvariantCultureIgnoreCase))
            {
                return false;
            }

            var resourceType = string.Format("{0}/{1}", tokens[5], tokens[6]);
            if (!string.Equals(resourceType, Constants.ResourceType, StringComparison.InvariantCultureIgnoreCase))
            {
                return false;
            }

            var subscriptionId = tokens[1];
            var resourceGroupName = tokens[3];
            var resourceName = tokens[7];

            resourceId = new ResourceId(subscriptionId, resourceGroupName, resourceName);

            return true;
        }

        public string ToStringOriginal()
        {
            return this.ToStringInternal(false /*toUpperCase*/);
        }

        public override string ToString()
        {
            return this.ToStringInternal(true /*toUpperCase*/);
        }

        public override bool Equals(object other)
        {
            var otherResourceId = other as ResourceId;
            if (otherResourceId == null)
            {
                return false;
            }

            return this.SubscriptionId == otherResourceId.SubscriptionId &&
                   EqualityUtility.Equals(this.ResourceGroupName, otherResourceId.ResourceGroupName) &&
                   EqualityUtility.Equals(this.Name, otherResourceId.Name);
        }

        public override int GetHashCode()
        {
            var hash = 27;
            hash = (13 * hash) + this.SubscriptionId.GetHashCode();
            hash = (13 * hash) + this.ResourceGroupName.ToUpperInvariant().GetHashCode();
            hash = (13 * hash) + this.Name.ToUpperInvariant().GetHashCode();
            return hash;
        }

        private string ToStringInternal(bool toUpperCase)
        {
            return string.Format(
                       "/subscriptions/{0}/resourcegroups/{1}/providers/{2}/{3}",
                       this.SubscriptionId,
                       toUpperCase ? this.ResourceGroupName.ToUpperInvariant() : this.ResourceGroupName,
                       Constants.ResourceType,
                       toUpperCase ? this.Name.ToUpperInvariant() : this.Name);
        }
    }
}