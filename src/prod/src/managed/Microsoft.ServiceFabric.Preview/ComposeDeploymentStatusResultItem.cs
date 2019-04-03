// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Preview.Client.Query
{
    using System;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Describes a compose deployment status result item which is characterized by deployment name, application name, compose deployment status
    /// , and status details.</para>
    /// </summary>
    public sealed class ComposeDeploymentStatusResultItem
    {
        internal ComposeDeploymentStatusResultItem()
        {
        }

        /// <summary>
        /// <para>Gets the name of the compose deployment.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the compose deployment.</para>
        /// </value>
        public string DeploymentName { get; internal set; }


        /// <summary>
        /// <para>Gets the name of the application as a URI.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the application.</para>
        /// </value>
        public Uri ApplicationName { get; internal set; }

        /// <summary>
        /// <para>Gets the status of the compose deployment as <see cref="ComposeDeploymentStatus" />.</para>
        /// </summary>
        /// <value>
        /// <para>The status of the compose deployment.</para>
        /// </value>
        public ComposeDeploymentStatus ComposeDeploymentStatus { get; internal set; }

        /// <summary>
        /// <para>Gets the status details of compose deployment including failure message. </para>
        /// </summary>
        /// <value>
        /// <para>The status details of compose deployment including failure message.</para>
        /// </value>
        [JsonCustomization(IsDefaultValueIgnored = true)]
        public string StatusDetails { get; internal set; }

        internal static unsafe ComposeDeploymentStatusResultItem CreateFromNative(
            NativeTypes.FABRIC_COMPOSE_DEPLOYMENT_STATUS_QUERY_RESULT_ITEM nativeResultItem)
        {
            var item = new ComposeDeploymentStatusResultItem();

            item.DeploymentName = NativeTypes.FromNativeString(nativeResultItem.DeploymentName);
            item.ApplicationName = new Uri(NativeTypes.FromNativeString(nativeResultItem.ApplicationName));
            item.ComposeDeploymentStatus = (ComposeDeploymentStatus)nativeResultItem.Status;
            item.StatusDetails = NativeTypes.FromNativeString(nativeResultItem.StatusDetails);

            return item;
        }
    }
}