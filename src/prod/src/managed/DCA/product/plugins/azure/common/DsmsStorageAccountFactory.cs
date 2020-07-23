//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

namespace Microsoft.ServiceFabric.Dca.Storage.Dsms
{
    using FabricDCA;
    using Microsoft.WindowsAzure.Security.CredentialsManagement.Client;
    using Microsoft.WindowsAzure.Storage;
    using System;
    using System.Fabric.Common.Tracing;

    /// <summary>
    /// Represents the storage account connection managed by dSMS
    /// </summary>
    public class DsmsStorageAccountFactory : StorageAccountFactory
    {
        private readonly DsmsStorageCredentials dsmsCredentials;
        private readonly FabricEvents.ExtensionsEvents traceSource;
        private const string TraceType = "Dca.Dsms";

        public DsmsStorageAccountFactory(StorageConnection connection) : base (connection)
        {
            this.traceSource = new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.FabricDCA);

            var sourceLocation = connection.DsmsSourceLocation;
            this.traceSource.WriteInfo(TraceType, "Invoking ClientInitialize and creating DsmsStorageCredentials instance for source location: {0} and EndPointSuffix: {1}", sourceLocation, connection.DsmsEndpointSuffix);

            try
            {
                if (connection.DsmsAutopilotServiceName == null)
                {
                    CredentialManagementClient.Instance.ClientInitialize();
                }
                else
                {
                    CredentialManagementClient.Instance.ClientInitializeForAp(connection.DsmsAutopilotServiceName);
                }

                this.dsmsCredentials = DsmsStorageCredentials.GetStorageCredentials(sourceLocation);
            }
            catch (Exception ex)
            {
                this.traceSource.WriteExceptionAsError(TraceType, ex);
                throw;
            }
        }

        public override CloudStorageAccount GetStorageAccount()
        {
            try
            {
                return this.dsmsCredentials.GetCloudStorageAccount(Connection.DsmsEndpointSuffix, true);
            }
            catch (Exception ex)
            {
                this.traceSource.WriteExceptionAsError(TraceType, ex);
                throw;
            }
        }
    }
}