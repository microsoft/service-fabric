// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Autopilot
{
    internal sealed class MachineInfo
    {
        public MachineInfo(MaintenanceRecordId id, string machineStatus, string repairActionState)
        {
            this.Id = id.Validate("id");
            this.Status = machineStatus;
            this.RepairActionState = repairActionState;
        }

        public MaintenanceRecordId Id { get; private set; }

        public string Status { get; private set; }

        public string RepairActionState { get; private set; }
    }
}