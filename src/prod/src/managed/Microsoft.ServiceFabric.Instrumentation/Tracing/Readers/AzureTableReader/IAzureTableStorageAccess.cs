// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Readers.AzureTableReader
{
    using System;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// Interface that represents the information needed to connect to
    /// Azure storage account
    /// </summary>
    public interface IAzureTableStorageAccess
    {
        /// <summary>
        /// Check if a Table exists
        /// </summary>
        /// <param name="tableName"></param>
        /// <param name="token"></param>
        /// <returns></returns>
        Task<bool> DoesTableExistsAsync(string tableName, CancellationToken token);

        /// <summary>
        /// Execute a Query
        /// </summary>
        /// <param name="tableName"></param>
        /// <param name="token"></param>
        /// <returns></returns>
        Task<AzureTableQueryResult> ExecuteQueryAsync(string tableName, CancellationToken token);

        /// <summary>
        /// Execute a Query.
        /// </summary>
        /// <param name="tableName"></param>
        /// <param name="filterBuilder">Create the filter</param>
        /// <param name="propertyFilter"></param>
        /// <param name="clientSideFilter">Client side filter function</param>
        /// <param name="startTime"></param>
        /// <param name="endTime"></param>
        /// <param name="token"></param>
        /// <returns></returns>
        Task<AzureTableQueryResult> ExecuteQueryAsync(string tableName, IFilterBuilder filterBuilder, string propertyFilter, Func<AzureTablePropertyBag, bool> clientSideFilter, DateTimeOffset startTime, DateTimeOffset endTime, CancellationToken token);

        /// <summary>
        /// Execute a Query.
        /// </summary>
        /// <param name="tableName"></param>
        /// <param name="filterBuilder"></param>
        /// <param name="propertyFilter"></param>
        /// <param name="clientSideFilter">Client side filter function</param>        
        /// <param name="startTime"></param>
        /// <param name="endTime"></param>
        /// <param name="takeCount"></param>
        /// <param name="token"></param>
        /// <returns></returns>
        Task<AzureTableQueryResult> ExecuteQueryAsync(string tableName, IFilterBuilder filterBuilder, string propertyFilter, Func<AzureTablePropertyBag, bool> clientSideFilter, DateTimeOffset startTime, DateTimeOffset endTime, int takeCount, CancellationToken token);

        /// <summary>
        /// Release the Access.
        /// </summary>
        /// <param name="token"></param>
        /// <returns></returns>
        Task ReleaseAccessAsync(CancellationToken token);
    }
}