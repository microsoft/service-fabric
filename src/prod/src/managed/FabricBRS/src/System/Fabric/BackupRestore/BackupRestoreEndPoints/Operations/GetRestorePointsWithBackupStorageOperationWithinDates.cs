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
    using System.Fabric.Interop;

    internal class GetRestorePointsWithBackupStorageOperationWithinDates : RestorePointOperation<RestorePointEnumerationResult>
    {
        private readonly DateTime startDateTime;
        private readonly DateTime endDateTime;
        private readonly BackupStorage backupStorage;

        public GetRestorePointsWithBackupStorageOperationWithinDates(BackupStorage backupStorage, string fabricRequestHeader,DateTime startDateTime , DateTime endDateTime
            ,string serviceName , string apiVersion ,StatefulService statefulService)
            : base(fabricRequestHeader,serviceName, apiVersion, statefulService)
        {
            this.backupStorage = backupStorage;
            this.startDateTime = startDateTime;
            this.endDateTime = endDateTime;
        }


        internal override async Task<RestorePointEnumerationResult> RunAsync(TimeSpan timeout , CancellationToken cancellationToken)
        {
            RestorePointEnumerationResult restorePoints = new RestorePointEnumerationResult();
            this.ThrowInvalidArgumentIfNull(this.backupStorage);
            BackupStorageModel backupStorageModel = BackupStorageModel.FromBackupStorageView(this.backupStorage);
            await this.PopulateApplicationServiceAndPartitionWithBackupStorage(timeout, cancellationToken);
            try
            {
                List<RestorePoint> fetchedRestorePoints = await
                        backupStorageModel.GetBackupEnumerationsTask(this.ApplicationName, this.ServiceName,
                            this.PartitionId, this.startDateTime,this.endDateTime , cancellationToken);
                if (fetchedRestorePoints != null && fetchedRestorePoints.Count > 0)
                {
                    restorePoints.RestorePoints = fetchedRestorePoints;
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
                        restorePointEnumerationFailureInfo.Add(new RestorePointEnumerationFailureInfo
                            ()
                        {
                            Error =
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
                    restorePointEnumerationFailureInfo.Add(new RestorePointEnumerationFailureInfo
                            ()
                    {
                        Error =
                                new FabricError()
                                {
                                    Code = (NativeTypes.FABRIC_ERROR_CODE)exception.HResult,
                                    Message = exception.Message
                                }
                    });

                }
            }
            return restorePoints;
        }

        public override string ToString()
        {
            StringBuilder stringBuilder = new StringBuilder();
            stringBuilder.AppendFormat(
                "GetBackupEnumerationsWithBackupStorage Operation with Backup Storage as {0} and start Date Time {1} and EndDateTime {2}",
                this.backupStorage, this.startDateTime,this.endDateTime);
            return stringBuilder .ToString();
        }
    }
}