// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Fabric.Query;

namespace System.Fabric.Health
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Description;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.Text;

    /// <summary>
    /// Filter for <see cref="System.Fabric.Health.DeployedServicePackageHealthState"/> objects.
    /// </summary>
    /// <remarks>The health state chunk queries can specify a list of deployed service package filters to fine-grain
    /// select the deployed service packages that should be included in the query result.
    /// Note that all the deployed service packages are used to evaluate parents aggregated health state, regardless of the filter passed in.</remarks>
    public sealed class DeployedServicePackageHealthStateFilter
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="System.Fabric.Health.DeployedServicePackageHealthStateFilter"/> class.
        /// </summary>
        public DeployedServicePackageHealthStateFilter()
        {   
        }

        /// <summary>
        /// Gets or sets filter for the aggregated health state of the <see cref="System.Fabric.Health.DeployedServicePackageHealthState"/> entries in the collection. 
        /// </summary>
        /// <value>The filter for the aggregated health state of the <see cref="System.Fabric.Health.DeployedServicePackageHealthState"/> entries in the collection.</value>
        /// <remarks>The health state filter value comes from members or bitwise combination of members of <see cref="System.Fabric.Health.HealthStateFilter"/>. 
        /// For a deployed service package to match the filter, its aggregated health state must match the specified health state filter.</remarks>
        public HealthStateFilter HealthStateFilter
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the service manifest name filter.
        /// </summary>
        /// <value>The service manifest name filter.</value>
        /// <remarks>If not specified, all deployed service packages that match the parent filters (if any) and the specified health state filters match the filter.
        /// Otherwise, the filter only applies to the deployed service package for the specified service manifest name and the health state filter will be matched against it.</remarks>
        [JsonCustomization(IsDefaultValueIgnored = true)]
        public string ServiceManifestNameFilter
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the service package ActivationId filter.
        /// </summary>
        /// <value>
        /// <para>
        /// The <see cref="System.Fabric.Query.DeployedServicePackage.ServicePackageActivationId"/> of deployed service package can be obtained
        ///  by using <see cref="System.Fabric.FabricClient.QueryClient.GetDeployedServicePackageListAsync(string, System.Uri)"/> query. 
        /// </para>
        /// <para>
        /// If <see cref="System.Fabric.Description.ServicePackageActivationMode"/>
        /// specified at the time of creating the service was <see cref="System.Fabric.Description.ServicePackageActivationMode.SharedProcess"/> (or if 
        /// it was not specified, in which case it defaults to <see cref="System.Fabric.Description.ServicePackageActivationMode.SharedProcess"/>), then 
        /// value of <see cref="System.Fabric.Query.DeployedServicePackage.ServicePackageActivationId"/> is always an empty string.
        /// For more details please see <see cref="System.Fabric.Description.ServicePackageActivationMode"/>.
        /// </para>
        /// </value>
        [JsonCustomization(IsDefaultValueIgnored = true)]
        public string ServicePackageActivationIdFilter
        {
            get;
            set;
        }

        /// <summary>
        /// Returns a string representation of the filter.
        /// </summary>
        /// <returns>A string representation of the filter.</returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder("DeployedServicePackageHealthStateFilter: ");
            if (!string.IsNullOrWhiteSpace(this.ServiceManifestNameFilter))
            {
                sb.Append(string.Format(CultureInfo.CurrentCulture, " ServiceManifestNameFilter={0}, ", this.ServiceManifestNameFilter));
            }

            if (this.ServicePackageActivationIdFilter != null)
            {
                sb.Append(string.Format(CultureInfo.CurrentCulture, " ServicePackageActivationIdFilter={0}, ", this.ServicePackageActivationIdFilter));
            }

            sb.Append(string.Format(CultureInfo.CurrentCulture, "{0}", this.HealthStateFilter));
            
            return sb.ToString();
        }

        internal static IntPtr ToNativeList(PinCollection pinCollection, IList<DeployedServicePackageHealthStateFilter> deployedServicePackageList)
        {
            if (deployedServicePackageList == null || deployedServicePackageList.Count == 0)
            {
                return IntPtr.Zero;
            }

            var nativeArray = new NativeTypes.FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATE_FILTER[deployedServicePackageList.Count];

            for (int i = 0; i < deployedServicePackageList.Count; i++)
            {
                nativeArray[i].HealthStateFilter = (UInt32)deployedServicePackageList[i].HealthStateFilter;
                nativeArray[i].ServiceManifestNameFilter = pinCollection.AddObject(deployedServicePackageList[i].ServiceManifestNameFilter);

                if (deployedServicePackageList[i].ServicePackageActivationIdFilter != null)
                {
                    var ex1 = new NativeTypes.FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATE_FILTER_EX1();
                    ex1.ServicePackageActivationIdFilter = pinCollection.AddBlittable(deployedServicePackageList[i].ServicePackageActivationIdFilter);
                    nativeArray[i].Reserved = pinCollection.AddBlittable(ex1);
                }
            }

            var nativeList = new NativeTypes.FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATE_FILTER_LIST();
            nativeList.Count = (uint)deployedServicePackageList.Count;
            nativeList.Items = pinCollection.AddBlittable(nativeArray);

            return pinCollection.AddBlittable(nativeList);
        }
    }
}