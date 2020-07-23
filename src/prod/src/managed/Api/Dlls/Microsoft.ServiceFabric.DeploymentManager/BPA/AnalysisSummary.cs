// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.BPA
{
    using Common;

    public class AnalysisSummary
    {
        //// True - valid
        //// False - invalid; needs to be assessed
        //// null - Could not be tested due to validation dependency

        public bool? LocalAdminPrivilege
        {
            get;
            internal set;
        }

        public bool? IsJsonValid
        {
            get;
            internal set;
        }

        public bool? IsCabValid
        {
            get;
            internal set;
        }

        public bool? RequiredPortsOpen
        {
            get;    // SMB ports open, also acts as reachability check
            internal set;
        }

        public bool? RemoteRegistryAvailable
        {
            get;    // Remote Registry service state is not disabled
            internal set;
        }

        public bool? FirewallAvailable
        {
            get;    // Firewall service state is not disabled
            internal set;
        }

        public bool? RpcCheckPassed
        {
            get;
            internal set;
        }

        public bool? NoDomainController
        {
            get;    // No target machine is domain controller
            internal set;
        }

        public bool? NoConflictingInstallations
        {
            get;
            internal set;
        }

        public bool? FabricInstallable
        {
            get;    // Fabric is not installed on target machines
            internal set;
        }

        public bool? DataDrivesAvailable
        {
            get;    // DataRoot, LogRoot, etc. locations are available on target machines
            internal set;
        }

        public bool? DrivesEnoughAvailableSpace
        {
            get;    // System drive, DataRoot drive and LogRoot drive have at least 1GB of space available
            internal set;
        }

        public bool? IsAllOrNoneIOTDevice
        {
            get;    // Checks whether all devices are IOT or none of them
            internal set;
        }

        public bool? DotNetExeInPath
        {
            get;    // dotnet exists in %PATH%
            internal set;
        }

        public bool Passed
        {
            get
            {
                return (!this.LocalAdminPrivilege.HasValue || this.LocalAdminPrivilege.Value)
                       && (!this.IsJsonValid.HasValue || this.IsJsonValid.Value)
                       && (!this.IsCabValid.HasValue || this.IsCabValid.Value)
                       && (!this.RequiredPortsOpen.HasValue || this.RequiredPortsOpen.Value)
                       && (!this.RemoteRegistryAvailable.HasValue || this.RemoteRegistryAvailable.Value)
                       && (!this.FirewallAvailable.HasValue || this.FirewallAvailable.Value)
                       && (!this.RpcCheckPassed.HasValue || this.RpcCheckPassed.Value)
                       && (!this.NoDomainController.HasValue || this.NoDomainController.Value)
                       && (!this.NoConflictingInstallations.HasValue || this.NoConflictingInstallations.Value)
                       && (!this.FabricInstallable.HasValue || this.FabricInstallable.Value)
                       && (!this.DataDrivesAvailable.HasValue || this.DataDrivesAvailable.Value)
                       && (!this.DrivesEnoughAvailableSpace.HasValue || this.DrivesEnoughAvailableSpace.Value)
                       && (!this.IsAllOrNoneIOTDevice.HasValue || this.IsAllOrNoneIOTDevice.Value)
                       && (!this.DotNetExeInPath.HasValue || this.DotNetExeInPath.Value);
            }
        }

        internal MachineHealthContainer MachineHealthContainer
        {
            get;    // Machines that passed all validations
            set;
        }
    }
}