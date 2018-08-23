// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using Globalization;
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;
    using System.Runtime.InteropServices;

    /// <summary>
    /// <para>
    /// Defines a map with max percentages unhealthy applications for specific application types. 
    /// </para>
    /// </summary>
    /// <remarks>The application type health policy map can be used during cluster health evaluation
    /// to describe special application types. The application types included in the map are evaluated against the percentage
    /// included in the map, and not with the global <see cref="System.Fabric.Health.ClusterHealthPolicy.MaxPercentUnhealthyApplications"/>.
    /// The applications of application types specified in the map are not counted against the 
    /// global pool of applications.</remarks>
    public sealed class ApplicationTypeHealthPolicyMap : Dictionary<string, byte>
    {
        /// <summary>
        /// <para>
        /// Instantiates an <see cref="System.Fabric.Health.ApplicationTypeHealthPolicyMap" /> class.
        /// </para>
        /// </summary>
        public ApplicationTypeHealthPolicyMap() : base()
        {
        }

        [JsonCustomization(PropertyName = JsonPropertyNames.ApplicationTypeHealthPolicyMap)]
        internal List<KeyValuePair<string, byte>> ApplicationTypeHealthPolicyMapWrapper
        {
            get
            {
                return new List<KeyValuePair<string, byte>>(this);
            }
            set
            {
                var emptyOrValue = (value == null) ? null : new List<KeyValuePair<string, byte>>();
                emptyOrValue.ForEach(kv => this.Add(kv.Key, kv.Value));
            }
        }

        /// <summary>
        /// <para>
        /// Adds an entry in the map for a specific application type, with max percent unhealthy applications as value.
        /// </para>
        /// </summary>
        /// <param name="appTypeName">
        /// <para>The application type name.</para>
        /// </param>
        /// <param name="value">
        /// <para>The max percent unhealthy applications allowed for the application type. Must be between 0 and 100.</para>
        /// </param>
        /// <exception cref="System.ArgumentOutOfRangeException">
        /// <para>The specified percentage value was outside the range of integer values from zero to 100.</para>
        /// </exception>p
        public new void Add(string appTypeName, byte value)
        {
            Requires.CheckPercentageArgument(value, "value");
            base.Add(appTypeName, value);
        }

        /// <summary>
        /// Gets a string representation of the <see cref="System.Fabric.Health.ApplicationTypeHealthPolicyMap"/>.
        /// </summary>
        /// <returns>A string representation of the <see cref="System.Fabric.Health.ApplicationTypeHealthPolicyMap"/>.</returns>
        public override string ToString()
        {
            return string.Format(
                CultureInfo.CurrentCulture,
                "ApplicationTypeHealthPolicyMap: {0} entries",
                this.Count);
        }

        internal static bool AreEqual(ApplicationTypeHealthPolicyMap current, ApplicationTypeHealthPolicyMap other)
        {
            if ((current != null) && (other != null))
            {
                bool equals = (current.Count == other.Count);

                if (!equals) { return false; }

                foreach (var currentItem in current)
                {
                    byte otherValue;
                    if (!other.TryGetValue(currentItem.Key, out otherValue))
                    {
                        return false;
                    }

                    equals = (currentItem.Value == otherValue);
                    if (!equals)
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
                var nativeApplicationTypeHealthPolicyMap = new NativeTypes.FABRIC_APPLICATION_TYPE_HEALTH_POLICY_MAP();

                var nativeArray = new NativeTypes.FABRIC_APPLICATION_TYPE_HEALTH_POLICY_MAP_ITEM[this.Count];

                int ix = 0;
                foreach (var pair in this)
                {
                    nativeArray[ix].ApplicationTypeName = pin.AddObject(pair.Key);
                    nativeArray[ix].MaxPercentUnhealthyApplications = pair.Value;
                    ++ix;
                }

                nativeApplicationTypeHealthPolicyMap.Count = (uint)nativeArray.Length;
                nativeApplicationTypeHealthPolicyMap.Items = pin.AddBlittable(nativeArray);

                return pin.AddBlittable(nativeApplicationTypeHealthPolicyMap);
            }
            else
            {
                return IntPtr.Zero;
            }
        }

        internal unsafe void FromNative(IntPtr nativeApplicationTypeHealthPolicyMapPtr)
        {
            if (nativeApplicationTypeHealthPolicyMapPtr == IntPtr.Zero)
            {
                return;
            }

            var nativeApplicationTypeHealthPolicyMap = (NativeTypes.FABRIC_APPLICATION_TYPE_HEALTH_POLICY_MAP*)nativeApplicationTypeHealthPolicyMapPtr;

            var offset = Marshal.SizeOf(typeof(NativeTypes.FABRIC_APPLICATION_TYPE_HEALTH_POLICY_MAP_ITEM));
            for (int i = 0; i < nativeApplicationTypeHealthPolicyMap->Count; i++)
            {
                var nativeMapItemPtr = nativeApplicationTypeHealthPolicyMap->Items + (i * offset);
                var nativeMapItem = (NativeTypes.FABRIC_APPLICATION_TYPE_HEALTH_POLICY_MAP_ITEM*)nativeMapItemPtr;

                string applicationTypeName = NativeTypes.FromNativeString(nativeMapItem->ApplicationTypeName);
                byte maxPercentUnhealthyApplications = nativeMapItem->MaxPercentUnhealthyApplications;

                this.Add(applicationTypeName, maxPercentUnhealthyApplications);
            }
        }
    }
}