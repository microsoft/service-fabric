// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System;
    using System.Fabric.Interop;
    using System.Globalization;

    /// <summary>
    /// <para>Represents information about how many health entities are in
    /// <see cref="System.Fabric.Health.HealthState.Ok"/>, <see cref="System.Fabric.Health.HealthState.Warning"/>,
    /// and <see cref="System.Fabric.Health.HealthState.Error"/> state.</para>
    /// </summary>
    public sealed class HealthStateCount
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Health.HealthStateCount" /> class.</para>
        /// </summary>
        internal HealthStateCount()
        {
        }

        /// <summary>
        /// Gets the number of entities that are in health state <see cref="System.Fabric.Health.HealthState.Ok"/>.
        /// </summary>
        /// <value>
        /// The number of entities that are in health state <see cref="System.Fabric.Health.HealthState.Ok"/>.
        /// </value>
        public long OkCount
        {
            get;
            internal set;
        }

        /// <summary>
        /// Gets the number of entities that are in health state <see cref="System.Fabric.Health.HealthState.Warning"/>.
        /// </summary>
        /// <value>
        /// The number of entities that are in health state <see cref="System.Fabric.Health.HealthState.Warning"/>.
        /// </value>
        public long WarningCount
        {
            get;
            internal set;
        }

        /// <summary>
        /// Gets the number of entities that are in health state <see cref="System.Fabric.Health.HealthState.Error"/>.
        /// </summary>
        /// <value>
        /// The number of entities that are in health state <see cref="System.Fabric.Health.HealthState.Error"/>.
        /// </value>
        public long ErrorCount
        {
            get;
            internal set;
        }

        /// <summary>
        /// Returns a string representation of the health state count.
        /// </summary>
        /// <returns>A string representation of the health state count.</returns>
        public override string ToString()
        {
            return string.Format(CultureInfo.CurrentCulture, "OK:{0},Warning:{1},Error:{2}", this.OkCount, this.WarningCount, this.ErrorCount);
        }

        internal static unsafe HealthStateCount FromNative(IntPtr nativeHealthStateCountPtr)
        {
            var healthStateCount = new HealthStateCount();
            if (nativeHealthStateCountPtr != IntPtr.Zero)
            {
                var nativeHealthStateCount = *(NativeTypes.FABRIC_HEALTH_STATE_COUNT*)nativeHealthStateCountPtr;
                healthStateCount.OkCount = (long)nativeHealthStateCount.OkCount;
                healthStateCount.WarningCount = (long)nativeHealthStateCount.WarningCount;
                healthStateCount.ErrorCount = (long)nativeHealthStateCount.ErrorCount;
            }

            return healthStateCount;
        }
    }
}
