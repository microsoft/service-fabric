// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System.Collections.Generic;
    using System.Fabric.Interop;
    using System.Globalization;

    /// <summary>
    /// <para>Represents the health state of a deployed application, which contains the entity identifier and the aggregated health state.</para>
    /// </summary>
    public sealed class DeployedApplicationHealthState : EntityHealthState
    {
        internal DeployedApplicationHealthState()
        {
        }

        /// <summary>
        /// <para>Gets the application name.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.Uri" /> of the application.</para>
        /// </value>
        public Uri ApplicationName
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the node name of the deployed application.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.String" /> representing the node name.</para>
        /// </value>
        public string NodeName
        {
            get;
            internal set;
        }


        /// <summary>
        /// Creates a string description of the application on the node, containing the id and the health state.
        /// </summary>
        /// <returns>String description of the <see cref="System.Fabric.Health.DeployedApplicationHealthState"/>.</returns>
        public override string ToString()
        {
            return string.Format(
                CultureInfo.CurrentCulture,
                "{0}+{1}: {2}",
                this.ApplicationName,
                this.NodeName,
                this.AggregatedHealthState);
        }

        internal static unsafe IList<DeployedApplicationHealthState> FromNativeList(IntPtr nativeListPtr)
        {
            if (nativeListPtr != IntPtr.Zero)
            {
                var nativeList = (NativeTypes.FABRIC_DEPLOYED_APPLICATION_HEALTH_STATE_LIST*)nativeListPtr;
                return DeployedApplicationHealthStateList.FromNativeList(nativeList);
            }
            else
            {
                return new DeployedApplicationHealthStateList();
            }
        }

        internal static unsafe DeployedApplicationHealthState FromNative(NativeTypes.FABRIC_DEPLOYED_APPLICATION_HEALTH_STATE nativeState)
        {
            var deployedApplicationHealthState = new DeployedApplicationHealthState();
            deployedApplicationHealthState.ApplicationName = NativeTypes.FromNativeUri(nativeState.ApplicationName);
            deployedApplicationHealthState.NodeName = NativeTypes.FromNativeString(nativeState.NodeName);
            deployedApplicationHealthState.AggregatedHealthState = (HealthState)nativeState.AggregatedHealthState;

            return deployedApplicationHealthState;
        }
    }
}