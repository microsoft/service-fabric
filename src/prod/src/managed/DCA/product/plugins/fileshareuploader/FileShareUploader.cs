// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA 
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Dca;
    using System.Fabric.Dca.FileUploader;
    using System.IO;
    using System.Security;
    using System.Security.AccessControl;
    using System.Security.Principal;

    // This class implements the logic to upload files in a folder to a file share
    internal class FileShareUploader : FileUploaderBase, IDisposable
    {
        // Constants
        private const string StagingFolderForCopy = "StagingFolderForCopy";

        // Trimmer objects responsible for deleting old files from various file shares.
        // Key is the path to the file share.
        private static readonly Dictionary<string, TrimmerInfo> Trimmers = new Dictionary<string, TrimmerInfo>();

        // Folder on the file share to which data needs to be uploaded
        private readonly string destinationPath;

        // Object that measures performance
        private readonly FileSharePerformance perfHelper;

        // Information used to access the file share
        private AccessInformation accessInfo;

        // Files are copied to the staging folder on the local node before
        // being copied to the destination if both the following are true:
        // - Impersonation is used to copy files to the destination
        // - The identity that is being impersonated does not have access to the
        //   source folder.
        // If both the above are true, we create a staging folder and allow the
        // identity being impersonated to access it.
        private string stagingFolderPath;

        internal FileShareUploader(
                          FabricEvents.ExtensionsEvents traceSource, 
                          string logSourceId,
                          bool runningOnBehalfOfApplication,
                          string folderName,
                          string destinationPath,
                          AccessInformation accessInfo, 
                          string workFolder,
                          TimeSpan uploadInterval,
                          TimeSpan fileSyncInterval,
                          TimeSpan fileDeletionAge,
                          bool destinationIsLocalAppFolder,
                          string fabricNodeId)
            : base(
            traceSource,
            logSourceId,
            folderName,
            destinationPath,
            workFolder,
            uploadInterval,
            fileSyncInterval)
        {
            // Initialization
            this.destinationPath = destinationPath;
            this.accessInfo = accessInfo;
            var impersonationRequiredForAppStoreAccess = Utility.IsImpersonationRequiredForAppDiagnosticStoreAccess();

            if (runningOnBehalfOfApplication &&
                (false == destinationIsLocalAppFolder) &&
                impersonationRequiredForAppStoreAccess &&
                (FileShareAccessAccountType.None == this.accessInfo.AccountType))
            {
                const string Message =
                    "Impersonation is required to access the file share, but credentials for impersonation have not been provided.";
                this.TraceSource.WriteError(
                    this.LogSourceId,
                    Message);
                throw new InvalidOperationException(Message);
            }

            // Create the staging folder where we copy the files temporarily before impersonating
            // and copying it to the final destination.
            try
            {
                Utility.PerformIOWithRetries(
                    () =>
                    {
                        if (IsStagingFolderNeededForCopy(folderName))
                        {
                            CreateStagingFolderForCopy(workFolder);
                        }
                    });
            }
            catch (Exception e)
            {
                throw new IOException(string.Format("Unable to create staging directory for work directory {0}.", workFolder), e);
            }

            // Check that the destination folder exists or can be created.
            try
            {
                Utility.PerformIOWithRetries(
                    () =>
                    this.PerformDestinationOperation(
                        ctx =>
                        {
                            var destPath = (string)ctx;
                            if (FabricDirectory.Exists(destPath))
                            {
                                return true;
                            }

                            FabricDirectory.CreateDirectory(destPath);
                            return true;
                        }, 
                    this.destinationPath));
            }
            catch (Exception e)
            {
                throw new IOException(string.Format("Unable to create destination directory {0}.", this.destinationPath), e);
            }

            this.perfHelper = new FileSharePerformance(this.TraceSource, this.LogSourceId);

            // File copy is done one at a time, so the concurrency count is 1.
            this.perfHelper.ExternalOperationInitialize(
                ExternalOperationTime.ExternalOperationType.FileShareCopy,
                1);

            // Check if a trimmer already exists to delete old files from this destination
            lock (Trimmers)
            {
                if (Trimmers.ContainsKey(this.destinationPath))
                {
                    // Trimmer already exists. Increment its reference count.
                    TrimmerInfo trimmerInfo = Trimmers[this.destinationPath];
                    trimmerInfo.RefCount++;
                }
                else
                {
                    // Trimmer does not exist. Create it.
                    FileShareTrimmer trimmer = new FileShareTrimmer(
                                                        folderName,
                                                        this.destinationPath,
                                                        this.PerformDestinationOperation,
                                                        LocalMapFolderPath,
                                                        fileDeletionAge);
                    TrimmerInfo trimmerInfo = new TrimmerInfo
                    {
                        RefCount = 1,
                        Trimmer = trimmer
                    };
                    Trimmers[this.destinationPath] = trimmerInfo;
                }
            }
        }

        internal enum FileShareAccessAccountType
        {
            None,
            DomainUser,
            ManagedServiceAccount
        }

        public void Dispose()
        {
            this.DisposeBase();

            lock (Trimmers)
            {
                if (Trimmers.ContainsKey(this.destinationPath))
                {
                    // Decrement reference count on the trimmer object
                    TrimmerInfo trimmerInfo = Trimmers[this.destinationPath];
                    trimmerInfo.RefCount--;

                    if (0 == trimmerInfo.RefCount)
                    {
                        // No one else is using the trimmer. Dispose it.
                        trimmerInfo.Trimmer.Dispose();
                        Trimmers.Remove(this.destinationPath);
                    }
                }
            }

            GC.SuppressFinalize(this);
        }

        protected override bool CopyFileToDestination(string source, string sourceRelative, int retryCount, out bool fileSkipped, out bool fileCompressed)
        {
            // No compression
            fileCompressed = false;

            // Compute the destination path
            string destination = Path.Combine(this.destinationPath, sourceRelative);

            // Figure out the name of the directory at the destination and in the
            // local map.
            var destinationDir = FabricPath.GetDirectoryName(destination);

            // If the directory at the destination doesn't exist, then create it
            if ((false == string.IsNullOrEmpty(destinationDir)) &&
                (false == this.PerformDestinationOperation(DirectoryExistsAtDestination, destinationDir)))
            {
                try
                {
                    Utility.PerformIOWithRetries(
                        () =>
                        {
                            PerformDestinationOperation(
                                CreateDirectoryAtDestination,
                                destinationDir);
                        },
                        retryCount);
                }
                catch (Exception e)
                {
                    this.TraceSource.WriteExceptionAsError(
                        this.LogSourceId,
                        e,
                        "Failed to create directory {0} for copying file {1}.",
                        destinationDir,
                        sourceRelative);
                    fileSkipped = true;
                    return false;
                }
            }

            // Determine whether we need to first copy the file to the staging folder
            string stagingFilePath = null;
            if (false == string.IsNullOrEmpty(this.stagingFolderPath))
            {
                stagingFilePath = Path.Combine(this.stagingFolderPath, Path.GetFileName(sourceRelative));
            }

            // Copy the file over to its destination
            DateTime sourceLastWriteTime = DateTime.MinValue;
            bool sourceFileNotFound = false;
            try
            {
                try
                {
                    Utility.PerformIOWithRetries(
                        () =>
                        {
                            if (string.IsNullOrEmpty(stagingFilePath))
                            {
                                this.perfHelper.ExternalOperationBegin(
                                    ExternalOperationTime.ExternalOperationType.FileShareCopy,
                                    0);
                            }

                            try
                            {
                                // Copy the file
                                if (false == string.IsNullOrEmpty(stagingFilePath))
                                {
                                    FabricFile.Copy(source, stagingFilePath, true);
                                }
                                else
                                {
                                    FileCopyInfo copyInfo = new FileCopyInfo
                                    {
                                        Source = source,
                                        Destination = destination
                                    };
                                    PerformDestinationOperation(CopyFileToDestination, copyInfo);
                                }
                            }
                            catch (FileNotFoundException)
                            {
                                sourceFileNotFound = true;
                            }

                            if (string.IsNullOrEmpty(stagingFilePath))
                            {
                                this.perfHelper.ExternalOperationEnd(
                                    ExternalOperationTime.ExternalOperationType.FileShareCopy,
                                    0);
                                this.perfHelper.FileUploaded();
                            }
                        },
                        retryCount);
                }
                catch (Exception e)
                {
                    this.TraceSource.WriteExceptionAsError(
                        this.LogSourceId,
                        e,
                        "File copy failed. Source: {0}, destination: {1}",
                        source,
                        destination);
                    fileSkipped = true;
                    return false;
                }

                if (sourceFileNotFound)
                {
                    // The source file was not found. Maybe it
                    // got deleted before we had a chance to copy
                    // it. Handle the error and move on.
                    this.TraceSource.WriteWarning(
                        this.LogSourceId,
                        "File {0} could not be uploaded to {1} because it was not found.",
                        source,
                        destination);
                    fileSkipped = true;
                    return true;
                }

                if (false == string.IsNullOrEmpty(stagingFilePath))
                {
                    try
                    {
                        Utility.PerformIOWithRetries(
                            () =>
                            {
                                this.perfHelper.ExternalOperationBegin(
                                    ExternalOperationTime.ExternalOperationType.FileShareCopy,
                                    0);

                                FileCopyInfo copyInfo = new FileCopyInfo
                                {
                                    Source = stagingFilePath,
                                    Destination = destination
                                };
                                PerformDestinationOperation(CopyFileToDestination, copyInfo);

                                this.perfHelper.ExternalOperationEnd(
                                    ExternalOperationTime.ExternalOperationType.FileShareCopy,
                                    0);
                                this.perfHelper.FileUploaded();
                            },
                            retryCount);
                    }
                    catch (Exception e)
                    {
                        this.TraceSource.WriteExceptionAsError(
                            this.LogSourceId,
                            e,
                            "File copy failed. Source: {0}, destination: {1}",
                            source,
                            destination);
                        fileSkipped = true;
                        return false;
                    }
                }
            }
            finally
            {
                if (false == string.IsNullOrEmpty(stagingFilePath))
                {
                    try
                    {
                        Utility.PerformIOWithRetries(
                            () =>
                            {
                                FabricFile.Delete(stagingFilePath);
                            },
                            retryCount);
                    }
                    catch (Exception e)
                    {
                        this.TraceSource.WriteExceptionAsError(
                            this.LogSourceId,
                            e,
                            "Failed to delete file {0}",
                            stagingFilePath);
                    }
                }
            }

            fileSkipped = false;
            return true;
        }

        protected override bool FileUploadPassBegin()
        {
            this.perfHelper.FileUploadPassBegin();
            return true;
        }

        protected override void FileUploadPassEnd()
        {
            this.perfHelper.FileUploadPassEnd();
        }

        private static bool DirectoryExistsAtDestination(object context)
        {
            string dirName = (string)context;
            return FabricDirectory.Exists(dirName);
        }

        private static bool CreateDirectoryAtDestination(object context)
        {
            string dirName = (string)context;
            FabricDirectory.CreateDirectory(dirName);
            return true;
        }

        private static bool CopyFileToDestination(object context)
        {
            FileCopyInfo fileCopyInfo = (FileCopyInfo)context;
            FabricFile.Copy(fileCopyInfo.Source, fileCopyInfo.Destination, true);
            return true;
        }

        private WindowsIdentity GetIdentityToImpersonate()
        {
            WindowsIdentity identity = null;
            if (this.accessInfo.AccountPasswordIsEncrypted)
            {
                SecureString password = NativeConfigStore.DecryptText(this.accessInfo.AccountPassword);

                identity = AccountHelper.CreateWindowsIdentity(
                    this.accessInfo.UserName,
                    this.accessInfo.DomainName,
                    password,
                    FileShareAccessAccountType.ManagedServiceAccount == this.accessInfo.AccountType);
            }
            else
            {
                identity = AccountHelper.CreateWindowsIdentity(
                    this.accessInfo.UserName,
                    this.accessInfo.DomainName,
                    this.accessInfo.AccountPassword,
                    FileShareAccessAccountType.ManagedServiceAccount == this.accessInfo.AccountType);
            }

            return identity;
        }

        private bool IsStagingFolderNeededForCopy(string sourceFolder)
        {
            if (FileShareAccessAccountType.None != this.accessInfo.AccountType)
            {
                // We are impersonating in order to copy files to destination.
                // Check if we can access the source folder while impersonating.
                bool sourceAccessDeniedUnderImpersonation = false;
                using (WindowsIdentity identity = this.GetIdentityToImpersonate())
                {
#if DotNetCoreClr
                    WindowsIdentity.RunImpersonated(identity.AccessToken, () =>
#else
                    using (WindowsImpersonationContext impersonationCtx = identity.Impersonate())
#endif
                    {
                        try
                        {
                            // Use Directory.GetFiles directly instead of using FabricDirectory.GetFiles
                            // because the latter fails silently and returns an empty array if access is 
                            // denied.
                            FabricDirectory.GetFiles(sourceFolder);
                        }
                        catch (UnauthorizedAccessException)
                        {
                            // We are unable to access the source folder while impersonating.
                            // Hence we need a staging folder, which we can access while 
                            // impersonating.
                            sourceAccessDeniedUnderImpersonation = true;
                        }
                    }
#if DotNetCoreClr
                    );
#endif

                    return sourceAccessDeniedUnderImpersonation;
                }
            }
            else
            {
                // We are not impersonating in order to copy files to destination.
                // So a staging folder is not needed.
                return false;
            }
        }

        private void CreateStagingFolderForCopy(string workFolder)
        {
            // Create folder
            this.stagingFolderPath = Path.Combine(workFolder, StagingFolderForCopy);
            FabricDirectory.CreateDirectory(this.stagingFolderPath);
            
            // Get folder ACLs
#if DotNetCoreClr
            DirectoryInfo dirInfo = new DirectoryInfo(this.stagingFolderPath);
            DirectorySecurity directorySecurity = dirInfo.GetAccessControl();
#else
            DirectorySecurity directorySecurity = Directory.GetAccessControl(this.stagingFolderPath);
#endif

            // Get the identity that we impersonate
            using (WindowsIdentity identity = this.GetIdentityToImpersonate())
            {
                // Provide read access for the identity that we impersonate
                directorySecurity.AddAccessRule(
                    new FileSystemAccessRule(
                            identity.Name, 
                            FileSystemRights.Read, 
                            InheritanceFlags.ContainerInherit | InheritanceFlags.ObjectInherit,
                            PropagationFlags.None,
                            AccessControlType.Allow));
            }

            // Update the folder ACLs
#if DotNetCoreClr
            dirInfo.SetAccessControl(directorySecurity);
#else
            Directory.SetAccessControl(this.stagingFolderPath, directorySecurity);
#endif
        }

        private bool PerformDestinationOperation(FolderTrimmer.DestinationOperation operation, object context)
        {
            bool result = false;
            if (FileShareAccessAccountType.None != this.accessInfo.AccountType)
            {
                using (WindowsIdentity identity = this.GetIdentityToImpersonate())
                {
#if DotNetCoreClr
                    WindowsIdentity.RunImpersonated(identity.AccessToken, () =>
#else
                    using (WindowsImpersonationContext impersonationCtx = identity.Impersonate())
#endif
                    {
                        result = operation(context);
                    }
#if DotNetCoreClr
                    );
#endif
                }
            }
            else
            {
                result = operation(context);
            }

            return result;
        }

        internal struct AccessInformation
        {
            internal FileShareAccessAccountType AccountType;
            internal string UserName;
            internal string DomainName;
            internal string AccountPassword;
            internal bool AccountPasswordIsEncrypted;
        }

        private class FileCopyInfo
        {
            internal string Source { get; set; }

            internal string Destination { get; set; }
        }

        private class TrimmerInfo
        {
            internal int RefCount { get; set; }

            internal FileShareTrimmer Trimmer { get; set; }
        }
    }
}