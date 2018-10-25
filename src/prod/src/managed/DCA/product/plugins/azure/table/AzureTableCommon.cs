// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;
    using System.Data.Services.Client;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Dca;
    using Microsoft.WindowsAzure.Storage;
    using Microsoft.WindowsAzure.Storage.RetryPolicies;
    using Microsoft.WindowsAzure.Storage.Table;

    // This is a utility class for Azure table uploaders. 
    class AzureTableCommon
    {
        // Constants
        internal const int MaxEntitiesInBatch = 100;
        internal const int HttpSuccessCodeMin = 200;
        internal const int HttpSuccessCodeMax = 299;
        internal const int HttpCodeEntityAlreadyExists = 409;
        internal const int HttpCodeResourceNotFound = 404;
        internal const int HttpCodePreconditionFailed = 412;

        internal static CloudTableClient CreateNewTableClient(CloudStorageAccount storageAccount)
        {
            CloudTableClient tableClient = storageAccount.CreateCloudTableClient();
            tableClient.DefaultRequestOptions.RetryPolicy = new ExponentialRetry();
            return tableClient;
        }

        internal enum TableServiceAction
        {
            SaveEntityBatchBegin,
            SaveEntityBatchEnd,
            QueryEntitiesForDeletion,
            DeleteEntityBatch
        }

        internal enum TableServiceExceptionAction
        {
            Abort,
            ProcessResponse
        }

        private static TableServiceExceptionAction ProcessStorageException(StorageException e)
        {
            if (Utility.IsNetworkError(e.RequestInformation.HttpStatusCode))
            {
                // We encountered a network error that wasn't resolved even
                // after retries.
                throw new MaxRetriesException();
            }

            return TableServiceExceptionAction.Abort;
        }

        internal static TableServiceExceptionAction ProcessTableServiceRequestException(FabricEvents.ExtensionsEvents traceSource, string logSourceId, Exception e, TableServiceAction failedAction)
        {
            traceSource.WriteError(
                logSourceId,
                "Exception encountered when performing operation {0}. Exception information: {1}",
                failedAction,
                e);

            StorageException eStorage = e as StorageException;
            if (null != eStorage)
            {
                return ProcessStorageException(eStorage);
            }

            return TableServiceExceptionAction.Abort;
        }

        internal static TableServiceExceptionAction ProcessTableServiceQueryException(FabricEvents.ExtensionsEvents traceSource, string logSourceId, Exception e, TableServiceAction failedAction)
        {
            traceSource.WriteError(
                logSourceId,
                "Exception encountered when performing operation {0}. Exception information: {1}",
                failedAction,
                e);

            DataServiceQueryException eDataServiceQuery = e as DataServiceQueryException;
            if (null != eDataServiceQuery)
            {
                if (Utility.IsNetworkError(eDataServiceQuery.Response.StatusCode))
                {
                    // We encountered a network error that wasn't resolved even
                    // after retries.
                    throw new MaxRetriesException();
                }
                else
                {
                    return TableServiceExceptionAction.Abort;
                }
            }

            StorageException eStorage = e as StorageException;
            if (null != eStorage)
            {
                return ProcessStorageException(eStorage);
            }

            return TableServiceExceptionAction.Abort;
        }
    }
}