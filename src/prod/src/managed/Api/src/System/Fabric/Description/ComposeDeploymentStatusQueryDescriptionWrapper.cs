// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;
    using System.Fabric.Strings;

    /// <summary>
    /// <para>Represents the query input used by <see cref="System.Fabric.FabricClient.QueryClient.GetComposeDeploymentStatusPagedListAsync(ComposeDeploymentStatusQueryDescriptionWrapper)" />.</para>
    /// </summary>
    internal sealed class ComposeDeploymentStatusQueryDescriptionWrapper
    {
        public ComposeDeploymentStatusQueryDescriptionWrapper()
        {
            DeploymentNameFilter = null;
            ContinuationToken = null;
            MaxResults = 0;
        }

        public string DeploymentNameFilter { get; set; }

        public string ContinuationToken { get; set; }

        public long MaxResults { get; set; }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeDescription = new NativeTypes.FABRIC_COMPOSE_DEPLOYMENT_STATUS_QUERY_DESCRIPTION();

            nativeDescription.DeploymentNameFilter = pinCollection.AddObject(this.DeploymentNameFilter);
            nativeDescription.ContinuationToken = pinCollection.AddObject(this.ContinuationToken);
            nativeDescription.MaxResults = (long)this.MaxResults;

            nativeDescription.Reserved = IntPtr.Zero;

            return pinCollection.AddBlittable(nativeDescription);
        }
    }
}