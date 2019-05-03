// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Collections.ObjectModel;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.ImageStore;
    using System.Fabric.Interop;
    using System.Fabric.Management.ImageBuilder;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Management.Automation;
    using System.Management.Automation.Runspaces;
    using System.Net;
    using System.Net.Sockets;
    using System.Reflection;
    using System.Security;
    using System.Security.Cryptography.X509Certificates;
    using System.Text;
    using System.Threading;
    using System.Xml;
    using System.Xml.Schema;
    using System.Xml.Serialization;

    public abstract class CommonCmdletBase : PSCmdlet
    {
        private const string ImageStoreConnectionStringParameterName = "Name=\"ImageStoreConnectionString\"";
        private const string ValueParameterName = "Value=\"";

        private static NativeConfigStore configStore = null;
        private static object configLock = new object();
        private static bool ignoreUpdateFailures = false;

        private CancellationTokenSource cancellationTokenSource;
        private PowerShell powershellHost;

        protected CommonCmdletBase()
        {
            AppDomain.CurrentDomain.AssemblyResolve += new ResolveEventHandler(LoadFromFabricCodePath);

            this.cancellationTokenSource = null;
            this.powershellHost = null;
            this.TimeoutSec = null;

            Ensure_IgnoreUpdateFailures();
        }

        [Parameter(Mandatory = false)]
        public int? TimeoutSec
        {
            get;
            set;
        }

        public static ErrorCategory GetErrorCategoryForException(Exception exception)
        {
            var errorCategory = ErrorCategory.NotSpecified;

            if (exception is ArgumentException)
            {
                errorCategory = ErrorCategory.InvalidArgument;
            }
            else if (exception is InvalidOperationException
                     || exception is FabricException
                     || exception is FabricValidationException)
            {
                errorCategory = ErrorCategory.InvalidOperation;
            }
            else if (exception is InvalidDataException
                     || exception is InvalidCastException)
            {
                errorCategory = ErrorCategory.InvalidData;
            }
            else if (exception is XmlSchemaValidationException
                     || exception is XmlException)
            {
                errorCategory = ErrorCategory.ParserError;
            }
            else if (exception is TimeoutException)
            {
                errorCategory = ErrorCategory.OperationTimeout;
                exception = new TimeoutException(StringResources.Error_ClusterConnectionTimeout);
            }
            else if (exception is OperationCanceledException)
            {
                errorCategory = ErrorCategory.OperationStopped;
            }
            else if (exception is UnauthorizedAccessException
                     || exception is SecurityException)
            {
                errorCategory = ErrorCategory.SecurityError;
            }
            else if (exception is NullReferenceException
                     || exception is FileNotFoundException
                     || exception is DirectoryNotFoundException)
            {
                errorCategory = ErrorCategory.ResourceUnavailable;
            }
            else if (exception is FabricTransientException)
            {
                errorCategory = ErrorCategory.ResourceUnavailable;
            }

            return errorCategory;
        }

        internal static Assembly LoadFromFabricCodePath(object sender, ResolveEventArgs args)
        {
            try
            {
                string folderPath = FabricEnvironment.GetCodePath();
                string assemblyPath = Path.Combine(folderPath, new AssemblyName(args.Name).Name + ".dll");
                if (File.Exists(assemblyPath))
                {
                    return Assembly.LoadFrom(assemblyPath);
                }
            }
            catch (Exception)
            {
                // Suppress any Exception so that we can continue to
                // load the assembly through other means
            }

            return null;
        }       

        internal void SetClusterConnection(IClusterConnection clusterConnection)
        {
            var oldClusterConnection = this.SessionState.PSVariable.GetValue(Constants.ClusterConnectionVariableName, null) as IClusterConnection;
            if (oldClusterConnection != null)
            {
                this.WriteWarning(StringResources.Info_ClusterConnectionAlreadyExisted);
            }

            this.SessionState.PSVariable.Set(Constants.ClusterConnectionVariableName, clusterConnection);
        }

        internal string GetFabricFilePath(string fileName)
        {
            var fabricfilePath = this.JoinPath(FabricEnvironment.GetCodePath(), fileName);
            if (string.IsNullOrEmpty(fabricfilePath) || !this.TestPath(fabricfilePath))
            {
                throw new FileNotFoundException(
                    string.Format(
                        CultureInfo.CurrentCulture,
                        StringResources.Error_FabricCodeFileNotFound, 
                        fileName));
            }

            return fabricfilePath;
        }

        internal bool ClusterNetworkIsUnreachable(string[] connectionEndpoint)
        {
            if (connectionEndpoint == null)
            {
                return false;
            }

            int connectFailures = 0;
            foreach (var endpoint in connectionEndpoint)
            {
                try
                {
                    this.Connect(endpoint);
                }
                catch (Exception e)
                {
                    ++connectFailures;
                    this.WriteWarning(e.Message);
                }
            }

            return (0 < connectFailures) && (connectFailures == connectionEndpoint.Length);
        }

        internal void TestClusterConnection(IClusterConnection clusterConnection)
        {
            this.TestClusterConnection(clusterConnection, false);
        }

        internal void TestClusterConnection(IClusterConnection clusterConnection, bool testNetwork)
        {
            try
            {
                var innerTimeout = TimeSpan.FromSeconds(this.GetTimeout().TotalSeconds / 3);

                try
                {
                    clusterConnection.NameExistsAsync(
                        new Uri(Constants.DummyUri),
                        innerTimeout,
                        this.GetCancellationToken()).Wait();

                    // For backwards compatibility with pre-query clusters, first attempt
                    // to message the Naming Service.
                    // If the Naming Service is potentially down, then fall back
                    // to a query that only depends on the Failover Manager
                    // Service being up.
                    this.WriteObject(true);
                    return;
                }
                catch (Exception)
                {
                    // Intentional no-op: Try a query to the FM
                    this.WriteWarning(StringResources.Powershell_NamingServiceTestFailed);
                }

                try
                {
                    clusterConnection.GetServicePagedListAsync(
                        new System.Fabric.Description.ServiceQueryDescription(new Uri(Constants.SystemApplicationUri))
                        {
                            ServiceNameFilter = new Uri(Constants.FailoverManagerUri)
                        },
                        innerTimeout,
                        this.GetCancellationToken()).Wait();

                    this.WriteObject(true);
                    return;
                }
                catch (Exception)
                {
                    this.WriteWarning(StringResources.Powershell_FailoverManagerServiceTestFailed);
                }

                try
                {
                    clusterConnection.GetPartitionListAsync(
                        new Uri(Constants.FailoverManagerUri),
                        null,
                        null,
                        innerTimeout,
                        this.GetCancellationToken()).Wait();

                    this.WriteObject(true);
                    return;
                }
                catch (Exception)
                {
                    if (testNetwork)
                    {
                        this.WriteWarning(StringResources.Powershell_FMMServiceTestFailed);
                    }
                    else
                    {
                        throw;
                    }
                }

                this.CheckConnectionAndThrowIfNeeded(clusterConnection);

                this.WriteObject(true);
            }
            catch (AggregateException aggregateException)
            {
                Exception firstInner = null;

                aggregateException.Handle((inner) =>
                {
                    this.WriteObject(false);

                    if (firstInner == null)
                    {
                        firstInner = inner;
                    }

                    return true;
                });

                if (firstInner is FabricException)
                {
                    this.CheckConnectionAndThrowIfNeeded(clusterConnection);
                }

                this.ThrowTerminatingError(
                    firstInner == null ? aggregateException : firstInner,
                    Constants.TestClusterConnectionErrorId,
                    null);
            }
        }

        internal IImageStore CreateImageStore(string imageStoreConnectionString, string workingDirectory)
        {
            if (!imageStoreConnectionString.StartsWith(Constants.ImageStoreConnectionFileType, StringComparison.OrdinalIgnoreCase)
                 && !imageStoreConnectionString.StartsWith(Constants.ImageStoreConnectionXStoreType, StringComparison.OrdinalIgnoreCase)
                 && !imageStoreConnectionString.StartsWith(Constants.ImageStoreConnectionFabricType, StringComparison.OrdinalIgnoreCase))
            {
                throw new ArgumentException(StringResources.Error_InvalidImageStoreConnectionString);
            }

            SecurityCredentials credentials = null;
            string[] connectionEndpoints = null;
            var clusterConnection = this.GetClusterConnection();

            if (clusterConnection != null)
            {
                credentials = clusterConnection.SecurityCredentials;
                connectionEndpoints = clusterConnection.ConnectionEndpoint;
            }

            return ImageStoreFactory.CreateImageStore(
                imageStoreConnectionString,
                null,
                connectionEndpoints,
                credentials,
                workingDirectory,
                false /*isInternal*/);
        }

        internal string GetPackageName(string packagePath)
        {
            return Path.GetFileName(packagePath);
        }

        internal string GetAbsolutePath(string path)
        {
            if (path == null)
            {
                return null;
            }

            return FabricPath.GetFullPath(Path.Combine(
               this.SessionState.Path.CurrentFileSystemLocation.Path,
               path));
        }

        internal void ChangeCurrentDirectoryToSession()
        {
            Directory.SetCurrentDirectory(this.SessionState.Path.CurrentFileSystemLocation.Path);
        }

        internal void UploadToImageStore(
            string imageStoreConnectionString,
            string src,
            string des,
            UploadProgressHandler progressHandler = null)
        {
            if (string.IsNullOrEmpty(des))
            {
                des = this.GetPackageName(src);
            }

            if (!this.ValidateImageStorePath(des))
            {
                return;
            }

            if (progressHandler != null)
            {
                progressHandler.SetStateUploading();
            }

            IClusterConnection clusterConnection = null;

            if (imageStoreConnectionString.StartsWith(Constants.ImageStoreConnectionFabricType, StringComparison.OrdinalIgnoreCase))
            {
                clusterConnection = this.GetClusterConnection();

                if (clusterConnection.SecurityCredentials != null && clusterConnection.SecurityCredentials.CredentialType == CredentialType.Claims)
                {
                    clusterConnection.FabricClient.ImageStore.UploadContent(imageStoreConnectionString, des, src, progressHandler, this.GetTimeout(), CopyFlag.AtomicCopy, false);
                }
                else
                {
                    clusterConnection = null;
                }
            }

            if (clusterConnection == null)
            {
                var imageStore = this.CreateImageStore(imageStoreConnectionString, Path.GetTempPath());
                imageStore.UploadContent(
                    des,
                    src,
                    progressHandler,
                    this.GetTimeout(),
                    CopyFlag.AtomicCopy,
                    false);
            }

            this.WriteObject(StringResources.Info_UploadToImageStoreSucceeded);
        }

        internal void ThrowIfInvalidReservedImageStoreOperation(string path)
        {
            path = path.TrimEnd('\\');
            if (string.Compare(path, Constants.StoreFolder, true) == 0 ||
                string.Compare(path, Constants.WindowsFabricStoreFolder, true) == 0)
            {
                this.ThrowTerminatingError(
                    new FabricImageBuilderReservedDirectoryException(string.Format(StringResources.Error_InvalidReservedImageStoreOperation, path)),
                    Constants.ReservedImageStoreFoldersName,
                    null);
            }
        }

        internal bool ValidateImageStorePath(string path)
        {
            string[] reservedDirectoryNames = new string[]
            {
                Constants.StoreFolder,
                Constants.WindowsFabricStoreFolder
            };

            try
            {
                ImageBuilderUtility.ValidateRelativePath(
                    path,
                    reservedDirectoryNames);
            }
            catch (FabricImageBuilderReservedDirectoryException ex)
            {
                return this.ShouldContinue(ex.Message, string.Empty);
            }

            return true;
        }
       
        protected internal IClusterConnection GetClusterConnection()
        {
            var clusterConnection = this.GetClusterConnectionNoThrow();

            if (clusterConnection == null)
            {
                this.ThrowTerminatingError(
                    new NullReferenceException(StringResources.Error_InvalidClusterConnection),
                    Constants.GetClusterConnectionErrorId,
                    null);
            }

            return clusterConnection;
        }

        protected internal IClusterConnection GetClusterConnectionNoThrow()
        {
            return this.SessionState.PSVariable.GetValue(Constants.ClusterConnectionVariableName, null) as IClusterConnection;
        }

        protected override void StopProcessing()
        {
            if (this.cancellationTokenSource != null)
            {
                this.cancellationTokenSource.Cancel(true);
                this.cancellationTokenSource.Dispose();
            }

            if (this.powershellHost != null)
            {
                this.powershellHost.Stop();
                this.powershellHost.Dispose();
            }
        }

        protected override void EndProcessing()
        {
            if (this.cancellationTokenSource != null)
            {
                this.cancellationTokenSource.Dispose();
            }

            if (this.powershellHost != null)
            {
                this.powershellHost.Dispose();
            }
        }

        protected virtual object FormatOutput(object output)
        {
            return output;
        }

        protected T ReadXml<T>(string xmlFilePath, string schemaFilePath)
        {
            var xmlReaderSettings = new XmlReaderSettings { ValidationType = ValidationType.Schema, XmlResolver = null };
            xmlReaderSettings.Schemas.Add(Constants.FabricNamespace, schemaFilePath);

            using (var fileStream = File.Open(xmlFilePath, FileMode.Open, FileAccess.Read, FileShare.Read))
            {
                using (var xmlReader = XmlReader.Create(fileStream, xmlReaderSettings))
                {
                    var serializer = new XmlSerializer(typeof(T));
                    var obj = (T)serializer.Deserialize(xmlReader);
                    return obj;
                }
            }
        }

        protected CancellationToken GetCancellationToken()
        {
            if (this.cancellationTokenSource == null)
            {
                this.cancellationTokenSource = new CancellationTokenSource();
            }

            return this.cancellationTokenSource.Token;
        }

        protected TimeSpan GetTimeout()
        {
            if (this.TimeoutSec.HasValue)
            {
                if (this.TimeoutSec.Value <= 0)
                {
                    throw new ArgumentException(StringResources.Error_InvalidPowershellTimeout);
                }

                return TimeSpan.FromSeconds(this.TimeoutSec.Value);
            }

            return TimeSpan.FromSeconds(Constants.DefaultTimeoutSec);
        }

        protected PSObject ExecuteScript(string script)
        {
            if (this.powershellHost == null)
            {
                this.powershellHost = PowerShell.Create();
                this.powershellHost.Runspace = RunspaceFactory.CreateRunspace(InitialSessionState.CreateDefault());
                this.powershellHost.Runspace.Open();
            }

            this.powershellHost.AddScript(script);
            this.WriteVerbose(script);

            var output = this.powershellHost.Invoke();

            if (output == null || !output.Any())
            {
                throw new InvalidDataException(StringResources.Error_InvalidPowershellScriptOutput);
            }

            this.WriteOutput(output);
            return output[output.Count - 1];
        }

        protected int GetExitCode(PSObject output)
        {
            if (output == null || output.ImmediateBaseObject == null)
            {
                throw new InvalidDataException(StringResources.Error_InvalidPowershellScriptExitCode);
            }

            return (int)output.ImmediateBaseObject;
        }

        protected void ThrowTerminatingError(Exception exception, string errorId, object target)
        {
            var errorCategory = GetErrorCategoryForException(exception);

            this.WriteVerbose(exception.ToString());

            this.ThrowTerminatingError(new ErrorRecord(exception, errorId, errorCategory, target));
        }

        protected void ThrowIfNegative(int? parameter, string parameterName)
        {
            if (parameter.HasValue && parameter.Value < 0)
            {
                throw new ArgumentException(string.Format(
                                                CultureInfo.InvariantCulture,
                                                "{0}: {1}",
                                                StringResources.Error_InvalidPowershellParameter_Negative,
                                                parameterName));
            }
        }

        protected string DecryptText(string text, StoreLocation storeLocation)
        {
            using (var pin = new PinCollection())
            {
                // Calling native API directly because we need to return string instead of SecureString for this cmdlet,
                // and we do not want to have a public managed API that returns string instead of SecureString.
                return StringResult.FromNative(NativeCommon.FabricDecryptText(
                    pin.AddBlittable(text),
                    (NativeTypes.FABRIC_X509_STORE_LOCATION)storeLocation));
            }
        }

        protected string TryFetchImageStoreConnectionString(string connectionStringArg, StoreLocation certStoreLocation)
        {
            if (!string.IsNullOrEmpty(connectionStringArg))
            {
                return connectionStringArg;
            }

            var clusterConnection = this.GetClusterConnectionNoThrow();

            if (clusterConnection == null)
            {
                this.ThrowTerminatingError(
                    new ArgumentException(StringResources.Error_MissingImageStoreConnectionStringArgument),
                    Constants.GetImageStoreConnectionStringErrorId,
                    null);
            }

            string clusterManifest = null;
            try
            {
                clusterManifest = clusterConnection.GetClusterManifestAsync(
                    this.GetTimeout(),
                    this.GetCancellationToken()).Result;
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.GetClusterManifestErrorId,
                        clusterConnection);
                    return true;
                });
            }

            int searchIndex = clusterManifest.IndexOf(ImageStoreConnectionStringParameterName) + ImageStoreConnectionStringParameterName.Length;
            int startIndex = clusterManifest.IndexOf(ValueParameterName, searchIndex) + ValueParameterName.Length;
            int endIndex = clusterManifest.IndexOf("\"", startIndex);

            if (startIndex < 0 || endIndex < 0)
            {
                this.ThrowTerminatingError(
                    new ArgumentException(StringResources.Error_ImageStoreConnectionStringParse),
                    Constants.GetImageStoreConnectionStringErrorId,
                    null);
            }

            var imageStoreString = clusterManifest.Substring(startIndex, endIndex - startIndex);

            if (ImageStoreUtility.GetImageStoreProviderType(imageStoreString) == ImageStoreProviderType.Invalid)
            {
                imageStoreString = this.DecryptText(imageStoreString, certStoreLocation);

                if (ImageStoreUtility.GetImageStoreProviderType(imageStoreString) == ImageStoreProviderType.Invalid)
                {
                    this.ThrowTerminatingError(
                        new ArgumentException(StringResources.Error_ImageStoreConnectionStringParse),
                        Constants.GetImageStoreConnectionStringErrorId,
                        null);
                }
            }

            this.WriteObject(string.Format(CultureInfo.InvariantCulture, StringResources.PowerShell_Using_ImageStoreConnectionString, imageStoreString));

            return imageStoreString;
        }

        private static void Ensure_IgnoreUpdateFailures()
        {
            if (!ignoreUpdateFailures)
            {
                lock (configLock)
                {
                    if (!ignoreUpdateFailures)
                    {
                        configStore = NativeConfigStore.FabricGetConfigStore();
                        configStore.IgnoreUpdateFailures = true;
                        ignoreUpdateFailures = true;
                    }
                }
            }
        }

        private void WriteOutput(Collection<PSObject> output)
        {
            var outputBuilder = new StringBuilder();
            foreach (var obj in output)
            {
                if (obj == null)
                {
                    outputBuilder.Append(Constants.NullObjectOutput);
                    continue;
                }

                outputBuilder.Append(obj);
                outputBuilder.Append(Environment.NewLine);
                foreach (var property in obj.Properties)
                {
                    outputBuilder.Append(property.Name);
                    outputBuilder.Append(": ");
                    outputBuilder.Append((property.Value == null) ? Constants.NullObjectOutput : property.Value.ToString());
                    outputBuilder.Append(Environment.NewLine);
                }
            }

            this.WriteVerbose(outputBuilder.ToString());
        }

        private bool TestPath(string filePath)
        {
            var output = this.ExecuteScript(
                             string.Format(
                                 CultureInfo.CurrentCulture,
                                 @"{0} ""{1}""",
                                 Constants.TestPathCommandName,
                                 filePath));

            if (output == null || output.ImmediateBaseObject == null)
            {
                throw new InvalidOperationException(
                    string.Format(
                        CultureInfo.CurrentCulture,
                        StringResources.Error_TestPathFailed,
                        filePath));
            }

            return (bool)output.ImmediateBaseObject;
        }

        private string JoinPath(string filePath, string fileName)
        {
            var output = this.ExecuteScript(
                             string.Format(
                                 CultureInfo.CurrentCulture,
                                 @"{0} ""{1}"" ""{2}""",
                                 Constants.JoinPathCommandName,
                                 filePath,
                                 fileName));

            if (output == null || output.ImmediateBaseObject == null)
            {
                throw new InvalidOperationException(
                    string.Format(
                        CultureInfo.CurrentCulture,
                        StringResources.Error_JoinPathFailed,
                        filePath,
                        fileName));
            }

            return (string)output.ImmediateBaseObject;
        }

        private string GetItemProperty(string path, string name)
        {
            var output = this.ExecuteScript(
                             string.Format(
                                 CultureInfo.CurrentCulture,
                                 @"({0} ""{1}"" {2}).{3}",
                                 Constants.GetItemPropertyCommandName,
                                 path,
                                 name,
                                 name));

            if (output == null || output.ImmediateBaseObject == null)
            {
                throw new InvalidOperationException(
                    string.Format(
                        CultureInfo.CurrentCulture,
                        StringResources.Error_ReadRegistryKeyFailed,
                        path,
                        name));
            }

            return (string)output.ImmediateBaseObject;
        }

        private void CheckConnectionAndThrowIfNeeded(IClusterConnection clusterConnection)
        {
            if (this.ClusterNetworkIsUnreachable(clusterConnection.ConnectionEndpoint))
            {
                this.ThrowTerminatingError(
                    new FabricException(
                        StringResources.PowerShell_NoClusterEndpointIsReachable,
                        null,
                        FabricErrorCode.CommunicationError),
                    Constants.TestClusterConnectionErrorId,
                    null);
            }
        }

        private void Connect(string endpoint)
        {
            var splitterIndex = endpoint.LastIndexOf(':');
            if (splitterIndex < 0)
            {
                throw new ArgumentException(endpoint);
            }

            var host = endpoint.Substring(0, splitterIndex);
            var portString = endpoint.Substring(splitterIndex + 1, endpoint.Length - splitterIndex - 1);
            int port;
            if (!int.TryParse(portString, out port))
            {
                throw new ArgumentException(endpoint);
            }

            IPAddress ip;
            TcpClient tcpClient = null;
            if (IPAddress.TryParse(host, out ip))
            {
                tcpClient = new TcpClient(ip.AddressFamily);
                tcpClient.Connect(ip, port);
            }
            else
            {
                tcpClient = new TcpClient();
                tcpClient.Connect(host, port);
            }

            tcpClient.Close();
        }
    }
}