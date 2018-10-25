// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using Runtime.InteropServices;
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;
    using System.Globalization;

    /// <summary>
    /// <para>Represents a map that contains specific application health policies for different applications.</para>
    /// </summary>
    /// <remarks>Can be used by queries like <see cref="System.Fabric.FabricClient.HealthClient.GetClusterHealthAsync(Description.ClusterHealthQueryDescription)"/> or <see cref="System.Fabric.FabricClient.HealthClient.GetClusterHealthChunkAsync(Description.ClusterHealthChunkQueryDescription)"/>
    /// to determine how the applications are evaluated.
    /// </remarks>
    public sealed class ApplicationHealthPolicyMap : Dictionary<Uri, ApplicationHealthPolicy>
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="System.Fabric.Health.ApplicationHealthPolicyMap"/> class.
        /// </summary>
        public ApplicationHealthPolicyMap()
        {
        }

        /// <summary>
        /// Gets a string representation of the <see cref="System.Fabric.Health.ApplicationHealthPolicyMap"/>.
        /// </summary>
        /// <returns>A string representation of the <see cref="System.Fabric.Health.ApplicationHealthPolicyMap"/>.</returns>
        public override string ToString()
        {
            return string.Format(
                CultureInfo.CurrentCulture,
                "ApplicationHealthPolicyMap: {0} entries",
                this.Count);
        }

        //// TODO: Add test for this class. RDBUG # 3290604
        [JsonCustomization(PropertyName = JsonPropertyNames.ApplicationHealthPolicyMap)]
        internal List<KeyValuePair<Uri, ApplicationHealthPolicy>> ApplicationHealthPolicyMapWrapper
        {
            get
            {
                return new List<KeyValuePair<Uri, ApplicationHealthPolicy>>(this);
            }
            set
            {
                var emptyOrValue = value == null ? null : new List<KeyValuePair<Uri, ApplicationHealthPolicy>>();
                emptyOrValue.ForEach(kv => this.Add(kv.Key, kv.Value));
            }
        }

        internal static bool AreEqual(ApplicationHealthPolicyMap current, ApplicationHealthPolicyMap other)
        {
            if ((current != null) && (other != null))
            {
                bool equals = (current.Count == other.Count);
                if (!equals) { return false; }

                foreach (var entry in current)
                {
                    ApplicationHealthPolicy otherValue;
                    var success = other.TryGetValue(entry.Key, out otherValue);

                    if (!success)
                    {
                        return false;
                    }

                    if (!ApplicationHealthPolicy.AreEqual(entry.Value, otherValue))
                    {
                        return false;
                    }
                }

                return true;
            }
            else
            {
                return (current == null) && (other == null);
            }
        }

        internal unsafe IntPtr ToNative(PinCollection pin)
        {
            if (this.Count > 0)
            {
                var nativeApplicationHealthPolicyMap = new NativeTypes.FABRIC_APPLICATION_HEALTH_POLICY_MAP();

                var nativeArray = new NativeTypes.FABRIC_APPLICATION_HEALTH_POLICY_MAP_ITEM[this.Count];

                int ix = 0;
                foreach (var pair in this)
                {
                    nativeArray[ix].ApplicationName = pin.AddObject(pair.Key);
                    nativeArray[ix].HealthPolicy = pair.Value.ToNative(pin);
                    ix++;
                }

                nativeApplicationHealthPolicyMap.Count = (uint)nativeArray.Length;
                nativeApplicationHealthPolicyMap.Items = pin.AddBlittable(nativeArray);

                return pin.AddBlittable(nativeApplicationHealthPolicyMap);
            }
            else
            {
                return IntPtr.Zero;
            }
        }

        internal unsafe void FromNative(IntPtr nativeApplicationHealthPolicyMapPtr)
        {
            if (nativeApplicationHealthPolicyMapPtr != IntPtr.Zero)
            {
                var nativeApplicationHealthPolicyMap = *(NativeTypes.FABRIC_APPLICATION_HEALTH_POLICY_MAP*)nativeApplicationHealthPolicyMapPtr;

                var offset = Marshal.SizeOf(typeof(NativeTypes.FABRIC_APPLICATION_HEALTH_POLICY_MAP_ITEM));
                for (int i = 0; i < nativeApplicationHealthPolicyMap.Count; i++)
                {
                    var nativeMapItemPtr = nativeApplicationHealthPolicyMap.Items + (i * offset);
                    var nativeMapItem = (NativeTypes.FABRIC_APPLICATION_HEALTH_POLICY_MAP_ITEM*)nativeMapItemPtr;

                    var applicationName = NativeTypes.FromNativeUri(nativeMapItem->ApplicationName);
                    var applicationHealthPolicy = ApplicationHealthPolicy.FromNative(nativeMapItem->HealthPolicy);

                    this.Add(applicationName, applicationHealthPolicy);
                }
            }
        }
    }
}