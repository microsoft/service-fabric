// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Strings;

    /// <summary>
    /// <para>Represents the multiple filters that can be specified to refine the return.
    /// Used by <see cref="System.Fabric.FabricClient.QueryClient.GetServicePagedListAsync(System.Fabric.Description.ServiceQueryDescription, TimeSpan, System.Threading.CancellationToken)" />.</para>
    /// </summary>
    public sealed class ServiceQueryDescription : PagedQueryDescriptionBase
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.ServiceQueryDescription" /> class.</para>
        /// </summary>
        public ServiceQueryDescription(Uri applicationName)
        {
            ApplicationName = applicationName;
            ServiceNameFilter = null;
            ServiceTypeNameFilter = null;
        }

        /// <summary>
        /// <para>Gets the URI name of application that contains services to query for.</para>
        /// </summary>
        /// <value>
        /// <para>The URI name of application that contains services to query for.</para>
        /// </value>
        public Uri ApplicationName { get; private set; }

        /// <summary>
        /// <para>Gets or sets the URI name of service to query for.</para>
        /// </summary>
        /// <value>
        /// <para>The URI name of service to query for.</para>
        /// </value>
        /// <remarks>
        /// <para>ServiceNameFilter and ServiceTypeNameFilter can not be specified together.</para>
        /// <para>If neither ServiceNameFilter nor ServiceTypeNameFilter is specified, all services of the specified application are returned.</para>
        /// </remarks>
        public Uri ServiceNameFilter { get; set; }

        /// <summary>
        /// <para>Gets or sets the service type name used to filter the services to query for.
        /// Services that are of this service type will be returned.</para>
        /// </summary>
        /// <value>
        /// <para>The service type name used to filter the services to query for.</para>
        /// </value>
        /// <remarks>
        /// <para>ServiceNameFilter and ServiceTypeNameFilter can not be specified together.</para>
        /// <para>If neither ServiceNameFilter nor ServiceTypeNameFilter is specified, all services of the specified application are returned.</para>
        /// <para>This filter is case sensitive.</para>
        /// </remarks>
        public string ServiceTypeNameFilter { get; set; }

        internal static void Validate(ServiceQueryDescription description)
        {
            if (description.ServiceNameFilter != null && !string.IsNullOrEmpty(description.ServiceTypeNameFilter))
            {
                throw new ArgumentException(StringResources.Error_ServiceQueryDescriptionIncompatibleSettings);
            }
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeDescription = new NativeTypes.FABRIC_SERVICE_QUERY_DESCRIPTION();
            nativeDescription.ApplicationName = pinCollection.AddObject(this.ApplicationName);
            nativeDescription.ServiceNameFilter = pinCollection.AddObject(this.ServiceNameFilter);

            var ex1 = new NativeTypes.FABRIC_SERVICE_QUERY_DESCRIPTION_EX1();
            ex1.ContinuationToken = pinCollection.AddObject(this.ContinuationToken);

            var ex2 = new NativeTypes.FABRIC_SERVICE_QUERY_DESCRIPTION_EX2();
            ex2.ServiceTypeNameFilter = pinCollection.AddObject(this.ServiceTypeNameFilter);

            var ex3 = new NativeTypes.FABRIC_SERVICE_QUERY_DESCRIPTION_EX3();
            if (this.MaxResults.HasValue)
            {
                ex3.MaxResults = this.MaxResults.Value;
            }

            ex3.Reserved = IntPtr.Zero;
            ex2.Reserved = pinCollection.AddBlittable(ex3);
            ex1.Reserved = pinCollection.AddBlittable(ex2);
            nativeDescription.Reserved = pinCollection.AddBlittable(ex1);

            return pinCollection.AddBlittable(nativeDescription);
        }
    }
}