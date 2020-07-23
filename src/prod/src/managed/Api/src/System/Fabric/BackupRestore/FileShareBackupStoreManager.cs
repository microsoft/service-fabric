// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Fabric.BackupRestore.DataStructures;
using System.Fabric.Common;
using System.IO;
using System.Security.AccessControl;
using System.Security.Principal;
using System.Fabric.Security;

namespace System.Fabric.BackupRestore
{
    /// <summary>
    /// File share store manager
    /// </summary>
    internal class FileShareBackupStoreManager : IBackupStoreManager
    {
        private const string TraceType = "FileShareBackupStoreManager";

        private const char PathSeparator = '\\';

        private readonly FileShareBackupStore _storeInformation;

        public FileShareBackupStoreManager(FileShareBackupStore fileShareStoreInformation)
        {
            this._storeInformation = fileShareStoreInformation;
        }

        #region IBackupStoreManager Methods

        public void Upload(string sourceFileOrFolderPath, string destinationFolderName, bool sourceIsFile = false)
        {
            var destinationFolder = this._storeInformation.FileSharePath;
            if (!String.IsNullOrEmpty(destinationFolderName))
            {
                destinationFolder += PathSeparator + destinationFolderName;
            }

            AppTrace.TraceSource.WriteInfo(TraceType, "Moving backup from {0} to {1}", sourceFileOrFolderPath, destinationFolder);

            var workFolder = Path.GetTempPath();

            string sourceFolder;
            if (!sourceIsFile)
            {
                sourceFolder = sourceFileOrFolderPath;
            }
            else
            {
                sourceFolder = Path.GetDirectoryName(sourceFileOrFolderPath);
            }
            
            // Check if the source folder can be accessed under impersonation, if not ACL it accordingly
            try
            {
                BackupRestoreUtility.PerformIOWithRetries(
                    () =>
                    {
                        if (CheckFolderAccessUnderImpersonation(sourceFolder, FileAccess.Read))
                        {
                            UpdateAclOnFolder(sourceFolder, FileSystemRights.Read);
                        }
                    });
            }
            catch (Exception e)
            {
                throw new IOException(String.Format("Unable to create staging directory for work directory {0}.", workFolder), e);
            }

            if (sourceIsFile)
            {
                CopyFileToDestination(sourceFileOrFolderPath, destinationFolder, "", 3);
            }
            else
            {
                var sourceDirInfo = new DirectoryInfo(sourceFolder);
                foreach (var file in sourceDirInfo.GetFiles("*", SearchOption.AllDirectories))
                {
                    var sourceRelative = file.DirectoryName.Substring(sourceFolder.Trim('\\').Length).Trim('\\');
                    CopyFileToDestination(file.FullName, destinationFolder, sourceRelative, 3);
                }
            }
        }

        public void Download(string sourceFileOrFolderName, string destinationFolder, bool sourceIsFile = false)
        {
            var sourceFileOrFolderPath = Path.Combine(this._storeInformation.FileSharePath, sourceFileOrFolderName);
            AppTrace.TraceSource.WriteInfo(TraceType, "Moving backup from {0} to {1}", sourceFileOrFolderPath, destinationFolder);

            // First check if the destination folder exists or not, if not create it
            BackupRestoreUtility.PerformIOWithRetries(
                () =>
                {
                    if (!Directory.Exists(destinationFolder))
                    {
                        Directory.CreateDirectory(destinationFolder);
                    }
                });

            // Check if post impersonation we would be able to copy the content to the destination dir, 
            // if not, add ACL appropriately.
            try
            {
                BackupRestoreUtility.PerformIOWithRetries(
                    () =>
                    {
                        if (CheckFolderAccessUnderImpersonation(destinationFolder, FileAccess.ReadWrite))
                        {
                            UpdateAclOnFolder(destinationFolder, FileSystemRights.FullControl);
                        }
                    });
            }
            catch (Exception e)
            {
                throw new IOException(String.Format("Unable to check for folder access or update ACL for destination folder {0}.", destinationFolder), e);
            }

            if (!sourceIsFile)
            {
                FileInfo[] files = null;
                var sourceDirInfo = new DirectoryInfo(sourceFileOrFolderPath);

                BackupRestoreUtility.PerformIOWithRetries(
                    () =>
                        PerformOperationUnderImpersonation(
                            () =>
                            {
                                files = sourceDirInfo.GetFiles("*", SearchOption.AllDirectories);
                                return true;
                            })
                    );

                if (null == files) throw new IOException(String.Format("Unable to enumerate files under directory {0}", sourceFileOrFolderPath));

                foreach (var file in files)
                {
                    var sourceRelative = file.DirectoryName.Substring(sourceFileOrFolderPath.Trim(PathSeparator).Length).Trim(PathSeparator);
                    CopyFileToDestination(file.FullName, destinationFolder, sourceRelative, 3);
                }
            }
            else
            {
                CopyFileToDestination(sourceFileOrFolderPath, destinationFolder, String.Empty, 3);
            }
        }

        #endregion

        #region Private Methods

        private bool PerformOperationUnderImpersonation(Func<bool> operation)
        {
            var result = false;
            if (FileShareAccessType.None != this._storeInformation.AccessType)
            {
                using (var identity = this.GetIdentityToImpersonate())
                {
                    using (var impersonationCtx = identity.Impersonate())
                    {
                        result = operation();
                    }
                }
            }
            else
            {
                result = operation();
            }
            
            return result;
        }

