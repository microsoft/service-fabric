// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Common.Model
{
    using System.Collections.Generic;
    using System.Fabric.BackupRestore.Enums;
    using System.Globalization;
    using System.IO;
    using System.Runtime.Serialization;
    using System.Threading;
    using System.Threading.Tasks;

    using BackupRestoreView = BackupRestoreTypes;

    [DataContract]
    [KnownType(typeof(FileShareBackupStorageInfo))]
    [KnownType(typeof(AzureBlobBackupStorageInfo))]
    [KnownType(typeof(DsmsAzureBlobBackupStorageInfo))]
    internal abstract class BackupStorage
    {
        [DataMember]
        internal BackupStorageType BackupStorageType { get; private set; }

        [DataMember]
        internal char PathSeparator { get; private set; }

        [DataMember]
        internal string FriendlyName { get; private set; }

        [DataMember]
        internal Guid StorageId { get; private set; }

        private const string BackupFolderNameFormat = Common.Constants.BackupFolderNameFormat;

        protected internal BackupStorage(BackupStorageType backupStorageType, char pathSeparator)
        {
            this.BackupStorageType = backupStorageType;
            this.PathSeparator = pathSeparator;
            this.StorageId = Guid.NewGuid();
        }

        internal abstract BackupStorage DeepCopy();

        internal bool CompareStorage(BackupStorage backupStorage)
        {
            if (this.BackupStorageType != backupStorage.BackupStorageType)
            {
                return false;
            }
            else if (this.BackupStorageType == BackupStorageType.FileShare)
            {
                var fileStoreInfo = (FileShareBackupStorageInfo)this;
                var fileStoreInfo1 = (FileShareBackupStorageInfo)backupStorage;
                return FileShareBackupStorageInfo.CompareStorage(fileStoreInfo, fileStoreInfo1);
            }
            else if (this.BackupStorageType == BackupStorageType.AzureBlobStore)
            {
                // it means AzureBlobShare
                var azureStoreInfo = (AzureBlobBackupStorageInfo)this;
                var azureStoreInfo1 = (AzureBlobBackupStorageInfo)backupStorage;
                return AzureBlobBackupStorageInfo.CompareStorage(azureStoreInfo, azureStoreInfo1);
            }
            else
            {
                // it means AzureBlobShare
                var dsmsazureStoreInfo = (DsmsAzureBlobBackupStorageInfo)this;
                var dsmsazureStoreInfo1 = (DsmsAzureBlobBackupStorageInfo)backupStorage;
                return DsmsAzureBlobBackupStorageInfo.CompareStorage(dsmsazureStoreInfo, dsmsazureStoreInfo1);
            }
        }


        protected internal static BackupStorage FromBackupStorageView(BackupRestoreView.BackupStorage backupStorageView)
        {
            BackupStorage backupStorage = null;
            if (backupStorageView == null)
            {
                throw new ArgumentException("backupStorageView is null, it must be valid instance of BackupStorage.");
            }
            else if (backupStorageView.StorageKind == BackupStorageType.FileShare)
            {
                backupStorage = FileShareBackupStorageInfo.FromfileShareBackupStorageInfoView((BackupRestoreView.FileShareBackupStorageInfo)backupStorageView);
            }
            else if (backupStorageView.StorageKind == BackupStorageType.AzureBlobStore)
            {
                backupStorage = AzureBlobBackupStorageInfo.FromAzureBlobBackupStorageInfoView((BackupRestoreView.AzureBlobBackupStorageInfo)backupStorageView);
            }
            else if (backupStorageView.StorageKind == BackupStorageType.DsmsAzureBlobStore)
            {
                backupStorage = DsmsAzureBlobBackupStorageInfo.FromDsmsAzureBlobBackupStorageInfoView((BackupRestoreView.DsmsAzureBlobBackupStorageInfo)backupStorageView);
            }
            if (backupStorage != null)
            {
                backupStorage.FriendlyName = string.IsNullOrEmpty(backupStorageView.FriendlyName)
                    ? string.Empty
                    : backupStorageView.FriendlyName;
            }
            return backupStorage;
        }

        protected internal BackupRestoreView.BackupStorage ToBackupStorageView()
        {
            BackupRestoreView.BackupStorage backupStorageView = null;
            if (this.BackupStorageType == BackupStorageType.FileShare)
            {
                backupStorageView = ((FileShareBackupStorageInfo)this).TofileShareBackupStorageInfoView();
            }
            else if (this.BackupStorageType == BackupStorageType.AzureBlobStore)
            {
                backupStorageView = ((AzureBlobBackupStorageInfo)this).ToAzureBlobBackupStorageInfoView();
            }
            else if (this.BackupStorageType == BackupStorageType.DsmsAzureBlobStore)
            {
                backupStorageView = ((DsmsAzureBlobBackupStorageInfo)this).ToDsmsAzureBlobBackupStorageInfoView();
            }
            if (backupStorageView != null)
            {
                backupStorageView.FriendlyName = this.FriendlyName ?? String.Empty;
            }

            return backupStorageView;
        }

        internal async Task<List<BackupRestoreView.RestorePoint>> StartGetBackupEnumerationsTask(string applicationName,
            string serviceName, string partitionId, bool isLatestRequested, CancellationToken cancellationToken, BRSContinuationToken continuationToken, DateTime? startDateTime = null,
            DateTime? endDateTime = null, int maxResults = 0)
        {
            try
            {
                if (!startDateTime.HasValue)
                {
                    startDateTime = DateTime.MinValue;
                }

                if (!endDateTime.HasValue)
                {
                    endDateTime = DateTime.MaxValue;
                }

                return await this.GetBackupEnumerationsTask(applicationName, serviceName, partitionId, startDateTime.Value, endDateTime.Value,
                    isLatestRequested, cancellationToken, continuationToken, maxResults);
            }
            catch (Exception)
            {
                throw;
            }
        }

        #region Protected methods

        protected internal async Task<List<BackupRestoreView.RestorePoint>> GetBackupEnumerationsTask(string applicationName, string serviceName, string partitionId,
            DateTime startDateTime, DateTime endDateTime, bool isLatestRequested, CancellationToken cancellationToken,
            BRSContinuationToken continuationToken = null, int maxResults = 0)
        {
            var storeManager = RecoveryPointManagerFactory.GetRecoveryPointManager(this);
            if (continuationToken == null)
            {
                continuationToken = new BRSContinuationToken();
            }

            var files = storeManager.EnumerateRecoveryPointMetadataFiles(GetRelativeStorePath(applicationName, serviceName, partitionId), startDateTime,
                endDateTime, isLatestRequested, continuationToken, maxResults);

            if ((files != null) && (files.Count > 0))
            {
                return await storeManager.GetRecoveryPointDetailsAsync(files, cancellationToken);
            }

            return null;
        }

        #endregion

        #region Private Methods

        internal string ParsePartitionId(string file)
        {
            var tokens = file.Split(this.PathSeparator);
            var tokensLength = tokens.Length;
            var partitionIdStr = tokens[tokensLength - 2];

            return partitionIdStr;
        }

        internal string GetRelativeStorePath(string applicationName, string serviceName, string partitionId)
        {
            if (String.IsNullOrEmpty(applicationName))
            {
                throw new ArgumentNullException(applicationName);
            }

            var relativePath = applicationName;

            if (String.IsNullOrEmpty(serviceName)) return relativePath;

            relativePath = String.Format("{0}{1}{2}", relativePath, this.PathSeparator, serviceName);

            if (String.IsNullOrEmpty(partitionId)) return relativePath;

            relativePath = String.Format("{0}{1}{2}", relativePath, this.PathSeparator, partitionId);
            return relativePath;
        }

        internal bool FilterFileBasedOnDateTime(string s, DateTime startDateTime, DateTime endDateTime)
        {
            // Let's parse the date time from file name
            var fileNameDateTime = Path.GetFileNameWithoutExtension(s);
            var backupTimeTicks = DateTime.ParseExact(fileNameDateTime, BackupFolderNameFormat, CultureInfo.InvariantCulture).Ticks;
            var backupTimeUTC = new DateTime(backupTimeTicks, DateTimeKind.Utc);

            // before comparing we always have to convert the DateTime to UTC.
            var startDateTimeUTC = startDateTime.ToUniversalTime();
            var endDateTimeUTC = endDateTime.ToUniversalTime();

            return backupTimeUTC >= startDateTimeUTC && backupTimeUTC <= endDateTimeUTC;
        }

        #endregion
    }
}