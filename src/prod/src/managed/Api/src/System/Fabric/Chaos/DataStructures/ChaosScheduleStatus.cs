//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

namespace System.Fabric.Chaos.DataStructures
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents the state of the Chaos schedule.</para>
    /// </summary>
    public enum ChaosScheduleStatus
    {
        /// <summary>
        /// <para>Indicates that the Chaos schedule is invalid and is not being used for scheduling.</para>
        /// </summary>
        Invalid = 0,

        /// <summary>
        /// <para>Indicates that the Chaos schedule is running and runs of Chaos will be scheduled based on the schedule.</para>
        /// </summary>
        Active = 1,

        /// <summary>
        /// <para>Indicates that Chaos schedule is not being used to schedule runs of Chaos because it is after the schedule's expiry date.</para>
        /// </summary>
        Expired = 2,

        /// <summary>
        /// <para>Indicates that Chaos schedule is not yet being used to schedule runs of Chaos because it is before the schedule's start date.</para>
        /// </summary>
        Pending = 3,

        /// <summary>
        /// <para>Indicates that Chaos schedule is stopped and is not being used for scheduling runs of Chaos.</para>
        /// </summary>
        Stopped = 4
    }
}