// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using Text;

    /// <summary>
    /// <para>Describes a set of filters used when running the query
    /// <see cref="System.Fabric.FabricClient.QueryClient.GetApplicationTypePagedListAsync()" />.</para>
    /// </summary>
    /// <remarks>
    /// <para>The default values of this query description ensure that results are returned from the first page and apply no filters.
    /// This query description can be customized by setting individual properties.</para>
    /// </remarks>
    public sealed class PagedApplicationTypeQueryDescription
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="System.Fabric.Description.PagedApplicationTypeQueryDescription"/> class.
        /// </summary>
        public PagedApplicationTypeQueryDescription()
        {
            ApplicationTypeDefinitionKindFilter = ApplicationTypeDefinitionKindFilter.Default;
        }

        /// <summary>
        /// <para>Gets or sets the application type to get details for.</para>
        /// </summary>
        /// <remarks>
        /// <para>
        /// Defaults to null, which matches all the application types.
        /// </para>
        /// <para>
        /// This parameter matches against the case sensitive exact application type names defined in the application manifest
        /// of all provisioned or provisioning application types. For example, the value "Test" does not match "TestApp" because it is only a partial match.
        /// This value should not contain the version of the application type, and matches all versions of the same application type name.
        /// To request all the application types do not set this value.
        /// </para>
        /// <para>
        /// ApplicationTypeNameFilter and ApplicationTypeDefinitionKindFilter can not be specified together.
        /// </para>
        /// </remarks>
        public string ApplicationTypeNameFilter { get; set; }

        /// <summary>
        /// <para>
        /// Gets or sets the application type version to get details for.
        /// </para>
        /// </summary>
        /// <remarks>
        /// <para>
        /// Defaults to null, which matches all the application type versions of a given application type name.
        /// This filter should be provided only if an application type name filter is also provided.
        /// </para>
        /// <para>
        /// This parameter matches against the case sensitive exact application type version defined in the application manifest
        /// of all provisioned or provisioning application types.
        /// For example, version "v1" does not match version "v1.0" because it is only a partial match.
        /// To request all the application type versions of an application type name, do not set this value.
        /// </para>
        /// </remarks>
        public string ApplicationTypeVersionFilter { get; set; }

        /// <summary>
        /// <para>Gets or sets the continuation token which can be used to retrieve the next page.</para>
        /// </summary>
        /// <remarks>
        /// <para>
        /// Defaults to null, which returns page one of query results.
        /// </para>
        /// <para>
        /// If too many results respect the filter, they may not fit into one message.
        /// Paging is used to account for this by splitting the collection of <see cref="System.Fabric.Query.ApplicationType"/>s into separate pages.
        /// The continuation token is used to know where the previous page left off, carrying significance only to the query itself.
        /// This value should be generated from
        /// running this query, and can be passed into the next query request in order to get subsequent pages.
        /// A non-null continuation token value is returned as part of the result only if there is a subsequent page.
        /// </para>
        /// </remarks>
        public string ContinuationToken { get; set; }

        /// <summary>
        /// <para>Gets or sets whether to exclude application parameters from the query result.</para>
        /// </summary>
        /// <remarks>
        /// <para>
        /// Defaults to false.
        /// </para>
        /// <para>
        /// Setting this to true leaves the <see cref="System.Fabric.Query.ApplicationType.DefaultParameters" /> blank.
        /// </para>
        /// </remarks>
        public bool ExcludeApplicationParameters { get; set; }

        /// <summary>
        /// Gets or sets the max number of <see cref="System.Fabric.Query.ApplicationType" />s that can be returned per page.
        /// </summary>
        /// <remarks>
        /// <para>
        /// Defaults to the max value of type long.
        /// </para>
        /// <para>This defines only the upper bound for the number of application types returned, not a minimum.
        /// For example, if the page fits at most 1000 returned items according to max message size restrictions defined in the configuration,
        /// and the MaxResults value is set to 2000, then only 1000 results are returned, even if 2000 application types match
        /// the query description.
        /// </para>
        /// </remarks>
        public long MaxResults { get; set; }

        /// <summary>
        /// <para>Gets or sets the definition kind used to filter the application types that will be returned.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The definition kind used to filter the application types.</para>
        /// </value>
        /// <remarks>
        /// <para>If ApplicationTypeDefinitionKindFilter is not specified, the result would not be filtered by definition kind.</para>
        /// <para>ApplicationTypeNameFilter and ApplicationTypeDefinitionKindFilter can not be specified together.</para>
        /// </remarks>
        public ApplicationTypeDefinitionKindFilter ApplicationTypeDefinitionKindFilter { get; set; }

        internal static void Validate(PagedApplicationTypeQueryDescription description)
        {
            if (!string.IsNullOrEmpty(description.ApplicationTypeNameFilter) && description.ApplicationTypeDefinitionKindFilter != ApplicationTypeDefinitionKindFilter.Default)
            {
                throw new ArgumentException(StringResources.Error_ApplicationTypeQueryDescriptionIncompatibleSettings);
            }
        }

        /// <summary>
        /// Overrides ToString() method to print all content of the query description.
        /// </summary>
        /// <returns>
        /// Returns a string containing all the properties of the query description.
        /// </returns>
        public override string ToString()
        {
            var sb = new StringBuilder();

            sb.AppendFormat("System.Fabric.Description.PagedApplicationTypeQueryDescription object:{0}", Environment.NewLine);
            sb.AppendFormat("ApplicationTypeNameFilter = {0}{1}", this.ApplicationTypeNameFilter, Environment.NewLine);
            sb.AppendFormat("ApplicationTypeVersionFilter = {0}{1}", this.ApplicationTypeVersionFilter, Environment.NewLine);
            sb.AppendFormat("ApplicationTypeDefinitionKindFilter = {0}{1}", this.ApplicationTypeDefinitionKindFilter, Environment.NewLine);
            sb.AppendFormat("ContinuationToken = {0}{1}", this.ContinuationToken, Environment.NewLine);
            sb.AppendFormat("ExcludeApplicationParameters = {0}{1}", this.ExcludeApplicationParameters, Environment.NewLine);
            sb.AppendFormat("MaxResults = {0}", this.MaxResults);

            return sb.ToString();
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeDescription = new NativeTypes.FABRIC_APPLICATION_TYPE_PAGED_QUERY_DESCRIPTION();

            nativeDescription.ApplicationTypeNameFilter = pinCollection.AddObject(this.ApplicationTypeNameFilter);

            // bools are not blittable (directly readable in native)
            nativeDescription.ExcludeApplicationParameters = NativeTypes.ToBOOLEAN(this.ExcludeApplicationParameters);
            nativeDescription.ContinuationToken = pinCollection.AddObject(this.ContinuationToken);
            nativeDescription.MaxResults = this.MaxResults;

            var ex1 = new NativeTypes.FABRIC_APPLICATION_TYPE_PAGED_QUERY_DESCRIPTION_EX1();
            ex1.ApplicationTypeVersionFilter = pinCollection.AddObject(this.ApplicationTypeVersionFilter);

            var ex2 = new NativeTypes.FABRIC_APPLICATION_TYPE_PAGED_QUERY_DESCRIPTION_EX2();
            ex2.ApplicationTypeDefinitionKindFilter = (UInt32)this.ApplicationTypeDefinitionKindFilter;
            ex2.Reserved = IntPtr.Zero;

            ex1.Reserved = pinCollection.AddBlittable(ex2);

            nativeDescription.Reserved = pinCollection.AddBlittable(ex1);

            return pinCollection.AddBlittable(nativeDescription);
        }
    }
}