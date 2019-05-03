// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Autopilot
{
    internal sealed class MaintenanceRecordId
    {
        public MaintenanceRecordId(string machineName, string repairType)
            : this(machineName, MapStringToRepairType(repairType.Validate("repairType")))
        {
        }

        public MaintenanceRecordId(string machineName, RepairType repairType)
        {
            this.MachineName = machineName.Validate("machineName").ToUpperInvariant();
            this.RepairType = repairType;
        }

        public string MachineName { get; private set; }

        public RepairType RepairType { get; private set; }

        public override bool Equals(object obj)
        {
            MaintenanceRecordId other = obj as MaintenanceRecordId;

            if (other == null)
                return false;

            return
                other != null &&
                this.MachineName.Equals(other.MachineName) &&
                this.RepairType == other.RepairType;
        }

        public override int GetHashCode()
        {
            return this.MachineName.GetHashCode() + (int)this.RepairType;
        }

        public override string ToString()
        {
            return $"{this.MachineName}/{this.RepairType}";
        }

        private static RepairType MapStringToRepairType(string repairString)
        {
            RepairType repairType;
            if (!Enum.TryParse(repairString, true, out repairType))
            {
                repairType = RepairType.Unknown;
            }
            return repairType;
        }
    }

    internal sealed class MachineMaintenanceRecord
    {
        private MachineMaintenanceRecord()
        {
        }

        public MaintenanceRecordId RecordId { get; private set; }

        public string MachineName { get { return this.RecordId.MachineName; } }

        public RepairType RepairType { get { return this.RecordId.RepairType; } }

        public bool IsPendingApproval { get; private set; }

        public bool IsApproved { get; set; }

        public TimeSpan OriginalDelay { get; private set; }

        public TimeSpan NewDelay { get; set; }

        public bool IsDelayModified
        {
            get { return this.OriginalDelay != this.NewDelay; }
        }

        public static MachineMaintenanceRecord FromPendingRepair(MaintenanceRecordId recordId, TimeSpan delay)
        {
            return new MachineMaintenanceRecord()
            {
                RecordId = recordId.Validate("recordId"),
                OriginalDelay = delay,
                NewDelay = delay,
                IsApproved = false,
                IsPendingApproval = (delay > TimeSpan.Zero),
            };
        }

        public static MachineMaintenanceRecord FromActiveRepair(MaintenanceRecordId recordId)
        {
            return new MachineMaintenanceRecord()
            {
                RecordId = recordId.Validate("recordId"),
                OriginalDelay = ActiveRepairSentinelDelayValue,
                NewDelay = ActiveRepairSentinelDelayValue,
                IsApproved = false,
                IsPendingApproval = false,
            };
        }

        // More obvious in traces than TimeSpan.MinValue when rendered as a string
        private static readonly TimeSpan ActiveRepairSentinelDelayValue = TimeSpan.FromDays(-99999);
    }
}