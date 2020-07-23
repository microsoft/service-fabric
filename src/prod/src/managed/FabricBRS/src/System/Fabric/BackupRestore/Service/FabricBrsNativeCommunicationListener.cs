// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Fabric.Common;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.ServiceFabric.Services.Communication.Runtime;

namespace System.Fabric.BackupRestore.Service
{
    /// <summary>
    /// Native communication listener for handling communication with Backup Agent
    /// </summary>
    public class FabricBrsNativeCommunicationListener : ICommunicationListener
    {
        /// <summary>
        /// Trace type string used by traces of this service.
        /// </summary>
        private const string TraceType = "FabricBrsNativeCommunicationListener";

        private readonly StatefulServiceContext _serviceContext;

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="serviceContext">Service context instance</param>
        public FabricBrsNativeCommunicationListener(ServiceContext serviceContext)
        {
            this._serviceContext = serviceContext as StatefulServiceContext;
        }

        /// <summary>
        /// Implementation for the backup restore service which exposes APIs which the Backup Agent would invoke
        /// </summary>
        internal BackupRestoreServiceImpl Impl { get; set; }

        /// <summary>
        /// Abort the communication listener
        /// </summary>
        public void Abort()
        {
            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "Abort invoked");
            this.CloseAsync(CancellationToken.None).Wait();
        }

        /// <summary>
        /// Close the communication listener asynchronously
        /// </summary>
        /// <param name="cancellationToken">Cancellation token</param>
        /// <returns>Return an asynchronous task</returns>
        public Task CloseAsync(CancellationToken cancellationToken)
        {
            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "CloseAsync invoked");
            Program.ServiceAgent.UnregisterBackupRestoreService(
                this._serviceContext.PartitionId,
                this._serviceContext.ReplicaId);

            return Task.FromResult(0);
        }

        /// <summary>
        /// Opens the native communication listener
        /// </summary>
        /// <param name="cancellationToken">Cancellation Token</param>
        /// <returns>Returns the asynchronous task with the endpoint for this listener</returns>
        public Task<string> OpenAsync(CancellationToken cancellationToken)
        {
            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "Enter OpenAsync for communication listener");
            var serviceEndpoint = Program.ServiceAgent.RegisterBackupRestoreService(
                this._serviceContext.PartitionId,
                this._serviceContext.ReplicaId,
                this.Impl);
            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "Endpoint returned: {0}", serviceEndpoint);
            return Task.FromResult(serviceEndpoint);
        }
    }
}