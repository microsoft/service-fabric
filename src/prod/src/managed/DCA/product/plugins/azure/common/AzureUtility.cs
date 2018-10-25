// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;
    using System.Security;
    using System.Runtime.InteropServices;
    using Microsoft.WindowsAzure.Storage;
    using Microsoft.WindowsAzure.Storage.Auth;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Dca;
    using AzureUploaderCommon;
    using System.Reflection;

    // This class implements miscellaneous utility functions for Azure
    internal class AzureUtility
    {
        static AzureUtility()
        {
            // Convert development storage account key to byte array
            SecureString devStoreKeySecure = new SecureString();
            foreach (char c in AzureConstants.DevelopmentStorageAccountWellKnownKey)
            {
                devStoreKeySecure.AppendChar(c);
            }

            devStoreKeySecure.MakeReadOnly();

#if DotNetCoreClr
            IntPtr passwordPtr = SecureStringMarshal.SecureStringToGlobalAllocUnicode(devStoreKeySecure);
            DevelopmentStorageAccountWellKnownKey = Marshal.PtrToStringUni(passwordPtr);
            Marshal.ZeroFreeGlobalAllocUnicode(passwordPtr);
#else
            DevelopmentStorageAccountWellKnownKey = Utility.SecureStringToByteArray(devStoreKeySecure);
#endif
        }

        internal AzureUtility(FabricEvents.ExtensionsEvents traceSource, string logSourceId)
        {
            this.TraceSource = traceSource;
            this.LogSourceId = logSourceId;
        }

        // Well-known key for development storage account
#if DotNetCoreClr
        internal static string DevelopmentStorageAccountWellKnownKey { get; private set; }
#else
        internal static byte[] DevelopmentStorageAccountWellKnownKey { get; private set; }
#endif

        internal static string DeploymentId
        {
#if DotNetCoreClrLinux
            get { return AzureFileStore.DeploymentId; }
#else
            get { return AzureRegistryStore.DeploymentId; }
#endif
        }

        internal static string RoleInstanceId
        {
#if DotNetCoreClrLinux
            get { return AzureFileStore.RoleInstanceId; }
#else
            get { return AzureRegistryStore.RoleInstanceId; }
#endif
        }

        internal static string RoleName
        {
#if DotNetCoreClrLinux
            get { return AzureFileStore.RoleName; }
#else
            get { return AzureRegistryStore.RoleName; }
#endif
        }

        // Id to use in log messages
        internal string LogSourceId { get; private set; }

        // Object used for tracing
        internal FabricEvents.ExtensionsEvents TraceSource { get; private set; }

        internal static bool IsAzureInterfaceAvailable()
        {
#if DotNetCoreClrLinux
            var deploymentIdExists = !string.IsNullOrEmpty(AzureFileStore.DeploymentId);
            var roleExists = !string.IsNullOrEmpty(AzureFileStore.RoleInstanceId) &&
                             !string.IsNullOrEmpty(AzureFileStore.RoleName);
#else
            var deploymentIdExists = !string.IsNullOrEmpty(AzureRegistryStore.DeploymentId);
            var roleExists = !string.IsNullOrEmpty(AzureRegistryStore.RoleInstanceId) &&
                             !string.IsNullOrEmpty(AzureRegistryStore.RoleName);
#endif
            return deploymentIdExists && roleExists;
        }

        internal StorageAccountFactory GetStorageAccountFactory(IConfigReader configReader, string sectionName, string valueName)
        {
            StorageConnection connection;
            this.GetStorageConnection(configReader, sectionName, valueName, out connection);

            if (connection == null)
            {
                return null;
            }

            return connection.IsDsms 
                ?  this.GetDsmsStorageAccountFactory(connection) 
                : this.GetCloudStorageAccountFactory(connection);
        }

        private StorageAccountFactory GetCloudStorageAccountFactory(StorageConnection connection)
        {
            this.TraceSource.WriteInfo(AzureConstants.DsmsTraceType, "Creating CloudStorageAccountFactory.");
            return new CloudStorageAccountFactory(connection);
        }

        private StorageAccountFactory GetDsmsStorageAccountFactory(StorageConnection connection)
        {
            this.TraceSource.WriteInfo(AzureConstants.DsmsTraceType, "Creating DsmsStorageAccountFactory for path {0}", connection.DsmsSourceLocation);

            try
            {

                var assembly = Assembly.Load("Microsoft.ServiceFabric.Dca.Storage.Dsms");
                var type = assembly.GetType("Microsoft.ServiceFabric.Dca.Storage.Dsms.DsmsStorageAccountFactory");
                var factory = Activator.CreateInstance(type, connection);
                return (StorageAccountFactory)factory;
            }
            catch (Exception ex)
            {
                this.TraceSource.WriteExceptionAsError(AzureConstants.DsmsTraceType, ex);
                throw;
            }
        }

        private bool GetStorageConnection(IConfigReader configReader, string sectionName, string valueName, out StorageConnection storageConnection)
        {
            storageConnection = new StorageConnection();

            bool isEncrypted;
            string newValue;
            if (configReader.IsReadingFromApplicationManifest)
            {
                newValue = configReader.GetUnencryptedConfigValue(
                                sectionName,
                                AzureConstants.AppManifestConnectionStringParamName,
                                String.Empty);
                string connectionStringEncrypted = configReader.GetUnencryptedConfigValue(
                                                     sectionName,
                                                     AzureConstants.ConnectionStringIsEncryptedParamName,
                                                     String.Empty);
                if (false == Boolean.TryParse(
                                connectionStringEncrypted,
                                out isEncrypted))
                {
                    this.TraceSource.WriteError(
                        this.LogSourceId,
                        "Unable to determine whether the connection string in section {0} is encrypted because {1} could not be parsed as a boolean value.",
                        sectionName,
                        connectionStringEncrypted);
                    return false;
                }
            }
            else
            {
                newValue = configReader.ReadString(sectionName, valueName, out isEncrypted);
            }

            if (false == string.IsNullOrEmpty(newValue))
            {
                if (isEncrypted)
                {
                    SecureString secureString = NativeConfigStore.DecryptText(newValue);

                    char[] chars = new char[secureString.Length];
#if DotNetCoreClrLinux
                    IntPtr ptr = SecureStringMarshal.SecureStringToCoTaskMemUnicode(secureString);
#else
                    IntPtr ptr = Marshal.SecureStringToCoTaskMemUnicode(secureString);
#endif
                    try
                    {
                        // copy the unmanaged char array into a managed char array
                        Marshal.Copy(ptr, chars, 0, secureString.Length);
                        if (false == ConnectionStringParser.ParseConnectionString(chars,
#if DotNetCoreClrLinux
                        this.TraceWarning,
#else
                        this.TraceError,
#endif
                            ref storageConnection))
                        {
                            return false;
                        }
                    }
                    finally
                    {
                        Marshal.ZeroFreeCoTaskMemUnicode(ptr);
                        Array.Clear(chars, 0, chars.Length);
                    }
                }
                else
                {
                    if (false == ConnectionStringParser.ParseConnectionString(newValue.ToCharArray(),
#if DotNetCoreClrLinux || DotNetCoreClrIOT
                        this.TraceWarning,
#else
                        this.TraceError,
#endif
                        ref storageConnection))
                    {
                        return false;
                    }
                }
            }

            return true;
        }

        private void TraceError(string errorMessage)
        {
            this.TraceSource.WriteError(
                    this.LogSourceId,
                    errorMessage);
        }

#if DotNetCoreClrLinux || DotNetCoreClrIOT
        private void TraceWarning(string errorMessage)
        {
            this.TraceSource.WriteWarning(
                    this.LogSourceId,
                    errorMessage);
        }
#endif

        internal RetriableOperationExceptionHandler.Response DiagnosticManagerExceptionHandler(Exception e)
        {
            this.TraceSource.WriteError(
                this.LogSourceId,
                "Exception encountered while configuring Azure Diagnostics. Exception information: {0}",
                e);

            if (e is StorageException)
            {
                // If this was a network error, we'll retry later. Else, we'll
                // just give up.
                StorageException storageException = (StorageException)e;
                if (Utility.IsNetworkError(storageException.RequestInformation.HttpStatusCode))
                {
                    return RetriableOperationExceptionHandler.Response.Retry;
                }
            }

            return RetriableOperationExceptionHandler.Response.Abort;
        }
    }
}