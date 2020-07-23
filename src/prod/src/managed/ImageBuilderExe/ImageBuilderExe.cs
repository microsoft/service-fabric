// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.ImageBuilderExe
{
    using Microsoft.Win32;
    using Newtonsoft.Json;
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.ImageStore;
    using System.Fabric.Interop;
    using System.Fabric.Management.ImageBuilder;
    using System.Fabric.Management.ImageBuilder.SingleInstance;
    using System.Fabric.Management.ImageStore;
    using System.Fabric.Management.WindowsFabricValidator;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.IO;
    using System.Net;
    using System.Text;
    using System.Threading;

    internal class ImageBuilderExe
    {
        internal static FabricEvents.ExtensionsEvents TraceSource;

        private static readonly string TraceType = "ImageBuilderExe";

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
            StringConstants.SchemaPath,
            StringConstants.WorkingDir,
            StringConstants.StoreRoot,
            StringConstants.Operation,
            StringConstants.AppTypeName,
            StringConstants.AppTypeVersion,
            StringConstants.AppId,
            StringConstants.NameUri,
            StringConstants.AppParam,
            StringConstants.BuildPath,
            StringConstants.DownloadPath,
            StringConstants.Output,
            StringConstants.Input,
            StringConstants.Progress,
            StringConstants.ErrorDetails,
            StringConstants.SecuritySettingsParam,
            StringConstants.CurrentAppInstanceVersion,
            StringConstants.CodePath,
            StringConstants.ConfigPath,
            StringConstants.CurrentFabricVersion,
            StringConstants.TargetFabricVersion,
            StringConstants.InfrastructureManifestFile,
            StringConstants.Timeout,
            StringConstants.DisableChecksumValidation,
            StringConstants.DisableServerSideCopy,
            StringConstants.ConfigVersion,
            StringConstants.ComposeFilePath,
            StringConstants.OverrideFilePath,
            StringConstants.RepositoryPassword,
            StringConstants.RepositoryUserName,
            StringConstants.OutputComposeFilePath,
            StringConstants.IsRepositoryPasswordEncrypted,
            StringConstants.CleanupComposeFiles,
            StringConstants.GenerateDnsNames,
            StringConstants.SFVolumeDiskServiceEnabled,
            StringConstants.AppName,
            StringConstants.CurrentAppTypeVersion,
            StringConstants.TargetAppTypeVersion,
            StringConstants.SingleInstanceApplicationDescription,
            StringConstants.UseOpenNetworkConfig,
            StringConstants.UseLocalNatNetworkConfig,
            StringConstants.GenerationConfig,
            StringConstants.MountPointForSettings,
        };

        static ImageBuilderExe()
        {
            TraceConfig.InitializeFromConfigStore();
            ImageBuilderExe.TraceSource = new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.ImageBuilder);

#if !DotNetCoreClr
            ServicePointManager.SecurityProtocol = SecurityProtocolType.Tls | SecurityProtocolType.Tls11 | SecurityProtocolType.Tls12;
#endif
        }

        /// return 0 if program completes successfully, -1 otherwise.
        public static int Main(string[] args)
        {
            if (args.Length < 1)
            {
                Console.WriteLine("This is for internal use only");
                return -1;
            }

#if DEBUG
            // To add a sleep time while debugging add a "Sleep" key (in milliseconds) to the following registry location:  HKLM\\Software\\Microsoft\\Service Fabric
            try
            {
                using (RegistryKey registryKey = Registry.LocalMachine.OpenSubKey(FabricConstants.FabricRegistryKeyPath, true))
                {
                    var sleepRegVal = registryKey.GetValue("Sleep");

                    if (sleepRegVal != null)
                    {
                        int sleepinms = int.Parse(sleepRegVal.ToString());
                        ImageBuilderExe.TraceSource.WriteNoise(ImageBuilderExe.TraceType, "ImageBuilderExe: Sleep time requested: {0}", sleepinms.ToString());

                        Thread.Sleep(sleepinms);
                    }
                }
            }
            catch (Exception e)
            {
                ImageBuilderExe.TraceSource.WriteNoise(ImageBuilderExe.TraceType, "ImageBuilderExe: Exception code:{0}, msg:{1} when attempting to read registry value \"Sleep\" in {2}.", e.HResult, e.Message, FabricConstants.FabricRegistryKeyPath);
            }
#endif

            // Parameters
            // /schemaPath:<The xsd schema file of Windows Fabric ServiceModel> - optional. Default: <Current Working Directory>\ServiceFabricServiceModel.xsd
            // /workingDir:<The root of working directory of the node> - optional. Default: <Current Working Directory>
            // /storeRoot:<The root path of ImageStore> - optional
            // /output: <The root folder|file where output will be placed> - required
            // /input: <The root folder|file where input will be read from> - required for Delete
            // /progress: <The file where operation progress will be read from> - optional. Used by BuildApplicationType and DownloadAndBuildApplicationType.
            // /operation:<BuildApplicationTypeInfo | BuildApplicationType | BuildApplication | BuildComposeDeployment | ValidateComposeDeployment | UpgradeApplication | GetFabricVersion | ProvisionFabric | UpgradeFabric | Delete | TestErrorDetails | DownloadAndBuildApplicationType> - required
            // /timeout:<ticks> - optional

            /* ImageBuilder arguments for DownloadAndBuildApplicationType */
            // /downloadPath: <path to .sfpkg file from a store that supports GET method> - required
            // /appTypeName:<Application Type Name> - required. Expected application type name in the downloaded package.
            // /appTypeVersion:<Application Type Version> - required. Expected application type version in the downloaded package.
            // /progress: <The file where operation progress will be read from> - optional.
            // /output: <The root folder|file where output will be placed> - required
            // /timeout:<ticks> - optional

            /* ImageBuilder arguments for BuildComposeDeployment */
            // /cf:<Compose File path> - required
            // /of:<Overrides File path> - optional
            // /appTypeName:<Application Type Name> - required
            // /appTypeVersion:<Application Type Version> - required
            // /output: <The root folder|file where output application package will be placed> - required
            // /ocf:<Output Merged compose file path> - required
            // /repoUserName:<Repository User Name> - optional
            // /repoPwd:<Repository Password> - optional
            // /pwdEncrypted:<Is Repository Password encrypted> - optional
            // /cleanup:<Cleanup the compose files> - optional
            // /timeout:<ticks> - optional

            /* ImageBuilder arguments for ValidateComposeDeployment */
            // /cf:<Compose File path> - required
            // /of:<Overrides File path> - optional
            // /appTypeName:<Application Type Name> - optional
            // /appTypeVersion:<Application Type Version> - optional
            // /cleanup:<Cleanup the compose files> - optional
            // /timeout:<ticks> - optional

            /* ImageBuilder arguments for Application */
            // /appTypeName:<Application Type Name> - required for BuildApplication and UpgradeApplication operation
            // /appTypeVersion:<Application Type Version> - required for BuildApplication and UpgradeApplication operation
            // /appId:<Application ID> - required for BuildApplication and UpgradeApplication operation
            // /nameUri:<Name URI> - required for BuildApplication operation
            // /appParam:<semi-colon separated key-value pair>. Multiple such parameters can be passed. Key cannot have ';'.
            // /buildPath: <The root folder of build layout> - required for ApplicationTypeInfo and BuildApplicationType operation
            // /currentAppInstanceVersion: <The current app instance of the Application that needs to be upgraded> - required for UpgradeApplication operation

            /* ImageBuilder arguments for Fabric Upgrade */
            // /codePath: <The path to MSP for FabricUpgrade>
            // /configPath: <The path to ClusterManifest for FabricUpgrade>
            // /currentFabricVersion: <The current FabricVersion>
            // /targetFabricVersion: <The target FabricVersion>
            // /im: <Path to InfrastructureManifest.xml file>
            //

            Dictionary<string, string> commandArgs = null;
            Exception exception = null;
            string errorDetailsFile = null;

            try
            {
                commandArgs = ParseParameters(args);

                // Ensure that required parameters are present
                EnsureRequiredCommandLineParameters(commandArgs);

                Dictionary<string, string> applicationParameters = ParseAppParameters(commandArgs);

                errorDetailsFile = commandArgs.ContainsKey(StringConstants.ErrorDetails)
                    ? commandArgs[StringConstants.ErrorDetails]
                    : null;

                string workingDirectory = commandArgs.ContainsKey(StringConstants.WorkingDir)
                    ? commandArgs[StringConstants.WorkingDir]
                    : Directory.GetCurrentDirectory();

                string imageStoreConnectionString;
                if (commandArgs.ContainsKey(StringConstants.StoreRoot))
                {
                    imageStoreConnectionString = commandArgs[StringConstants.StoreRoot];
                }
                else
                {
                    ImageBuilderExe.TraceSource.WriteInfo(ImageBuilderExe.TraceType, "Loading ImageStoreConnectionString from config.");

                    bool isEncrypted;
                    NativeConfigStore configStore = NativeConfigStore.FabricGetConfigStore();
                    var configValue = configStore.ReadString("Management", "ImageStoreConnectionString", out isEncrypted);
                    if (isEncrypted)
                    {
                        var secureString = NativeConfigStore.DecryptText(configValue);
                        var secureCharArray = FabricValidatorUtility.SecureStringToCharArray(secureString);
                        imageStoreConnectionString = new string(secureCharArray);
                    }
                    else
                    {
                        imageStoreConnectionString = configValue;
                    }

                    if (string.IsNullOrEmpty(imageStoreConnectionString))
                    {
                        throw new ArgumentException(StringResources.Error_MissingImageStoreConnectionStringInManifest);
                    }
                }

                StringBuilder stringToTrace = new StringBuilder();
                foreach (var commandArg in commandArgs)
                {
                    // Skipping tracing StoreRoot since it could be a secret value
                    if (!ImageBuilderUtility.Equals(commandArg.Key, StringConstants.StoreRoot))
                    {
                        stringToTrace.AppendFormat("{0}:{1}", commandArg.Key, commandArg.Value);
                        stringToTrace.AppendLine();
                    }
                }

                ImageBuilderExe.TraceSource.WriteInfo(
                    ImageBuilderExe.TraceType,
                    "ImageBuilderExe called with - {0}",
                    stringToTrace.ToString());

                string currentExecutingDirectory = Path.GetDirectoryName(System.Diagnostics.Process.GetCurrentProcess().MainModule.FileName);
                string schemaPath = commandArgs.ContainsKey(StringConstants.SchemaPath)
                    ? commandArgs[StringConstants.SchemaPath]
                    : Path.Combine(
                        currentExecutingDirectory,
                        StringConstants.DefaultSchemaPath);

                TimeSpan timeout = commandArgs.ContainsKey(StringConstants.Timeout)
                    ? TimeSpan.FromTicks(long.Parse(commandArgs[StringConstants.Timeout]))
                    : TimeSpan.MaxValue;

                Timer timer = null;
                if (timeout != TimeSpan.MaxValue)
                {
                    ImageBuilderExe.TraceSource.WriteInfo(
                        ImageBuilderExe.TraceType,
                        "ImageBuilderExe enabled timeout monitor: {0}",
                        timeout);

                    timer = new Timer(OnTimeout, errorDetailsFile, timeout, TimeSpan.FromMilliseconds(-1));
                }

                IImageStore imageStore = ImageStoreFactoryProxy.CreateImageStore(
                    imageStoreConnectionString,
                    null,
                    workingDirectory,
                    true /*isInternal*/);

                ImageBuilder imageBuilder = new ImageBuilder(imageStore, schemaPath, workingDirectory);

                string operationValue = commandArgs[StringConstants.Operation];

                bool sfVolumeDiskServiceEnabled = commandArgs.ContainsKey(StringConstants.SFVolumeDiskServiceEnabled)
                        ? ImageBuilderUtility.ConvertString<bool>(commandArgs[StringConstants.SFVolumeDiskServiceEnabled])
                        : false;

                imageBuilder.IsSFVolumeDiskServiceEnabled = sfVolumeDiskServiceEnabled;

                if (ImageBuilderUtility.Equals(operationValue, StringConstants.OperationDownloadAndBuildApplicationType))
                {
                    string progressFile = commandArgs.ContainsKey(StringConstants.Progress) ? commandArgs[StringConstants.Progress] : null;
                    bool shouldSkipChecksumValidation = commandArgs.ContainsKey(StringConstants.DisableChecksumValidation) ? ImageBuilderUtility.ConvertString<bool>(commandArgs[StringConstants.DisableChecksumValidation]) : false;

                    imageBuilder.DownloadAndBuildApplicationType(
                        commandArgs[StringConstants.DownloadPath],
                        commandArgs[StringConstants.AppTypeName],
                        commandArgs[StringConstants.AppTypeVersion],
                        commandArgs[StringConstants.Output],
                        timeout,
                        progressFile,
                        shouldSkipChecksumValidation);
                }
                else if (ImageBuilderUtility.Equals(operationValue, StringConstants.OperationBuildApplicationTypeInfo))
                {
                    imageBuilder.GetApplicationTypeInfo(commandArgs[StringConstants.BuildPath], timeout, commandArgs[StringConstants.Output]);
                }
                else if (ImageBuilderUtility.Equals(operationValue, StringConstants.OperationBuildApplicationType))
                {
                    string outputFile = commandArgs.ContainsKey(StringConstants.Output) ? commandArgs[StringConstants.Output] : null;
                    string progressFile = commandArgs.ContainsKey(StringConstants.Progress) ? commandArgs[StringConstants.Progress] : null;
                    bool shouldSkipChecksumValidation = commandArgs.ContainsKey(StringConstants.DisableChecksumValidation) ? ImageBuilderUtility.ConvertString<bool>(commandArgs[StringConstants.DisableChecksumValidation]) : false;
                    bool shouldSkipServerSideCopy = commandArgs.ContainsKey(StringConstants.DisableServerSideCopy) ? ImageBuilderUtility.ConvertString<bool>(commandArgs[StringConstants.DisableServerSideCopy]) : false;

                    imageBuilder.BuildApplicationType(commandArgs[StringConstants.BuildPath], timeout, outputFile, progressFile, shouldSkipChecksumValidation, shouldSkipServerSideCopy);
                }
                else if (ImageBuilderUtility.Equals(operationValue, StringConstants.OperationBuildSingleInstanceApplication))
                {
                    bool generateDnsNames = commandArgs.ContainsKey(StringConstants.GenerateDnsNames)
                        ? ImageBuilderUtility.ConvertString<bool>(commandArgs[StringConstants.GenerateDnsNames])
                        : false;

                    bool useOpenNetworkConfig = commandArgs.ContainsKey(StringConstants.UseOpenNetworkConfig)
                        ? ImageBuilderUtility.ConvertString<bool>(commandArgs[StringConstants.UseOpenNetworkConfig])
                        : false;

                    bool useLocalNatNetworkConfig = commandArgs.ContainsKey(StringConstants.UseLocalNatNetworkConfig)
                        ? ImageBuilderUtility.ConvertString<bool>(commandArgs[StringConstants.UseLocalNatNetworkConfig])
                        : false;

                    Application application = null;

                    using (StreamReader file = File.OpenText(commandArgs[StringConstants.SingleInstanceApplicationDescription]))
                    {
                        JsonSerializer serializer = new JsonSerializer();
                        serializer.Converters.Add(new DiagnosticsSinkJsonConverter());
                        serializer.Converters.Add(new VolumeMountJsonConverter());

                        application =
                            (Application)serializer.Deserialize(file, typeof(Application));
                    }

                    GenerationConfig config = null;
                    if (commandArgs.ContainsKey(StringConstants.GenerationConfig))
                    {
                        using (StreamReader file = File.OpenText(commandArgs[StringConstants.GenerationConfig]))
                        {
                            JsonSerializer serializer = new JsonSerializer();
                            config =
                                (GenerationConfig)serializer.Deserialize(file, typeof(GenerationConfig));
                        }
                    }

                    imageBuilder.BuildSingleInstanceApplication(
                        application,
                        commandArgs[StringConstants.AppTypeName],
                        commandArgs[StringConstants.AppTypeVersion],
                        commandArgs[StringConstants.AppId],
                        new Uri(commandArgs[StringConstants.AppName]),
                        generateDnsNames,
                        timeout,
                        commandArgs[StringConstants.BuildPath],
                        commandArgs[StringConstants.Output],
                        useOpenNetworkConfig,
                        useLocalNatNetworkConfig,
                        commandArgs[StringConstants.MountPointForSettings],
                        config);
                }
                else if (ImageBuilderUtility.Equals(operationValue, StringConstants.OperationBuildSingleInstanceApplicationForUpgrade))
                {
                    bool generateDnsNames = commandArgs.ContainsKey(StringConstants.GenerateDnsNames)
                        ? ImageBuilderUtility.ConvertString<bool>(commandArgs[StringConstants.GenerateDnsNames])
                        : false;

                    bool useOpenNetworkConfig = commandArgs.ContainsKey(StringConstants.UseOpenNetworkConfig)
                        ? ImageBuilderUtility.ConvertString<bool>(commandArgs[StringConstants.UseOpenNetworkConfig])
                        : false;

                    bool useLocalNatNetworkConfig = commandArgs.ContainsKey(StringConstants.UseLocalNatNetworkConfig)
                        ? ImageBuilderUtility.ConvertString<bool>(commandArgs[StringConstants.UseLocalNatNetworkConfig])
                        : false;

                    Application application = null;
                    
                    using (StreamReader file = File.OpenText(commandArgs[StringConstants.SingleInstanceApplicationDescription]))
                    {
                        JsonSerializer serializer = new JsonSerializer();
                        serializer.Converters.Add(new VolumeMountJsonConverter());
                        application =
                            (Application)serializer.Deserialize(file, typeof(Application));
                    }

                    GenerationConfig config = null;
                    if (commandArgs.ContainsKey(StringConstants.GenerationConfig))
                    {
                        using (StreamReader file = File.OpenText(commandArgs[StringConstants.GenerationConfig]))
                        {
                            JsonSerializer serializer = new JsonSerializer();
                            config =
                                (GenerationConfig)serializer.Deserialize(file, typeof(GenerationConfig));
                        }
                    }

                    imageBuilder.BuildSingleInstanceApplicationForUpgrade(
                        application,
                        commandArgs[StringConstants.AppTypeName],
                        commandArgs[StringConstants.CurrentAppTypeVersion],
                        commandArgs[StringConstants.TargetAppTypeVersion],
                        commandArgs[StringConstants.AppId],
                        int.Parse(commandArgs[StringConstants.CurrentAppInstanceVersion], CultureInfo.InvariantCulture),
                        new Uri(commandArgs[StringConstants.AppName]),
                        generateDnsNames,
                        timeout,
                        commandArgs[StringConstants.BuildPath],
                        commandArgs[StringConstants.Output],
                        useOpenNetworkConfig,
                        useLocalNatNetworkConfig,
                        commandArgs[StringConstants.MountPointForSettings],
                        config);

                }
                else if (ImageBuilderUtility.Equals(operationValue, StringConstants.OperationBuildApplication))
                {
                    imageBuilder.BuildApplication(
                        commandArgs[StringConstants.AppTypeName],
                        commandArgs[StringConstants.AppTypeVersion],
                        commandArgs[StringConstants.AppId],
                        new Uri(commandArgs[StringConstants.NameUri]),
                        applicationParameters,
                        timeout,
                        commandArgs[StringConstants.Output]);
                }
                else if (ImageBuilderUtility.Equals(operationValue, StringConstants.OperationUpgradeApplication))
                {
                    imageBuilder.UpgradeApplication(
                        commandArgs[StringConstants.AppId],
                        commandArgs[StringConstants.AppTypeName],
                        int.Parse(commandArgs[StringConstants.CurrentAppInstanceVersion], CultureInfo.InvariantCulture),
                        commandArgs[StringConstants.AppTypeVersion],
                        applicationParameters,
                        timeout,
                        commandArgs[StringConstants.Output]);
                }
                else if (ImageBuilderUtility.Equals(operationValue, StringConstants.OperationDelete))
                {
                    imageBuilder.Delete(commandArgs[StringConstants.Input], timeout);
                }
                else if (ImageBuilderUtility.Equals(operationValue, StringConstants.OperationCleanupApplicationPackage))
                {
                    imageBuilder.DeleteApplicationPackage(commandArgs[StringConstants.BuildPath], timeout);
                }
                else if (ImageBuilderUtility.Equals(operationValue, StringConstants.OperationGetFabricVersion))
                {
                    imageBuilder.GetFabricVersionInfo(
                        commandArgs[StringConstants.CodePath],
                        commandArgs[StringConstants.ConfigPath],
                        timeout,
                        commandArgs[StringConstants.Output]);
                }
                else if (ImageBuilderUtility.Equals(operationValue, StringConstants.OperationProvisionFabric))
                {
                    string configurationCsvFilePath = Path.Combine(currentExecutingDirectory, StringConstants.ConfigurationsFileName);
                    imageBuilder.ProvisionFabric(
                        commandArgs[StringConstants.CodePath],
                        commandArgs[StringConstants.ConfigPath],
                        configurationCsvFilePath,
                        commandArgs[StringConstants.InfrastructureManifestFile],
                        timeout);
                }
                else if (ImageBuilderUtility.Equals(operationValue, StringConstants.OperationGetClusterManifest))
                {
                    imageBuilder.GetClusterManifestContents(
                        commandArgs[StringConstants.ConfigVersion],
                        commandArgs[StringConstants.Output],
                        timeout);
                }
                else if (ImageBuilderUtility.Equals(operationValue, StringConstants.OperationUpgradeFabric))
                {
                    string configurationCsvFilePath = Path.Combine(currentExecutingDirectory, StringConstants.ConfigurationsFileName);

                    imageBuilder.UpgradeFabric(
                        commandArgs[StringConstants.CurrentFabricVersion],
                        commandArgs[StringConstants.TargetFabricVersion],
                        configurationCsvFilePath,
                        timeout,
                        commandArgs[StringConstants.Output]);
                }
                else if (ImageBuilderUtility.Equals(operationValue, StringConstants.OperationGetManifests))
                {
                    imageBuilder.GetManifests(commandArgs[StringConstants.Input], timeout);
                }
                else if (ImageBuilderUtility.Equals(operationValue, StringConstants.OperationValidateComposeDeployment))
                {
                    bool cleanupComposeFiles = commandArgs.ContainsKey(StringConstants.CleanupComposeFiles) && ImageBuilderUtility.ConvertString<bool>(commandArgs[StringConstants.CleanupComposeFiles]);

                    HashSet<string> ignoredKeys;
                    imageBuilder.ValidateComposeDeployment(
                        commandArgs[StringConstants.ComposeFilePath],
                        commandArgs.ContainsKey(StringConstants.OverrideFilePath) ? commandArgs[StringConstants.OverrideFilePath] : null,
                        commandArgs.ContainsKey(StringConstants.AppName) ? commandArgs[StringConstants.AppName] : null,
                        commandArgs.ContainsKey(StringConstants.AppTypeName) ? commandArgs[StringConstants.AppTypeName] : null,
                        commandArgs.ContainsKey(StringConstants.AppTypeVersion) ? commandArgs[StringConstants.AppTypeVersion] : null,
                        timeout,
                        out ignoredKeys,
                        cleanupComposeFiles);
                }
                else if (ImageBuilderUtility.Equals(operationValue, StringConstants.OperationBuildComposeDeployment))
                {
                    bool cleanupComposeFiles = commandArgs.ContainsKey(StringConstants.CleanupComposeFiles) && ImageBuilderUtility.ConvertString<bool>(commandArgs[StringConstants.CleanupComposeFiles]);
                    bool shouldSkipChecksumValidation = commandArgs.ContainsKey(StringConstants.DisableChecksumValidation) && ImageBuilderUtility.ConvertString<bool>(commandArgs[StringConstants.DisableChecksumValidation]);
                    bool isPasswordEncrypted = commandArgs.ContainsKey(StringConstants.IsRepositoryPasswordEncrypted) && ImageBuilderUtility.ConvertString<bool>(commandArgs[StringConstants.IsRepositoryPasswordEncrypted]);

                    bool generateDnsNames = commandArgs.ContainsKey(StringConstants.GenerateDnsNames) && ImageBuilderUtility.ConvertString<bool>(commandArgs[StringConstants.GenerateDnsNames]);

                    imageBuilder.BuildComposeDeploymentPackage(
                        commandArgs[StringConstants.ComposeFilePath],
                        commandArgs.ContainsKey(StringConstants.OverrideFilePath) ? commandArgs[StringConstants.OverrideFilePath] : null,
                        timeout,
                        commandArgs.ContainsKey(StringConstants.AppName) ? commandArgs[StringConstants.AppName] : null,
                        commandArgs[StringConstants.AppTypeName],
                        commandArgs[StringConstants.AppTypeVersion],
                        commandArgs.ContainsKey(StringConstants.RepositoryUserName) ? commandArgs[StringConstants.RepositoryUserName] : null,
                        commandArgs.ContainsKey(StringConstants.RepositoryPassword) ? commandArgs[StringConstants.RepositoryPassword] : null,
                        isPasswordEncrypted,
                        generateDnsNames,
                        commandArgs[StringConstants.OutputComposeFilePath],
                        commandArgs[StringConstants.Output],
                        cleanupComposeFiles,
                        shouldSkipChecksumValidation);
                }
                else if (ImageBuilderUtility.Equals(operationValue, StringConstants.OperationBuildComposeApplicationForUpgrade))
                {
                    bool cleanupComposeFiles = commandArgs.ContainsKey(StringConstants.CleanupComposeFiles) && ImageBuilderUtility.ConvertString<bool>(commandArgs[StringConstants.CleanupComposeFiles]);
                    bool shouldSkipChecksumValidation = commandArgs.ContainsKey(StringConstants.DisableChecksumValidation) && ImageBuilderUtility.ConvertString<bool>(commandArgs[StringConstants.DisableChecksumValidation]);
                    bool isPasswordEncrypted = commandArgs.ContainsKey(StringConstants.IsRepositoryPasswordEncrypted) && ImageBuilderUtility.ConvertString<bool>(commandArgs[StringConstants.IsRepositoryPasswordEncrypted]);

                    bool generateDnsNames = commandArgs.ContainsKey(StringConstants.GenerateDnsNames) && ImageBuilderUtility.ConvertString<bool>(commandArgs[StringConstants.GenerateDnsNames]);

                    imageBuilder.BuildComposeDeploymentPackageForUpgrade(
                        commandArgs[StringConstants.ComposeFilePath],
                        commandArgs.ContainsKey(StringConstants.OverrideFilePath) ? commandArgs[StringConstants.OverrideFilePath] : null,
                        timeout,
                        commandArgs.ContainsKey(StringConstants.AppName) ? commandArgs[StringConstants.AppName] : null,
                        commandArgs[StringConstants.AppTypeName],
                        commandArgs[StringConstants.CurrentAppTypeVersion],
                        commandArgs[StringConstants.TargetAppTypeVersion],
                        commandArgs.ContainsKey(StringConstants.RepositoryUserName) ? commandArgs[StringConstants.RepositoryUserName] : null,
                        commandArgs.ContainsKey(StringConstants.RepositoryPassword) ? commandArgs[StringConstants.RepositoryPassword] : null,
                        isPasswordEncrypted,
                        generateDnsNames,
                        commandArgs[StringConstants.OutputComposeFilePath],
                        commandArgs[StringConstants.Output],
                        cleanupComposeFiles,
                        shouldSkipChecksumValidation);
                }
                else if (ImageBuilderUtility.Equals(operationValue, StringConstants.TestErrorDetails))
                {
                    throw new Exception(StringConstants.TestErrorDetails);
                }
                else
                {
                    throw new ArgumentException(
                        string.Format(
                            CultureInfo.CurrentCulture,
                            StringResources.Error_ImageBuilderExeCommandLineInvalidOperation,
                            StringConstants.Operation,
                            operationValue));
                }
            }
            catch (AggregateException ae)
            {
                exception = ae.InnerException;

                StringBuilder exceptionStringBuilder = new StringBuilder("Aggregate exception has ");
                exceptionStringBuilder.Append(ae.InnerExceptions.Count);
                exceptionStringBuilder.Append(" exceptions. ");
                ae.InnerExceptions.ForEach(innerException => exceptionStringBuilder.AppendLine(innerException.Message));

                ImageBuilderExe.TraceSource.WriteWarning(
                    ImageBuilderExe.TraceType,
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

                return unchecked((int)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_IMAGEBUILDER_UNEXPECTED_ERROR);
            }
            else
            {
                ImageBuilderExe.TraceSource.WriteInfo(
                    ImageBuilderExe.TraceType,
                    "ImageBuilderExe operation completed successfully.");

                return 0;
            }
        }

        public static void EnsureRequiredCommandLineParameters(Dictionary<string, string> commandArgs)
        {
            HashSet<string> requiredImageBuilderKeys = new HashSet<string>(StringComparer.OrdinalIgnoreCase) { StringConstants.Operation };
            foreach (string requiredKey in requiredImageBuilderKeys)
            {
                if (!commandArgs.ContainsKey(requiredKey))
                {
                    throw new ArgumentException(
                        string.Format(
                            CultureInfo.CurrentCulture,
                            StringResources.Error_ImageBuilderExeMissingRequiredArgument,
                            requiredKey));
                }
            }

            string operationValue = commandArgs[StringConstants.Operation];
            if (ImageBuilderUtility.Equals(operationValue, StringConstants.OperationBuildApplicationTypeInfo))
            {
                string[] requiredBuildApplicationTypeInfoParams = new string[]
                {
                    StringConstants.BuildPath,
                    StringConstants.Output
                };

                CheckRequiredCommandLineArguments(requiredBuildApplicationTypeInfoParams, operationValue, commandArgs);
            }
            else if (ImageBuilderUtility.Equals(operationValue, StringConstants.OperationBuildApplicationType))
            {
                string[] requiredBuildApplicationTypeParams = new string[]
                {
                    StringConstants.BuildPath
                };

                CheckRequiredCommandLineArguments(requiredBuildApplicationTypeParams, operationValue, commandArgs);
            }
            else if (ImageBuilderUtility.Equals(operationValue, StringConstants.OperationBuildApplication))
            {
                string[] requiredBuildApplicationParams = new string[]
                {
                    StringConstants.AppId,
                    StringConstants.AppTypeName,
                    StringConstants.AppTypeVersion,
                    StringConstants.NameUri,
                    StringConstants.Output
                };

                CheckRequiredCommandLineArguments(requiredBuildApplicationParams, operationValue, commandArgs);
            }
            else if (ImageBuilderUtility.Equals(operationValue, StringConstants.OperationValidateComposeDeployment))
            {
                string[] requiredBuildApplicationParams = new string[]
                {
                    StringConstants.ComposeFilePath
                };

                CheckRequiredCommandLineArguments(requiredBuildApplicationParams, operationValue, commandArgs);
            }
            else if (ImageBuilderUtility.Equals(operationValue, StringConstants.OperationBuildComposeDeployment))
            {
                string[] requiredBuildApplicationParams = new string[]
                {
                    StringConstants.ComposeFilePath,
                    StringConstants.AppTypeName,
                    StringConstants.AppTypeVersion,
                    StringConstants.Output,
                    StringConstants.OutputComposeFilePath
                };

                CheckRequiredCommandLineArguments(requiredBuildApplicationParams, operationValue, commandArgs);
            }
            else if (ImageBuilderUtility.Equals(operationValue, StringConstants.OperationUpgradeApplication))
            {
                string[] requiredUpgradeApplicationParams = new string[]
                {
                    StringConstants.AppId,
                    StringConstants.AppTypeName,
                    StringConstants.AppTypeVersion,
                    StringConstants.CurrentAppInstanceVersion,
                    StringConstants.Output
                };

                CheckRequiredCommandLineArguments(requiredUpgradeApplicationParams, operationValue, commandArgs);
            }
            else if (ImageBuilderUtility.Equals(operationValue, StringConstants.OperationDelete))
            {
                string[] requiredUpgradeApplicationParams = new string[]
                {
                    StringConstants.Input
                };

                CheckRequiredCommandLineArguments(requiredUpgradeApplicationParams, operationValue, commandArgs);
            }
            else if (ImageBuilderUtility.Equals(operationValue, StringConstants.OperationGetFabricVersion))
            {
                string[] requiredGetFabricVersionParams = new string[]
                {
                    StringConstants.Output
                };

                CheckRequiredCommandLineArguments(requiredGetFabricVersionParams, operationValue, commandArgs);
            }
            else if (ImageBuilderUtility.Equals(operationValue, StringConstants.OperationProvisionFabric))
            {
                if (!commandArgs.ContainsKey(StringConstants.CodePath) && !commandArgs.ContainsKey(StringConstants.ConfigPath))
                {
                    throw new ArgumentException(
                        string.Format(
                            CultureInfo.CurrentCulture,
                            StringResources.Error_ImageBuilderExeMissingCodeAndConfigPath,
                            operationValue));
                }

                string[] requiredProvisionFabricParams = new string[]
                {
                    StringConstants.InfrastructureManifestFile
                };

                CheckRequiredCommandLineArguments(requiredProvisionFabricParams, operationValue, commandArgs);
            }
            else if (ImageBuilderUtility.Equals(operationValue, StringConstants.OperationUpgradeFabric))
            {
                string[] requiredUpgradeFabricParams = new string[]
                {
                    StringConstants.CurrentFabricVersion,
                    StringConstants.TargetFabricVersion
                };

                CheckRequiredCommandLineArguments(requiredUpgradeFabricParams, operationValue, commandArgs);
            }
        }

        private static Dictionary<string, string> ParseParameters(string[] args)
        {
            Dictionary<string, string> commandArgs = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);

            foreach (string arg in args)
            {
                int delimiterIndex = arg.IndexOf(":", StringComparison.OrdinalIgnoreCase);

                if (delimiterIndex == -1)
                {
                    throw new ArgumentException(
                        string.Format(
                            CultureInfo.CurrentCulture,
                            StringResources.Error_ImageBuilderExeInvalidArgument,
                            arg));
                }

                string key = arg.Substring(0, delimiterIndex);
                string value = arg.Substring(delimiterIndex + 1);

                if (!SupportedCommandArgs.Contains(key))
                {
                    throw new ArgumentException(
                        string.Format(
                            CultureInfo.CurrentCulture,
                            StringResources.Error_ImageBuilderExeInvalidArgumentUnrecognizedKey,
                            arg,
                            key));
                }

                if (commandArgs.ContainsKey(key))
                {
                    throw new ArgumentException(
                        string.Format(
                            CultureInfo.CurrentCulture,
                            StringResources.Error_ImageBuilderExeInvalidArgumentDuplicate,
                            arg,
                            key));
                }

                commandArgs.Add(key, value);
            }

            return commandArgs;
        }

        private static Dictionary<string, string> ParseAppParameters(Dictionary<string, string> commandArgs)
        {
            Dictionary<string, string> applicationParameters = null;
            if (commandArgs.ContainsKey(StringConstants.AppParam))
            {
                string workingDirectory = null;
                if (commandArgs.ContainsKey(StringConstants.WorkingDir))
                {
                    workingDirectory = commandArgs[StringConstants.WorkingDir];
                }
                else
                {
                    workingDirectory = Directory.GetCurrentDirectory();
                }

                string appparametersFile = commandArgs.ContainsKey(StringConstants.AppParam) ?
                    Path.Combine(workingDirectory, commandArgs[StringConstants.AppParam]) : null;
                ImageBuilderExe.ParseApplicationParametersFile(appparametersFile, out applicationParameters);
            }

            return applicationParameters;
        }

        private static void ParseApplicationParametersFile(string inputFilePath, out Dictionary<string, string> applicationParameters)
        {
            applicationParameters = new Dictionary<string, string>();
            if (string.IsNullOrEmpty(inputFilePath))
            {
                return;
            }

            if (!File.Exists(inputFilePath))
            {
                throw new FileNotFoundException(
                    string.Format(
                        CultureInfo.CurrentCulture,
                        StringResources.Error_ImageBuilderExeNoApplicationParameterFile),
                    inputFilePath);
            }

            using (StreamReader reader = new StreamReader(File.Open(inputFilePath, FileMode.Open)))
            {
                string param;
                while ((param = reader.ReadLine()) != null)
                {
                    string val;
                    if((val = reader.ReadLine()) == null)
                    {
                        throw new ArgumentException(
                            string.Format(
                                CultureInfo.CurrentCulture,
                                StringResources.Error_ImageBuilderExeFailedParseApplicationParameterFile,
                                inputFilePath,
                                param));
                    }
                    if(!applicationParameters.ContainsKey(param))
                    {
                        applicationParameters.Add(param, val);
                    }
                } // readline
            } // using StreamReader
        }

        private static void CheckRequiredCommandLineArguments(string[] requiredArguments, string operationValue, Dictionary<string, string> commandArgs)
        {
            foreach (string requiredArgument in requiredArguments)
            {
                if (!commandArgs.ContainsKey(requiredArgument))
                {
                    throw new ArgumentException(
                        string.Format(
                            CultureInfo.CurrentCulture,
                            StringResources.Error_ImageBuilderExeMissingRequiredArgumentForOperation,
                            requiredArgument,
                            operationValue));
                }
            }
        }

        private static int GetFabricErrorCodeOrEFail(Exception e)
        {
            int errorCode = 0;

            if (!ImageBuilderExe.ManagedToNativeExceptionTranslation.TryGetValue(e.GetType(), out errorCode))
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
                    errorCode = unchecked((int)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_IMAGEBUILDER_UNEXPECTED_ERROR);
                }
            }

            return errorCode;
        }

        private static void OnTimeout(object state)
        {
            OnFailure(new TimeoutException("ImageBuilderExe self-terminating on timeout"), state as string);

            try
            {
                Environment.Exit(unchecked((int)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_IMAGEBUILDER_UNEXPECTED_ERROR));
            }
            catch (Exception e)
            {
                ImageBuilderExe.TraceSource.WriteError(
                    ImageBuilderExe.TraceType,
                    "ImageBuilderExe failed to exit cleanly: {0}", e);

                throw;
            }
        }

        private static void OnFailure(Exception exception, string errorDetailsFile)
        {
            ImageBuilderExe.TraceSource.WriteWarning(
                ImageBuilderExe.TraceType,
                "ImageBuilderExe operation failed with the following exception: {0}",
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

                    ImageBuilderUtility.WriteStringToFile(
                        errorDetailsFile,
                        ImageBuilderExe.GetFabricErrorCodeOrEFail(exception) + "," + errorDetails.ToString(),
                        false,
                        Encoding.Unicode);
                }
                catch (Exception innerEx)
                {
                    ImageBuilderExe.TraceSource.WriteWarning(
                        ImageBuilderExe.TraceType,
                        "Failed to write error details: {0}",
                        innerEx.ToString());
                }
            }
        }
    }
}
