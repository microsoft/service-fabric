// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Common
{

    using System.Fabric.Query;
    using System.Threading;
    using System.Threading.Tasks;

    internal static class FabricClientHelper
    {
        private static FabricClient FabricClient = new FabricClient();
        private static bool isFabricClientRecycleInProgress = false;
        private static readonly TimeSpan TimeOut = new TimeSpan(5000);
        private static readonly object RecycleLock = new object();
        private static readonly string FaulAnaylisServiceUri = @"fabric:/System/FaultAnalysisService";

        #region Application,Service and Partition Validation Functions

        internal static async Task<bool> ValidateApplicationUri(string applicationUri, TimeSpan timeout , CancellationToken cancellationToken)
        {
            var serviceList = await InvokeWithRetryAsync<ServiceList>( () => FabricClient.QueryManager.GetServiceListAsync(new Uri(UtilityHelper.GetUriFromCustomUri(applicationUri)),null,timeout,cancellationToken) );
            return serviceList != null;
        }

        internal static async Task<bool> ValidatePartition(string serviceUri, string partitionId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            var serviceReplicaList = await InvokeWithRetryAsync(() => 
                    FabricClient.QueryManager.GetReplicaListAsync(Guid.Parse(partitionId), 0, timeout, cancellationToken)
                );
            return serviceReplicaList != null;
        }

        internal static async Task<bool> ValidateServiceUri(string serviceUri, TimeSpan timeout, CancellationToken cancellationToken)
        {
            var applicationUri = UtilityHelper.GetApplicationUriFromServiceUri(serviceUri);
            var serviceList = await InvokeWithRetryAsync(() =>
                FabricClient.QueryManager.GetServiceListAsync(new Uri(UtilityHelper.GetUriFromCustomUri(applicationUri)), new Uri(UtilityHelper.GetUriFromCustomUri(serviceUri)), timeout, cancellationToken)
                );

            if (serviceList.Count == 0)
            {
                throw new FabricException(StringResources.ServiceNotFound,
                       FabricErrorCode.ServiceNotFound);
            }

            if (serviceList?[0] != null && serviceList[0].ServiceKind == ServiceKind.Stateful && ((StatefulService)serviceList[0]).HasPersistedState)
            {
                return true;
            }

            throw new FabricException(StringResources.InvalidForStatelessServices, FabricErrorCode.InvalidForStatelessServices);
        }

        internal static async Task IsFaultServiceExisting(TimeSpan timeout, CancellationToken cancellationToken)
        {
            var applicationUri = UtilityHelper.GetApplicationUriFromServiceUri(FaulAnaylisServiceUri);
            var serviceList = await InvokeWithRetryAsync(() =>
                FabricClient.QueryManager.GetServiceListAsync(new Uri(applicationUri), new Uri(FaulAnaylisServiceUri), timeout, cancellationToken)
                );
            if(serviceList.Count == 0)
            {
                throw new FabricException(StringResources.FaultAnalysisServiceNotEnabled,
                       FabricErrorCode.FaultAnalysisServiceNotEnabled);
            }
        }

        #endregion

        internal static async Task<ServicePartitionInformation> GetPartitionDetails(string partitionId, TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            ServicePartitionList servicePartitionList =  await FabricClient.QueryManager.GetPartitionAsync(Guid.Parse(partitionId));
            if (servicePartitionList.Count == 1)
            {
                Partition partition = servicePartitionList[0];
                return partition.PartitionInformation;
            }
            throw new FabricException(StringResources.InvalidPartitionKey,FabricErrorCode.InvalidPartitionKey);
        }

        #region Data Loss Helper Functions
        internal static async Task<PartitionDataLossProgress> GetPartitionDataLossProgress(Guid dataLossGuid ,TimeSpan timeout )
        {

            PartitionDataLossProgress partitionDataLossProgress = null;
            try
            {
                partitionDataLossProgress = await InvokeWithRetryAsync<PartitionDataLossProgress>(() =>
                    {
                        return FabricClient.TestManager.GetPartitionDataLossProgressAsync(dataLossGuid);
                    }
                );
            }
            catch (Exception)
            {
                
            }
            return partitionDataLossProgress;
        }

        internal static async Task InitiatePartitionDataLoss(Guid dataLossGuid, string serviceNameUri, string partitionId, TimeSpan timeout)
        {
            await InvokeWithRetryAsync(() =>
            {
                return FabricClient.TestManager.StartPartitionDataLossAsync(dataLossGuid,
                    PartitionSelector.PartitionIdOf(new Uri(UtilityHelper.GetUriFromCustomUri(serviceNameUri)), Guid.Parse(partitionId) ), DataLossMode.PartialDataLoss);
            }
            );
        }

        internal static async Task CancelPartitionDataLoss(Guid dataLossGuid, TimeSpan timeout)
        {
            await InvokeWithRetryAsync(() => FabricClient.TestManager.CancelTestCommandAsync(dataLossGuid,false,timeout));
        }
#endregion

        internal static async Task<ServicePartitionList> GetPartitionList(string serviceUri)
        {
            return await InvokeWithRetryAsync<ServicePartitionList>(() =>
            FabricClient.QueryManager.GetPartitionListAsync(new Uri(UtilityHelper.GetUriFromCustomUri(serviceUri)))
           );
        }

        internal static async Task<ServiceList> GetServiceList(string applicationUri)
        {
            return await InvokeWithRetryAsync<ServiceList>(() =>
               FabricClient.QueryManager.GetServiceListAsync(new Uri(UtilityHelper.GetUriFromCustomUri(applicationUri)))
            );
        }

        internal static async Task<string> GetFabricServiceUriFromPartitionId(string partitionId , TimeSpan timeout , CancellationToken cancellationToken)
        {
            ServiceNameResult serviceNameResult = 
            await InvokeWithRetryAsync<ServiceNameResult>( ()=>
                    FabricClient.QueryManager.GetServiceNameAsync(Guid.Parse(partitionId),timeout, cancellationToken)
                );
            return serviceNameResult.ServiceName.ToString();
        }

        internal static async Task<string> GetFabricApplicationUriFromServiceUri(string serviceNameUri, TimeSpan timeout, CancellationToken cancellationToken)
        {
            string[] serviceNameUriSplits = serviceNameUri.Split('/');
            if (serviceNameUriSplits.Length == 3)
            {
                return string.Format("fabric:/{0}", serviceNameUriSplits[1]);
            }
            ApplicationNameResult applicationNameResult=
                await InvokeWithRetryAsync<ApplicationNameResult>(() =>
                    FabricClient.QueryManager.GetApplicationNameAsync(new Uri(serviceNameUri), timeout, cancellationToken)
                );
            return applicationNameResult.ApplicationName.ToString();
        }

        #region  Fabric Client Retry and Recycle Helper Functions

        internal static async Task<TResult> InvokeWithRetryAsync<TResult>(Func<Task<TResult>> fabricClientFunc)
        {
            int retryCount = 0;
            while (retryCount++ < 3)
            {
                try
                {
                    return await fabricClientFunc.Invoke();
                }
                catch (Exception exception)
                {
                    if ( exception is FabricObjectClosedException && retryCount < 3 )
                    {
                        RecycleFabricClient();
                    }
                    else if (exception is FabricTransientException && retryCount < 3)
                    {
                        
                    }
                    else
                    {
                        throw exception;
                    }
                }
            }
            throw new FabricException(StringResources.FabricEndpointNotFound, FabricErrorCode.FabricEndpointNotFound);
        }

        internal static async Task InvokeWithRetryAsync(Func<Task> fabricClientFunc)
        {
            int retryCount = 0;
            while (retryCount++ < 3)
            {
                try
                {
                    await fabricClientFunc.Invoke();
                    return;
                }
                catch (Exception exception)
                {
                    if (exception is FabricObjectClosedException && retryCount < 3)
                    {
                        RecycleFabricClient();
                    }
                    else if (exception is FabricTransientException && retryCount < 3)
                    {

                    }
                    else
                    {
                        throw;
                    }
                }
            }
            throw new FabricException(StringResources.FabricEndpointNotFound, FabricErrorCode.FabricEndpointNotFound);
        }

        internal static void RecycleFabricClient()
        {
            if (!isFabricClientRecycleInProgress)
            {
                lock (RecycleLock)
                {
                    if (!isFabricClientRecycleInProgress)
                    {
                        FabricClient = new FabricClient();
                    }
                }
                
            }
        }

#endregion

    }
}