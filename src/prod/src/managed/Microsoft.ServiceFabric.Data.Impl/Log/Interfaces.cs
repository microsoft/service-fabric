// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.Log
{
    using System.IO;
    using System.Security.AccessControl;
    using System.Threading;
    using System.Threading.Tasks;

    //*** TODO:
    // 1) OnResolveAliasLogsAsync() and OnReplaceAliasLogsAsync() not tested.
    // 2) Test security/ACL protection
    // 3) Test WaitCapacityNotificationAsync
    // 4) Test WaitBufferFullNotificationAsync
    //      4.1) Not canceled during Close/Abort...
    // 5) Triage remaining TODOs:
    // 7) Generally make sure all exception contracts are clear and work.

    /// <summary>
    /// Each AppDomain has a static instance of a System.Fabric.Data.Log.LogManager class. It is through this
    /// single API that all other Logical Log behavior is exposed indirectly. 
    /// 
    /// In order to gain access to this AppDomain-wide LogManager singleton, a handle to that facility must
    /// first be opened via LogManager.OpenAsync(). Once that handle (ILogManager) is opened and returned
    /// further API access is provided.
    /// </summary>
    public static partial class LogManager
    {
        /// <summary>
        /// Enum for different types of loggers
        /// </summary>
        public enum LoggerType
        {
            /// <summary>
            /// Default logger type
            /// </summary>
            Default,

            /// <summary>
            /// Unknown logger type
            /// </summary>
            Unknown,

            /// <summary>
            /// Driver logger type
            /// </summary>
            Driver,

            /// <summary>
            /// Inproc logger type
            /// </summary>
            Inproc
        }

        /// <summary>
        /// Open access to the Logical Log Manager API
        /// </summary>
        /// <param name="logManagerType">Specifies the type of log manager to use</param>
        /// <param name="cancellationToken">Used to cancel the OpenAsync operation</param>
        /// <returns>Caller's ILogManager (Handle)</returns>
        public static Task<ILogManager> OpenAsync(LogManager.LoggerType logManagerType, CancellationToken cancellationToken)
        {
            return InternalCreateAsync(logManagerType, cancellationToken);
        }
    }

    /// <summary>
    /// A LogManager implementation is the root of the log system object hierarchy within an AppDomain. To 
    /// support isolated/partitioned views into this AppDomain-wide hierarchy, ILogManager handle instances 
    /// are provided via the LogManager.OpenAsync() static method. 
    ///
    /// All other log-related objects are created directly or indirectly through the ILogManager instances.
    /// </summary>
    public interface ILogManager : IDisposable
    {
        /// <summary>
        /// Close the log manager asynchronously
        /// 
        ///  NOTE: Any currently opened IPhysicalLog instances that were created/opened through a current ILogManager
        ///        will remain open - they have an independent life-span once created.
        ///        
        /// Known Exceptions:
        /// 
        /// </summary>
        /// <param name="cancellationToken">Used to cancel the OpenAsync operation</param>
        Task CloseAsync(CancellationToken cancellationToken);

        /// <summary>
        /// Abort the log manager handle synchronously - will occur automatically through GC if CloseAsync is not called.
        /// 
        ///  NOTE: Any currently opened IPhysicalLog instances that were created/opened through a current ILogManager
        ///        will remain open - they have an independent life-span once created.
        ///        
        /// Known Exceptions:
        /// 
        /// </summary>
        void Abort();

        /// <summary>
        /// Create a new physical log after which the container storage structures exists for the new physical log.
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
        /// <param name="pathToCommonContainer">Supplies fully qualified pathname to the logger file</param>
        /// <param name="physicalLogId">Supplies ID (Guid) of the log container to create</param>
        /// <param name="containerSize">Supplies the desired size of the log container in bytes</param>
        /// <param name="maximumNumberStreams">Supplies the maximum number of streams that can be created in the log container</param>
        /// <param name="maximumLogicalLogBlockSize">Supplies the maximum size of a physical log stream block to write in a shared intermediate 
        /// log containers - stream that write blocks larger than this may not use such a shared container</param>
        /// <param name="creationFlags"></param>
        /// <param name="cancellationToken">Used to cancel the CreatePhysicalLogAsync operation</param>
        /// <returns>An open IPhysicalLog handle used to further manipulate the created physical log</returns>
        Task<IPhysicalLog> CreatePhysicalLogAsync(
            string pathToCommonContainer,
            Guid physicalLogId,
            Int64 containerSize,
            UInt32 maximumNumberStreams,
            UInt32 maximumLogicalLogBlockSize,
            LogManager.LogCreationFlags creationFlags,
            CancellationToken cancellationToken);

        /// <summary>
        /// Opens an existing physical log. Once this operation completes, the returned IPhysicalLog represents the 
        /// underlying physical log state that has been recovered and is ready to use.
        /// 
        /// Known Exceptions:
        /// 
        ///     System.Fabric.FabricException
        ///     System.IOException
        ///     System.IO.DirectoryNotFoundException
        ///     System.IO.DriveNotFoundException
        ///     System.IO.FileNotFoundException
        ///     System.IO.PathTooLongException
        ///     FabricObjectClosedException
        ///     
        /// </summary>
        /// <param name="pathToCommonPhysicalLog">Fully qualified pathname to the log container file</param>
        /// <param name="physicalLogId">Supplies ID (Guid) of the log container</param>
        /// <param name="isStagingLog">TRUE if opening as staging log else false</param>
        /// <param name="cancellationToken">Used to cancel the OpenPhysicalLogAsync operation</param>
        /// <returns>An open IPhysicalLog handle used to further manipulate the opened physical log</returns>
        Task<IPhysicalLog> OpenPhysicalLogAsync(
            string pathToCommonPhysicalLog,
            Guid physicalLogId,
            bool isStagingLog,
            CancellationToken cancellationToken);

        /// <summary>
        /// Deletes an existing log container
        /// 
        /// Known Exceptions:
        /// 
        ///     System.Fabric.FabricException
        ///     System.IOException
        ///     System.IO.DirectoryNotFoundException
        ///     System.IO.DriveNotFoundException
        ///     System.IO.FileNotFoundException
        ///     System.IO.PathTooLongException
        ///     FabricObjectClosedException
        ///     
        /// </summary>
        /// <param name="pathToCommonPhysicalLog">Fully qualified pathname to the log container file</param>
        /// <param name="physicalLogId">Supplies ID (Guid) of the log container</param>
        /// <param name="cancellationToken">Used to cancel the DeletePhysicalLogAsync operation</param>
        Task DeletePhysicalLogAsync(
            string pathToCommonPhysicalLog,
            Guid physicalLogId,
            CancellationToken cancellationToken);

        /// <summary>
        /// Returns the type of KtlLogger being used
        /// 
        /// Known Exceptions:
        /// 
        ///     FabricObjectClosedException
        ///     
        /// </summary>
        /// <returns>Type of KtlLogger used</returns>
        LogManager.LoggerType GetLoggerType();
    }


    /// <summary>
    /// An open IPhysicalLog instance is a handle abstraction that provides access and control over the corresponding 
    /// system resources that make up a container for multiple separate logical log instances. 
    ///
    /// Once an application gets a reference to an opened IPhysicalLog instance, the backing physical log is 
    /// already mounted (recovered) and available. 
    /// </summary>
    public interface IPhysicalLog : IDisposable
    {
        /// <summary>
        /// Close a current handle's access to a physical log resource
        /// 
        ///  NOTE: Any currently opened ILogicalLog instances that were created/opened through a current IPhysicalLog
        ///        will remain open - they have an independent life-span once created.
        /// </summary>
        /// <param name="cancellationToken">Used to cancel the CloseAsync operation</param>
        Task CloseAsync(CancellationToken cancellationToken);

        /// <summary>
        /// Abort the physical log handle instance synchronously - will occur automatically through GC if CloseAsync is
        /// not called
        /// 
        ///  NOTE: Any currently opened ILogicalLog instances that were created/opened through a current IPhysicalLog
        ///        will remain open - they have an independent life-span once created.
        /// </summary>
        void Abort();

        /// <summary>
        /// Determine if the underlying resources that contain the data of a physical log are in a known-good
        /// state
        /// </summary>
        bool IsFunctional { get; }

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
        /// <param name="traceType">Supplies the type string to use when tracing</param>
        /// <param name="cancellationToken">Used to cancel the CreateLogicalLogAsync operation</param>
        /// <returns>The open ILogicalLog (handle) used to further manipulate the related logical log resources</returns>
        Task<ILogicalLog> CreateLogicalLogAsync(
            Guid logicalLogId,
            string optionalLogStreamAlias,
            string path,
            FileSecurity optionalSecurityInfo,
            Int64 maximumSize,
            UInt32 maximumBlockSize,
            LogManager.LogCreationFlags creationFlags,
            string traceType,
            CancellationToken cancellationToken);

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
        /// <param name="traceType">Supplies the type string to use when tracing</param>
        /// <param name="cancellationToken">Used to cancel the OpenLogicalLogAsync operation</param>
        /// <returns>The open ILogicalLog (handle) used to further manipulate the related logical log resources</returns>
        Task<ILogicalLog> OpenLogicalLogAsync(Guid logicalLogId, string traceType, CancellationToken cancellationToken);

        /// <summary>
        /// Delete an existing logical log
        /// 
        /// Known Exceptions:
        /// 
        ///     System.Fabric.FabricException
        ///     System.IOException
        ///     System.IO.DirectoryNotFoundException
        ///     System.IO.DriveNotFoundException
        ///     System.IO.FileNotFoundException
        ///     System.IO.PathTooLongException
        ///     FabricObjectClosedException
        ///     
        /// NOTE: This operation is only valid iff the logical log is not currently open.
        /// </summary>
        /// <param name="logicalLogId">Supplies the logical log's unique ID (Guid)</param>
        /// <param name="cancellationToken">Used to cancel the DeleteLogicalLogAsync operation</param>
        Task DeleteLogicalLogAsync(Guid logicalLogId, CancellationToken cancellationToken);

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
        Task AssignAliasAsync(Guid logicalLogId, string alias, CancellationToken cancellationToken);

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
        Task<Guid> ResolveAliasAsync(string alias, CancellationToken cancellationToken);

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
        Task RemoveAliasAsync(string alias, CancellationToken cancellationToken);

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
        Task ReplaceAliasLogsAsync(
            string sourceLogAliasName,
            string logAliasName,
            string backupLogAliasName,
            CancellationToken cancellationToken);

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
        Task<Guid> RecoverAliasLogsAsync(
            string sourceLogAliasName,
            string logAliasName,
            string backupLogAliasName,
            CancellationToken cancellationToken);
    }


    /// <summary>
    /// A logical log represents a sequence of logged bytes via simple streaming semantics. An application can 
    /// use an ILogicalLog handle instance to write and read log records - format defined entirely by that user. 
    /// ILogicalLogs are also used to truncate a log stream - on either end - Head or Tail. New bytes are Appended 
    /// (written) only at the Tail and the oldest bytes are at the Head.
    ///
    /// Logical Logs may be physically multiplexed into a single log container (aka physical log), but they are 
    /// logically separate with their own 63-bit address space.
    /// </summary>
    public interface ILogicalLog : IDisposable
    {
        /// <summary>
        /// Close access to a current logical log and release any associated resources
        /// 
        /// Known Exceptions:
        /// 
        ///     System.Fabric.FabricException
        ///     FabricObjectClosedException
        ///     System.IOException
        ///     
        /// </summary>
        /// <param name="cancellationToken">Used to cancel the CloseAsync operation</param>
        /// <returns></returns>
        Task CloseAsync(CancellationToken cancellationToken);

        /// <summary>
        /// Abort (rude close) the logical log use synchronously - will occur automatically through GC if CloseAsync is not called
        /// </summary>
        void Abort();

        /// <summary>
        /// Determine if a current ILogicalLog and underlying resources are functional and available
        /// </summary>
        bool IsFunctional { get; }

        /// <summary>
        /// Size of log stream's contents - only non-truncated space
        /// </summary>
        long Length { get; }

        /// <summary>
        /// Return the next stream-space address to be written into
        /// </summary>
        long WritePosition { get; }

        /// <summary>
        /// Return the next stream-space to be read from
        /// </summary>
        long ReadPosition { get; }

        /// <summary>
        /// Return the current Head truncation stream-space location - no information logically exists at and 
        /// below this point in the logical log
        /// </summary>
        long HeadTruncationPosition { get; }

        /// <summary>
        /// Return the maximum block size available for a physical record
        /// </summary>
        long MaximumBlockSize { get; }

        /// <summary>
        /// Return the size of the header used in the metadata block
        /// </summary>
        uint MetadataBlockHeaderSize { get; }

        /// <summary>
        /// Return an abstract Stream reference for this ILogicalLog optimized for sequential or random reads
        /// </summary>
        /// <param name="SequentialAccessReadSize">Number of bytes to read and cache on each read request for sequential access. Zero for random access with no read ahead.</param>
        Stream CreateReadStream(int SequentialAccessReadSize);

        /// <summary>
        /// Set the read size for a sequential access stream
        /// </summary>
        /// <param name="LogStream">Sequential access LogicalLogStream on which to set the size</param>
        /// <param name="SequentialAccessReadSize">Number of bytes to read and cache on each read request. Zero for random access with no read ahead.</param>
        void SetSequentialAccessReadSize(Stream LogStream, int SequentialAccessReadSize);

        /// <summary>
        ///  Read a sequence of bytes from the current logical log, advancing the ReadPosition within the stream by the 
        ///  number of bytes read
        /// 
        /// Known Exceptions:
        /// 
        ///     System.Fabric.FabricException
        ///     FabricObjectClosedException
        ///     System.IOException
        ///     ArgumentOutOfRangeException
        ///     InvalidDataException
        ///     
        /// </summary>
        /// <param name="buffer">buffer to receive the bytes read</param>
        /// <param name="offset">offset into buffer to start the transfer</param>
        /// <param name="count">length of desired read</param>
        /// <param name="bytesToRead">number of bytes to read from log</param>
        /// <param name="cancellationToken">Used to cancel the ReadAsync operation</param>
        /// <returns>The number of bytes actually read - a zero indicates end-of-stream (WritePosition)</returns>
        Task<int> ReadAsync(byte[] buffer, int offset, int count, int bytesToRead, CancellationToken cancellationToken);

        /// <summary>
        /// Set the ReadPosition within the current ILogicalLog
        /// 
        /// Known Exceptions:
        /// 
        ///     System.Fabric.FabricException
        ///     FabricObjectClosedException
        ///     System.IOException
        ///     ArgumentOutOfRangeException
        /// 
        /// </summary>
        /// <param name="offset">Offset to the new ReadPosition</param>
        /// <param name="origin">Starting position: current, start, or end</param>
        /// <returns>New ReadLocation</returns>
        long SeekForRead(long offset, SeekOrigin origin);

        /// <summary>
        /// Write a sequence of bytes to the current stream at WriteLocation, advancing the position within 
        /// this stream by the number of bytes written
        /// 
        /// Known Exceptions:
        /// 
        ///     System.Fabric.FabricException
        ///     FabricObjectClosedException
        ///     System.IOException
        ///     
        /// </summary>
        /// <param name="buffer">source buffer for the bytes to be written</param>
        /// <param name="offset">offset into the source buffer at which the write starts</param>
        /// <param name="count">Size of the write</param>
        /// <param name="cancellationToken">Used to cancel the AppendAsync operation</param>
        Task AppendAsync(byte[] buffer, int offset, int count, CancellationToken cancellationToken);

        /// <summary>
        /// Cause any new buffered data from AppendAsync() to be written to the underlying device
        /// 
        /// Known Exceptions:
        /// 
        ///     System.Fabric.FabricException
        ///     FabricObjectClosedException
        ///     System.IOException
        ///     
        /// </summary>
        /// <param name="cancellationToken">Used to cancel the FlushAsync</param>
        Task FlushAsync(CancellationToken cancellationToken);

        /// <summary>
        /// Cause any new buffered data from AppendAsync() to be written to the underlying device along with the
        /// barrier flag marking the end of a logical log record
        /// 
        /// Known Exceptions:
        /// 
        ///     System.Fabric.FabricException
        ///     FabricObjectClosedException
        ///     System.IOException
        ///     
        /// </summary>
        /// <param name="cancellationToken">Used to cancel the FlushWithMarkerAsync</param>
        Task FlushWithMarkerAsync(CancellationToken cancellationToken);

        /// <summary>
        /// Trigger the background truncation of the stream-space [0, StreamOffset]
        /// 
        /// Known Exceptions:
        /// 
        ///     System.Fabric.FabricException
        ///     FabricObjectClosedException
        ///     System.IOException
        ///     ArgumentOutOfRangeException
        ///     
        /// </summary>
        /// <param name="StreamOffset">New HeadTruncationPosition</param>
        void TruncateHead(long StreamOffset);

        /// <summary>
        /// Truncation the stream-space [StreamOffset, WritePosition] - repositions WritePosition
        /// 
        /// Known Exceptions:
        /// 
        ///     System.Fabric.FabricException
        ///     FabricObjectClosedException
        ///     System.IOException
        ///     ArgumentOutOfRangeException
        ///     
        /// </summary>
        /// <param name="StreamOffset">New WritePosition</param>
        /// <param name="cancellationToken">Used to cancel the TruncateTail</param>
        Task TruncateTail(long StreamOffset, CancellationToken cancellationToken);

        /// <summary>
        /// Asynchronously wait for a given percentage of the logical log's stream-space to be used
        /// 
        /// Known Exceptions:
        /// 
        ///     System.Fabric.FabricException
        ///     FabricObjectClosedException
        ///     
        /// </summary>
        /// <param name="percentOfSpaceUsed">Percentage full trigger point</param>
        /// <param name="cancellationToken">Used to cancel the WaitCapacityNotificationAsync</param>
        Task WaitCapacityNotificationAsync(uint percentOfSpaceUsed, CancellationToken cancellationToken);

        /// <summary>
        /// Asynchronously wait for the write buffer of a current logical log to come close to full
        /// 
        /// Known Exceptions:
        /// 
        ///     NotImplementedException
        ///     
        /// </summary>
        /// <param name="cancellationToken">Used to cancel the WaitBufferFullNotificationAsync</param>
        Task WaitBufferFullNotificationAsync(CancellationToken cancellationToken);

        /// <summary>
        /// Asynchronously configure logical log to use dedicated log only
        /// 
        /// Known Exceptions:
        /// 
        ///     System.Fabric.FabricException
        ///     FabricObjectClosedException
        ///     
        /// </summary>
        Task ConfigureWritesToOnlyDedicatedLogAsync(CancellationToken cancellationToken);

        /// <summary>
        /// Asynchronously configure logical log to use both shared and dedicated log
        /// 
        /// Known Exceptions:
        /// 
        ///     System.Fabric.FabricException
        ///     FabricObjectClosedException
        ///     
        /// </summary>
        Task ConfigureWritesToSharedAndDedicatedLogAsync(CancellationToken cancellationToken);

        /// <summary>
        /// Asynchronously query the percentagee usage of the logical log
        /// 
        /// Known Exceptions:
        /// 
        ///     System.Fabric.FabricException
        ///     FabricObjectClosedException
        ///     
        /// </summary>
        Task<uint> QueryLogUsageAsync(CancellationToken cancellationToken);
    }
}