// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Common.Serialization;
    using System.Fabric.Description;
    using System.Fabric.Interop;

    internal sealed class ComposeDeploymentStatusResultItemWrapper
    {
        internal ComposeDeploymentStatusResultItemWrapper()
        {
        }

        [JsonCustomization(PropertyName = JsonPropertyNames.Name)]
        public string DeploymentName { get; internal set; }
        
        public Uri ApplicationName { get; internal set; }

        [JsonCustomization(PropertyName = JsonPropertyNames.Status)]
        public ComposeDeploymentStatusWrapper ComposeDeploymentStatus { get; internal set; }

        [JsonCustomization(IsDefaultValueIgnored = true)]
        public string StatusDetails { get; internal set; }

        internal static unsafe ComposeDeploymentStatusResultItemWrapper CreateFromNative(
            NativeTypes.FABRIC_COMPOSE_DEPLOYMENT_STATUS_QUERY_RESULT_ITEM nativeResultItem)
        {
            var item = new ComposeDeploymentStatusResultItemWrapper();

            item.DeploymentName = NativeTypes.FromNativeString(nativeResultItem.DeploymentName);
            item.ApplicationName = new Uri(NativeTypes.FromNativeString(nativeResultItem.ApplicationName));
            item.ComposeDeploymentStatus = (ComposeDeploymentStatusWrapper)nativeResultItem.Status;
            item.StatusDetails = NativeTypes.FromNativeString(nativeResultItem.StatusDetails);

            return item;
        }
    }
}