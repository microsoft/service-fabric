// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.New, "ServiceFabricPackageDebugParameter")]
    public sealed class CreateDebugParameters : CommonCmdletBase
    {
        [Parameter(Mandatory = true, Position = 0)]
        public string CodePackageName
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, Position = 1)]
        public string ServiceManifestName
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public string EntryPointType
        {
            get;
            set;
        }

        [Parameter(Mandatory = true)]
        public string DebuggerExePath
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public string DebuggerArguments
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public string CodePackageLinkFolder
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public string ConfigPackageLinkFolder
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public string DataPackageLinkFolder
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public string LockFile
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public string WorkingFolder
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public string DebugParametersFile
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public string EnvironmentBlock
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public string ConfigPackageName
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public string DataPackageName
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public string[] ContainerEntryPoints
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public string[] ContainerMountedVolumes
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public string[] ContainerEnvironmentBlock
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public string[] ContainerLabels
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public bool DisableReliableCollectionReplicationMode
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            CodePackageDebugParameters debugParameters = new CodePackageDebugParameters(
                this.ServiceManifestName,
                this.CodePackageName,
                this.ConfigPackageName,
                this.DataPackageName,
                this.EntryPointType,
                this.DebuggerExePath,
                this.DebuggerArguments,
                this.CodePackageLinkFolder,
                this.ConfigPackageLinkFolder,
                this.DataPackageLinkFolder,
                this.LockFile,
                this.WorkingFolder,
                this.DebugParametersFile,
                this.EnvironmentBlock,
                new ContainerDebugParameters(
                    this.ContainerEntryPoints,
                    this.ContainerMountedVolumes,
                    this.ContainerEnvironmentBlock,
                    this.ContainerLabels),
                    this.DisableReliableCollectionReplicationMode);
            this.WriteObject(new PSObject(debugParameters));
        }
    }
    
    [SuppressMessage("StyleCop.CSharp.MaintainabilityRules", "SA1402:FileMayOnlyContainASingleClass", Justification = "Reviewed. Suppression is OK here for CreateDebugParameters and GetDebugParameters.")]
    [Cmdlet(VerbsCommon.Get, "ServiceFabricPackageDebugParameters")]
    public sealed class GetDebugParameters : CommonCmdletBase
    {
        [Parameter(Mandatory = true, Position = 0)]
        public CodePackageDebugParameters[] DebugParameters
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            this.WriteObject(new PSObject(CodePackageDebugParameters.GetDebugParameters(this.DebugParameters)));
        }
    }
}