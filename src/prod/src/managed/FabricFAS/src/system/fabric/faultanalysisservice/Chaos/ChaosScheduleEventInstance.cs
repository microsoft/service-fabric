//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Chaos
{
    using System.Collections;
    using System.Collections.Generic;
    using System.Fabric.Chaos.DataStructures;

    internal sealed class ChaosScheduleEventInstance
        : IComparable<DateTime>
    {
        internal ChaosScheduleEventInstance(string chaosParametersReferenceName, ChaosParameters chaosParameters, DateTime start, DateTime end)
        {
            this.ChaosParametersReferenceName = chaosParametersReferenceName;
            this.ChaosParameters = chaosParameters;
            this.Start = start;
            this.End = end;
        }

        public DateTime Start { get; internal set; }

        public DateTime End { get; internal set; }

        public ChaosParameters ChaosParameters { get; private set; }

        public string ChaosParametersReferenceName { get; private set; }

        public int CompareTo(DateTime datetime)
        {
            if (datetime < this.Start)
            {
                return 1;
            }
            else if (datetime > this.End)
            {
                return -1;
            }
            else
            {
                return 0;
            }
        }

        internal void UpdateToNextInstance()
        {
            this.Start = this.Start.AddDays(7);
            this.End = this.End.AddDays(7);
        }
    }
}