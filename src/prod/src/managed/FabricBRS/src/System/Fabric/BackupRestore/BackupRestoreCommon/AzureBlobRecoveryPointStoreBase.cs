// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Common
{
    using Microsoft.WindowsAzure.Storage;
    using Microsoft.WindowsAzure.Storage.Blob;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Fabric.BackupRestore.BackupRestoreTypes;
    using System.Fabric.BackupRestore.Enums;
    using System.Fabric.Common;
    using System.Fabric.Security;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// Represents base class for Azure Blob Storage based recovery point store.
    /// </summary>
    internal class AzureBlobRecoveryPointStoreBase
    {
        private readonly Model.BackupStorage _storeInformation;
        protected CloudBlobContainer container;

        private const string RecoveryPointMetadataFileExtension = ".bkmetadata";
        private const int HttpNotFoundStatusCode = Constants.HttpNotFoundStatusCode;
        private const char ContinuationTokenSeparatorChar = Constants.ContinuationTokenSeparatorChar;
        private const string ZipFileExtension = ".zip";
        private const string BackupFolderNameFormat = Constants.BackupFolderNameFormat;
        private const int MaximumCount = 4998;

        public AzureBlobRecoveryPointStoreBase(Model.BackupStorage backupStorage)
        {
            this._storeInformation = backupStorage;
        }

        #region IRecoveryPointManager methods

        public List<string> EnumerateSubFolders(string relativePath)
        {
            if (String.IsNullOrEmpty(relativePath))
            {
                throw new ArgumentException();
            }

            // Convert relative path to AzureStore format.
            string blobStoreRelativePath = AzureBlobStoreHelper.ConvertToAzureBlobStorePath(relativePath);

            // TODO
            throw new NotImplementedException();
        }

        public List<string> EnumerateRecoveryPointMetadataFiles(string relativePath)
        {
            if (String.IsNullOrEmpty(relativePath))
            {
                throw new ArgumentException();
            }

            // Convert relative path to AzureStore format.
            string blobStoreRelativePath = AzureBlobStoreHelper.ConvertToAzureBlobStorePath(relativePath);

            try
            {
                CloudBlobDirectory blobDirectory = AzureBlobStoreHelper.GetCloudBlobDirectoryRef(this.container, blobStoreRelativePath);

                List<string> metadataBlobs = new List<string>();

                foreach (IListBlobItem blobItem in AzureBlobStoreHelper.GetBlobList(blobDirectory, true))
                {
                    if (blobItem is CloudBlockBlob)
                    {
                        CloudBlockBlob blob = (CloudBlockBlob)blobItem;

                        if (blob.Name.EndsWith(RecoveryPointMetadataFileExtension))
                        {
                            metadataBlobs.Add(blob.Name);
                        }
                    }
                }

                return metadataBlobs;
            }
            catch (Exception e)
            {
                throw new IOException(String.Format("Unable to enumerate metadata files from container:{0} and relative path: {1}.", this.container.Uri, blobStoreRelativePath), e);
            }
        }

        public List<string> EnumerateRecoveryPointMetadataFiles(string relativePath, DateTime startDateTime, DateTime endDateTime, bool isLatestRequested, BRSContinuationToken continuationToken, int maxResults)
        {
            if (String.IsNullOrEmpty(relativePath))
            {
                throw new ArgumentException("relativePath cannot be null");
            }

            if (continuationToken == null)
            {
                throw new ArgumentException("continuationToken cannot be null");
            }

            // Convert relative path to AzureStore format.
            string blobStoreRelativePath = AzureBlobStoreHelper.ConvertToAzureBlobStorePath(relativePath);
            BlobContinuationToken blobContinuationTokenFromQuery = null;
            if (!String.IsNullOrEmpty(continuationToken.IncomingContinuationToken))
            {
                string[] continuationTokenList = new string[] { };
                try
                {
                    // continuationToen here equals targetlocation "+" nextMarker.
                    continuationTokenList = continuationToken.IncomingContinuationToken.Split(new char[] { ContinuationTokenSeparatorChar }, 2);
                }
                catch (Exception ex)
                {
                    throw new ArgumentException("continuationToken provided is not correct. Exception thrown: {0}", ex);
                }

                if (continuationTokenList.Count() != 2)
                {
                    throw new ArgumentException("continuationToken provided is not correct. token: {0}", continuationToken.IncomingContinuationToken);
                }

                string targetLocation = continuationTokenList[0];
                string nextMarker = continuationTokenList[1];
                blobContinuationTokenFromQuery = new BlobContinuationToken();
                blobContinuationTokenFromQuery.NextMarker = nextMarker;
                if (targetLocation == "Null")
                {
                    blobContinuationTokenFromQuery.TargetLocation = null;
                }
                else
                {

                    StorageLocation storageLocation;
                    if (Enum.TryParse<StorageLocation>(targetLocation, out storageLocation))
                    {
                        blobContinuationTokenFromQuery.TargetLocation = storageLocation;
                    }
                    else
                    {
                        throw new ArgumentException("targetLocation in continuationToken is not correct. {0} ", targetLocation);
                    }

                }
            }

            Dictionary<string, string> latestInPartition = new Dictionary<string, string>();

            try
            {
                CloudBlobDirectory blobDirectory = AzureBlobStoreHelper.GetCloudBlobDirectoryRef(this.container, blobStoreRelativePath);

                List<string> metadataBlobs = new List<string>();

                int counter = 0;
                BlobContinuationToken finalBlobContinuationToken = null;
                BlobContinuationToken currentToken = blobContinuationTokenFromQuery;
                do
                {
                    if (maxResults != 0 && counter == maxResults)
                    {
                        break;
                    }

                    int maxResultsQuery = MaximumCount;
                    if (maxResults != 0 && counter + maxResultsQuery > maxResults)
                    {
                        maxResultsQuery = (maxResults - counter) * 2;
                    }
                    maxResultsQuery = (maxResultsQuery < MaximumCount) ? maxResultsQuery : MaximumCount;

                    BlobResultSegment blobResultSegment = AzureBlobStoreHelper.GetBlobList(blobDirectory, true, currentToken, maxResultsQuery);
                    currentToken = blobResultSegment.ContinuationToken;
                    foreach (IListBlobItem blobItem in blobResultSegment.Results)
                    {
                        if (blobItem is CloudBlockBlob)
                        {
                            CloudBlockBlob blob = (CloudBlockBlob)blobItem;
                            string blobName = blob.Name;
                            if (blobName.EndsWith(RecoveryPointMetadataFileExtension))
                            {
                                // Lets parse the files with respect to date here itself.
                                // Let's parse the date time from file name
                                ListBackupFile(startDateTime, endDateTime, blobName, metadataBlobs,
                                latestInPartition, isLatestRequested, ref counter);
                            }

                        }
                    }
                } while (currentToken != null);


                finalBlobContinuationToken = currentToken;
                if (finalBlobContinuationToken != null)
                {

                    string nextMarkerFinal = finalBlobContinuationToken.NextMarker;
                    string targetLocationFinal = "Null";
                    if (finalBlobContinuationToken.TargetLocation != null)
                    {
                        targetLocationFinal = finalBlobContinuationToken.TargetLocation.ToString();

                    }
                    continuationToken.OutgoingContinuationToken = targetLocationFinal + ContinuationTokenSeparatorChar + nextMarkerFinal;
                }

                if (isLatestRequested)
                {
                    List<string> newMetaDataFileList = new List<string>();
                    foreach (var tuple in latestInPartition)
                    {
                        newMetaDataFileList.Add(tuple.Value);
                    }
                    return newMetaDataFileList;
                }
                return metadataBlobs;
            }
            catch (Exception e)
            {
                throw new IOException(String.Format("Unable to enumerate metadata files from container:{0} and relative path: {1}.", this.container.Uri, blobStoreRelativePath), e);
            }
        }

        private void ListBackupFile(DateTime startDateTime, DateTime endDateTime, string blobName, List<string> metadataBlobs, Dictionary<string, string> latestInPartition, bool isLatestRequested, ref int counter)
        {
            if (startDateTime != DateTime.MinValue || endDateTime != DateTime.MaxValue)
            {
                if (this._storeInformation.FilterFileBasedOnDateTime(blobName, startDateTime, endDateTime))
                {
                    AddFileToAppropriateObject(metadataBlobs, latestInPartition, blobName, isLatestRequested, ref counter);
                }
            }
            else
            {
                AddFileToAppropriateObject(metadataBlobs, latestInPartition, blobName, isLatestRequested, ref counter);

            }
        }

        /// <Summary>This method delete backup files(.zip and .metadata) for the file path of backup(.zip)</Summary>
        public bool DeleteBackupFiles(string filePathOrSourceFolderPath)
        {
            /**
              Now filePathOrSourceFolderPath could be in zip format or folder format.
             */
            try
            {
                bool isFolder = true;
                string filePathWithoutExtension = filePathOrSourceFolderPath;
                if (filePathOrSourceFolderPath.EndsWith(ZipFileExtension))
                {
                    filePathWithoutExtension = filePathOrSourceFolderPath.Remove(filePathOrSourceFolderPath.Length - ZipFileExtension.Length, ZipFileExtension.Length);
                    isFolder = false;

                }
                string bkmetedataFileName = filePathWithoutExtension + RecoveryPointMetadataFileExtension;
                DeleteBlobFolderOrFile(bkmetedataFileName, isFolder);
                DeleteBlobFolderOrFile(filePathOrSourceFolderPath, isFolder);

                return true;
            }
            catch (Exception ex)
            {
                BackupRestoreTrace.TraceSource.WriteWarning("AzureBlobRecoveryPointManager", "The backups are not deleted because of this exception {0}", ex);
                return false;
            }
        }

        private void DeleteBlobFolderOrFile(string filePathOrSourceFolderPath, bool isFolder = false)
        {
            if (!CheckIfBackupExists(filePathOrSourceFolderPath))
            {
                BackupRestoreTrace.TraceSource.WriteWarning("AzureBlobRecoveryPointManager", "relativefilePathOrSourceFolderPath: {0} not found"
                    , filePathOrSourceFolderPath);
                return;
            }
            if (!isFolder)
            {
                AzureBlobStoreHelper.DeleteBlob(AzureBlobStoreHelper.GetCloudBlockBlobRef(this.container, filePathOrSourceFolderPath));
            }
            else
            {
                CloudBlobDirectory blobDirectory = AzureBlobStoreHelper.GetCloudBlobDirectoryRef(this.container, filePathOrSourceFolderPath);
                foreach (IListBlobItem blobItem in AzureBlobStoreHelper.GetBlobList(blobDirectory, true))
                {
                    if (blobItem is CloudBlockBlob)
                    {
                        CloudBlockBlob blob = (CloudBlockBlob)blobItem;

                        AzureBlobStoreHelper.DeleteBlob(blob);
                    }
                }
            }
        }

        public async Task<List<RestorePoint>> GetRecoveryPointDetailsAsync(IEnumerable<string> recoveryPointMetadataFiles, CancellationToken cancellationToken)
        {
            try
            {
                return await BackupRestoreUtility.PerformIOWithRetriesAsync<IEnumerable<string>, List<RestorePoint>>(
                    GetRecoveryPointDetailsInternalAsync,
                    recoveryPointMetadataFiles,
                    cancellationToken);
            }
            catch (Exception e)
            {
                throw new IOException("Unable to get recovery point details.", e);
            }
        }

        public async Task<RestorePoint> GetRecoveryPointDetailsAsync(string backupLocation, CancellationToken cancellationToken)
        {
            var recoveryPointMetadataFileName = GetRecoveryPointMetadataFileNameFromBackupLocation(backupLocation);
            var list = await GetRecoveryPointDetailsAsync(new List<string> { recoveryPointMetadataFileName }, cancellationToken);
            return list.FirstOrDefault();
        }

        public async Task<List<string>> GetBackupLocationsInBackupChainAsync(string backupLocation, CancellationToken cancellationToken)
        {
            try
            {
                return await BackupRestoreUtility.PerformIOWithRetriesAsync<string, List<string>>(
                    GetBackupLocationsInBackupChainInternalAsync,
                    backupLocation,
                    cancellationToken);
            }
            catch (Exception e)
            {
                throw new IOException("Unable to get backup locations in chain.", e);
            }
        }

        #endregion


        #region UtilityMethods
        private void AddFileToAppropriateObject(List<string> fileNameList, Dictionary<string, string> latestInPartition, string fileName, bool isLatestRequested, ref int counter)
        {
            if (isLatestRequested)
            {
                if (UpdateLatestInPartition(latestInPartition, fileName))
                {
                    counter++;
                }
            }
            else
            {
                fileNameList.Add(fileName);
                counter++;
            }
        }

        // It will only be called when isLatestRequired is true.
        private bool UpdateLatestInPartition(Dictionary<string, string> latestInPartition, string metaDataFileName)
        {
            var parsedPartitionId = this._storeInformation.ParsePartitionId(metaDataFileName);
            string fileNameStored;

            if (!latestInPartition.TryGetValue(parsedPartitionId, out fileNameStored))
            {
                latestInPartition.Add(parsedPartitionId, metaDataFileName);
                return true;
            }

            var fileDateTimeValue = Path.GetFileNameWithoutExtension(metaDataFileName);
            var storedFileDateTime = Path.GetFileNameWithoutExtension(fileNameStored);
            if (DateTime.ParseExact(fileDateTimeValue, BackupFolderNameFormat, CultureInfo.InvariantCulture) >= DateTime.ParseExact(storedFileDateTime, BackupFolderNameFormat, CultureInfo.InvariantCulture))
            {
                latestInPartition[parsedPartitionId] = metaDataFileName;
            }

            return false;
        }

        #endregion

        #region Operation Specific inner classes/ methods

        private async Task<List<RestorePoint>> GetRecoveryPointDetailsInternalAsync(IEnumerable<string> metadataFiles, CancellationToken cancellationToken)
        {
            var backupList = new List<RestorePoint>();
            foreach (var metadataFile in metadataFiles)
            {
                cancellationToken.ThrowIfCancellationRequested();

                using (MemoryStream ms = new MemoryStream())
                {
                    CloudBlockBlob blockBlob = AzureBlobStoreHelper.GetCloudBlockBlobRef(this.container, metadataFile);
                    await AzureBlobStoreHelper.DownloadToStreamAsync(blockBlob, ms, cancellationToken);

                    var recoveryPointMetadataFile =
                        await RecoveryPointMetadataFile.OpenAsync(ms, metadataFile, cancellationToken);

                    var recoveryPoint = new RestorePoint()
                    {
                        BackupChainId = recoveryPointMetadataFile.BackupChainId,
                        BackupId = recoveryPointMetadataFile.BackupId,
                        ParentRestorePointId = recoveryPointMetadataFile.ParentBackupId,
                        BackupLocation = recoveryPointMetadataFile.BackupLocation,
                        CreationTimeUtc = recoveryPointMetadataFile.BackupTime,
                        BackupType =
                            recoveryPointMetadataFile.ParentBackupId == Guid.Empty ? BackupOptionType.Full : BackupOptionType.Incremental,
                        EpochOfLastBackupRecord = new BackupEpoch
                        {
                            ConfigurationNumber = recoveryPointMetadataFile.EpochOfLastBackupRecord.ConfigurationNumber,
                            DataLossNumber = recoveryPointMetadataFile.EpochOfLastBackupRecord.DataLossNumber
                        },
                        LsnOfLastBackupRecord = recoveryPointMetadataFile.LsnOfLastBackupRecord,
                        PartitionInformation = this.GetBackupServicePartitionInformationFromServicePartitionInformation(recoveryPointMetadataFile.PartitionInformation),
                        ServiceManifestVersion = recoveryPointMetadataFile.ServiceManifestVersion,
                    };

                    PopulateApplicationServiceAndPartitionInfo(recoveryPoint, metadataFile);

                    backupList.Add(recoveryPoint);
                }
            }

            return backupList;
        }

        bool CheckIfBackupExists(string fileOrFolderPath)
        {
            string azBlobPath = AzureBlobStoreHelper.ConvertToAzureBlobStorePath(fileOrFolderPath);
            CloudBlob blob = this.container.GetBlobReference(azBlobPath);
            return blob.Exists();
        }

        string GetRecoveryPointMetadataFileNameFromBackupLocation(string fullBackupLocation)
        {
            if (fullBackupLocation.EndsWith(ZipFileExtension))
            {
                return fullBackupLocation.Substring(0, fullBackupLocation.Length - ZipFileExtension.Length) +
                       RecoveryPointMetadataFileExtension;
            }
            return fullBackupLocation + RecoveryPointMetadataFileExtension;
        }

        private async Task<List<string>> GetBackupLocationsInBackupChainInternalAsync(string backupLocation, CancellationToken cancellationToken)
        {
            var backupLocationList = new List<string>();
            var fullBackupLocation = backupLocation;

            // Check if backup exists
            if (!CheckIfBackupExists(fullBackupLocation))
            {
                throw new FabricElementNotFoundException(String.Format("Missing backup!! Couldn't find backup folder {0} which is there in backup chain", fullBackupLocation));
            }

            var recoveryPointMetadataFileName = GetRecoveryPointMetadataFileNameFromBackupLocation(fullBackupLocation);
            RecoveryPointMetadataFile recoveryPointMetadataFile = null;

            using (MemoryStream ms = new MemoryStream())
            {

                CloudBlockBlob blockBlob = AzureBlobStoreHelper.GetCloudBlockBlobRef(this.container, recoveryPointMetadataFileName);
                await AzureBlobStoreHelper.DownloadToStreamAsync(blockBlob, ms, cancellationToken);
                recoveryPointMetadataFile = await RecoveryPointMetadataFile.OpenAsync(ms, recoveryPointMetadataFileName, cancellationToken);
            }

            backupLocationList.Add(recoveryPointMetadataFile.BackupLocation);

            while (recoveryPointMetadataFile.ParentBackupId != Guid.Empty)
            {
                fullBackupLocation = recoveryPointMetadataFile.ParentBackupLocation;

                // Check if backup folder exists
                if (!CheckIfBackupExists(fullBackupLocation))
                {
                    throw new FabricElementNotFoundException(String.Format("Missing backup!! Couldn't find backup folder {0} which is there in backup chain", fullBackupLocation));
                }

                recoveryPointMetadataFileName = GetRecoveryPointMetadataFileNameFromBackupLocation(fullBackupLocation);
                using (MemoryStream ms = new MemoryStream())
                {

                    CloudBlockBlob blockBlob = AzureBlobStoreHelper.GetCloudBlockBlobRef(this.container, recoveryPointMetadataFileName);
                    await AzureBlobStoreHelper.DownloadToStreamAsync(blockBlob, ms, cancellationToken);

                    recoveryPointMetadataFile = await RecoveryPointMetadataFile.OpenAsync(ms, recoveryPointMetadataFileName, cancellationToken);

                    backupLocationList.Add(recoveryPointMetadataFile.BackupLocation);
                }
            }

            Debug.Assert(recoveryPointMetadataFile.BackupId == recoveryPointMetadataFile.BackupChainId, "Backup ID for root doesn't match with backup chain ID");
            return backupLocationList;
        }

        private void PopulateApplicationServiceAndPartitionInfo(RestorePoint recoveryPoint, string metadataFile)
        {
            var tokens = metadataFile.Split('/');
            var tokensLength = tokens.Length;

            if (tokensLength >= 4)
            {
                var partitionIdStr = tokens[tokensLength - 2];
                var serviceNameStr = tokens[tokensLength - 3];
                var applicationNameStr = tokens[tokensLength - 4];

                recoveryPoint.PartitionInformation.Id = Guid.Parse(partitionIdStr);
                recoveryPoint.ApplicationName = new Uri(String.Format("fabric:/{0}", UtilityHelper.GetUriFromCustomUri(applicationNameStr)));
                recoveryPoint.ServiceName =
                    new Uri(String.Format("{0}/{1}", recoveryPoint.ApplicationName, UtilityHelper.GetUriFromCustomUri(serviceNameStr)));
            }
        }

        private PartitionInformation GetBackupServicePartitionInformationFromServicePartitionInformation(ServicePartitionInformation servicePartitionInformation)
        {
            PartitionInformation backupServicePartitionInformation = null;
            switch (servicePartitionInformation.Kind)
            {
                case ServicePartitionKind.Singleton:
                    backupServicePartitionInformation = new SingletonPartitionInformation();
                    break;

                case ServicePartitionKind.Int64Range:
                    var int64RangePartitionInformation = servicePartitionInformation as Fabric.Int64RangePartitionInformation;
                    ThrowIf.Null(int64RangePartitionInformation, "int64RangePartitionInformation");
                    backupServicePartitionInformation = new Int64RangePartitionInformation()
                    {
                        HighKey = int64RangePartitionInformation.HighKey,
                        LowKey = int64RangePartitionInformation.LowKey
                    };
                    break;

                case ServicePartitionKind.Named:
                    var namedPartitionInformation = servicePartitionInformation as Fabric.NamedPartitionInformation;
                    ThrowIf.Null(namedPartitionInformation, "namedPartitionInformation");
                    backupServicePartitionInformation = new NamedPartitionInformation()
                    {
                        Name = namedPartitionInformation.Name
                    };
                    break;

                default:
                    throw new ArgumentOutOfRangeException();
            }

            return backupServicePartitionInformation;
        }

        #endregion
    }
}