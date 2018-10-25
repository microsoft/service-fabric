// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.Text;

    /// <summary>
    /// Filter for <see cref="System.Fabric.Health.ApplicationHealthState"/> objects.
    /// </summary>
    /// <remarks>The entity health state chunk queries can specify a list of application filters to fine-grain
    /// select the applications that should be included in the query result.
    /// Note that all the applications are used to evaluate cluster aggregated health state, regardless of the filter passed in.</remarks>
    public sealed class ApplicationHealthStateFilter
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="System.Fabric.Health.ApplicationHealthStateFilter"/> class.
        /// </summary>
        public ApplicationHealthStateFilter()
        {
            this.DeployedApplicationFilters = new List<DeployedApplicationHealthStateFilter>();
            this.ServiceFilters = new List<ServiceHealthStateFilter>();
        }

        /// <summary>
        /// Gets or sets filter for the aggregated health state of the <see cref="System.Fabric.Health.ApplicationHealthState"/> entries in the collection. 
        /// </summary>
        /// <value>The filter for the aggregated health state of the <see cref="System.Fabric.Health.ApplicationHealthState"/> entries in the collection.</value>
        /// <remarks>The health state filter value comes from members or bitwise combination of members of <see cref="System.Fabric.Health.HealthStateFilter"/>. 
        /// For an application to match the filter, its aggregated health state must match the specified health state filter.
        /// If the health state filter is Default, the health state filter defaults to None if there are no specific filters (application name or application type name).
        /// Otherwise, the Default filter is considered to apply to All.
        /// </remarks>
        public HealthStateFilter HealthStateFilter
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the application name filter.
        /// </summary>
        /// <value>The application name filter.</value>
        /// <remarks>If specified, the filter applies only to the desired application. 
        /// For the application to match the filter, its aggregated health state must match
        /// the specified health state filter.
        /// </remarks>
        [JsonCustomization(IsDefaultValueIgnored = true)]
        public Uri ApplicationNameFilter
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the application type name filter.
        /// </summary>
        /// <value>The application type name filter.</value>
        /// <remarks>If specified, the filter applies to all applications of specified application type that don't have a more specific filter (for a specific application name).
        /// For an application to match the filter, its aggregated health state must match the specified health state filter.
        /// </remarks>
        [JsonCustomization(IsDefaultValueIgnored = true)]
        public string ApplicationTypeNameFilter
        {
            get;
            set;
        }

        /// <summary>
        /// Gets the list of <see cref="System.Fabric.Health.ServiceHealthStateFilter"/> to be applied to the service children health states.
        /// </summary>
        /// <value>The list of <see cref="System.Fabric.Health.ServiceHealthStateFilter"/> to be applied to the service children health states.</value>
        /// <remarks>The list can contain one default service filter or service filters for specific services to fine-grain entities returned by the query.
        /// All service children that match the filter will be returned as children of the application.
        /// If empty, the children are not returned by default.</remarks>
        public IList<ServiceHealthStateFilter> ServiceFilters
        {
            get;
            internal set;
        }

        /// <summary>
        /// Gets the list of <see cref="System.Fabric.Health.DeployedApplicationHealthStateFilter"/> to be applied to the deployed application children health states.
        /// </summary>
        /// <value>The list of <see cref="System.Fabric.Health.DeployedApplicationHealthStateFilter"/> to be applied to the deployed application children health states.</value>
        /// <remarks>The list can contain one default deployed application filter or deployed application filters for specific deployed applications to fine-grain entities returned by the query.
        /// All deployed application children that match the filter will be returned as children of the application.
        /// If empty, the children are not returned by default.</remarks>
        public IList<DeployedApplicationHealthStateFilter> DeployedApplicationFilters
        {
            get;
            internal set;
        }

        /// <summary>
        /// Returns a string representation of the filter.
        /// </summary>
        /// <returns>A string representation of the filter.</returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder("ApplicationHealthStateFilter: ");
            if (this.ApplicationNameFilter != null)
            {
                sb.Append(string.Format(CultureInfo.CurrentCulture, "{0}, ", this.ApplicationNameFilter));
            }

            if (!string.IsNullOrEmpty(this.ApplicationTypeNameFilter))
            {
                sb.Append(string.Format(CultureInfo.CurrentCulture, "{0}, ", this.ApplicationTypeNameFilter));
            }

            sb.Append(string.Format(CultureInfo.CurrentCulture, "{0}", this.HealthStateFilter));
            return sb.ToString();
        }

        internal void ToNative(PinCollection pinCollection, ref NativeTypes.FABRIC_APPLICATION_HEALTH_STATE_FILTER publicFilter)
        {
            publicFilter.HealthStateFilter = (UInt32)this.HealthStateFilter;
            publicFilter.ApplicationNameFilter = pinCollection.AddObject(this.ApplicationNameFilter);
            publicFilter.ServiceFilters = ServiceHealthStateFilter.ToNativeList(pinCollection, this.ServiceFilters);
            publicFilter.DeployedApplicationFilters = DeployedApplicationHealthStateFilter.ToNativeList(pinCollection, this.DeployedApplicationFilters);

            if (!string.IsNullOrEmpty(this.ApplicationTypeNameFilter))
            {
                var ex1 = new NativeTypes.FABRIC_APPLICATION_HEALTH_STATE_FILTER_EX1[1];
                ex1[0].ApplicationTypeNameFilter = pinCollection.AddObject(this.ApplicationTypeNameFilter);
                ex1[0].Reserved = IntPtr.Zero;
                publicFilter.Reserved = pinCollection.AddBlittable(ex1);
            }
        }

        internal static IntPtr ToNativeList(PinCollection pinCollection, IList<ApplicationHealthStateFilter> applicationList)
        {
            if (applicationList == null || applicationList.Count == 0)
            {
                return IntPtr.Zero;
            }

            var nativeArray = new NativeTypes.FABRIC_APPLICATION_HEALTH_STATE_FILTER[applicationList.Count];

            for (int i = 0; i < applicationList.Count; i++)
            {
                applicationList[i].ToNative(pinCollection, ref nativeArray[i]);
            }

            var nativeList = new NativeTypes.FABRIC_APPLICATION_HEALTH_STATE_FILTER_LIST();
            nativeList.Count = (uint)applicationList.Count;
            nativeList.Items = pinCollection.AddBlittable(nativeArray);

            return pinCollection.AddBlittable(nativeList);
        }
    }
}
