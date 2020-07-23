// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreEndPoints.Operations
{
    using System.Fabric.BackupRestore.BackupRestoreTypes;
    using Microsoft.ServiceFabric.Services.Runtime;
    using BackupStorageModel = System.Fabric.BackupRestore.Common.Model.BackupStorage;
    using System.Collections.Generic;
    using System.Fabric.BackupRestore.Common;
    using System.Linq;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Fabric.BackupRestore.BackupRestoreExceptions;
    using BRSContinuationToken = System.Fabric.BackupRestore.Common.BRSContinuationToken;
    using System.Fabric.Interop;

    internal class GetRestorePointsWithBackupStorageOperation : RestorePointOperation<PagedBackupInfoList>
    {
        private readonly BackupByStorageQueryDescription backupByStorageQueryDescription;
        private string continuationToken;
        private int maxResults;

        public GetRestorePointsWithBackupStorageOperation(BackupByStorageQueryDescription backupByStorageQueryDescription, 
            string apiVersion ,StatefulService statefulService, string continuationToken = null, int maxResults = 0)
            : base(string.Empty, apiVersion, statefulService)
        {
            this.backupByStorageQueryDescription = backupByStorageQueryDescription;
            this.continuationToken = continuationToken;
            this.maxResults = maxResults;
        }


        internal override async Task<PagedBackupInfoList> RunAsync(TimeSpan timeout , CancellationToken cancellationToken)
        {
            BRSContinuationToken continuationTokenForNextStage = new BRSContinuationToken();
            if(continuationToken != null)
            {
                continuationTokenForNextStage.IncomingContinuationToken = continuationToken;
            }
            List<RestorePoint> restorePoints = new List<RestorePoint>();
            this.ThrowInvalidArgumentIfNull(this.backupByStorageQueryDescription.Storage);
            BackupStorageModel backupStorageModel = BackupStorageModel.FromBackupStorageView(this.backupByStorageQueryDescription.Storage);
            this.PopulateApplicationServiceAndPartitionWithBackupEntity(this.backupByStorageQueryDescription.BackupEntity,
                timeout, cancellationToken);
            try
            {
                List<RestorePoint> fetchedRestorePoints = await
                        backupStorageModel.GetBackupEnumerationsTask(
                            this.ApplicationName,
                            this.ServiceName,
                            this.PartitionId,
                            this.backupByStorageQueryDescription.StartDateTimeFilter,
                            this.backupByStorageQueryDescription.EndDateTimeFilter,
                            this.backupByStorageQueryDescription.Latest,
                            cancellationToken, continuationTokenForNextStage, maxResults);

                if (fetchedRestorePoints != null && fetchedRestorePoints.Count > 0)
                {
                    restorePoints = fetchedRestorePoints;
                }
            }
            catch (Exception exception)
            {
                List<RestorePointEnumerationFailureInfo> restorePointEnumerationFailureInfo =
                                        new List<RestorePointEnumerationFailureInfo>();
                AggregateException aggregateException = exception as AggregateException;
                if (aggregateException != null)
                {
                    foreach (var innerException in aggregateException.InnerExceptions)
                    {
                        restorePoints.Add(new RestorePoint
                            ()
                        {
                            FailureError= 
                                new FabricError()
                                {
                                    Code = (NativeTypes.FABRIC_ERROR_CODE)innerException.HResult,
                                    Message = innerException.Message
                                }
                        });
                    }
                }
                else
                {
                    restorePoints.Add(new RestorePoint
                            ()
                    {
                        FailureError = 
                                new FabricError()
                                {
                                    Code = (NativeTypes.FABRIC_ERROR_CODE)exception.HResult,
                                    Message = exception.Message
                                }
                    });

                }
            }

            return new PagedBackupInfoList()
            {
                Items = restorePoints,
                ContinuationToken = continuationTokenForNextStage.OutgoingContinuationToken
            };
        }

        public override string ToString()
        {
            StringBuilder stringBuilder = new StringBuilder();
            stringBuilder.AppendFormat("BackupByStorageQueryDescription : {0}",this.backupByStorageQueryDescription);
            return stringBuilder .ToString();
        }
    }
}