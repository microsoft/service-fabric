// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System.Fabric.Common;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.Text;

    /// <summary>
    /// <para>Describes the health of a node as returned by <see cref="System.Fabric.FabricClient.HealthClient.GetNodeHealthAsync(Description.NodeHealthQueryDescription)" />.</para>
    /// </summary>
    public sealed class NodeHealth : EntityHealth
    {
        internal NodeHealth()
        {
        }

        /// <summary>
        /// <para>Gets the node name.</para>
        /// </summary>
        /// <value>
        /// <para>The node name.</para>
        /// </value>
        [JsonCustomization(PropertyName = "Name")]
        public string NodeName
        {
            get;
            internal set;
        }

        /// <summary>
        /// Gets a string representation of the <see cref="System.Fabric.Health.NodeHealth"/>.
        /// </summary>
        /// <returns>A string representation of the <see cref="System.Fabric.Health.NodeHealth"/>.</returns>
        public override string ToString()
        {
            var sb = new StringBuilder();
            sb.Append(
                string.Format(
                    CultureInfo.CurrentCulture,
                    "NodeHealth {0}: {1}, {2} events",
                    this.NodeName,
                    this.AggregatedHealthState,
                    this.HealthEvents.Count));

            if (this.UnhealthyEvaluations != null && this.UnhealthyEvaluations.Count > 0)
            {
                sb.Append(string.Format(CultureInfo.CurrentCulture, ", {0}", this.UnhealthyEvaluations));
            }

            return sb.ToString();
        }

        internal static unsafe NodeHealth FromNativeResult(NativeClient.IFabricNodeHealthResult nativeResult)
        {
            var nativePtr = nativeResult.get_NodeHealth();
            ReleaseAssert.AssertIf(nativePtr == IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "NodeHealth"));

            var retval = FromNative((NativeTypes.FABRIC_NODE_HEALTH*)nativePtr);

            GC.KeepAlive(nativeResult);
            return retval;
        }

        private static unsafe NodeHealth FromNative(NativeTypes.FABRIC_NODE_HEALTH* nativeHealth)
        {
            var managedHealth = new NodeHealth();

            managedHealth.AggregatedHealthState = (HealthState)nativeHealth->AggregatedHealthState;
            managedHealth.HealthEvents = HealthEvent.FromNativeList(nativeHealth->HealthEvents);
            managedHealth.NodeName = NativeTypes.FromNativeString(nativeHealth->NodeName);

            if (nativeHealth->Reserved != IntPtr.Zero)
            {
                var nativeHealthEx1 = (NativeTypes.FABRIC_NODE_HEALTH_EX1*)nativeHealth->Reserved;
                managedHealth.UnhealthyEvaluations = HealthEvaluation.FromNativeList(nativeHealthEx1->UnhealthyEvaluations);
            }

            return managedHealth;
        }
    }
}