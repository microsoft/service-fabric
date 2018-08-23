//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Chaos
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using Fabric.Chaos.DataStructures;
    using Fabric.Common;
    using Newtonsoft.Json;

    [Serializable]
    internal sealed class SchedulerState : ByteSerializable
    {
        public static readonly SchedulerState NoChaosScheduleStopped   = new SchedulerState(ChaosScheduleStatus.Stopped, ChaosStatus.Stopped);
        public static readonly SchedulerState NoChaosSchedulePending   = new SchedulerState(ChaosScheduleStatus.Pending, ChaosStatus.Stopped);
        public static readonly SchedulerState NoChaosScheduleActive    = new SchedulerState(ChaosScheduleStatus.Active,  ChaosStatus.Stopped);
        public static readonly SchedulerState ChaosScheduleActive      = new SchedulerState(ChaosScheduleStatus.Active,  ChaosStatus.Running);
        public static readonly SchedulerState NoChaosScheduleExpired   = new SchedulerState(ChaosScheduleStatus.Expired, ChaosStatus.Stopped);

        public SchedulerState(SchedulerState other)
        {
            this.ScheduleStatus = other.ScheduleStatus;
            this.ChaosStatus = other.ChaosStatus;
        }

        public SchedulerState(ChaosScheduleStatus scheduleStatus, ChaosStatus chaosStatus)
        {
            this.ScheduleStatus = scheduleStatus;
            this.ChaosStatus = chaosStatus;
        }

        public ChaosScheduleStatus ScheduleStatus
        {
            get;
            set;
        }

        public ChaosStatus ChaosStatus
        {
            get;
            set;
        }

        public override int GetHashCode()
        {
            return 13
                    + (23 * this.ScheduleStatus.GetHashCode())
                    + (29 * this.ChaosStatus.GetHashCode());
        }

        public override bool Equals(object obj)
        {
            SchedulerState other = obj as SchedulerState;

            return other != null
                    && this.ScheduleStatus == other.ScheduleStatus
                    && this.ChaosStatus == other.ChaosStatus;
        }

        public override string ToString()
        {
            return string.Format("{0} {1}", Enum.GetName(typeof(ChaosScheduleStatus), this.ScheduleStatus), Enum.GetName(typeof(ChaosStatus), this.ChaosStatus));
        }

        #region ByteSerializable
        public override void Read(BinaryReader br)
        {
            this.ScheduleStatus = (ChaosScheduleStatus)br.ReadInt32();
            this.ChaosStatus = (ChaosStatus)br.ReadInt32();
        }

        public override void Write(BinaryWriter bw)
        {
            bw.Write((int)this.ScheduleStatus);
            bw.Write((int)this.ChaosStatus);
        }
        #endregion
    }
}