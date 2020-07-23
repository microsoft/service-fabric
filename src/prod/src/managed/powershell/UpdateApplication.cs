// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Collections;
    using System.Fabric.Description;
    using System.Management.Automation;

    [Cmdlet(VerbsData.Update, "ServiceFabricApplication")]
    public sealed class UpdateApplication : ApplicationCmdletBase
    {
        [Parameter(Mandatory = true, Position = 0)]
        public Uri ApplicationName
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public SwitchParameter RemoveApplicationCapacity
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public long? MaximumNodes
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public long? MinimumNodes
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public string[] Metrics
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            this.UpdateApplicationInstance(
                this.ApplicationName,
                this.RemoveApplicationCapacity,
                this.MinimumNodes,
                this.MaximumNodes,
                this.Metrics);
        }
    }
}