// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Collections.Generic;
using System.Diagnostics;
using System.Fabric.BackupRestore.BackupRestoreTypes;
using System.Fabric.BackupRestore.Enums;
using System.Fabric.Common;
using System.Fabric.Security;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Security.Principal;
using System.Threading;
using System.Threading.Tasks;

namespace System.Fabric.BackupRestore.Common
{
    /// <summary>
    /// File share recovery point manager
    /// </summary>
    internal class FileShareRecoveryPointManager : IRecoveryPointManager
    {
        private readonly Model.FileShareBackupStorageInfo storeInformation;

        private const string RecoveryPointMetadataFileExtension = ".bkmetadata";
        private const string ZipFileExtension = ".zip";
        private const char ContinuationTokenSeparatorChar = Constants.ContinuationTokenSeparatorChar;
        private const string BackupFolderNameFormat = Constants.BackupFolderNameFormat;

        public FileShareRecoveryPointManager(Model.FileShareBackupStorageInfo fileShareStoreInformation)
        {
            this.storeInformation = fileShareStoreInformation;
        }

        #region IRecoveryPointManager methods

        public List<string> EnumerateSubFolders(string relativePath)
        {
            if (String.IsNullOrEmpty(relativePath))
            {
                throw new ArgumentException();
            }

            var remotePath = Path.Combine(this.storeInformation.Path, relativePath);

            try
            {
                var enumerateInfo = new FolderEnumerateInfo
                {
                    DirectoryPath = remotePath,
                    GetFullPath = false
                };

                BackupRestoreUtility.PerformIOWithRetries(
                    enumInfo =>
                    {
                        PerformDestinationOperation(EnumerateFolders, enumInfo);
                    },
                    enumerateInfo);

                return enumerateInfo.SubFolders.ToList();
            }
            catch (Exception e)
            {
                throw new IOException(String.Format("Unable to enumerate sub folders from location {0}.", remotePath), e);
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

            char pathSeparator = this.storeInformation.PathSeparator;
            string relativePathContinuationToken = null;
            string fileNameContinuationToken = null;
            if (!String.IsNullOrEmpty(continuationToken.IncomingContinuationToken))
            {
                List<string> continuationTokenList = continuationToken.IncomingContinuationToken.Split(ContinuationTokenSeparatorChar).ToList();
                if(continuationTokenList.Count != 4)
                {
                    throw new ArgumentException("Invalid ContinuationToken is given. token: {0}", continuationToken.IncomingContinuationToken);
                }
                relativePathContinuationToken = string.Format("{0}{1}{2}{1}{3}", continuationTokenList[0], pathSeparator, continuationTokenList[1], continuationTokenList[2]);
                fileNameContinuationToken = continuationTokenList[3];
            }

            var remotePath = Path.Combine(this.storeInformation.Path, relativePath);

            string remotePathContinuationToken = null;
            if (!String.IsNullOrEmpty(relativePathContinuationToken))
            {
                remotePathContinuationToken = Path.Combine(this.storeInformation.Path, relativePathContinuationToken);
            }

            try
            {
                Dictionary<string, string> latestInPartition = new Dictionary<string, string>();
                List<string> foldersToIterate = ListFoldersToIterate(remotePath, remotePathContinuationToken);

                foldersToIterate.Sort();

                List<string> finalFileList = new List<string>();

                string finalContinuationTokenPath = null;
                int counter = 0;

                bool toBreak = false;
                bool anyMoreLeft = true;
                foreach (var directory in foldersToIterate)
                {
                    if(toBreak)
                    {
                        break;
                    }

                    string fileNameContinuationTokenFinal = null;
                    if (directory == remotePathContinuationToken)
                    {
                        fileNameContinuationTokenFinal = fileNameContinuationToken;
                    }

                    List<string> fileNames = ListMetadataFilesInTheFolder(directory).Where(x => ConvertFileNameToDateTime(Path.GetFileNameWithoutExtension(x)) >
                    ConvertFileNameToDateTime(Path.GetFileNameWithoutExtension(fileNameContinuationTokenFinal))).ToList();

                    int fileCount = fileNames.Count;
                    if (maxResults != 0 && fileCount + counter >= maxResults)
                    {
                        if(maxResults == fileCount + counter && directory == foldersToIterate.Last())
                        {
                            anyMoreLeft = false;
                        }

                        var sortedFileNames = fileNames.OrderBy(fileName => ConvertFileNameToDateTime(Path.GetFileNameWithoutExtension(fileName)));
                        foreach (var fileName in sortedFileNames)
                        {
                            if (startDateTime != DateTime.MinValue || endDateTime != DateTime.MaxValue)
                            {
                                if(this.storeInformation.FilterFileBasedOnDateTime(fileName, startDateTime, endDateTime))
                                {
                                    AddFileToAppropriateObject(finalFileList, latestInPartition, fileName, isLatestRequested, ref counter);
                                }
                            }
                            else
                            {
                                AddFileToAppropriateObject(finalFileList, latestInPartition, fileName, isLatestRequested, ref counter);
                            }

                            if (counter == maxResults)
                            {
                                finalContinuationTokenPath = fileName;
                                toBreak = true;
                                break;
                            }
                        }
                    }
                    else
                    {
                        foreach(var fileName in fileNames)
                        {
                            if (startDateTime != DateTime.MinValue || endDateTime != DateTime.MinValue)
                            {
                                if (this.storeInformation.FilterFileBasedOnDateTime(fileName, startDateTime, endDateTime))
                                {
                                    AddFileToAppropriateObject(finalFileList, latestInPartition, fileName, isLatestRequested, ref counter);
                                }
                            }
                            else
                            {
                                AddFileToAppropriateObject(finalFileList, latestInPartition, fileName, isLatestRequested, ref counter);
                            }
                        }
                    }
                }


                if(!string.IsNullOrEmpty(finalContinuationTokenPath))
                {
                    if(anyMoreLeft)
                    {
                        // First remove the store path from the start.
                        if (finalContinuationTokenPath.StartsWith(this.storeInformation.Path))
                        {
                            finalContinuationTokenPath = finalContinuationTokenPath.Remove(0, this.storeInformation.Path.Length);
                        }
                        continuationToken.OutgoingContinuationToken = finalContinuationTokenPath.Trim(pathSeparator).Replace(pathSeparator, ContinuationTokenSeparatorChar);
                    }
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

                return finalFileList;
            }

            catch (Exception e)
            {
                throw new IOException(String.Format("Unable to enumerate metadata files from location {0}.", remotePath), e);
            }
        }

        private List<string> ListFoldersToIterate(string remotePath, string remotePathContinuationToken)
        {
            List<string> foldersToIterate = new List<string>();
            if (ListMetadataFilesInTheFolder(remotePath).ToList().Count > 0)
            {
                foldersToIterate.Add(remotePath);
            }
            else
            {
                List<string> folders = ListFolders(remotePath).ToList();
                foreach (var folder in folders)
                {
                    List<string> folderOneFileList = ListMetadataFilesInTheFolder(folder).ToList();
                    if (folderOneFileList.Count > 0)
                    {
                        // If only one file will have metadata files in it. It means that we were looking for service backuplist.
                        // So, add all the folders to foldersToIterate and return.
                        foldersToIterate.AddRange(folders.Where(f => f.CompareTo(remotePathContinuationToken) >= 0).ToList());
                        break;
                    }
                    else
                    {
                        // Here, there is no need to check if metadata files are present or not as it would only be the case when,
                        // we were passed the application URI.
                        foldersToIterate.AddRange(ListFolders(folder, remotePathContinuationToken).ToList());
                    }
                }
            }
            return foldersToIterate;
        }

        public bool DeleteBackupFiles(string relativefilePathOrSourceFolderPath)
        {
            /**
              Now filePathOrSourceFolderPath could be in zip format or folder format.
             */
            try
            {
                bool isFolder = true;
                string filePathWithoutExtension = relativefilePathOrSourceFolderPath;
                if (relativefilePathOrSourceFolderPath.EndsWith(ZipFileExtension))
                {
                    filePathWithoutExtension = relativefilePathOrSourceFolderPath.Remove(relativefilePathOrSourceFolderPath.Length - ZipFileExtension.Length, ZipFileExtension.Length);
                    isFolder = false;

                }
                string bkmetedataFileName = filePathWithoutExtension + RecoveryPointMetadataFileExtension;
                DeleteFolderOrFile(bkmetedataFileName, false);
                DeleteFolderOrFile(relativefilePathOrSourceFolderPath, isFolder);

                return true;
            }
            catch (Exception ex)
            {
                BackupRestoreTrace.TraceSource.WriteWarning("FileShareRecoveryPointManager", "The backups are not deleted because of this exception {0}", ex);
                return false;
            }
        }

        private void DeleteFolderOrFile(string relativefilePathOrSourceFolderPath, bool isFolder)
        {
            var remotePath = Path.Combine(this.storeInformation.Path, relativefilePathOrSourceFolderPath);
            if (!CheckIfBackupExists(remotePath))
            {
                BackupRestoreTrace.TraceSource.WriteWarning("FileShareRecoveryPointManager", "relativefilePathOrSourceFolderPath: {0} with storePath : {1} not found"
                    , relativefilePathOrSourceFolderPath, this.storeInformation.Path);
                return;
            }
            if (isFolder)
            {
                var enumerateInfo = new FileEnumerateInfo
                {
                    DirectoryPath = remotePath,
                    SearchOptionEnum =  SearchOption.AllDirectories
                };

                BackupRestoreUtility.PerformIOWithRetries(
                    enumInfo =>
                    {
                        PerformDestinationOperation(EnumerateFiles, enumInfo);
                    },
                    enumerateInfo);

                List<string> filePathList = enumerateInfo.Files.ToList();
                foreach (var filePath in filePathList)
                {
                    BackupRestoreUtility.PerformIOWithRetries(
                    file =>
                    {
                        PerformDestinationOperation(DeleteFile, file);
                    },
                    filePath);
                }
            }
            else
            {
                BackupRestoreUtility.PerformIOWithRetries(
                    file =>
                    {
                        PerformDestinationOperation(DeleteFile, file);
                    },
                    remotePath);
            }
        }

        public async Task<List<RestorePoint>> GetRecoveryPointDetailsAsync(IEnumerable<string> recoveryPointMetadataFiles, CancellationToken cancellationToken)
        {
            try
            {
                return await BackupRestoreUtility.PerformIOWithRetriesAsync(
                    async (info, ct) => await PerformDestinationOperationAsync(GetRecoveryPointDetailsInternalAsync, info, ct),
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
            var list = await GetRecoveryPointDetailsAsync(new List<string> {recoveryPointMetadataFileName}, cancellationToken);
            return list.FirstOrDefault();
        }

        public async Task<List<string>> GetBackupLocationsInBackupChainAsync(string backupLocation, CancellationToken cancellationToken)
        {
            try
            {
                return await BackupRestoreUtility.PerformIOWithRetriesAsync(
                    async (bkpLocation, ct) => await PerformDestinationOperationAsync(GetBackupLocationsInBackupChainInternalAsync, bkpLocation, ct),
                    backupLocation,
                    cancellationToken);
            }
            catch (Exception e)
            {
                throw new IOException("Unable to get backup locations in chain.", e);
            }
        }

        #endregion

        #region Utility Methods

        private bool PerformDestinationOperation(Func<object, bool> operation, object context)
        {
            var result = false;
            if (!String.IsNullOrEmpty(this.storeInformation.PrimaryUserName))
            {
                using (var identity = this.GetIdentityToImpersonate())
                {
                    using (var impersonationCtx = identity.Impersonate())
                    {
                        result = operation(context);
                    }
                }
            }
            else
            {
                result = operation(context);
            }

            return result;
        }


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
            var parsedPartitionId = this.storeInformation.ParsePartitionId(metaDataFileName);

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

        internal DateTime ConvertFileNameToDateTime(string fileName)
        {
            if (fileName == null)
            {
                return DateTime.MinValue;
            }
            var backupTime = DateTime.ParseExact(fileName, BackupFolderNameFormat, CultureInfo.InvariantCulture);
            return backupTime;
        }

        public IEnumerable<string> ListMetadataFilesInTheFolder(string directoryName)
        {
            var enumerateInfo = new FileEnumerateInfo
            {
                DirectoryPath = directoryName,
                FileExtension = "*" + RecoveryPointMetadataFileExtension,
                SearchOptionEnum = SearchOption.TopDirectoryOnly
            };

            BackupRestoreUtility.PerformIOWithRetries(
                enumInfo =>
                {
                    PerformDestinationOperation(EnumerateFiles, enumInfo);
                },
                enumerateInfo);

            return enumerateInfo.Files;
        }

        public IEnumerable<string> ListFolders(string directory, string directoryContinuationToken = null)
        {
            var directoryEnumerateInfo = new FolderEnumerateInfo()
            {
                DirectoryPath = directory,
                DirectoryPathContinuationToken = directoryContinuationToken,
                GetFullPath = true
            };
            BackupRestoreUtility.PerformIOWithRetries(
                enumInfo =>
                {
                    PerformDestinationOperation(EnumerateFolders, enumInfo);
                },
                directoryEnumerateInfo);
            return directoryEnumerateInfo.SubFolders;
        }

        private WindowsIdentity GetIdentityToImpersonate()
        {
            WindowsIdentity identity;

            var domainUserNameToTry = this.storeInformation.PrimaryUserName;
            var passwordToTry = this.storeInformation.PrimaryPassword;

            var retrying = false;

            do
            {
                string domainName, userName;
                var tokens = domainUserNameToTry.Split('\\');

                if (tokens.Length == 2)
                {
                    domainName = tokens[0];
                    userName = tokens[1];
                }
                else
                {
                    domainName = ".";
                    userName = domainUserNameToTry;
                }

                try
                {
                    if (this.storeInformation.IsPasswordEncrypted)
                    {
                        var password = EncryptionUtility.DecryptText(passwordToTry);

                        identity = AccountHelper.CreateWindowsIdentity(
                            userName,
                            domainName,
                            password,
                            false,
                            NativeHelper.LogonType.LOGON32_LOGON_NEW_CREDENTIALS,
                            NativeHelper.LogonProvider.LOGON32_PROVIDER_WINNT50);
                    }
                    else
                    {
                        identity = AccountHelper.CreateWindowsIdentity(
                            userName,
                            domainName,
                            passwordToTry,
                            false,
                            NativeHelper.LogonType.LOGON32_LOGON_NEW_CREDENTIALS,
                            NativeHelper.LogonProvider.LOGON32_PROVIDER_WINNT50);
                    }

                    break;
                }
                catch (InvalidOperationException)
                {
                    // Retry with secondary if secondary password exist
                    if (retrying || String.IsNullOrEmpty(this.storeInformation.SecondaryUserName)) throw;

                    domainUserNameToTry = this.storeInformation.SecondaryUserName;
                    passwordToTry = this.storeInformation.SecondaryPassword;
                    retrying = true;
                }
            } while (true);

            return identity;
        }

        private async Task<TResult> PerformDestinationOperationAsync<T, TResult>(Func<T, CancellationToken, Task<TResult>> asyncOperation, T context, CancellationToken cancellationToken)
        {
            if (!String.IsNullOrEmpty(this.storeInformation.PrimaryUserName))
            {
                using (var identity = this.GetIdentityToImpersonate())
                {
                    using (var impersonationCtx = identity.Impersonate())
                    {
                        return await asyncOperation(context, cancellationToken);
                    }
                }
            }

            return await asyncOperation(context, cancellationToken);
        }

        #endregion

        #region Operation Specific inner classes/ methods

        private class FileEnumerateInfo
        {
            internal string DirectoryPath { get; set; }

            internal string FileExtension { get; set; }

            internal IList<string> Files { get; set; }

            internal SearchOption SearchOptionEnum { get; set; } = SearchOption.TopDirectoryOnly;
        }

        private class FolderEnumerateInfo
        {
            internal string DirectoryPath { get; set; }

            internal IList<string> SubFolders { get; set; }

            internal string DirectoryPathContinuationToken { get; set; }    

            internal bool GetFullPath { get; set; } = true;
        }

        private bool EnumerateFiles(object context)
        {
            var fileEnumerateInfo = (FileEnumerateInfo)context; ;
            fileEnumerateInfo.Files = FabricDirectory.GetFiles(fileEnumerateInfo.DirectoryPath, fileEnumerateInfo.FileExtension, fileEnumerateInfo.SearchOptionEnum);
            return true;
        }

        private bool EnumerateFolders(object context)
        {
            var folderEnumerateInfo = (FolderEnumerateInfo)context;
            folderEnumerateInfo.SubFolders = FabricDirectory.GetDirectories(folderEnumerateInfo.DirectoryPath, "*", folderEnumerateInfo.GetFullPath, SearchOption.TopDirectoryOnly).Where(x => x.CompareTo(folderEnumerateInfo.DirectoryPathContinuationToken) >= 0).ToList();
            return true; 
        }

        private bool DeleteFile(object context)
        {
            var filePath = (string)context;
            FabricFile.Delete(filePath);
            return true;
        }

        private async Task<List<RestorePoint>> GetRecoveryPointDetailsInternalAsync(IEnumerable<string> metadataFiles, CancellationToken cancellationToken)
        {
            var backupList = new List<RestorePoint>();
            foreach (var metadataFile in metadataFiles)
            {
                cancellationToken.ThrowIfCancellationRequested();

                var recoveryPointMetadataFile =
                    await RecoveryPointMetadataFile.OpenAsync(Path.Combine(this.storeInformation.Path, metadataFile), cancellationToken);

                var recoveryPoint = new RestorePoint()
                {
                    BackupChainId = recoveryPointMetadataFile.BackupChainId,
                    BackupId = recoveryPointMetadataFile.BackupId,
                    ParentRestorePointId = recoveryPointMetadataFile.ParentBackupId,
                    BackupLocation = recoveryPointMetadataFile.BackupLocation,
                    CreationTimeUtc = recoveryPointMetadataFile.BackupTime,
                    BackupType = 
                        recoveryPointMetadataFile.ParentBackupId == Guid.Empty? BackupOptionType.Full : BackupOptionType.Incremental,
                    EpochOfLastBackupRecord = new BackupEpoch
                    {
                        ConfigurationNumber  = recoveryPointMetadataFile.EpochOfLastBackupRecord.ConfigurationNumber ,
                        DataLossNumber =  recoveryPointMetadataFile.EpochOfLastBackupRecord.DataLossNumber
                    },
                    LsnOfLastBackupRecord = recoveryPointMetadataFile.LsnOfLastBackupRecord,
                    PartitionInformation = this.GetBackupServicePartitionInformationFromServicePartitionInformation(recoveryPointMetadataFile.PartitionInformation),
                    ServiceManifestVersion = recoveryPointMetadataFile.ServiceManifestVersion,
                };

                PopulateApplicationServiceAndPartitionInfo(recoveryPoint, metadataFile);

                backupList.Add(recoveryPoint);
            }

            return backupList;
        }

        bool CheckIfBackupExists(string fileOrFolderPath)
        {
            if (fileOrFolderPath.EndsWith(ZipFileExtension) || fileOrFolderPath.EndsWith(RecoveryPointMetadataFileExtension))
            {
                return FabricFile.Exists(fileOrFolderPath);
            }
            return FabricDirectory.Exists(fileOrFolderPath);
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

            var fullBackupLocation = Path.Combine(this.storeInformation.Path, backupLocation);

            // Check if backup exists
            if (!CheckIfBackupExists(fullBackupLocation))
            {
                throw new FabricElementNotFoundException(String.Format("Missing backup!! Couldnt find backup folder {0} which is there in backup chain", fullBackupLocation));
            }

            var recoveryPointMetadataFileName = GetRecoveryPointMetadataFileNameFromBackupLocation(fullBackupLocation);
            var recoveryPointMetadataFile = await RecoveryPointMetadataFile.OpenAsync(recoveryPointMetadataFileName, cancellationToken);

            backupLocationList.Add(recoveryPointMetadataFile.BackupLocation);

            while (recoveryPointMetadataFile.ParentBackupId != Guid.Empty)
            {
                fullBackupLocation = Path.Combine(this.storeInformation.Path,
                    recoveryPointMetadataFile.ParentBackupLocation);

                // Check if backup folder exists
                if (!CheckIfBackupExists(fullBackupLocation))
                {
                    throw new FabricElementNotFoundException(String.Format("Missing backup!! Couldnt find backup folder {0} which is there in backup chain", fullBackupLocation));
                }

                recoveryPointMetadataFileName = GetRecoveryPointMetadataFileNameFromBackupLocation(fullBackupLocation);
                recoveryPointMetadataFile = await RecoveryPointMetadataFile.OpenAsync(recoveryPointMetadataFileName, cancellationToken);

                backupLocationList.Add(recoveryPointMetadataFile.BackupLocation);
            }

            Debug.Assert(recoveryPointMetadataFile.BackupId == recoveryPointMetadataFile.BackupChainId, "Backup ID for root doesnt match with backup chain ID");
            return backupLocationList;
        }

        private void PopulateApplicationServiceAndPartitionInfo(RestorePoint recoveryPoint, string metadataFile)
        {
            var tokens = metadataFile.Split('\\');
            var tokensLength = tokens.Length;

            if (tokensLength >= 4)
            {
                var partitionIdStr = tokens[tokensLength - 2];
                var serviceNameStr = tokens[tokensLength - 3];
                var applicationNameStr = tokens[tokensLength - 4];

                recoveryPoint.PartitionInformation.Id = Guid.Parse(partitionIdStr);
                recoveryPoint.ApplicationName = new Uri(String.Format("fabric:/{0}", UtilityHelper.GetUriFromCustomUri( applicationNameStr)));
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
                    backupServicePartitionInformation = new BackupRestoreTypes.SingletonPartitionInformation();
                    break;

                case ServicePartitionKind.Int64Range:
                    var int64RangePartitionInformation = servicePartitionInformation as Int64RangePartitionInformation;
                    ThrowIf.Null(int64RangePartitionInformation, "int64RangePartitionInformation");
                    backupServicePartitionInformation = new BackupRestoreTypes.Int64RangePartitionInformation()
                    {
                        
                        HighKey = int64RangePartitionInformation.HighKey ,
                        LowKey =  int64RangePartitionInformation.LowKey
                    };
                    break;

                case ServicePartitionKind.Named:
                    var namedPartitionInformation = servicePartitionInformation as NamedPartitionInformation;
                    ThrowIf.Null(namedPartitionInformation, "namedPartitionInformation");
                    backupServicePartitionInformation = new BackupRestoreTypes.NamedPartitionInformation()
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