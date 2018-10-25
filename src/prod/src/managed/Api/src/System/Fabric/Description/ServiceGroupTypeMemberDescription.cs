// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Collections.ObjectModel;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Runtime.InteropServices;

    /// <summary>
    /// <para>Represents the type of member description for the service group.</para>
    /// </summary>
    public sealed class ServiceGroupTypeMemberDescription
    {
        private KeyedItemCollection<string, ServiceLoadMetricDescription> loadMetrics = new KeyedItemCollection<string, ServiceLoadMetricDescription>(d => d.Name);

        /// <summary>
        /// <para>initializes a new instance of the <see cref="System.Fabric.Description.ServiceGroupTypeMemberDescription" /> class.</para>
        /// </summary>
        public ServiceGroupTypeMemberDescription()
        {
        }

        /// <summary>
        /// <para>Gets the name of the service type.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the service type.</para>
        /// </value>
        public string ServiceTypeName
        {
            get;
            set;
        }

        /// <summary>
        /// <para>Gets or sets the collection of <see cref="System.Fabric.Description.ServiceLoadMetricDescription" /> for this service.</para>
        /// </summary>
        /// <value>
        /// <para>The collection of <see cref="System.Fabric.Description.ServiceLoadMetricDescription" /> for this service.</para>
        /// </value>
        public KeyedCollection<string, ServiceLoadMetricDescription> LoadMetrics
        {
            get { return this.loadMetrics; }
        }

        internal static unsafe ServiceGroupTypeMemberDescription CreateFromNative(IntPtr descriptionPtr, bool isStateful)
        {
            NativeTypes.FABRIC_SERVICE_GROUP_TYPE_MEMBER_DESCRIPTION* nativeDescription = (NativeTypes.FABRIC_SERVICE_GROUP_TYPE_MEMBER_DESCRIPTION*)descriptionPtr;

            ServiceGroupTypeMemberDescription description = new ServiceGroupTypeMemberDescription()
            {
                ServiceTypeName = NativeTypes.FromNativeString(nativeDescription->ServiceTypeName)
            };

            NativeTypes.FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION_LIST* nativeMetrics = (NativeTypes.FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION_LIST*)nativeDescription->LoadMetrics;

            if (nativeMetrics != null && nativeMetrics->Count > 0)
            {
                for (int i = 0; i < nativeMetrics->Count; ++i)
                {
                    description.loadMetrics.Add(ServiceLoadMetricDescription.CreateFromNative(nativeMetrics->Items + (i * Marshal.SizeOf(typeof(NativeTypes.FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION))), isStateful));
                }
            }

            return description;
        }
    }
}