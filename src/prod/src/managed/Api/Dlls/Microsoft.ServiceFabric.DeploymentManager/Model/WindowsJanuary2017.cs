// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Model
{
    using Microsoft.ServiceFabric.ClusterManagementCommon;
    using Newtonsoft.Json;

    public class WindowsJanuary2017 : WindowsBase
    {
        public string ClustergMSAIdentity { get; set; }

        internal override Windows ToInternal()
        {
            var windowsCredentials = base.ToInternal();
            windowsCredentials.ClustergMSAIdentity = this.ClustergMSAIdentity;
            return windowsCredentials;
        }

        internal override void FromInternal(Windows windowsIdentities)
        {
            base.FromInternal(windowsIdentities);
            this.ClustergMSAIdentity = windowsIdentities.ClustergMSAIdentity;
        }
    }
}