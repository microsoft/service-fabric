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
    /// <para>Represents the base class for all paged query descriptions.
    /// This class handles all functionality related to paging.</para>
    /// </summary>
    /// <remarks>
    /// <para>The default values of this query description ensure that results are returned from the first page and apply no paging related filters.
    /// This query description can be customized by setting individual properties.</para>
    /// </remarks>
    public abstract class PagedQueryDescriptionBase
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="System.Fabric.Description.PagedQueryDescriptionBase"/> class.
        /// </summary>
        protected PagedQueryDescriptionBase() { }

        /// <summary>
        /// <para>Gets or sets the continuation token which can be used to retrieve the next page.</para>
        /// </summary>
        /// <remarks>
        /// <para>
        /// Defaults to null, which returns page one of query results.
        /// </para>
        /// <para>
        /// If too many results respect the filter, they may not fit into one message.
        /// Paging is used to account for this by splitting the collection of returned results into separate pages.
        /// The continuation token is used to know where the previous page left off, carrying significance only to the query itself.
        /// This value should be generated from
        /// running this query, and can be passed into the next query request in order to get subsequent pages.
        /// A non-null continuation token value is returned as part of the result only if there is a subsequent page.
        /// </para>
        /// </remarks>
        public string ContinuationToken { get; set; }

        /// <summary>
        /// Gets or sets the max number of result items that can be returned per page.
        /// </summary>
        /// <remarks>
        /// <para>
        /// If value is null, then no restrictions are placed on the number of results per page.
        /// </para>
        /// <para>This defines only the upper bound for the number of results returned, not a minimum.
        /// For example, if the page fits at most 1000 returned items according to max message size restrictions defined in the configuration,
        /// and the MaxResults value is set to 2000, then only 1000 results are returned, even if 2000 result items match
        /// the query description.
        /// </para>
        /// </remarks>
        public long? MaxResults { get; set; }

        /// <summary>
        /// Overrides ToString() method to print all content of the query description.
        /// </summary>
        /// <returns>
        /// Returns a string containing all the properties of the query description.
        /// </returns>
        public override string ToString()
        {
            var sb = new StringBuilder();

            sb.AppendFormat("PagedQueryDescriptionBase:{0}", Environment.NewLine);
            sb.AppendFormat("ContinuationToken = {0}{1}", this.ContinuationToken, Environment.NewLine);
            sb.AppendFormat("MaxResults = {0}", this.MaxResults);

            return sb.ToString();
        }

        internal unsafe IntPtr ToNativePagingDescription(PinCollection pinCollection)
        {
            var nativeDescription = new NativeTypes.FABRIC_QUERY_PAGING_DESCRIPTION();

            if (this.ContinuationToken != null)
            {
                nativeDescription.ContinuationToken = pinCollection.AddObject(this.ContinuationToken);
            }

            if (this.MaxResults.HasValue)
            {
                nativeDescription.MaxResults = (long)this.MaxResults.Value;
            }

            nativeDescription.Reserved = IntPtr.Zero;
            return pinCollection.AddBlittable(nativeDescription);
        }

        static internal void ValidatePaging(PagedQueryDescriptionBase description)
        {
            if (description.MaxResults.HasValue && description.MaxResults.Value <= 0)
            {
                throw new ArgumentException(StringResources.Error_InvalidMaxResults);
            }
        }
    }
}