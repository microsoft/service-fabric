// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System.Collections.Generic;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;
    using System.Globalization;

    /// <summary>
    /// <para>Represents the health state of an application, which contains the application identifier and the aggregated health state.</para>
    /// </summary>
    public sealed class ApplicationHealthState : EntityHealthState
    {
        internal ApplicationHealthState()
        {
        }

        /// <summary>
        /// <para>Gets the application name.</para>
        /// </summary>
        /// <value>
        /// <para>The application name.</para>
        /// </value>
        [JsonCustomization(PropertyName = "Name")]
        public Uri ApplicationName
        {
            get;
            internal set;
        }

        /// <summary>
        /// Gets a string description of the application health state.
        /// </summary>
        /// <returns>String description of the application.</returns>
        public override string ToString()
        {
            return string.Format(
                CultureInfo.CurrentCulture,
                "{0}: {1}",
                this.ApplicationName,
                this.AggregatedHealthState);
        }

        internal static unsafe IList<ApplicationHealthState> FromNativeList(IntPtr nativeListPtr)
        {
            if (nativeListPtr != IntPtr.Zero)
            {
                var nativeList = (NativeTypes.FABRIC_APPLICATION_HEALTH_STATE_LIST*)nativeListPtr;
                return ApplicationHealthStateList.FromNativeList(nativeList);
            }
            else
            {
                return new ApplicationHealthStateList();
            }
        }

        internal static unsafe ApplicationHealthState FromNative(NativeTypes.FABRIC_APPLICATION_HEALTH_STATE nativeState)
        {
            var applicationHealthState = new ApplicationHealthState();
            applicationHealthState.ApplicationName = NativeTypes.FromNativeUri(nativeState.ApplicationName);
            applicationHealthState.AggregatedHealthState = (HealthState)nativeState.AggregatedHealthState;

            return applicationHealthState;
        }
    }
}