        private class FileCopyInfo
        {
            internal string Source { get; set; }

            internal string Destination { get; set; }
        }
        
        private bool CopyFileToDestination(object context)
        {
            var fileCopyInfo = (FileCopyInfo)context;
            FabricFile.Copy(fileCopyInfo.Source, fileCopyInfo.Destination, true);
            return true;
        }

        private void UpdateAclOnFolder(string folderPath, FileSystemRights rights)
        {
            // Get folder ACLs
            var directorySecurity = new DirectorySecurity(folderPath,  AccessControlSections.All);

            // Get the identity that we impersonate
            using (var identity = this.GetIdentityToImpersonate())
            {
                // Provide read access for the identity that we impersonate
                directorySecurity.AddAccessRule(
                    new FileSystemAccessRule(
                            identity.Name,
                            rights, 
                            InheritanceFlags.ContainerInherit | InheritanceFlags.ObjectInherit,
                            PropagationFlags.None,
                            AccessControlType.Allow));
            }

            // Update the folder ACLs
            var directoryInfo = new DirectoryInfo(folderPath);
            directoryInfo.SetAccessControl(directorySecurity);
        }

        private bool CheckFolderAccessUnderImpersonation(string folderToCheck, FileAccess accessType)
        {
            // Check if we are impersonating to copy files to the specified folder, if not, we dont need a staging folder
            if (FileShareAccessType.None == this._storeInformation.AccessType) return false;

            // We are impersonating in order to copy files to the specified folder.
            // Check if we can access the specified folder while impersonating.
            var folderAccessDeniedUnderImpersonation = false;
            using (var identity = this.GetIdentityToImpersonate())
            {
                using (var impersonationCtx = identity.Impersonate())
                {
                    try
                    {
                        if (accessType == FileAccess.Read)
                        {
                            // Use Directory.GetFiles directly instead of using FabricDirectory.GetFiles
                            // because the latter fails silently and returns an empty array if access is 
                            // denied.
                            Directory.GetFiles(folderToCheck);
                        }
                        else
                        {
                            // Need to check for read write access, create a temp file at the directory
                            using (var fs = File.Create(
                                Path.Combine(
                                    folderToCheck,
                                    Path.GetRandomFileName()
                                    ),
                                1,
                                FileOptions.DeleteOnClose))
                            {
                            }
                        }
                    }
                    catch (UnauthorizedAccessException)
                    {
                        // We are unable to access the specified folder while impersonating.
                        folderAccessDeniedUnderImpersonation = true;
                    }
                }
                return folderAccessDeniedUnderImpersonation;
            }
        }

        private WindowsIdentity GetIdentityToImpersonate()
        {
            WindowsIdentity identity;

            var domainUserNameToTry = this._storeInformation.PrimaryUserName;
            var passwordToTry = this._storeInformation.PrimaryPassword;

            var retrying = false;

            do
            {
                string domainName, userName;
                var tokens = domainUserNameToTry.Split(PathSeparator);

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
                    if (this._storeInformation.IsPasswordEncrypted)
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
                    if (retrying || String.IsNullOrEmpty(this._storeInformation.SecondaryUserName)) throw;

                    domainUserNameToTry = this._storeInformation.SecondaryUserName;
                    passwordToTry = this._storeInformation.SecondaryPassword;
                    retrying = true;
                }
            } while (true);

            return identity;
        }

        private void CopyFileToDestination(string source, string destinationPath, string sourceRelative, int retryCount)
        {
            // Compute the destination path
            var sourceFileName = Path.GetFileName(source);
            string destination = Path.Combine(destinationPath, sourceRelative);
            destination = Path.Combine(destination, sourceFileName);

            // Figure out the name of the directory at the destination
            string destinationDir;

            try
            {
                destinationDir = Path.GetDirectoryName(destination);
            }
            catch (PathTooLongException e)
            {
                // The path to the destination directory is too long. Skip it.
                AppTrace.TraceSource.WriteExceptionAsError(
                    TraceType,
                    e,
                    "The destination path is too long {0}",
                    destination);
                throw;
            }

            // If the directory at the destination doesn't exist, then create it
            if (!String.IsNullOrEmpty(destinationDir))
            {
                try
                {
                    BackupRestoreUtility.PerformIOWithRetries(
                        () =>
                        this.PerformOperationUnderImpersonation(
                            () =>
                            {
                                if (!FabricDirectory.Exists(destinationDir))
                                {
                                    FabricDirectory.CreateDirectory(destinationDir);
                                }

                                return true;
                            }),
                        retryCount);
                }
                catch (Exception e)
                {
                    AppTrace.TraceSource.WriteExceptionAsError(
                        TraceType,
                        e,
                        "Failed to create directory {0} for copying file {1}.",
                        destinationDir,
                        sourceRelative);
                    throw;
                }
            }

            // Copy the file over to its destination
            try
            {
                BackupRestoreUtility.PerformIOWithRetries(
                    () =>
                    {
                        FileCopyInfo copyInfo = new FileCopyInfo
                        {
                            Source = source,
                            Destination = destination
                        };
                        PerformOperationUnderImpersonation(
                            () => CopyFileToDestination(copyInfo));
                    },
                    retryCount);
            }
            catch (Exception e)
            {
                AppTrace.TraceSource.WriteExceptionAsError(
                    TraceType,
                    e,
                    "File copy failed. Source: {0}, destination: {1}",
                    source,
                    destination);
                throw;
            }
        }

        #endregion
    }
}