// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data
{
    using System;
    using System.Fabric;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// <cref name="RestoreContext"/> contains the <cref name="RestoreContext.RestoreAsync(RestoreDescription)"/> that can be used to restore the state of the replica from a backup. 
    /// </summary>
    public struct RestoreContext
    {
        private readonly IStateProviderReplica stateProviderReplica;

        /// <summary>
        /// Initializes a new instance of the <cref name="RestoreContext"/> structure.
        /// </summary>
        /// <param name="stateProviderReplica">
        /// An <see cref="IStateProviderReplica"/> representing a reliable state provider replica.
        /// </param>
        public RestoreContext(IStateProviderReplica stateProviderReplica)
        {
            this.stateProviderReplica = stateProviderReplica;
        }

        /// <summary>
        /// Restores a backup described by <see cref="RestoreDescription"/>.
        /// </summary>
        /// <param name="restoreDescription">Description for the restore request.</param>
        /// <exception cref="System.Fabric.FabricMissingFullBackupException">
        /// Indicates that the input backup folder does not contain a full backup.
        /// For a backup folder to be restorable, it must contain exactly one full backup and any number of incremental backups.
        /// </exception>
        /// <exception cref="System.ArgumentException">
        /// Indicates that one of the arguments is not valid. For example, when restoring a Reliable Service if RestorePolicy is set to Safe, 
        /// but the input backup folder contains a version of the state that is older than the state maintained in the current replica.
        /// 
        /// When restoring an Actor Service this exception is thrown if specified <see cref="Microsoft.ServiceFabric.Data.RestoreDescription.BackupFolderPath"/>
        /// is empty.
        /// </exception>
        /// <exception cref="System.IO.DirectoryNotFoundException">
        /// Indicates that the supplied restore directory does not exist.
        /// </exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">
        /// Indicates that the replica is closing.
        /// </exception>
        /// <exception cref="System.InvalidOperationException">
        /// Indicates that current restore operation is not valid. For example, the <see cref="System.Fabric.ServicePartitionKind"/> 
        /// of the partition from where backup was taken is different than that of current partition being restored.
        /// </exception>
        /// <exception cref="System.IO.FileNotFoundException">
        /// Indicates the expected backup files under the supplied restore directory is not found.
        /// </exception>
        /// <exception cref="System.Fabric.FabricException">
        /// Indicates either the restore operation encountered an unexpected error or the backup files in restore directory are not valid.
        /// The <see cref="System.Fabric.FabricException.ErrorCode"/> property indicates the type of error that occurred.
        /// <list type="bullet">
        ///     <item>
        ///         <term><see cref="System.Fabric.FabricErrorCode.InvalidBackup"/></term>
        ///         <description>
        ///         Indicates that the backup files supplied in the restore directory are either missing files or have extra unexpected files.
        ///         </description>
        ///     </item>
        ///     <item>
        ///         <term><see cref="System.Fabric.FabricErrorCode.InvalidRestoreData"/></term>
        ///         <description>
        ///         Indicates that metadata files (restore.dat) present in restore directory is either corrupt or contains invalid information.
        ///         </description>
        ///     </item>
        ///     <item>
        ///         <term><see cref="System.Fabric.FabricErrorCode.InvalidBackupChain"/></term>
        ///         <description>
        ///         Indicates that the backup chain (i.e. one full backup and zero or more contiguous incremental backups that were taken after it) 
        ///         supplied in the restore directory is broken. 
        ///         </description>
        ///     </item>
        ///     <item>
        ///         <term><see cref="System.Fabric.FabricErrorCode.DuplicateBackups"/></term>
        ///         <description>
        ///         Indicates that the backup chain (i.e. one full backup and zero or more contiguous incremental backups that were taken after it) 
        ///         supplied in the restore directory contains duplicate backups. 
        ///         </description>
        ///     </item>
        ///     <item>
        ///         <term><see cref="System.Fabric.FabricErrorCode.RestoreSafeCheckFailed"/></term>
        ///         <description>
        ///         If <see cref="Microsoft.ServiceFabric.Data.RestorePolicy.Safe"/> is specified as part of <see cref="Microsoft.ServiceFabric.Data.RestoreDescription"/>, it 
        ///         indicates that the backup provided for restore has older data than currently present in service.
        ///         </description>
        ///     </item>
        /// </list>
        /// </exception>
        /// <returns>
        /// Task that represents the asynchronous restore operation.
        /// </returns>
        /// <remarks>
        /// This API must be called from OnDataLossAsync method. Only one RestoreAsync API can be inflight per replica at any given point of time.
        /// 
        /// Note that exceptions thrown by this API differ depending on of underlying state provider. The exceptions that are currently documented for
        /// this API applies only to out-of-box state providers provided by Service Fabric for Reliable Services and Reliable Actors.
        /// <para>
        /// Following exceptions are thrown by this API when invoked in Reliable Service:
        /// <list type="bullet">
        ///     <item>
        ///         <description><see cref="System.Fabric.FabricMissingFullBackupException"/></description>
        ///     </item>
        ///     <item>
        ///         <description><see cref="System.ArgumentException"/></description>
        ///     </item>
        /// </list>
        /// </para>
        /// <para>
        /// Following exceptions are thrown by this API when invoked in Actor Service with KvsActorStateProvider as its state provider (which is the
        /// default state provider for Reliable Actors):
        /// <list type="bullet">
        ///     <item>
        ///         <description><see cref="System.ArgumentException"/></description>
        ///     </item>
        ///     <item>
        ///         <description><see cref="System.IO.DirectoryNotFoundException"/></description>
        ///     </item>
        ///     <item>
        ///         <description><see cref="System.Fabric.FabricObjectClosedException"/></description>
        ///     </item>
        ///     <item>
        ///         <description><see cref="System.InvalidOperationException"/></description>
        ///     </item>
        ///     <item>
        ///         <description><see cref="System.IO.FileNotFoundException"/></description>
        ///     </item>
        ///     <item>
        ///         <description><see cref="System.Fabric.FabricException"/></description>
        ///     </item>
        /// </list>
        /// </para>
        /// </remarks>
        public Task RestoreAsync(RestoreDescription restoreDescription)
        {
            return this.stateProviderReplica.RestoreAsync(
                restoreDescription.BackupFolderPath, 
                restoreDescription.Policy, 
                CancellationToken.None);
        }

        /// <summary>
        /// Restore a backup described by <see cref="RestoreDescription"/>.
        /// </summary>
        /// <param name="restoreDescription">Description for the restore request.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <exception cref="FabricMissingFullBackupException">
        /// Indicates that the input backup folder does not contain a full backup.
        /// For a backup folder to be restorable, it must contain exactly one full backup and any number of incremental backups.
        /// </exception>
        /// <exception cref="System.ArgumentException">
        /// Indicates that one of the arguments is not valid. For example, when restoring a Reliable Service if RestorePolicy is set to Safe, 
        /// but the input backup folder contains a version of the state that is older than the state maintained in the current replica.
        /// 
        /// When restoring an Actor Service this exception is thrown if specified <see cref="Microsoft.ServiceFabric.Data.RestoreDescription.BackupFolderPath"/>
        /// is empty.
        /// </exception>
        /// <exception cref="System.IO.DirectoryNotFoundException">
        /// Indicates that the supplied restore directory does not exist.
        /// </exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">
        /// Indicates that the replica is closing.
        /// </exception>
        /// <exception cref="System.InvalidOperationException">
        /// Indicates that current restore operation is not valid. For example, the <see cref="System.Fabric.ServicePartitionKind"/> 
        /// of the partition from where backup was taken is different than that of current partition being restored.
        /// </exception>
        /// <exception cref="System.IO.FileNotFoundException">
        /// Indicates the expected backup files under the supplied restore directory is not found.
        /// </exception>
        /// <exception cref="System.Fabric.FabricException">
        /// Indicates either the restore operation encountered an unexpected error or the backup files in restore directory are not valid.
        /// The <see cref="System.Fabric.FabricException.ErrorCode"/> property indicates the type of error that occurred.
        /// <list type="bullet">
        ///     <item>
        ///         <term><see cref="System.Fabric.FabricErrorCode.InvalidBackup"/></term>
        ///         <description>
        ///         Indicates that the backup files supplied in the restore directory are either missing files or have extra unexpected files.
        ///         </description>
        ///     </item>
        ///     <item>
        ///         <term><see cref="System.Fabric.FabricErrorCode.InvalidRestoreData"/></term>
        ///         <description>
        ///         Indicates that metadata files (restore.dat) present in restore directory is either corrupt or contains invalid information.
        ///         </description>
        ///     </item>
        ///     <item>
        ///         <term><see cref="System.Fabric.FabricErrorCode.InvalidBackupChain"/></term>
        ///         <description>
        ///         Indicates that the backup chain (i.e. one full backup and zero or more contiguous incremental backups that were taken after it) 
        ///         supplied in the restore directory is broken. 
        ///         </description>
        ///     </item>
        ///     <item>
        ///         <term><see cref="System.Fabric.FabricErrorCode.DuplicateBackups"/></term>
        ///         <description>
        ///         Indicates that the backup chain (i.e. one full backup and zero or more contiguous incremental backups that were taken after it) 
        ///         supplied in the restore directory contains duplicate backups. 
        ///         </description>
        ///     </item>
        ///     <item>
        ///         <term><see cref="System.Fabric.FabricErrorCode.RestoreSafeCheckFailed"/></term>
        ///         <description>
        ///         If <see cref="Microsoft.ServiceFabric.Data.RestorePolicy.Safe"/> is specified as part of <see cref="Microsoft.ServiceFabric.Data.RestoreDescription"/>, it 
        ///         indicates that the backup provided for restore has older data than currently present in service.
        ///         </description>
        ///     </item>
        /// </list>
        /// </exception>
        /// <returns>
        /// Task that represents the asynchronous restore operation.
        /// </returns>
        /// <remarks>
        /// This API must be called from OnDataLossAsync method. Only one RestoreAsync API can be inflight per replica at any given point of time.
        /// 
        /// Note that exceptions thrown by this API differ depending on of underlying state provider. The exceptions that are currently documented for
        /// this API applies only to out-of-box state providers provided by Service Fabric for Reliable Services and Reliable Actors.
        /// <para>
        /// Following exceptions are thrown by this API when invoked in Reliable Service:
        /// <list type="bullet">
        ///     <item>
        ///         <description><see cref="System.Fabric.FabricMissingFullBackupException"/></description>
        ///     </item>
        ///     <item>
        ///         <description><see cref="System.ArgumentException"/></description>
        ///     </item>
        /// </list>
        /// </para>
        /// <para>
        /// Following exceptions are thrown by this API when invoked in Actor Service with KvsActorStateProvider as its state provider (which is the
        /// default state provider for Reliable Actors):
        /// <list type="bullet">
        ///     <item>
        ///         <description><see cref="System.ArgumentException"/></description>
        ///     </item>
        ///     <item>
        ///         <description><see cref="System.IO.DirectoryNotFoundException"/></description>
        ///     </item>
        ///     <item>
        ///         <description><see cref="System.Fabric.FabricObjectClosedException"/></description>
        ///     </item>
        ///     <item>
        ///         <description><see cref="System.InvalidOperationException"/></description>
        ///     </item>
        ///     <item>
        ///         <description><see cref="System.IO.FileNotFoundException"/></description>
        ///     </item>
        ///     <item>
        ///         <description><see cref="System.Fabric.FabricException"/></description>
        ///     </item>
        /// </list>
        /// </para>
        /// </remarks>
        public Task RestoreAsync(RestoreDescription restoreDescription, CancellationToken cancellationToken)
        {
            return this.stateProviderReplica.RestoreAsync(
                restoreDescription.BackupFolderPath, 
                restoreDescription.Policy, 
                cancellationToken);
        }
    }
}