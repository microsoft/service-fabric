// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Log
    {
        /// <summary>
        /// An open IPhysicalLog instance is a handle abstraction that provides access and control over the corresponding 
        /// system resources that make up a container for multiple separate logical log instances. 
        ///
        /// Once an application gets a reference to an opened IPhysicalLog instance, the backing physical log is 
        /// already mounted (recovered) and available. 
        /// </summary>
        interface IPhysicalLogHandle 
            : public Utilities::IDisposable
        {
            K_SHARED_INTERFACE(IPhysicalLogHandle)
            
        public:

            /// <summary>
            /// Close a current handle's access to a physical log resource
            /// 
            ///  NOTE: Any currently opened ILogicalLog instances that were created/opened through a current IPhysicalLog
            ///        will remain open - they have an independent life-span once created.
            /// </summary>
            /// <param name="cancellationToken">Used to cancel the CloseAsync operation</param>
            virtual ktl::Awaitable<NTSTATUS> CloseAsync(__in ktl::CancellationToken const & cancellationToken) = 0;

            /// <summary>
            /// Abort the physical log handle instance synchronously - will occur automatically through GC if CloseAsync is
            /// not called
            /// 
            ///  NOTE: Any currently opened ILogicalLog instances that were created/opened through a current IPhysicalLog
            ///        will remain open - they have an independent life-span once created.
            /// </summary>
            virtual VOID Abort() = 0;

            /// <summary>
            /// Create a new logical log
            /// 
            /// Known Exceptions:
            /// 
            ///     System.Fabric.FabricException
            ///     System.IOException
            ///     System.IO.DirectoryNotFoundException
            ///     System.IO.DriveNotFoundException
            ///     System.IO.FileNotFoundException
            ///     System.IO.PathTooLongException
            ///     UnauthorizedAccessException
            ///     FabricObjectClosedException
            ///     
            /// </summary>
            /// <param name="logicalLogId">Supplies the logical log's unique ID (Guid)</param>
            /// <param name="optionalLogStreamAlias">An optional string that may serve as an alias for 
            /// the logicalLogId</param>
            /// <param name="path">Required location of the logical log's private store</param>
            /// <param name="optionalSecurityInfo"> Contains the security information to associate with the logical log being created</param>
            /// <param name="maximumSize">Supplies the maximum physical size of all record data and metadata that can be stored in the logical log</param>
            /// <param name="maximumBlockSize">Supplies the default maximum size of record data buffered and written to the backing logical log store</param>
            /// <param name="creationFlags"></param>
            /// <param name="cancellationToken">Used to cancel the CreateAndOpenLogicalLogAsync operation</param>
            /// <returns>The open ILogicalLog (handle) used to further manipulate the related logical log resources</returns>
            virtual ktl::Awaitable<NTSTATUS> CreateAndOpenLogicalLogAsync(
                __in KGuid const & logicalLogId,
                __in_opt KString::CSPtr & optionalLogStreamAlias,
                __in KString const & path,
                __in_opt KBuffer::SPtr securityDescriptor, // todo: is a SecurityDescriptor (was FileSecurity in managed).  We may want to create a strong KTL type for this backed by a KBuffer.
                __in LONGLONG maximumSize,
                __in ULONG maximumBlockSize,
                __in LogCreationFlags creationFlags,
                __in ktl::CancellationToken const & cancellationToken,
                __out ILogicalLog::SPtr& logicalLog) = 0;

            /// <summary>
            /// Open an existing logical log
            /// 
            /// Known Exceptions:
            /// 
            ///     System.Fabric.FabricException
            ///     System.IOException
            ///     System.IO.DirectoryNotFoundException
            ///     System.IO.DriveNotFoundException
            ///     System.IO.FileNotFoundException
            ///     System.IO.PathTooLongException
            ///     UnauthorizedAccessException
            ///     FabricObjectClosedException
            ///     
            /// NOTE: A given logical log may only be opened through one IPhysicalLog instance at a time - and only
            ///       once on that IPhysicalLog (i.e. open is exclusive).
            /// </summary>
            /// <param name="logicalLogId">Supplies the logical log's unique ID (Guid)</param>
            /// <param name="cancellationToken">Used to cancel the OpenLogicalLogAsync operation</param>
            /// <returns>The open ILogicalLog (handle) used to further manipulate the related logical log resources</returns>
            virtual ktl::Awaitable<NTSTATUS> OpenLogicalLogAsync(
                __in KGuid const & logicalLogId,
                __in ktl::CancellationToken const & cancellationToken,
                __out ILogicalLog::SPtr& logicalLog) = 0;

            // Delete the logical and maybe the physical log.  BUG: 10286784
            virtual ktl::Awaitable<NTSTATUS> DeleteLogicalLogAsync(
                __in KGuid const & logicalLogId,
                __in ktl::CancellationToken const & cancellationToken) = 0;
            // Delete only the logical log
            virtual ktl::Awaitable<NTSTATUS> DeleteLogicalLogOnlyAsync(
                __in KGuid const & logicalLogId,
                __in ktl::CancellationToken const & cancellationToken) = 0;

            /// <summary>
            /// Create a named alias for a given logical log
            /// 
            /// This method asynchronously assigns a string that serves as an alias to the logicalLogId. The alias can be
            /// resolved to the logicalLogId Guid using the ResolveAliasAsync() method. The alias can be removed using the 
            /// RemoveAliasAsync() method.
            ///
            /// If the stream already has an alias that alias is overwritten.
            /// 
            /// Known Exceptions:
            /// 
            ///     System.Fabric.FabricException
            ///     FabricObjectClosedException
            ///     
            /// </summary>
            /// <param name="logicalLogId">Supplies the logical log's unique ID (Guid)</param>
            /// <param name="alias">Alias to associate with the Log stream id</param>
            /// <param name="cancellationToken">Used to cancel the AssignAliasAsync operation</param>
            virtual ktl::Awaitable<NTSTATUS> AssignAliasAsync(
                __in KGuid const & logicalLogId,
                __in KString const & alias,
                __in ktl::CancellationToken const & cancellationToken) = 0;

            /// <summary>
            /// Resolves a supplied alias to the corresponding logicalLogId Guid if that alias had been previously 
            /// set via the AssignAliasAsync() or CreateLogStreamAsync() methods.
            /// 
            /// Known Exceptions:
            /// 
            ///     System.Fabric.FabricException
            ///     FabricObjectClosedException
            ///     
            /// </summary>
            /// <param name="alias">Alias associated with the Log stream id</param>
            /// <param name="cancellationToken">Used to cancel the ResolveAliasAsync operation</param>
            /// <returns>log stream id associated with the alias</returns>
            virtual ktl::Awaitable<NTSTATUS> ResolveAliasAsync(
                __in KString const & alias,
                __in ktl::CancellationToken const & cancellationToken,
                __out KGuid& logicalLogId) = 0;

            /// <summary>
            /// Removes an alias to the logicalLogId binding
            /// 
            /// Known Exceptions:
            /// 
            ///     System.Fabric.FabricException
            ///     FabricObjectClosedException
            ///     
            /// </summary>
            /// <param name="alias">Alias associated with the Log stream id</param>
            /// <param name="cancellationToken">Used to cancel the RemoveAliasAsync operation</param>
            virtual ktl::Awaitable<NTSTATUS> RemoveAliasAsync(
                __in KString const & alias,
                __in ktl::CancellationToken const & cancellationToken) = 0;

            /// <summary>
            /// Replaces the associated log for an alias (logAliasName) with another log (sourceLogAliasName) 
            /// setting a third log alias (backupLogAliasName) to the existing Guid - deleting any old 
            /// backupLogAliasName log and removing sourceLogAliasName. 
            /// 
            /// Known Exceptions:
            /// 
            ///     System.Fabric.FabricException
            ///     FabricObjectClosedException
            ///     System.IOException
            ///     System.IO.DirectoryNotFoundException
            ///     System.IO.DriveNotFoundException
            ///     System.IO.FileNotFoundException
            ///     System.IO.PathTooLongException
            ///     FabricObjectClosedException
            ///     
            /// </summary>
            /// <param name="sourceLogAliasName">alias log to become assigned to logAliasName</param>
            /// <param name="logAliasName">subject alias name</param>
            /// <param name="backupLogAliasName">old contents of logAliasName before the operation</param>
            /// <param name="cancellationToken">Used to cancel the ReplaceAliasLogsAsync operation</param>
            virtual ktl::Awaitable<NTSTATUS> ReplaceAliasLogsAsync(
                __in KString const & sourceLogAliasName,
                __in KString const & logAliasName,
                __in KString const & backupLogAliasName,
                __in ktl::CancellationToken const & cancellationToken) = 0;

            /// <summary>
            /// Recovers and cleans up after corresponding ReplaceAliasLogsAsync() calls
            /// 
            /// Known Exceptions:
            /// 
            ///     System.Fabric.FabricException
            ///     FabricObjectClosedException
            ///     
            /// </summary>
            /// <param name="sourceLogAliasName">same value passed to corresponding ReplaceAliasLogsAsync() call</param>
            /// <param name="logAliasName">same value passed to corresponding ReplaceAliasLogsAsync() call</param>
            /// <param name="backupLogAliasName">same value passed to corresponding ReplaceAliasLogsAsync() call</param>
            /// <param name="cancellationToken">Used to cancel the RecoverAliasLogsAsync operation</param>
            virtual ktl::Awaitable<NTSTATUS> RecoverAliasLogsAsync(
                __in KString const & sourceLogAliasName,
                __in KString const & logAliasName,
                __in KString const & backupLogAliasName,
                __in ktl::CancellationToken const & cancellationToken,
                __out KGuid& logicalLogId) = 0;
        };
    }
}
