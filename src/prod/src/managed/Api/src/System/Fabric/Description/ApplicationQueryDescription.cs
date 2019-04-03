// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Interop;
    using System.Fabric.Strings;

    /// <summary>
    /// <para>Represents the query input used by <see cref="System.Fabric.FabricClient.QueryClient.GetApplicationPagedListAsync(System.Fabric.Description.ApplicationQueryDescription, TimeSpan, System.Threading.CancellationToken)" />.</para>
    /// </summary>
    public sealed class ApplicationQueryDescription : PagedQueryDescriptionBase
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.ApplicationQueryDescription" /> class.</para>
        /// </summary>
        public ApplicationQueryDescription()
        {
            ApplicationNameFilter = null;
            ApplicationTypeNameFilter = null;
            ApplicationDefinitionKindFilter = ApplicationDefinitionKindFilter.Default;
            ExcludeApplicationParameters = false;
        }

        /// <summary>
        /// <para>Gets or sets the URI name of application to query for.</para>
        /// </summary>
        /// <value>
        /// <para>The URI name of application to query for.</para>
        /// </value>
        /// <remarks>
        /// <para>At most one of ApplicationNameFilter, ApplicationTypeNameFilter, or ApplicationDefinitionKindFilter can be specified.</para>
        /// <para>If no filters are specified, all applications which fit a page are returned.</para>
        /// </remarks>
        public Uri ApplicationNameFilter { get; set; }

        /// <summary>
        /// <para>Gets or sets the application type name used to filter the applications to query for.
        /// Applications that are of this application type will be returned.</para>
        /// </summary>
        /// <value>
        /// <para>The application type name used to filter the applications to query for.</para>
        /// </value>
        /// <remarks>
        /// <para>At most one of ApplicationNameFilter, ApplicationTypeNameFilter, or ApplicationDefinitionKindFilter can be specified.</para>
        /// <para>If no filters are specified, all applications which fit a page are returned.</para>
        /// <para>This filter is case sensitive.</para>
        /// </remarks>
        public string ApplicationTypeNameFilter { get; set; }

        /// <summary>
        /// <para>Gets or sets the flag that specifies whether application parameters will be excluded from the result.</para>
        /// </summary>
        /// <value>
        /// <para>Flag that specifies whether application parameters will be excluded from the result.</para>
        /// </value>
        /// <remarks>
        /// <para>This flag can be set to true in order to limit the result size when parameters are huge.
        /// Default value is false.</para>
        /// </remarks>
        public bool ExcludeApplicationParameters { get; set; }

        /// <summary>
        /// <para>Gets or sets the definition kind used to filter the applications that will be returned.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The definition kind used to filter the applications.</para>
        /// </value>
        /// <remarks>
        /// <para>At most one of ApplicationNameFilter, ApplicationTypeNameFilter, or ApplicationDefinitionKindFilter can be specified.</para>
        /// <para>If no filters are specified, all applications are returned which fits a page.</para>
        /// </remarks>
        public ApplicationDefinitionKindFilter ApplicationDefinitionKindFilter { get; set; }

        private static void ExclusiveFilterHelper(bool isValid, ref bool hasFilterSet)
        {
            if (isValid)
            {
                if (hasFilterSet)
                {
                    throw new ArgumentException(StringResources.Error_ApplicationQueryDescriptionIncompatibleSettings);
                }
                else
                {
                    hasFilterSet = true;
                }
            }
        }

        internal static void Validate(ApplicationQueryDescription description)
        {
            bool hasFilterSet = false;

            ExclusiveFilterHelper(description.ApplicationNameFilter != null, ref hasFilterSet);

            ExclusiveFilterHelper(!string.IsNullOrEmpty(description.ApplicationTypeNameFilter), ref hasFilterSet);

            ExclusiveFilterHelper(description.ApplicationDefinitionKindFilter != ApplicationDefinitionKindFilter.Default, ref hasFilterSet);
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeDescription = new NativeTypes.FABRIC_APPLICATION_QUERY_DESCRIPTION();
            nativeDescription.ApplicationNameFilter = pinCollection.AddObject(this.ApplicationNameFilter);

            var ex1 = new NativeTypes.FABRIC_APPLICATION_QUERY_DESCRIPTION_EX1();
            ex1.ContinuationToken = pinCollection.AddObject(this.ContinuationToken);

            var ex2 = new NativeTypes.FABRIC_APPLICATION_QUERY_DESCRIPTION_EX2();
            ex2.ApplicationTypeNameFilter = pinCollection.AddObject(this.ApplicationTypeNameFilter);
            ex2.ExcludeApplicationParameters = NativeTypes.ToBOOLEAN(this.ExcludeApplicationParameters);

            var ex3 = new NativeTypes.FABRIC_APPLICATION_QUERY_DESCRIPTION_EX3();
            ex3.ApplicationDefinitionKindFilter = (UInt32)this.ApplicationDefinitionKindFilter;

            var ex4 = new NativeTypes.FABRIC_APPLICATION_QUERY_DESCRIPTION_EX4();
            if (this.MaxResults.HasValue)
            {
                ex4.MaxResults = this.MaxResults.Value;
            }

            ex4.Reserved = IntPtr.Zero;
            ex3.Reserved = pinCollection.AddBlittable(ex4);

            ex2.Reserved = pinCollection.AddBlittable(ex3);

            ex1.Reserved = pinCollection.AddBlittable(ex2);

            nativeDescription.Reserved = pinCollection.AddBlittable(ex1);

            return pinCollection.AddBlittable(nativeDescription);
        }
    }

}