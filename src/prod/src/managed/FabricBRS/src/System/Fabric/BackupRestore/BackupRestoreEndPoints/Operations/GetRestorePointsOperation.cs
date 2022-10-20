// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreEndPoints.Operations
{
    using System.Linq;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Fabric.Interop;
    using System.Web.Http;
    using System.Fabric.Common;
    using System.Collections.Generic;
    using Microsoft.ServiceFabric.Services.Runtime;
    using System.Fabric.BackupRestore.BackupRestoreTypes;
    using System.Fabric.BackupRestore.BackupRestoreExceptions;
    using BackupMappingModel = System.Fabric.BackupRestore.Common.Model.BackupMapping;
    using BackupStorageModel = System.Fabric.BackupRestore.Common.Model.BackupStorage;
    using BRSContinuationToken = System.Fabric.BackupRestore.Common.BRSContinuationToken;

    internal class GetRestorePointsOperation : RestorePointOperation<PagedBackupInfoList>
    {
        private const string TraceType = "GetRestorePointsOperation";
        private readonly DateTime? startDateTime;
        private readonly DateTime? endDateTime;
        private readonly bool isLatestRequested;
        private readonly string continuationToken;
        private readonly int maxResults;
        private const char ContinuationTokenSeparatorChar = System.Fabric.BackupRestore.Common.Constants.ContinuationTokenSeparatorChar;

        public GetRestorePointsOperation(string fabricRequestHeader, bool isLatestRequested, string apiVersion, StatefulService statefulService,
            DateTime? startDateTime = null, DateTime? endDateTime = null, string continuationToken = null, int maxResults = 0)
            : base(fabricRequestHeader, apiVersion, statefulService)
        {
            this.endDateTime = endDateTime;
            this.startDateTime = startDateTime;
            this.isLatestRequested = isLatestRequested;
            this.continuationToken = continuationToken;
            this.maxResults = maxResults;
        }

        internal override async Task<PagedBackupInfoList> RunAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            // Reading the value of continuationToken and maxResults.
            string continuationTokenLocal = this.continuationToken;
            int maxResultsLocal = this.maxResults;

            // Finding all the continuationTokens.
            string storageIdToContinue = null;
            BRSContinuationToken continuationTokenForNextStage = new BRSContinuationToken();
            string incomingContinuationTokenForNextStage = null;
            if (!String.IsNullOrEmpty(continuationTokenLocal))
            {
                // Splitting only in 2 as first one will be storage id and other one will be continuationToken for next stage.
                List<string> continuationTokenList = continuationTokenLocal.Split(new char[] { ContinuationTokenSeparatorChar }, 2).ToList();
                storageIdToContinue = continuationTokenList[0];
                if (continuationTokenList.Count > 1)
                {
                    incomingContinuationTokenForNextStage = continuationTokenList[1];
                }
            }

            List<RestorePoint> restorePoints = new List<RestorePoint>();
            List<BackupMappingModel> listOfBackupMappings = await this.GetBackupMappings(timeout, cancellationToken);

            IEnumerable<string> backupPolicyMappingList =
                listOfBackupMappings.Select<BackupMappingModel, string>((backupMapping) =>
                {
                    return backupMapping.BackupPolicyName;
                });
            HashSet<string> backupPolicyHashSetName = new HashSet<string>();
            HashSet<BackupStorageModel> backupStorages = new HashSet<BackupStorageModel>();

            foreach (var backPolicyName in backupPolicyMappingList)
            {
                if (!backupPolicyHashSetName.Contains(backPolicyName))
                {
                    backupPolicyHashSetName.Add(backPolicyName);
                    BackupStorageModel backupStorage = (await this.BackupPolicyStore.GetValueAsync(backPolicyName, timeout, cancellationToken)).Storage;

                    if (!backupStorages.Contains(backupStorage))
                    {
                        backupStorages.Add(backupStorage);
                    }
                }
            }

            bool isSuccess = true;
            string continuationTokenFinal = null;
            List<BackupStorageModel> backupStorageList = backupStorages.ToList();

            if (maxResultsLocal != 0)
            {
                // Ordering only when maxResults is not equal to 0.
                backupStorageList = backupStorageList.OrderBy(backupStorage => backupStorage.StorageId.ToString()).ToList();
                int maxResultsLeft = maxResultsLocal;
                bool breakTheLoop = false;


                // TO DO:
                // It is used for the case when maxResults is equal to number of elements in that storage.
                // In that condition, we will check for the next storage, if there is any more storages left to scan, we will send the storage id of the last storage scanned.
                // Here, we dont scan the next storage. There is a possibility that the next storage is empty.
                bool sendTheSameStorageAsContinuationToken = false;
                bool otherStorageFound = false;
                foreach (var backupStorage in backupStorageList)
                {
                    if (backupStorage.StorageId.ToString().CompareTo(storageIdToContinue) >= 0)
                    {
                        if (sendTheSameStorageAsContinuationToken)
                        {
                            // It means that we have found the next storage and we should break.
                            otherStorageFound = true;
                            break;
                        }
                        continuationTokenFinal = backupStorage.StorageId.ToString();
                        if (storageIdToContinue == backupStorage.StorageId.ToString())
                        {
                            if (String.IsNullOrEmpty(incomingContinuationTokenForNextStage))
                            {
                                // if we are here, then, we are in the condition of sendTheSameStorageAsContinuationToken = true in previouse query.
                                continue;
                            }
                            // continuationTokenForNextStage for this storage should be the incomingContinuationTokenForNextStage
                            continuationTokenForNextStage.IncomingContinuationToken = incomingContinuationTokenForNextStage;
                        }
                        else
                        {
                            // continuationTokenForNextStage for other storages should have IncomingContinuationToken to be null.
                            continuationTokenForNextStage.IncomingContinuationToken = null;
                        }
                        try
                        {
                            List<RestorePoint> newRestorePoints = new List<RestorePoint>();
                            newRestorePoints = await backupStorage.StartGetBackupEnumerationsTask(
                                this.ApplicationName,
                                this.ServiceName,
                                this.PartitionId,
                                this.isLatestRequested,
                                cancellationToken, continuationTokenForNextStage,
                                this.startDateTime,
                                this.endDateTime, maxResultsLeft);

                            restorePoints.AddRange(newRestorePoints);
                            maxResultsLeft = maxResultsLeft - newRestorePoints.Count;
                            if (maxResultsLeft == 0)
                            {
                                if (continuationTokenForNextStage.OutgoingContinuationToken == null)
                                {
                                    sendTheSameStorageAsContinuationToken = true;
                                }
                                else
                                {
                                    breakTheLoop = true;
                                }
                            }

                        }
                        catch (Exception ex)
                        {
                            isSuccess = false;
                            restorePoints[0] = new RestorePoint()
                            {
                                FailureError = new FabricError()
                                {
                                    Code = (NativeTypes.FABRIC_ERROR_CODE)ex.HResult,
                                    Message = ex.Message
                                }
                            };
                        }
                    }
                    if (breakTheLoop || isSuccess == false)
                    {
                        break;
                    }
                }

                if (!isSuccess && restorePoints.Count > 0)
                {
                    // Here, always the restorePoints's first element will be having error details. Look DoThisOnTaskFaulted method.
                    FabricErrorError fabricErrorError = new FabricErrorError(restorePoints[0].FailureError);
                    throw new HttpResponseException(fabricErrorError.ToHttpResponseMessage());
                }

                continuationTokenLocal = null;
                if (continuationTokenForNextStage.OutgoingContinuationToken == null)
                {
                    if (sendTheSameStorageAsContinuationToken && otherStorageFound)
                    {
                        // if there is some next store available to scan, then, we have to send the current storageID as continuationToken
                        continuationTokenLocal = continuationTokenFinal;
                    }
                }
                else
                {
                    continuationTokenLocal = continuationTokenFinal + ContinuationTokenSeparatorChar + continuationTokenForNextStage.OutgoingContinuationToken;
                }

                return new PagedBackupInfoList()
                {
                    Items = restorePoints,
                    ContinuationToken = continuationTokenLocal
                };
            }
            else
            {
                // we don't have to sort the storage list here as we need to return the complete list.
                List<Task> backupStorageTaskList = new List<Task>();
                foreach (var backupStorage in backupStorageList)
                {
                    Task backupStorageTask;
                    if (backupStorage.StorageId.ToString().CompareTo(storageIdToContinue) >= 0)
                    {
                        continuationTokenFinal = backupStorage.StorageId.ToString();
                        if (storageIdToContinue == backupStorage.StorageId.ToString())
                        {
                            if (String.IsNullOrEmpty(incomingContinuationTokenForNextStage))
                            {
                                continue;
                            }

                            // It is a possibility that continuationToken is not null for this stage then, we need to send the appropriate continuationToken.
                            continuationTokenForNextStage.IncomingContinuationToken = incomingContinuationTokenForNextStage;
                            backupStorageTask = backupStorage.StartGetBackupEnumerationsTask(
                                this.ApplicationName,
                                this.ServiceName,
                                this.PartitionId,
                                this.isLatestRequested,
                                cancellationToken, continuationTokenForNextStage,
                                this.startDateTime,
                                this.endDateTime).ContinueWith((task) =>
                                {
                                    if ((!task.IsCanceled) && (!task.IsFaulted) && (task.IsCompleted))
                                    {
                                        DoThisOnTaskSuccess(task, restorePoints, ref isSuccess);
                                    }
                                    else if (task.IsFaulted)
                                    {
                                        DoThisOnTaskFaulted(task, restorePoints, ref isSuccess);

                                    }
                                });
                        }
                        else
                        {
                            // Usual case for other storages.
                            backupStorageTask = backupStorage.StartGetBackupEnumerationsTask(
                                this.ApplicationName,
                                this.ServiceName,
                                this.PartitionId,
                                this.isLatestRequested,
                                cancellationToken, null,
                                this.startDateTime,
                                this.endDateTime).ContinueWith((task) =>
                                {
                                    if ((!task.IsCanceled) && (!task.IsFaulted) && (task.IsCompleted))
                                    {
                                        DoThisOnTaskSuccess(task, restorePoints, ref isSuccess);
                                    }
                                    else if (task.IsFaulted)
                                    {
                                        DoThisOnTaskFaulted(task, restorePoints, ref isSuccess);
                                    }
                                });
                        }
                        backupStorageTaskList.Add(backupStorageTask);
                    }
                }
                Task.WaitAll(backupStorageTaskList.ToArray(), cancellationToken);
                if (!isSuccess && restorePoints.Count > 0)
                {
                    FabricErrorError fabricErrorError = new FabricErrorError(restorePoints[0].FailureError);
                    throw new HttpResponseException(fabricErrorError.ToHttpResponseMessage());
                }

                return new PagedBackupInfoList()
                {
                    Items = restorePoints,
                };
            }
        }
        void DoThisOnTaskSuccess(Task<List<RestorePoint>> task, List<RestorePoint> restorePoints, ref bool isSuccess)
        {
            lock (restorePoints)
            {
                if (task.Result != null && task.Result.Count > 0)
                {
                    restorePoints.AddRange(task.Result);
                }
            }
        }

        void DoThisOnTaskFaulted(Task<List<RestorePoint>> faultedTask, List<RestorePoint> restorePoints, ref bool isSuccess)
        {
            // Only updating restorePoints[0] everytime to have the error.
            isSuccess = false;
            AggregateException aggregateException = faultedTask.Exception as AggregateException;
            if (aggregateException != null)
            {
                foreach (var exception in aggregateException.InnerExceptions)
                {
                    lock (restorePoints)
                    {
                        restorePoints[0] =
                        new RestorePoint
                            ()
                        {
                            FailureError =
                                new FabricError()
                                {
                                    Code = (NativeTypes.FABRIC_ERROR_CODE)faultedTask.Exception.HResult,
                                    Message = exception.Message
                                }
                        };
                    }

                }
            }
            else if (faultedTask.Exception != null)
            {
                lock (restorePoints)
                {
                    restorePoints[0] =
                    new RestorePoint
                        ()
                    {
                        FailureError =
                            new FabricError()
                            {
                                Code =
                                    (NativeTypes.FABRIC_ERROR_CODE)faultedTask.Exception.HResult,
                                Message = faultedTask.Exception.Message
                            }
                    };
                }
            }
        }


        public override string ToString()
        {
            StringBuilder stringBuilder = new StringBuilder();
            stringBuilder.AppendFormat("Get Backup Enumerations, Fabric Request Header {0} . is Latest Requested {1} ", this.FabricRequestHeader, this.isLatestRequested);
            return base.ToString();
        }
    }
}