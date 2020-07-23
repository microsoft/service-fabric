// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupCopier
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Fabric.BackupRestore;
    using System.Fabric.BackupRestore.DataStructures;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.IO;
    using System.Text;
    using System.Threading;
    using System.Net;

    internal class BackupCopier
    {
        internal static FabricEvents.ExtensionsEvents TraceSource;

        private static readonly string TraceType = "BackupCopier";
        private static readonly string TelemetryTraceType = "Telemetry";

        internal static TimeSpan InitialRetryInterval { get; private set; }
        internal static TimeSpan MaxRetryInterval { get; private set; }
        internal static int MaxRetryCount { get; private set; }

        private static readonly Dictionary<Type, int> ManagedToNativeExceptionTranslation = new Dictionary<Type, int>()
        {
            { typeof(ArgumentException), unchecked((int) NativeTypes.FABRIC_ERROR_CODE.E_INVALIDARG) },
            { typeof(FileNotFoundException), unchecked((int) NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_FILE_NOT_FOUND) },
            { typeof(DirectoryNotFoundException), unchecked((int) NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_DIRECTORY_NOT_FOUND) },
            { typeof(OperationCanceledException), unchecked((int) NativeTypes.FABRIC_ERROR_CODE.E_ABORT) },
            { typeof(PathTooLongException), unchecked((int) NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_PATH_TOO_LONG) },
            { typeof(UnauthorizedAccessException), unchecked((int) NativeTypes.FABRIC_ERROR_CODE.E_ACCESSDENIED) },
            { typeof(TimeoutException), unchecked((int) NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_TIMEOUT) }
        };

        private static readonly HashSet<string> SupportedCommandArgs = new HashSet<string>(StringComparer.OrdinalIgnoreCase)
        {
            StringConstants.OperationKeyName,

            StringConstants.StorageCredentialsSourceLocationKeyName,

            StringConstants.ConnectionStringKeyName,
            StringConstants.IsConnectionStringEncryptedKeyName,
            StringConstants.ContainerNameKeyName,
            StringConstants.BackupStoreBaseFolderPathKeyName,

            StringConstants.SourceFileOrFolderPathKeyName,
            StringConstants.TargetFolderPathKeyName,
            StringConstants.BackupMetadataFilePathKeyName,

            StringConstants.FileSharePathKeyName,
            StringConstants.FileShareAccessTypeKeyName,

            StringConstants.PrimaryUserNameKeyName,
            StringConstants.PrimaryPasswordKeyName,
            StringConstants.SecondaryUserNameKeyName,
            StringConstants.SecondaryPasswordKeyName,
            StringConstants.IsPasswordEncryptedKeyName,

            StringConstants.Timeout,
            StringConstants.WorkingDir, // TODO: Revisit to see if this is needed.
            StringConstants.ErrorDetailsFile,
            StringConstants.ProgressFile
        };

        private static readonly HashSet<string> CommandArgsForCredentials = new HashSet<string>(StringComparer.OrdinalIgnoreCase)
        {
            StringConstants.ConnectionStringKeyName,
            StringConstants.PrimaryPasswordKeyName,
            StringConstants.SecondaryPasswordKeyName,
        };

        private static readonly HashSet<string> CommandArgsWithEncodedValues = new HashSet<string>(StringComparer.OrdinalIgnoreCase)
        {
            StringConstants.PrimaryPasswordKeyName,
            StringConstants.SecondaryPasswordKeyName,
        };

        private static readonly List<string> RequiredCommandParameters = new List<string>() { StringConstants.OperationKeyName };

        private static readonly Dictionary<string, string[]> OperationSpecificRequiredParameters = new Dictionary<string, string[]>()
        {
            {
                StringConstants.AzureBlobStoreUpload,
                new string[]
                {
                    StringConstants.ConnectionStringKeyName,
                    StringConstants.IsConnectionStringEncryptedKeyName,
                    StringConstants.ContainerNameKeyName,
                    StringConstants.BackupStoreBaseFolderPathKeyName,

                    StringConstants.SourceFileOrFolderPathKeyName,
                    StringConstants.TargetFolderPathKeyName,
                    StringConstants.BackupMetadataFilePathKeyName
                }
            },
            {
                StringConstants.AzureBlobStoreDownload,
                new string[]
                {
                    StringConstants.ConnectionStringKeyName,
                    StringConstants.IsConnectionStringEncryptedKeyName,
                    StringConstants.ContainerNameKeyName,
                    StringConstants.BackupStoreBaseFolderPathKeyName,

                    StringConstants.SourceFileOrFolderPathKeyName,
                    StringConstants.TargetFolderPathKeyName
                }
            },
            {
                StringConstants.DsmsAzureBlobStoreUpload,
                new string[]
                {
                    StringConstants.StorageCredentialsSourceLocationKeyName,
                    StringConstants.ContainerNameKeyName,
                    StringConstants.BackupStoreBaseFolderPathKeyName,

                    StringConstants.SourceFileOrFolderPathKeyName,
                    StringConstants.TargetFolderPathKeyName,
                    StringConstants.BackupMetadataFilePathKeyName
                }
            },
            {
                StringConstants.DsmsAzureBlobStoreDownload,
                new string[]
                {
                    StringConstants.StorageCredentialsSourceLocationKeyName,
                    StringConstants.ContainerNameKeyName,
                    StringConstants.BackupStoreBaseFolderPathKeyName,

                    StringConstants.SourceFileOrFolderPathKeyName,
                    StringConstants.TargetFolderPathKeyName
                }
            },
            {
                StringConstants.FileShareUpload,
                new string[]
                {
                    StringConstants.FileSharePathKeyName,
                    StringConstants.FileShareAccessTypeKeyName,

                    StringConstants.SourceFileOrFolderPathKeyName,
                    StringConstants.TargetFolderPathKeyName,
                    StringConstants.BackupMetadataFilePathKeyName
                }
            },
            {
                StringConstants.FileShareDownload,
                new string[]
                {
                    StringConstants.FileSharePathKeyName,
                    StringConstants.FileShareAccessTypeKeyName,

                    StringConstants.SourceFileOrFolderPathKeyName,
                    StringConstants.TargetFolderPathKeyName
                }
            }
        };

        static BackupCopier()
        {
            TraceConfig.InitializeFromConfigStore();
            BackupCopier.TraceSource = new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.BackupCopier);
            BackupRestoreUtility.TraceSource = BackupCopier.TraceSource;
        }

        /// return 0 if program completes successfully, -1 otherwise.
        public static int Main(string[] args)
        {

#if !DotNetCoreClr
            ServicePointManager.SecurityProtocol = SecurityProtocolType.Tls | SecurityProtocolType.Tls11 | SecurityProtocolType.Tls12;
#endif

            if (args.Length < 1)
            {
                Console.WriteLine("This is for internal use only");
                return -1;
            }

            /* BackupCopier arguments for UploadBackup to Azure BlobStore */
            // /operation:AzureBlobStoreUpload - required
            // /connectionString:<Connectionstring> - required
            // /isConnectionStringEncrypted:<true|false> - required
            // /containerName:<Name of the Azure Blob Container> - required
            // /backupStoreBaseFolderPath:<Base folder under given container> - required
            // /sourceFileOrFolderPath:<Full path to source file or folder which needs to be uploaded> - required
            // /backupMetadataFilePath:<Full path to source backup metadata file which needs to be uploaded > -required
            // /targetFolderPath:<path to target folder where above file/folder needs to be uploaded> - required
            // /timeout:<ticks> - optional
            // /workingDir:<DirectoryPath> - optional
            // /errorDetailsFile:<ErrorDetailsFilePath> - optional

            /* BackupCopier arguments for DownloadBackup from Azure BlobStore */
            // /operation:AzureBlobStoreload - required
            // /connectionString:<Connectionstring> - required
            // /isConnectionStringEncrypted:<true|false> - required
            // /containerName:<Name of the Azure Blob Container> - required
            // /backupStoreBaseFolderPath:<Base folder under given container> - required
            // /sourceFileOrFolderPath:<Full path to source file or folder which needs to be downloaded> - required
            // /targetFolderPath:<path to target folder where above file/folder needs to be downloaded> - required
            // /timeout:<ticks> - optional
            // /workingDir:<DirectoryPath> - optional
            // /errorDetailsFile:<ErrorDetailsFilePath> - optional

            /* BackupCopier arguments for UploadBackup to File Share */
            // /operation:FileShareUpload - required
            // /fileShareAccessType:<DomainUser|None> - required
            // /fileSharePath:<File share path> - required
            // /isPasswordEncrypted:<true|false> - required
            // /primaryUserName:<Primary user Name for FileShare access> - optional; primaryUserName & primaryPassword both should be either present or absent together.
            // /primaryPassword:<Password for Primary user Name for FileShare access> - optional; primaryUserName & primaryPassword both should be either present or absent together.
            // /secondaryUserName:<Secondary user Name for FileShare access> - optional; secondaryUserName & secondaryPassword both should be either present or absent together.
            // /secondaryPassword:<Password for Secondary user Name for FileShare access> - optional; secondaryUserName & secondaryPassword both should be either present or absent together.
            // /sourceFileOrFolderPath:<Full path to source file or folder which needs to be uploaded> - required
            // /backupMetadataFilePath:<Full path to source backup metadata file which needs to be uploaded > -required
            // /targetFolderPath:<path to target folder where above file/folder needs to be uploaded> - required
            // /timeout:<ticks> - optional
            // /workingDir:<DirectoryPath> - optional
            // /errorDetailsFile:<ErrorDetailsFilePath> - optional

            /* BackupCopier arguments for DownloadBackup from File Share */
            // /operation:FileShareDownload - required
            // /fileShareAccessType:<DomainUser|None> - required
            // /fileSharePath:<File share path> - required
            // /isPasswordEncrypted:<true|false> - required
            // /primaryUserName:<Primary user Name for FileShare access> - optional; primaryUserName & primaryPassword both should be either present or absent together.
            // /primaryPassword:<Password for Primary user Name for FileShare access> - optional; primaryUserName & primaryPassword both should be either present or absent together.
            // /secondaryUserName:<Secondary user Name for FileShare access> - optional; secondaryUserName & secondaryPassword both should be either present or absent together.
            // /secondaryPassword:<Password for Secondary user Name for FileShare access> - optional; secondaryUserName & secondaryPassword both should be either present or absent together.
            // /sourceFileOrFolderPath:<Full path to source file or folder which needs to be downloaded> - required
            // /targetFolderPath:<path to target folder where above file/folder needs to be downloaded> - required
            // /timeout:<ticks> - optional
            // /workingDir:<DirectoryPath> - optional
            // /errorDetailsFile:<ErrorDetailsFilePath> - optional

            Dictionary<string, string> commandArgs = null;
            Exception exception = null;
            string errorDetailsFile = null;

            try
            {

                Dictionary<string, string> applicationParameters = null;

                commandArgs = ParseAndValidateParameters(
                    args,
                    out applicationParameters);

                errorDetailsFile = commandArgs.ContainsKey(StringConstants.ErrorDetailsFile)
                    ? commandArgs[StringConstants.ErrorDetailsFile]
                    : null;

                string workingDirectory = commandArgs.ContainsKey(StringConstants.WorkingDir)
                    ? commandArgs[StringConstants.WorkingDir]
                    : Directory.GetCurrentDirectory();

                string progressFile = commandArgs.ContainsKey(StringConstants.ProgressFile) ? commandArgs[StringConstants.ProgressFile] : null;

                StringBuilder stringToTrace = new StringBuilder();
                foreach (var commandArg in commandArgs)
                {
                    // Skipping tracing credentials.
                    if (CommandArgsForCredentials.Contains(commandArg.Key))
                    {
                        stringToTrace.AppendFormat("{0}:{1}", commandArg.Key, "***");
                        stringToTrace.AppendLine();
                    }
                    else
                    {
                        stringToTrace.AppendFormat("{0}:{1}", commandArg.Key, commandArg.Value);
                        stringToTrace.AppendLine();
                    }
                }

                BackupCopier.TraceSource.WriteInfo(
                    BackupCopier.TraceType,
                    "BackupCopier called with - {0}",
                    stringToTrace.ToString());

                TimeSpan timeout = commandArgs.ContainsKey(StringConstants.Timeout)
                    ? TimeSpan.FromTicks(long.Parse(commandArgs[StringConstants.Timeout], CultureInfo.InvariantCulture))
                    : TimeSpan.MaxValue;

                Timer timer = null;
                if (timeout != TimeSpan.MaxValue)
                {
                    BackupCopier.TraceSource.WriteInfo(
                        BackupCopier.TraceType,
                        "BackupCopier enabled timeout monitor: {0}",
                        timeout);

                    timer = new Timer(OnTimeout, errorDetailsFile, timeout, TimeSpan.FromMilliseconds(-1));
                }

                string operationValue = commandArgs[StringConstants.OperationKeyName];

                IBackupStoreManager backupStoreManager;
                bool isUpload = true;

                switch(operationValue)
                {
                    case StringConstants.AzureBlobStoreUpload:
                        {
                            backupStoreManager = ConstuctAzureBlobBackupStoreManager(commandArgs);
                            isUpload = true;
                        }
                        break;

                    case StringConstants.AzureBlobStoreDownload:
                        {

                            backupStoreManager = ConstuctAzureBlobBackupStoreManager(commandArgs);
                            isUpload = false;
                        }
                        break;

                    case StringConstants.DsmsAzureBlobStoreUpload:
                        {
                            backupStoreManager = ConstuctDsmsAzureBlobBackupStoreManager(commandArgs);
                            isUpload = true;
                        }
                        break;

                    case StringConstants.DsmsAzureBlobStoreDownload:
                        {

                            backupStoreManager = ConstuctDsmsAzureBlobBackupStoreManager(commandArgs);
                            isUpload = false;
                        }
                        break;

                    case StringConstants.FileShareUpload:
                        {
                            backupStoreManager = ConstuctFileShareBackupStoreManager(commandArgs);
                            isUpload = true;
                        }
                        break;

                    case StringConstants.FileShareDownload:
                        {
                            backupStoreManager = ConstuctFileShareBackupStoreManager(commandArgs);
                            isUpload = false;
                        }
                        break;
                    default:
                        throw new ArgumentException(string.Format(CultureInfo.InvariantCulture, "Argument {0} with value {1} is not supported", StringConstants.OperationKeyName, operationValue));
                }

                if(isUpload)
                {
                    long totalFileSize = BackupCopier.GetSize(commandArgs[StringConstants.SourceFileOrFolderPathKeyName]) +
                        BackupCopier.GetSize(commandArgs[StringConstants.BackupMetadataFilePathKeyName]);

                    Stopwatch stopwatch = new Stopwatch();

                    stopwatch.Restart();
                    // Upload backup file or folder.
                    backupStoreManager.Upload(
                        commandArgs[StringConstants.SourceFileOrFolderPathKeyName],
                        commandArgs[StringConstants.TargetFolderPathKeyName],
                        !File.GetAttributes(commandArgs[StringConstants.SourceFileOrFolderPathKeyName]).HasFlag(FileAttributes.Directory));

                    // Upload metadata file.
                    backupStoreManager.Upload(
                        commandArgs[StringConstants.BackupMetadataFilePathKeyName],
                        null,
                        true);
                    stopwatch.Stop();

                    BackupCopier.TraceSource.WriteInfo(
                        BackupCopier.TelemetryTraceType,
                        "{0}, {1}, {2}, {3}, {4}, {5}, {6}",
                        commandArgs[StringConstants.SourceFileOrFolderPathKeyName],
                        commandArgs[StringConstants.BackupMetadataFilePathKeyName],
                        commandArgs.ContainsKey(StringConstants.FileSharePathKeyName) ? commandArgs[StringConstants.FileSharePathKeyName] : String.Empty,
                        commandArgs.ContainsKey(StringConstants.TargetFolderPathKeyName) ? commandArgs[StringConstants.TargetFolderPathKeyName] : String.Empty,
                        commandArgs.ContainsKey(StringConstants.BackupStoreBaseFolderPathKeyName) ? commandArgs[StringConstants.BackupStoreBaseFolderPathKeyName] : String.Empty,
                        totalFileSize,
                        stopwatch.Elapsed.TotalMilliseconds);
                }
                else
                {
                    backupStoreManager.Download(commandArgs[StringConstants.SourceFileOrFolderPathKeyName],
                        commandArgs[StringConstants.TargetFolderPathKeyName],
                        commandArgs[StringConstants.SourceFileOrFolderPathKeyName].EndsWith(StringConstants.ZipExtension));
                }
            }
            catch (AggregateException ae)
            {
                exception = ae.InnerException;

                StringBuilder exceptionStringBuilder = new StringBuilder("Aggregate exception has ");
                exceptionStringBuilder.Append(ae.InnerExceptions.Count);
                exceptionStringBuilder.Append(" exceptions. ");
                ae.InnerExceptions.ForEach(innerException => exceptionStringBuilder.AppendLine(innerException.Message));

                BackupCopier.TraceSource.WriteWarning(
                    BackupCopier.TraceType,
                    "AggregateException: {0}",
                    exceptionStringBuilder.ToString());
            }
            catch (Exception e)
            {
                exception = e;
            }

            if (exception != null)
            {
                OnFailure(exception, errorDetailsFile);

                return unchecked((int)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_BACKUPCOPIER_UNEXPECTED_ERROR);
            }
            else
            {
                BackupCopier.TraceSource.WriteInfo(
                    BackupCopier.TraceType,
                    "BackupCopier operation completed successfully.");

                return 0;
            }
        }

        private static IBackupStoreManager ConstuctAzureBlobBackupStoreManager(Dictionary<string, string> commandArgs)
        {
            AzureBlobBackupStore azureBlobBackupStore = new AzureBlobBackupStore();
            azureBlobBackupStore.ConnectionString = commandArgs[StringConstants.ConnectionStringKeyName];
            azureBlobBackupStore.ContainerName = commandArgs[StringConstants.ContainerNameKeyName];
            azureBlobBackupStore.FolderPath = commandArgs[StringConstants.BackupStoreBaseFolderPathKeyName];
            azureBlobBackupStore.IsAccountKeyEncrypted = string.Compare(commandArgs[StringConstants.IsConnectionStringEncryptedKeyName], "true", true) == 0 ? true : false;

            if (string.Compare(commandArgs[StringConstants.IsConnectionStringEncryptedKeyName], StringConstants.BooleanStringValue_True, true) == 0)
            {
                azureBlobBackupStore.IsAccountKeyEncrypted = true;
            }
            else if (string.Compare(commandArgs[StringConstants.IsConnectionStringEncryptedKeyName], StringConstants.BooleanStringValue_False, true) == 0)
            {
                azureBlobBackupStore.IsAccountKeyEncrypted = false;
            }
            else
            {
                throw new ArgumentException(string.Format(CultureInfo.InvariantCulture, "Argument {0} with value {1} is not supported", StringConstants.IsConnectionStringEncryptedKeyName, commandArgs[StringConstants.IsConnectionStringEncryptedKeyName]));
            }

            return new AzureBlobBackupStoreManager(azureBlobBackupStore);
        }

        private static IBackupStoreManager ConstuctDsmsAzureBlobBackupStoreManager(Dictionary<string, string> commandArgs)
        {
            DsmsAzureBlobBackupStore dsmsAzureBlobBackupStore = new DsmsAzureBlobBackupStore();
            dsmsAzureBlobBackupStore.StorageCredentialsSourceLocation = commandArgs[StringConstants.StorageCredentialsSourceLocationKeyName];
            dsmsAzureBlobBackupStore.ContainerName = commandArgs[StringConstants.ContainerNameKeyName];
            dsmsAzureBlobBackupStore.FolderPath = commandArgs[StringConstants.BackupStoreBaseFolderPathKeyName];

            return new DsmsAzureBlobBackupStoreManager(dsmsAzureBlobBackupStore);
        }

        private static IBackupStoreManager ConstuctFileShareBackupStoreManager(Dictionary<string, string> commandArgs)
        {
            FileShareBackupStore fileShareBackupStore = new FileShareBackupStore();
            
            if (commandArgs.ContainsKey(StringConstants.FileShareAccessTypeKeyName))
            {
                switch(commandArgs[StringConstants.FileShareAccessTypeKeyName])
                {
                    case StringConstants.FileShareAccessTypeValue_DomainUser:
                        fileShareBackupStore.AccessType = FileShareAccessType.DomainUser;
                        break;

                    case StringConstants.FileShareAccessTypeValue_None:
                        fileShareBackupStore.AccessType = FileShareAccessType.None;
                        break;

                    default:
                        throw new ArgumentException(string.Format(CultureInfo.InvariantCulture, "Argument {0} with value {1} is not supported", StringConstants.FileShareAccessTypeKeyName, commandArgs[StringConstants.FileShareAccessTypeKeyName]));
                }
            }
            else
            {
                fileShareBackupStore.AccessType = FileShareAccessType.None;
            }

            fileShareBackupStore.FileSharePath = commandArgs[StringConstants.FileSharePathKeyName];

            if (commandArgs.ContainsKey(StringConstants.PrimaryUserNameKeyName) != commandArgs.ContainsKey(StringConstants.PrimaryPasswordKeyName))
            {
                throw new ArgumentException(string.Format(CultureInfo.InvariantCulture, "Arguments {0} & {1} should be present in pair", StringConstants.PrimaryUserNameKeyName, commandArgs[StringConstants.PrimaryPasswordKeyName]));
            }

            if (commandArgs.ContainsKey(StringConstants.PrimaryUserNameKeyName) && commandArgs.ContainsKey(StringConstants.PrimaryPasswordKeyName))
            {
                // Ensure that IsPasswordEncrypted Argument is present.
                if (!commandArgs.ContainsKey(StringConstants.IsPasswordEncryptedKeyName))
                {
                    throw new ArgumentException(string.Format(CultureInfo.InvariantCulture, "Argument {0} is mandatory when passwords are specified.", StringConstants.IsPasswordEncryptedKeyName, commandArgs[StringConstants.IsPasswordEncryptedKeyName]));
                }
                else
                {
                    if (string.Compare(commandArgs[StringConstants.IsPasswordEncryptedKeyName], StringConstants.BooleanStringValue_True, true) == 0)
                    {
                        fileShareBackupStore.IsPasswordEncrypted = true;
                    }
                    else if (string.Compare(commandArgs[StringConstants.IsPasswordEncryptedKeyName], StringConstants.BooleanStringValue_False, true) == 0)
                    {
                        fileShareBackupStore.IsPasswordEncrypted = false;
                    }
                    else
                    {
                        throw new ArgumentException(string.Format(CultureInfo.InvariantCulture, "Argument {0} with value {1} is not supported", StringConstants.IsPasswordEncryptedKeyName, commandArgs[StringConstants.IsPasswordEncryptedKeyName]));
                    }
                }

                fileShareBackupStore.PrimaryUserName = commandArgs[StringConstants.PrimaryUserNameKeyName];
                fileShareBackupStore.PrimaryPassword = commandArgs[StringConstants.PrimaryPasswordKeyName];
            }

            if (commandArgs.ContainsKey(StringConstants.SecondaryUserNameKeyName) != commandArgs.ContainsKey(StringConstants.SecondaryPasswordKeyName))
            {
                throw new ArgumentException(string.Format(CultureInfo.InvariantCulture, "Arguments {0} & {1} should be present in pair", StringConstants.SecondaryUserNameKeyName, commandArgs[StringConstants.SecondaryPasswordKeyName]));
            }

            if (commandArgs.ContainsKey(StringConstants.SecondaryUserNameKeyName))
            {
                if (commandArgs.ContainsKey(StringConstants.PrimaryUserNameKeyName))
                {
                    throw new ArgumentException(string.Format(CultureInfo.InvariantCulture, "Primary user credentials are mandatory before specifying secondary user credentials."));
                }

                fileShareBackupStore.SecondaryUserName = commandArgs[StringConstants.SecondaryUserNameKeyName];
                fileShareBackupStore.SecondaryPassword = commandArgs[StringConstants.SecondaryPasswordKeyName];
            }

            return new FileShareBackupStoreManager(fileShareBackupStore);
        }

        private static bool Equals(string string1, string string2)
        {
            return string.Equals(string1, string2, StringComparison.OrdinalIgnoreCase);
        }

        private static Dictionary<string, string> ParseAndValidateParameters(
            string[] args,
            out Dictionary<string, string> applicationParameters)
        {
            Dictionary<string, string> commandArgs = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            applicationParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);

            foreach (string arg in args)
            {
                int delimiterIndex = arg.IndexOf(":", StringComparison.OrdinalIgnoreCase);

                if (delimiterIndex == -1)
                {
                    throw new ArgumentException(string.Format(CultureInfo.InvariantCulture, "The argument {0} is invalid.", arg));
                }
                else
                {
                    string key = arg.Substring(0, delimiterIndex);
                    string value = arg.Substring(delimiterIndex + 1);

                    if (!SupportedCommandArgs.Contains(key))
                    {
                        throw new ArgumentException(string.Format(CultureInfo.InvariantCulture, "The argument {0} is not expected.", key));
                    }

                    if (commandArgs.ContainsKey(key))
                    {
                        throw new ArgumentException(string.Format(CultureInfo.InvariantCulture, "The command line argument {0} is found more than once.", key));
                    }

                    if (CommandArgsWithEncodedValues.Contains(key))
                    {
                        value = DecodeArgumentValue(value);
                    }

                    commandArgs.Add(key, value);
                }
            }

            // Ensure that required parameters are present
            EnsureRequiredCommandLineParameters(commandArgs);

            return commandArgs;
        }

        public static void EnsureRequiredCommandLineParameters(Dictionary<string, string> commandArgs)
        {
            HashSet<string> requiredCommandParameters = new HashSet<string>(StringComparer.OrdinalIgnoreCase) { StringConstants.OperationKeyName };
            foreach (string requiredKey in requiredCommandParameters)
            {
                if (!commandArgs.ContainsKey(requiredKey))
                {
                    throw new ArgumentException(string.Format(CultureInfo.InvariantCulture, "The required command line argument {0} is not found.", requiredKey));
                }
            }

            string operationValue = commandArgs[StringConstants.OperationKeyName];

            if (BackupCopier.OperationSpecificRequiredParameters.ContainsKey(operationValue))
            {
                string[] requiredParametsrsForOperation = BackupCopier.OperationSpecificRequiredParameters[operationValue];
                CheckRequiredCommandLineArguments(requiredParametsrsForOperation, operationValue, commandArgs);
            }
        }

        private static void CheckRequiredCommandLineArguments(string[] requiredArguments, string operationValue, Dictionary<string, string> commandArgs)
        {
            foreach (string requiredArgument in requiredArguments)
            {
                if (!commandArgs.ContainsKey(requiredArgument))
                {
                    throw new ArgumentException(string.Format(CultureInfo.InvariantCulture, "The required command line argument {0} for operation {1} is not found.", requiredArgument, operationValue));
                }
            }
        }

        private static string DecodeArgumentValue(string encodedArgumentValue)
        {
            // First decode using base64 decoder.
            byte[] utf8Bytes = System.Convert.FromBase64String(encodedArgumentValue);
            return System.Text.Encoding.UTF8.GetString(utf8Bytes);
        }

        private static int GetFabricErrorCodeOrEFail(Exception e)
        {
            int errorCode = 0; 

            if (!BackupCopier.ManagedToNativeExceptionTranslation.TryGetValue(e.GetType(), out errorCode))
            {
                FabricException fabricException = e as FabricException;
                if (fabricException != null)
                {
                    errorCode = (int) fabricException.ErrorCode;
                }
                else
                {
                    // TryGetValue() sets the out parameter to default(T) if the entry is not found
                    //
                    errorCode = unchecked((int)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_BACKUPCOPIER_UNEXPECTED_ERROR);
                }
            }

            return errorCode;
        }

        private static void OnTimeout(object state)
        {
            OnFailure(new TimeoutException("BackupCopier self-terminating on timeout"), state as string);

            try
            {
                Environment.Exit(unchecked((int)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_BACKUPCOPIER_TIMEOUT));
            }
            catch (Exception e)
            {
                BackupCopier.TraceSource.WriteError(
                    BackupCopier.TraceType,
                    "BackupCopier failed to exit cleanly: {0}", e);

                throw;
            }
        }

        private static void OnFailure(Exception exception, string errorDetailsFile)
        {
            BackupCopier.TraceSource.WriteWarning(
                BackupCopier.TraceType,
                "BackupCopier operation failed with the following exception: {0}",
                exception);

            if (!string.IsNullOrEmpty(errorDetailsFile))
            {
                try
                {
                    var errorDetails = new StringBuilder(exception.Message);
                    var inner = exception.InnerException;
                    while (inner != null)
                    {
                        errorDetails.AppendFormat(" --> {0}: {1}", inner.GetType().Name, inner.Message);
                        inner = inner.InnerException;
                    }

                    BackupCopier.WriteStringToFile(
                        errorDetailsFile,
                        BackupCopier.GetFabricErrorCodeOrEFail(exception) + "," + errorDetails.ToString(),
                        false,
                        Encoding.Unicode);
                }
                catch (Exception innerEx)
                {
                    BackupCopier.TraceSource.WriteWarning(
                        BackupCopier.TraceType,
                        "Failed to write error details: {0}",
                        innerEx.ToString());
                }
            }
        }

        private static void WriteStringToFile(string fileName, string value, bool writeLine = true, Encoding encoding = null)
        {
            var directory = FabricPath.GetDirectoryName(fileName);

            if (!FabricDirectory.Exists(directory))
            {
                FabricDirectory.CreateDirectory(directory);
            }

            if (encoding == null)
            {
#if DotNetCoreClrLinux
                encoding = new UTF8Encoding(false);
#else
                encoding = Encoding.GetEncoding(0);
#endif
            }

            using (StreamWriter writer = new StreamWriter(FabricFile.Open(fileName, FileMode.OpenOrCreate), encoding))
            {
                if (writeLine)
                {
                    writer.WriteLine(value);
                }
                else
                {
                    writer.Write(value);
                }
#if DotNetCoreClrLinux
                Helpers.UpdateFilePermission(fileName);
#endif
            }
        }

        private static long GetSize(string path)
        {
            long size = 0;
            try
            {
                if (File.GetAttributes(path).HasFlag(FileAttributes.Directory))
                {
                    DirectoryInfo d = new DirectoryInfo(path);
                    size = DirSize(d);
                }
                else
                {
                    FileInfo f = new FileInfo(path);
                    size = f.Length;
                }
            }
            catch(Exception ex)
            {
                // Log exception and continue.
                BackupCopier.TraceSource.WriteWarning(
                        BackupCopier.TraceType,
                        "Failed to calculate file/directory size of: {0}. Exception: {1}",
                        path,
                        ex);
            }

            return size;
        }

        private static long DirSize(DirectoryInfo d)
        {
            long size = 0;

            // Add file sizes.
            FileInfo[] fis = d.GetFiles();
            foreach (FileInfo fi in fis)
            {
                size += fi.Length;
            }
            // Add subdirectory sizes.
            DirectoryInfo[] dis = d.GetDirectories();
            foreach (DirectoryInfo di in dis)
            {
                size += DirSize(di);
            }

            return size;
        }
    }
}