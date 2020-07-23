// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageStore
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common.Tracing;
    using System.Fabric.ImageStore;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.Fabric.Query;
    using System.IO;
    using System.Net;
    using System.Text;
    using System.Threading;
    using System.Xml;
    using System.Fabric.Common;
    using System.Fabric.Strings;
    using Microsoft.WindowsAzure.Storage.Blob;
    using Microsoft.WindowsAzure.Storage.RetryPolicies;
    using Microsoft.WindowsAzure.Storage;

    /// <summary>
    /// The XStore file operation type, this is used by
    /// external to call XStoreFileOperation functions
    /// </summary>
    internal enum XStoreFileOperationType
    {
        /// <summary>
        /// File operation that copy files from xstore to smb
        /// </summary>
        CopyFromXStoreToSMB,

        /// <summary>
        /// File operation that copy files from smb to xstore
        /// </summary>
        CopyFromSMBToXStore,

        /// <summary>
        /// File operation that copy files from xstore to xstore
        /// </summary>
        CopyFromXStoreToXStore,

        /// <summary>
        /// File operation that removes a directory/file from xstore
        /// </summary>
        RemoveFromXStore
    }

    /// <summary>
    /// File operation tasks information, including
    /// operation type, offset, length or partial download id.
    /// This is used inside of XStoreFileOperation classes
    /// </summary>
    internal class XStoreFileOperationTask
    {
        /// <summary>
        /// Initializes a new instance of the XStoreFileOperationTask class.
        /// </summary>
        /// <param name="operationType">Operation type</param>
        /// <param name="srcUri">Source uri of this operation</param>
        /// <param name="dstUri">Destination uri of this operation</param>
        /// <param name="isFolder">Bool to indicate if this operaton is on a folder</param>
        /// <param name="retryCount">Retry count of this operation</param>
        /// <param name="timeoutHelper">The timeout helper object</param>
        public XStoreFileOperationTask(
            XStoreTaskType operationType,
            string srcUri,
            string dstUri,
            bool isFolder,
            int retryCount,
            TimeoutHelper timeoutHelper)
                 : this(operationType, srcUri, dstUri, isFolder, retryCount, timeoutHelper, null)
        {
        }

        /// <summary>
        /// Initializes a new instance of the XStoreFileOperationTask class.
        /// </summary>
        /// <param name="operationType">Operation type</param>
        /// <param name="srcUri">Source uri of this operation</param>
        /// <param name="dstUri">Destination uri of this operation</param>
        /// <param name="isFolder">Bool to indicate if this operaton is on a folder</param>
        /// <param name="retryCount">Retry count of this operation</param>
        /// <param name="timeoutHelper">The timeout helper object</param>
        /// <param name="operationId">The unique ID of the XStore operation.</param>
        public XStoreFileOperationTask(
            XStoreTaskType operationType,
            string srcUri,
            string dstUri,
            bool isFolder,
            int retryCount,
            TimeoutHelper timeoutHelper,
            Guid? operationId)
        {
            this.OpType = operationType;
            this.SrcUri = srcUri;
            this.DstUri = dstUri;
            this.IsRoot = false;
            this.IsFolder = isFolder;
            this.RetryCount = retryCount;
            this.PartialID = -1;
            this.FileCopyFlag = CopyFlag.CopyIfDifferent;
            this.Offset = 0;
            this.Length = 0;
            this.IsSucceeded = false;
            this.TimeoutHelper = timeoutHelper;
            this.OperationId = operationId;

            if ((this.OpType & TypesWithExtraTracing) == this.OpType)
            {
                this.ExtraTracingEnabled = true;
                this.OperationContext = new OperationContext();
                this.OperationContext.ClientRequestID = (this.OperationId == null ? Guid.NewGuid().ToString() : this.OperationId.ToString());
            }
            else
            {
                this.OperationContext = null;
            }
        }

        /// <summary>
        /// The XStore file operation type
        /// </summary>
        [Flags]
        internal enum XStoreTaskType
        {
            /// <summary>
            /// File operation that copy files from xstore to smb
            /// </summary>
            CopyFromXStoreToSMB = 0,

            /// <summary>
            /// File operation that copy files from smb to xstore
            /// </summary>
            CopyFromSMBToXStore = 1 << 0,

            /// <summary>
            /// File operation that copy files from xstore to xstore
            /// </summary>
            CopyFromXStoreToXStore = 1 << 1,

            /// <summary>
            /// File operation that removes a directory/file from xstore
            /// </summary>
            RemoveFromXStore = 1 << 2,

            /// <summary>
            /// File operation that combines partial downloaded files
            /// </summary>
            CombinePartialFiles = 1 << 3,

            /// <summary>
            /// File operation that performs a post-copy task for downloading from xstore
            /// </summary>
            EndCopyFromSMBToXStore = 1 << 4,

            /// <summary>
            /// File operation that performs a post-copy task for uploading to xstore
            /// </summary>
            EndCopyFromXStoreToSMB = 1 << 5,

            /// <summary>
            /// File operation that performs a post-copy task for xstore to xstore
            /// </summary>
            EndCopyFromXStoreToXStore = 1 << 6
        }

        //Define the types which need to extract extra trace information from OperationContext object
        private readonly XStoreTaskType TypesWithExtraTracing = XStoreTaskType.CopyFromXStoreToSMB | XStoreTaskType.CopyFromSMBToXStore | XStoreTaskType.RemoveFromXStore;

        /// <summary>Gets or sets a value indicating the operation type </summary>
        public XStoreTaskType OpType { get; private set; }

        /// <summary>Gets or sets a value indicating the copy flag </summary>
        public CopyFlag FileCopyFlag { get; set; }

        /// <summary>Gets or sets a value indicating the source uri </summary>
        public string SrcUri { get; private set; }

        /// <summary>Gets or sets a value indicating the destination uri </summary>
        public string DstUri { get; set; }

        /// <summary>Gets or sets a value indicating whether this operation is on root folder </summary>
        public bool IsRoot { get; set; }

        /// <summary>Gets or sets a value indicating whether this operation is on folder </summary>
        public bool IsFolder { get; private set; }

        /// <summary>Gets or sets a value indicating the operation on a partial file (for download) </summary>
        public int PartialID { get; set; }

        /// <summary>Gets or sets a value indicating the offset of blob </summary>
        public int Offset { get; set; }

        /// <summary>Gets or sets a value indicatingthe partial file size of the operation </summary>
        public int Length { get; set; }

        /// <summary>Gets or sets a value indicating the retry count </summary>
        public int RetryCount { get; set; }

        /// <summary>Gets or sets a value indicating the task content </summary>
        public string Content { get; set; }

        /// <summary>Gets or sets a value indicating whether the task is succeeded </summary>
        public bool IsSucceeded { get; set; }

        /// <summary>The context for a request to provides additional information about its execution</summary>
        public OperationContext OperationContext { get; set; }

        /// <summary> Get the unique ID of the XStore opertion </summary>
        public Guid? OperationId { get; }

        /// <summary>
        /// Indicates that the request needs to be tracked with extra trace information
        /// </summary>
        public bool ExtraTracingEnabled
        {
            get;
            private set;
        }

        ///// <summary>Gets or sets a timeout in second </summary>
        //public TimeSpan Timeout { get; set; }


        /// <summary>Get timeout helper</summary>
        public TimeoutHelper TimeoutHelper { get; private set; }

        /// <summary>
        /// String representation of the task class
        /// </summary>
        /// <returns>String representation of the task.</returns>
        public override string ToString()
        {
            return string.Format(
                CultureInfo.CurrentCulture,
                "OPTYPE = {0}\r\nCOPYFLAG = {1}\r\nSRCURI = {2}\r\nDSTURI = {3}\r\nRETRY COUNT = {4}\r\nCONTENT = {5}\r\nISFOLDER = {6}",
                this.OpType,
                this.FileCopyFlag,
                string.IsNullOrEmpty(this.SrcUri) ? string.Empty : this.SrcUri,
                string.IsNullOrEmpty(this.DstUri) ? string.Empty : this.DstUri,
                this.RetryCount,
                string.IsNullOrEmpty(this.Content) ? string.Empty : this.Content,
                this.IsFolder);
        }

        public string GetExtraTracing()
        {
            if (this.ExtraTracingEnabled && this.OperationContext.RequestResults != null && this.OperationContext.RequestResults.Count > 0)
            {
                StringBuilder builder = new StringBuilder();
                if (OperationId != null)
                {
                    builder.Append(string.Format(CultureInfo.CurrentCulture, "OperationID:{0}", this.OperationId));
                }

                builder.Append(string.Format(CultureInfo.CurrentCulture, "RequestID:{0}", this.OperationContext.ClientRequestID));
                builder.Append(string.Format(CultureInfo.CurrentCulture, ",SourceUri:{0}", this.SrcUri));
                builder.Append(string.Format(CultureInfo.CurrentCulture, ",RequestResults:"));
                foreach (var result in this.OperationContext.RequestResults)
                {
                    builder.Append(string.Format(CultureInfo.CurrentCulture, "[Etag:{0},StatusCode:{1},StatusMessage:{2},TargetLocation:{3},IngressBytes:{4}",
                        result.Etag,
                        result.HttpStatusCode,
                        result.HttpStatusMessage,
                        result.TargetLocation.ToString(),
#if DotNetCoreClr
                        "[NotAvailableInCoreCLR]"));
#else
                        result.IngressBytes));
#endif

                    if (result.ExtendedErrorInformation != null)
                    {
                        builder.Append(string.Format(CultureInfo.CurrentCulture, ",ExtendErrorInfo:{0}-{1}",
                            result.ExtendedErrorInformation.ErrorCode,
                            result.ExtendedErrorInformation.ErrorMessage));
                    }

                    if (result.Exception != null)
                    {
                        builder.Append(string.Format(CultureInfo.CurrentCulture, ",Exception:{0}", result.Exception.Message));
                        if (result.Exception.InnerException != null)
                        {
                            builder.Append(string.Format(CultureInfo.CurrentCulture, "InnerException:{0}", result.Exception.InnerException.Message));
                        }
                    }

                    builder.Append("]");
                }

                return builder.ToString();
            }

            return string.Empty;
        }
    }

    /// <summary>
    /// This class implements a task pool for XStore
    /// File Operations. It also signal about new task
    /// event and all task completion event.
    /// </summary>
    [SuppressMessage(FxCop.Category.StyleCopMaintainability, FxCop.Rule.FileMayOnlyContainASingleClass, Justification = "TODO:huich - Need to fix.")]
    internal class XStoreTaskPool
    {
        /// <summary>
        /// The synchronization event.
        /// </summary>
        private readonly XStoreFileOperationSyncEvents syncEvent = null;

        /// <summary>Task queue for file operations</summary>
        private readonly Queue<XStoreFileOperationTask> taskQueue = null;

        /// <summary>Post-processing task queue for file operations</summary>
        private readonly Queue<XStoreFileOperationTask> endTaskQueue = null;

        /// <summary>The count of in-process work items</summary>
        private long workItemsInProgress = 0;

        /// <summary>
        /// Initializes a new instance of the XStoreTaskPool class.
        /// </summary>
        /// <param name="syncEvent">The event to be used for synchronization.</param>
        public XStoreTaskPool(XStoreFileOperationSyncEvents syncEvent)
        {
            this.taskQueue = new Queue<XStoreFileOperationTask>();
            this.endTaskQueue = new Queue<XStoreFileOperationTask>();
            this.syncEvent = syncEvent;
            this.workItemsInProgress = 0;
        }

        /// <summary>
        /// Gets the number of all work in progress tasks
        /// </summary>
        internal long WorkItemsInProgress
        {
            get { return this.workItemsInProgress; }
        }

        /// <summary>
        /// Gets the endtaskqueue object
        /// </summary>
        internal Queue<XStoreFileOperationTask> EndTaskQueue
        {
            get { return this.endTaskQueue; }
        }

        /// <summary>
        /// Add a new task to the task queue
        /// </summary>
        /// <param name="task">The New task </param>
        internal void AddTaskToTaskQueue(XStoreFileOperationTask task)
        {
            if (null == task)
            {
                throw new ArgumentException("null task is added to task queue");
            }

            lock (((ICollection)this.taskQueue).SyncRoot)
            {
                Interlocked.Increment(ref this.workItemsInProgress);
                this.taskQueue.Enqueue(task);
                this.syncEvent.NewTaskEvent.Set();
            }
        }

        /// <summary>
        /// Add new tasks to the task queue
        /// </summary>
        /// <param name="tasks">
        /// New task queue
        /// </param>
        internal void AddTaskToTaskQueue(Queue<XStoreFileOperationTask> tasks)
        {
            lock (((ICollection)this.taskQueue).SyncRoot)
            {
                foreach (var task in tasks)
                {
                    if (null != task)
                    {
                        // increate work items count
                        Interlocked.Increment(ref this.workItemsInProgress);
                        this.taskQueue.Enqueue(task);
                    }
                    else
                    {
                        throw new ArgumentException("null task is added to task queue");
                    }
                }

                this.syncEvent.NewTaskEvent.Set();
            }
        }

        /// <summary>
        /// Add task to endtaskqueue (post-processing queue)
        /// </summary>
        /// <param name="task"> The new task </param>
        internal void AddTaskToEndTaskQueue(XStoreFileOperationTask task)
        {
            lock (((ICollection)this.taskQueue).SyncRoot)
            {
                this.endTaskQueue.Enqueue(task);
            }
        }

        /// <summary>
        /// Get a task from task queue
        /// </summary>
        /// <returns>A task object.</returns>
        internal XStoreFileOperationTask GetTaskFromTaskQueue()
        {
            XStoreFileOperationTask task = null;

            lock (((ICollection)this.taskQueue).SyncRoot)
            {
                if (this.taskQueue.Count > 0)
                {
                    task = this.taskQueue.Dequeue();
                }
                else
                {
                    // Reset NewFileEvent as we don't have any in the queue
                    this.syncEvent.NewTaskEvent.Reset();
                }
            }

            return task;
        }

        /// <summary>
        /// Decrement work item and get the current count after this decrement
        /// </summary>
        internal void DecrementTaskCount()
        {
            long currentItems = Interlocked.Decrement(ref this.workItemsInProgress);

            // if the worktiems count revert to 0, there is no more items queued,
            // signal the event to indicate that the operation has been completed
            if (0 == currentItems)
            {
                this.syncEvent.AllTasksCompletedEvent.Set();
            }
        }
    }

    /// <summary>
    /// The sync envents for xstore file operations. This class hosts a list of events,
    /// to sync the actions between main thread and worker threads
    /// </summary>
    internal class XStoreFileOperationSyncEvents : IDisposable
    {
        /// <summary>
        /// Event array for newfileevent and exitthreadevent
        /// </summary>
        private readonly WaitHandle[] eventArray;

        /// <summary>
        /// Event to indicate that new task is ready
        /// </summary>
        private EventWaitHandle newTaskEvent;

        /// <summary>
        /// Event to indicate that worker threads should exit after done their work
        /// </summary>
        private EventWaitHandle allTasksCompletedEvent;

        /// <summary>
        /// To implement IDisposable functionality
        /// </summary>
        private bool disposed;

        /// <summary>
        /// Initializes a new instance of the XStoreFileOperationSyncEvents class.
        /// </summary>
        public XStoreFileOperationSyncEvents()
        {
            this.newTaskEvent = new ManualResetEvent(false);
            this.allTasksCompletedEvent = new ManualResetEvent(false);
            this.eventArray = new WaitHandle[2];
            this.eventArray[0] = this.newTaskEvent;
            this.eventArray[1] = this.allTasksCompletedEvent;
        }

        /// <summary>
        /// Gets the exit thread event
        /// </summary>
        public EventWaitHandle AllTasksCompletedEvent
        {
            get
            {
                this.ThrowIfDisposed();
                return this.allTasksCompletedEvent;
            }
        }

        /// <summary>
        /// Gets the new task event.
        /// </summary>
        public EventWaitHandle NewTaskEvent
        {
            get
            {
                this.ThrowIfDisposed();
                return this.newTaskEvent;
            }
        }

        /// <summary>
        /// Gets the event array
        /// </summary>
        public WaitHandle[] EventArray
        {
            get
            {
                this.ThrowIfDisposed();
                return this.eventArray;
            }
        }

        /// <summary>
        /// Dispose events
        /// </summary>
        public void Dispose()
        {
            this.Dispose(true /* disposing */);
            GC.SuppressFinalize(this);
        }

        protected void Dispose(bool disposing)
        {
            if (!this.disposed && disposing)
            {
                if (this.newTaskEvent != null)
                {
                    this.newTaskEvent.Dispose();
                    this.newTaskEvent = null;
                }

                if (this.allTasksCompletedEvent != null)
                {
                    this.allTasksCompletedEvent.Dispose();
                    this.allTasksCompletedEvent = null;
                }

                this.disposed = true;
            }
        }

        private void ThrowIfDisposed()
        {
            if (this.disposed)
            {
                throw new ObjectDisposedException("XStoreFileOperationSyncEvents");
            }
        }
    }

    /// <summary>
    /// The Paramters for accessing a xstore, including account name,
    /// account key and container information
    /// </summary>
    internal class XStoreParameters
    {
        /// <summary>
        /// Initializes a new instance of the XStoreParameters class.
        /// </summary>
        /// <param name="connectionString">Azure storage connection string</param>
        /// <param name="container">Azure blob storage container</param>
        /// <returns></returns>
        public XStoreParameters(
            string connectionString,
            string container) : this(connectionString, connectionString, container)
        {
        }

        /// <summary>
        /// Initializes a new instance of the XStoreParameters class.
        /// </summary>
        /// <param name="connectionString">Primary azure storage connection string</param>
        /// <param name="secondaryConnectionString">Secondary azure storage connection string</param>
        /// <param name="container">Azure blob storage container</param>
        public XStoreParameters(
            string connectionString,
            string secondaryConnectionString,
            string container)
        {
            this.ConnectionString = connectionString;
            this.SecondayConnectionString = secondaryConnectionString;
            this.Container = container;
        }

        /// <summary>
        /// Gets or sets the primary azure storage connection string
        /// </summary>
        public string ConnectionString { get; set; }

        /// <summary>
        /// Gets or sets the secondary azure storage connection string
        /// </summary>
        public string SecondayConnectionString { get; set; }

        /// <summary>
        /// Gets or sets the azure storage container
        /// </summary>
        public string Container { get; set; }
    }

    /// <summary>
    /// This is a simple connection string provider.
    /// </summary>
    internal class SimpleConnectionStringProvider
    {
        /// <summary>
        /// An array of connection strings.
        /// </summary>
        private readonly string[] connectionStrings = new string[2];

        /// <summary>
        /// Initializes a new instance of the SimpleConnectionStringProvider class.
        /// </summary>
        /// <param name="primaryConnectionString">The primary connection string.</param>
        /// <param name="secondaryConnectionString">The secondary connection string.</param>
        public SimpleConnectionStringProvider(string primaryConnectionString, string secondaryConnectionString)
        {
            this.connectionStrings[0] = primaryConnectionString;
            this.connectionStrings[1] = secondaryConnectionString;
        }

        /// <summary>
        /// Gets a connection string.
        /// </summary>
        /// <param name="usePrimary">A value indicating whether to use the primary connection string.</param>
        /// <returns>Primary connection string if usePrimary is true, the secondary connection string otherwise.</returns>
        public string GetConnectionString(bool usePrimary)
        {
            return this.connectionStrings[usePrimary ? 0 : 1];
        }
    }

    /// <summary>
    /// Implementation of XStore file operations
    /// including copy, delelte, exists, etc.
    /// </summary>
    internal class XStoreFileOperations : IDisposable
    {
        //// Private FabricValidatorConstants:

        /// <summary>Class name </summary>
        private const string ClassName = "XStoreFileOperations";

        /// <summary>Default timeout for blob operation </summary>
        private const int DefaultTimeoutSeconds = 180;

        /// <summary>The maximum number of worker threads </summary>
        private static readonly int MaxWorkerNum;

        /// <summary>The maximum number of concurrent connections: default 2 for .net programs </summary>
        private static readonly int MaxConnectionNum;

        /// <summary>Sync object to pretend multiple instances of copyfolder/removefolder at the same time</summary>
        private static readonly object SyncLock = new object();

        /// <summary> The available number of worker threads </summary>
        private static int availableWorkerNum;

        /// <summary>Provide to provide xstore connection strings </summary>
        private readonly SimpleConnectionStringProvider connectionStringProvider = null;

        /// <summary>Xstore task queue utility </summary>
        private readonly XStoreTaskPool xstoreTaskPool;

        /// <summary>XStore account information to access blob storage </summary>
        private readonly XStoreParameters storeParams;

        /// <summary>Default blob request option </summary>
        private BlobRequestOptions defaultRequestOption = null;

        /// <summary> Source root uri for this operation </summary>
        private string srcRootUri;

        /// <summary>Destination root uri for this operation </summary>
        private string dstRootUri;

        /// <summary>Sync events to sync the multi threads file operation</summary>
        private XStoreFileOperationSyncEvents syncEvents;

        /// <summary>The list of file operation workers</summary>
        private List<XStoreFileOperationsWorker> workerList = null;

        /// <summary>File extensions that are to be excluded from file operations</summary>
        private Dictionary<string, bool> filterExtensions = null;

        /// <summary>Files that are to be excluded from file operations</summary>
        private HashSet<string> filterFileNames = null;

        /// <summary>Flag to indicate if the container has exists </summary>
        private bool containerCreated = false;

        /// <summary> Whether to use primary credentials </summary>
        private bool usePrimaryConnectionString = true;

        private bool disposed;

        /// <summary>
        /// Source for writing traces.
        /// </summary>
        private FabricEvents.ExtensionsEvents traceSource = new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.ImageStoreClient);

        static XStoreFileOperations()
        {
            // Default values
            MaxWorkerNum = 25;
            MaxConnectionNum = 5000;

            bool isEncrypted;
            var configStore = NativeConfigStore.FabricGetConfigStore();
            var maxWorkerThreadsString = configStore.ReadString(
                FabricValidatorConstants.SectionNames.Management,
                FabricValidatorConstants.ParameterNames.AzureStorageMaxWorkerThreads,
                out isEncrypted);
            if (!string.IsNullOrEmpty(maxWorkerThreadsString))
            {
                MaxWorkerNum = ImageBuilder.ImageBuilderUtility.ConvertString<int>(maxWorkerThreadsString);
            }

            var maxConnectionsString = configStore.ReadString(
                FabricValidatorConstants.SectionNames.Management,
                FabricValidatorConstants.ParameterNames.AzureStorageMaxConnections,
                out isEncrypted);
            if (!string.IsNullOrEmpty(maxConnectionsString))
            {
                MaxConnectionNum = ImageBuilder.ImageBuilderUtility.ConvertString<int>(maxConnectionsString);
            }

            availableWorkerNum = MaxWorkerNum;
        }

        /// <summary>
        /// Initializes a new instance of the XStoreFileOperations class.
        /// </summary>
        /// <param name="storeParams">XStore account, container information</param>
        /// <returns></returns>
        public XStoreFileOperations(XStoreParameters storeParams)
        {
            this.storeParams = storeParams;
            this.syncEvents = new XStoreFileOperationSyncEvents();
            this.xstoreTaskPool = new XStoreTaskPool(this.syncEvents);
            this.connectionStringProvider = new SimpleConnectionStringProvider(storeParams.ConnectionString, storeParams.SecondayConnectionString ?? storeParams.ConnectionString);
            if (ServicePointManager.DefaultConnectionLimit < MaxConnectionNum)
            {
                ServicePointManager.DefaultConnectionLimit = MaxConnectionNum;
                this.traceSource.WriteInfo(
                    ClassName,
                    "Change concurrent connection limitation to " + ServicePointManager.DefaultConnectionLimit);
            }
        }

        /// <summary>
        /// Gets a value indicating whether to use the primary connection string.
        /// </summary>
        internal bool UsePrimaryConnectionString
        {
            get
            {
                this.ThrowIfDisposed();
                return this.usePrimaryConnectionString;
            }
        }

        /// <summary>
        /// Gets the blob client
        /// </summary>
        private CloudBlobClient BlobClient
        {
            get
            {
                return this.GetCloudBlobClientConnection();
            }
        }

        /// <summary>
        /// Gets the blob container object
        /// </summary>
        private CloudBlobContainer BlobContainer
        {
            get
            {
                return this.BlobClient.GetContainerReference(this.storeParams.Container);
            }
        }

        /// <summary>
        /// Gets the default requestion option that used for file operations
        /// </summary>
        private BlobRequestOptions DefaultRequestOption
        {
            get
            {
                if (null == this.defaultRequestOption)
                {
                    this.defaultRequestOption = new BlobRequestOptions
                    {
                        RetryPolicy = XStoreCommon.DefaultRetryPolicy, //ExponentialRetry with 3 max retry attempts and 3 second back off interval
                        MaximumExecutionTime = new TimeSpan(0, 0, DefaultTimeoutSeconds)
                    };
                }

                return this.defaultRequestOption;
            }
        }

        /// <summary>
        /// Wrapper of copyfolder. We have this function to prevent multiple instances of copyfolder running at the same time for the reason below:
        ///         We have multiple worker threads for copyfolder. And each worker thread can probably spin more
        ///         threads to max the i/o. If we have muliple of copyfolder running at the same time (like imagebuilder,
        ///         servicemgr use threadpool to run the copyfolder in multi-threads) it can easily overload the system and
        ///         degrade performance.
        /// However we are now change the code and multiple copyfolder is fine. Keep the wrapper for a while in case we need to tune it later
        /// </summary>
        /// <param name="srcRootUri">Source uri that needs to be copied. Example: FolderName (under container) for xstore path</param>
        /// <param name="dstRootUri">Destination uri. Example: \\buildserver\\sql\\buildnumber</param>
        /// <param name="operationType">File operation type</param>
        /// <param name="copyFlag">Copy flag to indicate which kind of copy is used: AtomicCopy, CopyIfDifferent, etc.</param>
        /// <param name="filterExtensions">File extensions to be filtered out of the operation</param>
        /// <param name="filterFileNames">File names to be filtered out of the operation.</param>
        /// <param name="helper">The timeout helper object.</param>
        public void CopyFolder(
             string srcRootUri,
             string dstRootUri,
             XStoreFileOperationType operationType,
             CopyFlag copyFlag,
             string[] filterExtensions,
             string[] filterFileNames,
             TimeoutHelper helper)
        {
            this.CopyFolder(srcRootUri, dstRootUri, operationType, copyFlag, filterExtensions, filterFileNames, helper, null);
        }

        /// <summary>
        /// Wrapper of copyfolder. We have this function to prevent multiple instances of copyfolder running at the same time for the reason below:
        ///         We have multiple worker threads for copyfolder. And each worker thread can probably spin more
        ///         threads to max the i/o. If we have muliple of copyfolder running at the same time (like imagebuilder,
        ///         servicemgr use threadpool to run the copyfolder in multi-threads) it can easily overload the system and
        ///         degrade performance.
        /// However we are now change the code and multiple copyfolder is fine. Keep the wrapper for a while in case we need to tune it later
        /// </summary>
        /// <param name="srcRootUri">Source uri that needs to be copied. Example: FolderName (under container) for xstore path</param>
        /// <param name="dstRootUri">Destination uri. Example: \\buildserver\\sql\\buildnumber</param>
        /// <param name="operationType">File operation type</param>
        /// <param name="copyFlag">Copy flag to indicate which kind of copy is used: AtomicCopy, CopyIfDifferent, etc.</param>
        /// <param name="filterExtensions">File extensions to be filtered out of the operation</param>
        /// <param name="filterFileNames">File names to be filtered out of the operation.</param>
        /// <param name="helper">The timeout helper object.</param>
        /// <param name="operationId">The unique ID of the XStore operation.</param>
        public void CopyFolder(
             string srcRootUri,
             string dstRootUri,
             XStoreFileOperationType operationType,
             CopyFlag copyFlag,
             string[] filterExtensions,
             string[] filterFileNames,
             TimeoutHelper helper,
             Guid? operationId)
        {
            this.ThrowIfDisposed();
            this.traceSource.WriteInfo(
                ClassName,
                string.Format(CultureInfo.CurrentCulture, "Start CopyFolder from {0} to {1} with OpType {2}", srcRootUri, dstRootUri, operationType));

            this.CopyFolderImpl(srcRootUri, dstRootUri, operationType, copyFlag, filterExtensions, filterFileNames, helper, operationId);

            this.traceSource.WriteInfo(
                ClassName,
                string.Format(CultureInfo.CurrentCulture, "Complete CopyFolder from {0} to {1} with OpType {2}", srcRootUri, dstRootUri, operationType));
        }

        /// <summary>
        /// Wrapper of RemoveXStoreFolder to prevent multiple instances of RemoveXStoreFolder running at the same time for the reason below:
        ///         We have multiple worker threads for RemoveXStoreFolder. And each worker thread can probably spin more
        ///         threads to max the i/o. If we have muliple of RemoveXStoreFolder running at the same time,
        ///         it can easily overload the system and degrade performance.
        /// We have now changed the code and remove the restriction. Keep the wrapper for now and will remove it later if everything works fine
        /// </summary>
        /// <param name="blobUri">Directory uri that needs to be removed</param>
        /// <param name="helper">The timeout helper object.</param>
        public void RemoveXStoreFolder(string blobUri, TimeoutHelper helper)
        {
            this.ThrowIfDisposed();
            this.traceSource.WriteInfo(
                ClassName,
                string.Format(CultureInfo.CurrentCulture, "Start RemoveXStoreFolder for {0}", blobUri));

            this.RemoveXStoreFolderImpl(blobUri, helper);

            this.traceSource.WriteInfo(
                ClassName,
                string.Format(CultureInfo.CurrentCulture, "Complete RemoveXStoreFolder for {0}", blobUri));
        }

        /// <summary>
        /// Copy method that blocks till the operartion is complete - these parameters are the root uris to be copied
        /// </summary>
        /// <param name="srcUri">Source uri that needs to be copied. Example: FolderName(under container)/1.txt</param>
        /// <param name="dstUri">Destination uri. Example: \\buildserver\\sql\\buildnumber\\1.txt</param>
        /// <param name="operationType">File operation type</param>
        /// <param name="helper">The timeout helper object.</param>
        public void CopyFile(
            string srcUri,
            string dstUri,
            XStoreFileOperationType operationType,
            TimeoutHelper helper)
        {
            this.ThrowIfDisposed();
            XStoreFileOperationTask task = null;

            this.traceSource.WriteInfo(
                ClassName,
                string.Format(CultureInfo.CurrentCulture, "Start CopyFile from {0} to {1}", srcUri, dstUri));

            if (operationType == XStoreFileOperationType.CopyFromSMBToXStore)
            {
                this.SafeCreateContainer(helper);
            }

            task = new XStoreFileOperationTask(
                GetXStoreTaskTypeFromFileOperationType(operationType),
                srcUri,
                dstUri,
                false,
                0,
                helper);


            task.FileCopyFlag = CopyFlag.CopyIfDifferent;
            this.xstoreTaskPool.AddTaskToTaskQueue(task);

            this.ExecFileOperation(GetXStoreTaskTypeFromFileOperationType(operationType));

            this.traceSource.WriteInfo(
                ClassName,
                string.Format(CultureInfo.CurrentCulture, "Complete CopyFile from {0} to {1}", srcUri, dstUri));
        }

        /// <summary>
        /// Remove method that blocks till the specified file is removed from xstore
        /// </summary>
        /// <param name="blobUri">Destination uri that needs to be removed</param>
        /// <param name="helper">The timeout helper object.</param>
        public void RemoveXStoreFile(string blobUri, TimeoutHelper helper)
        {
            this.ThrowIfDisposed();
            var blob = this.BlobContainer.GetBlockBlobReference(blobUri);
            if (helper != null)
            {
#if !DotNetCoreClr
                blob.DeleteIfExists(DeleteSnapshotsOption.None, null, GetRequestOptions(helper));
#else
                blob.DeleteIfExistsAsync(DeleteSnapshotsOption.None, null, GetRequestOptions(helper), null).Wait();
#endif
            }
            else
            {

#if !DotNetCoreClr
                blob.DeleteIfExists(DeleteSnapshotsOption.None);

#else
                blob.DeleteIfExistsAsync(DeleteSnapshotsOption.None, null, DefaultRequestOption, null).Wait();
#endif
            }
        }

        /// <summary>
        /// List all blobs at the given container
        /// </summary>
        /// <param name="blobUri">Blob uri that to be retrieved from XStore</param>
        /// <param name="isRecursive">flag list subhierarchy</param>
        /// <param name="helper">The timeout helper object.</param>
        /// <returns></returns>
        public ImageStoreContent ListXStoreFile(string blobUri, bool isRecursive, TimeoutHelper helper)
        {
            ImageStoreContent content = new ImageStoreContent();

            Func<string, CloudBlockBlob, ImageStoreFile> convertToImageStoreFile = (string relativePath, CloudBlockBlob blob) =>
            {
                return new ImageStoreFile(relativePath.Replace('/', '\\'),
                    string.Empty,
                    (Int64)blob.Properties.Length,
                    blob.Properties.LastModified.Value.DateTime);
            };

            Action<string, CloudBlockBlob> addFile = (string subRelativePath, CloudBlockBlob blob) =>
            {
                if (content.Files == null)
                {
                    content.Files = new List<ImageStoreFile>() { convertToImageStoreFile(subRelativePath, blob) };
                }
                else
                {
                    content.Files.Add(convertToImageStoreFile(subRelativePath, blob));
                }
            };

#if !DotNetCoreClr
            IEnumerable<IListBlobItem> blobs = this.BlobContainer.ListBlobs(blobUri, true, BlobListingDetails.All, helper != null ? GetRequestOptions(helper) : null);
#else
            BlobContinuationToken continuationToken = null;
            do
            {
                BlobResultSegment resultSegment =
                    this.BlobContainer.ListBlobsSegmentedAsync(blobUri, true, BlobListingDetails.All, null,
                        continuationToken, null, null).Result;
                IEnumerable<IListBlobItem> blobs = resultSegment.Results;
                continuationToken = resultSegment.ContinuationToken;
#endif
            foreach (var blobItem in blobs)
            {
                var blob = blobItem as CloudBlockBlob;
                try
                {
#if DotNetCoreClr
                    blob.FetchAttributesAsync().Wait();
#else
                    blob.FetchAttributes();
#endif

                    if (isRecursive)
                    {
                        addFile(blob.Name, blob);
                        continue;
                    }

                    if (string.Equals(blob.Name, blobUri, StringComparison.CurrentCultureIgnoreCase))
                    {
                        addFile(blobUri, blob);
                        continue;
                    }

                    string subRelativePath = string.IsNullOrEmpty(blobUri) ? blob.Name : blob.Name.Substring(blobUri.Length + 1);


                    int index = subRelativePath.IndexOf('/');

                    if (index < 0)
                    {
                        addFile(blob.Name, blob);
                        continue;
                    }


                    string subFolder = blob.Name.Substring(0, string.IsNullOrEmpty(blobUri) ? index : blobUri.Length + 1 + index).Replace('/', '\\');

                    if (content.Folders == null)
                    {
                        content.Folders = new List<ImageStoreFolder>() { new ImageStoreFolder(subFolder, 1) };
                    }
                    else
                    {
                        var exist = content.Folders.Find(fd => string.Equals(subFolder, fd.StoreRelativePath));
                        if (exist == null)
                        {
                            content.Folders.Add(new ImageStoreFolder(subFolder, 1));
                        }
                        else
                        {
                            exist.FileCount++;
                        }
                    }
                }
                catch (StorageException ex)
                {
                    this.traceSource.WriteWarning(
                        ClassName,
                        string.Format(CultureInfo.CurrentCulture, "Cannot retrieve blob {0}: {1}", blob.Name, ex.Message));

                }
            }

#if DotNetCoreClr
            } while (continuationToken != null);
#endif
            return content;
        }

        /// <summary>
        /// Write content to the specified blob
        /// </summary>
        /// <param name="blobUri">Blob uri that to be write</param>
        /// <param name="content">Blob content</param>
        /// <param name="helper">The timeout helper object.</param>
        public void WriteAllText(string blobUri, string content, TimeoutHelper helper)
        {
            this.ThrowIfDisposed();
            this.SafeCreateContainer(helper);
            var blob = this.BlobContainer.GetBlockBlobReference(blobUri);
            if (helper != null)
            {
#if !DotNetCoreClr
                blob.UploadText(content, null, null, GetRequestOptions(helper));
#else
                blob.UploadTextAsync(content, null, null, GetRequestOptions(helper), null).Wait();
#endif
            }
            else
            {
#if !DotNetCoreClr
                blob.UploadText(content);
#else
                blob.UploadTextAsync(content).Wait();
#endif
            }
        }

        /// <summary>
        /// Check to see if the specified blob folder exists
        /// </summary>
        /// <param name="blobUri">Blob uri of the blob folder</param>
        /// <param name="checkXStoreFolderManifest"></param>
        /// <param name="helper">The timeout helper object.</param>
        /// <returns>
        /// Returns true if exists
        /// </returns>
        public bool XStoreFolderExists(string blobUri, bool checkXStoreFolderManifest, TimeoutHelper helper)
        {
            this.ThrowIfDisposed();
            return XStoreCommon.XStoreFolderExists(this.BlobContainer, blobUri, checkXStoreFolderManifest, helper != null ? GetRequestOptions(helper) : null);
        }

        /// <summary>
        /// Check if the specified file exists on xstore
        /// </summary>
        /// <param name="blobUri">Blob uri to be checked</param>
        /// <param name="helper">The timeout helper object.</param>
        /// <returns>
        /// Returns true if exists
        /// </returns>
        public bool XStoreFileExists(string blobUri, TimeoutHelper helper)
        {
            this.ThrowIfDisposed();
            return XStoreCommon.XStoreFileExists(this.BlobContainer, blobUri, helper != null ? GetRequestOptions(helper) : null);
        }

        public bool XStoreContainerExists(TimeoutHelper helper)
        {
            this.ThrowIfDisposed();
            return XStoreCommon.XStoreContainerExists(this.BlobContainer, helper != null ? GetRequestOptions(helper) : null);
        }

        /// <summary>
        /// Remove the container
        /// </summary>
        public void RemoveContainer(TimeoutHelper helper)
        {
            this.ThrowIfDisposed();
            if (helper != null)
            {
                if (XStoreCommon.XStoreContainerExists(this.BlobContainer, GetRequestOptions(helper)))
                {
#if !DotNetCoreClr
                    this.BlobContainer.Delete(null, GetRequestOptions(helper));
#else
                    this.BlobContainer.DeleteAsync(null, GetRequestOptions(helper), null).Wait();
#endif
                }
            }
            else
            {
                if (XStoreCommon.XStoreContainerExists(this.BlobContainer, null))
                {
#if !DotNetCoreClr
                    this.BlobContainer.Delete();
#else
                    this.BlobContainer.DeleteAsync().Wait();
#endif
                }
            }

            if (XStoreCommon.XStoreContainerExists(this.BlobContainer, helper != null ? GetRequestOptions(helper) : null))
            {
                throw new IOException("Not able to remove the specified container from XStore. Container: " + this.storeParams.Container);
            }
        }

        /// <summary>
        /// Create a container can fail some time. (e.g., delete a container and immediatly creating a same-named one)
        /// This function try to back up a one minute if the first creation fails
        /// </summary>
        public void SafeCreateContainer(TimeoutHelper helper)
        {
            this.ThrowIfDisposed();
            if (this.containerCreated)
            {
                return;
            }

            this.containerCreated = XStoreCommon.XStoreContainerExists(this.BlobContainer, helper != null ? GetRequestOptions(helper) : null);
            if (this.containerCreated)
            {
                return;
            }


#if !DotNetCoreClr
            this.BlobContainer.CreateIfNotExists(helper != null ? GetRequestOptions(helper) : null);
#else
            this.BlobContainer.CreateIfNotExistsAsync(helper != null ? GetRequestOptions(helper) : null, null).Wait();
#endif

            this.containerCreated = XStoreCommon.XStoreContainerExists(this.BlobContainer, helper != null ? GetRequestOptions(helper) : null);

            if (!this.containerCreated)
            {
                // Retry the creation; this time we wait for one minute
                // Wait 60 seconds to ensure we are not failing because it is too short from previous operation
                this.traceSource.WriteInfo(
                    ClassName,
                    string.Format(CultureInfo.CurrentCulture, "Sleep 60 seconds before re-creating the container {0}", this.storeParams.Container));

                System.Threading.Thread.Sleep(60000);

#if !DotNetCoreClr
                this.BlobContainer.CreateIfNotExists(helper != null ? GetRequestOptions(helper) : null);
#else
                this.BlobContainer.CreateIfNotExistsAsync(helper != null ? GetRequestOptions(helper) : null, null).Wait();
#endif

                this.containerCreated = XStoreCommon.XStoreContainerExists(this.BlobContainer, helper != null ? GetRequestOptions(helper) : null);

                if (!this.containerCreated)
                {
                    // We had issue creating the container
                    throw new IOException("Unable to create the container : " + this.storeParams.Container);
                }
            }
        }

        /// <summary>
        /// Dispose function
        /// </summary>
        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <summary>
        /// Dispose function
        /// </summary>
        /// <param name="isDisposing">True if it is already disposing, false otherwise.</param>
        protected virtual void Dispose(bool isDisposing)
        {
            if (!this.disposed && isDisposing)
            {
                // Clean managed resources
                if (this.syncEvents != null)
                {
                    this.syncEvents.Dispose();
                    this.syncEvents = null;
                }

                this.disposed = true;
            }

            // Clean native resources.
        }

        /// <summary>
        /// Check if we already reach the per-process limitation of worker number
        /// </summary>
        /// <param name="requiredWorkerNum">
        /// The number of workers that is required
        /// </param>
        /// <returns>
        /// Returns true if there is still available worker
        /// </returns>
        private static bool TryGetWorkers(int requiredWorkerNum)
        {
            if (requiredWorkerNum <= 0)
            {
                throw new ArgumentException("invalid argument: requiredWorkerNum must be positive");
            }

            lock (SyncLock)
            {
                if (availableWorkerNum >= requiredWorkerNum)
                {
                    availableWorkerNum -= requiredWorkerNum;
                    return true;
                }
                else
                {
                    return false;
                }
            }
        }

        /// <summary>
        /// Return back the worker quote
        /// </summary>
        /// <param name="workerNum">
        /// Number of workers to be returned
        /// </param>
        private static void ReleaseWorkers(int workerNum)
        {
            if (workerNum <= 0)
            {
                throw new ArgumentException("invalid argument: workerNum must be positive");
            }

            lock (SyncLock)
            {
                availableWorkerNum += workerNum;
                if (availableWorkerNum > MaxWorkerNum)
                {
                    throw new InvalidDataException(
                        string.Format(CultureInfo.CurrentCulture, "error: we have more workers ({0}) than the defined maximum number ({1})", availableWorkerNum, MaxWorkerNum));
                }
            }
        }

        /// <summary>
        /// Map the XStore File Operation type (public to caller) to internal XStore task types
        /// </summary>
        /// <param name="fileOperationType">
        /// XStore File Operation type (public to caller)
        /// </param>
        /// <returns>
        /// Internal XStore task types
        /// </returns>
        private static XStoreFileOperationTask.XStoreTaskType GetXStoreTaskTypeFromFileOperationType(XStoreFileOperationType fileOperationType)
        {
            switch (fileOperationType)
            {
                case XStoreFileOperationType.CopyFromSMBToXStore:
                    return XStoreFileOperationTask.XStoreTaskType.CopyFromSMBToXStore;
                case XStoreFileOperationType.CopyFromXStoreToSMB:
                    return XStoreFileOperationTask.XStoreTaskType.CopyFromXStoreToSMB;
                case XStoreFileOperationType.CopyFromXStoreToXStore:
                    return XStoreFileOperationTask.XStoreTaskType.CopyFromXStoreToXStore;
                case XStoreFileOperationType.RemoveFromXStore:
                    return XStoreFileOperationTask.XStoreTaskType.RemoveFromXStore;
            }

            throw new ArgumentException(string.Format(CultureInfo.CurrentCulture, "This xstore file operation type is not supported: {0}", fileOperationType));
        }

        /// <summary>
        /// Combine partial files to the original file
        /// </summary>
        /// <param name="combineTask">The task to be performed.</param>
        private static void CombinePartialFiles(XStoreFileOperationTask combineTask)
        {
            using (FileStream fs = FabricFile.Open(combineTask.Content, FileMode.OpenOrCreate, FileAccess.Write, FileShare.None))
            {
                BinaryWriter combineWriter = new BinaryWriter(fs);

                for (int i = 0; i < combineTask.PartialID; i++)
                {
                    string partialFileName = XStoreCommon.GetPartialFileName(combineTask.Content, i);

                    if (FabricFile.Exists(partialFileName))
                    {
                        using (FileStream partfs = FabricFile.Open(partialFileName, FileMode.Open, FileAccess.Read, FileShare.Read))
                        {
                            BinaryReader reader = new BinaryReader(partfs);
                            combineWriter.Write(reader.ReadBytes((int)partfs.Length));
                            reader.Dispose();
                        }

                        FabricFile.Delete(partialFileName, deleteReadonly: true);
                    }
                }

                combineWriter.Dispose();
            }
        }

        private void ThrowIfDisposed()
        {
            if (this.disposed)
            {
                throw new ObjectDisposedException("XStoreFileOperations");
            }
        }

        private BlobRequestOptions GetRequestOptions(TimeoutHelper helper)
        {
            return helper.GetRemainingTime().TotalSeconds > DefaultTimeoutSeconds
            ? new BlobRequestOptions()
            {
                RetryPolicy = new Microsoft.WindowsAzure.Storage.RetryPolicies.NoRetry(),
                MaximumExecutionTime = helper.GetRemainingTime()
            }
            : this.defaultRequestOption;
        }

        /// <summary>
        /// Common method to execute a file operation
        /// </summary>
        /// <param name="operationType">
        /// File operation type
        /// </param>
        /// <returns>A null value.</returns>
        private string ExecFileOperation(XStoreFileOperationTask.XStoreTaskType operationType)
        {
            this.DispatchFileOperationTasks();
            int failedThread = 0;
            int timeoutThread = 0;

            if (null != this.workerList)
            {
                foreach (var worker in this.workerList)
                {
                    worker.ThreadInst.Join();
                    if (worker.HasTimeoutError)
                    {
                        timeoutThread++;
                    }
                    else if (worker.HasFatalError)

                    {
                        failedThread++;
                    }
                }
            }

            ReleaseWorkers(this.workerList.Count);
            this.workerList.Clear();

            if (timeoutThread > 0)
            {
                throw new TimeoutException(string.Format(CultureInfo.CurrentCulture, "{0} non-recoverable thread timeout(s) happened", timeoutThread));
            }

            if (failedThread > 0)
            {
                throw new InvalidDataException(string.Format(CultureInfo.CurrentCulture, "{0} non-recoverable thread failure(s) happened", failedThread));
            }

            switch (operationType)
            {
                case XStoreFileOperationTask.XStoreTaskType.CopyFromSMBToXStore:
                    this.EndCopyFromSMBToXStore();
                    break;
                case XStoreFileOperationTask.XStoreTaskType.CopyFromXStoreToSMB:
                    this.EndCopyFromXStoreToSMB();
                    break;
                case XStoreFileOperationTask.XStoreTaskType.CopyFromXStoreToXStore:
                    this.EndCopyFromXStoreToXStore();
                    break;
                case XStoreFileOperationTask.XStoreTaskType.RemoveFromXStore:
                    this.EndRemoveXStoreFolder();
                    break;
                default:
                    this.CheckFailedTasks();
                    break;
            }

            return null;
        }

        /// <summary>
        /// Get client connection to connect to azure storage.
        /// </summary>
        /// <returns>The blob client.</returns>
        private CloudBlobClient GetCloudBlobClientConnection()
        {
            CloudBlobClient blobClient = null;
            
            string connectionString = this.connectionStringProvider.GetConnectionString(this.usePrimaryConnectionString);
            string primaryConnectionString = connectionString;
            if (!this.OpenConnectionToXStoreWithConnectionString(connectionString, this.usePrimaryConnectionString, out blobClient))
            {
                this.usePrimaryConnectionString = !this.usePrimaryConnectionString;
                connectionString = this.connectionStringProvider.GetConnectionString(this.usePrimaryConnectionString);
                this.OpenConnectionToXStoreWithConnectionString(connectionString, this.usePrimaryConnectionString, out blobClient);
            }

            if (blobClient == null)
            {
                var errorMessage = string.Format(CultureInfo.CurrentCulture, StringResources.XStore_CheckConnectionString, primaryConnectionString, connectionString);
                throw new ArgumentException(errorMessage);
            }
            
            return blobClient;
        }

        /// <summary>
        /// Open the xstore connection and see if the connection string is good.
        /// </summary>
        /// <param name="connectionString">
        /// Connection string to connect to storage account.
        /// </param>
        /// <param name="isPrimaryConnecionString">
        /// Is primary connection string.
        /// </param>
        /// <param name="blobClient">
        /// Client object created using the specified connection string.
        /// </param>
        /// <returns>True if no exception happens, false otherwise.</returns>
        private bool OpenConnectionToXStoreWithConnectionString(
            string connectionString,
            bool isPrimaryConnecionString,
            out CloudBlobClient blobClient)
        {
            blobClient = null;

            try
            {
                var storageAccount = CloudStorageAccount.Parse(connectionString);
                var bc = storageAccount.CreateCloudBlobClient();

                // CloudBlobClient doesn't have methods to verify whether connection string is good.
                // Tries to look for a specific container.

                XStoreCommon.XStoreContainerExists(bc.GetContainerReference(this.storeParams.Container), null);

                // Verified connection string. Use the blob.
                blobClient = bc;

                return true;
            }
            catch (Exception e)
            {
                // FormatException: if the account key itself is invalid. Try alternative one.

                // Storage client exception.
                StorageException ex = e as StorageException;

                this.traceSource.WriteInfo(
                    ClassName,
                    "OpenConnectionToXStoreWithConnectionString: Exception occured while using {0} connection string, storage exception errorcode: {1}, message: {2}.",
                    isPrimaryConnecionString ? "Primary" : "Secondary",
                    (ex != null) ? ex.RequestInformation.HttpStatusMessage : "<null>",
                    e.Message);

                if (ExceptionHandler.IsFatalException(e))
                {
                    throw;
                }

                // This could be a temporary error other than login errors, but lets try which ever is better.
                //  StorageErrorCode.AuthenticationFailure
                //  StorageErrorCode.AccountNotFound
                //  StorageErrorCode.AccessDenied
                return false;
            }
        }

        /// <summary>
        /// Copy method that blocks till the operartion is complete - these parameters are the root uris to be copied
        /// </summary>
        /// <param name="srcRootUri">Source uri that needs to be copied. Example: FolderName (under container) for xstore path</param>
        /// <param name="destRootUri">Destination uri. Example: \\buildserver\\sql\\buildnumber</param>
        /// <param name="operationType">File operation type</param>
        /// <param name="copyFlag">Copy flag to indicate which kind of copy is used: AtomicCopy, CopyIfDifferent, etc.</param>
        /// <param name="filterExtensions">File extensions to be filtered out of the operation</param>
        /// <param name="filterFileNames">File names to be filtered out of the operation.</param>
        /// <param name="helper">The timeout helper object.</param>
        /// <param name="operationId">The unique ID of the XStore operation.</param>
        private void CopyFolderImpl(
            string srcRootUri,
            string destRootUri,
            XStoreFileOperationType operationType,
            CopyFlag copyFlag,
            IEnumerable<string> filterExtensions,
            IEnumerable<string> filterFileNames,
            TimeoutHelper helper,
            Guid? operationId)
        {
            XStoreFileOperationTask task = null;

            switch (operationType)
            {
                case XStoreFileOperationType.CopyFromXStoreToSMB:
                    this.srcRootUri = XStoreCommon.FormatXStoreUri(srcRootUri);
                    this.dstRootUri = XStoreCommon.FormatSMBUri(destRootUri);

                    // Do similar thing in FolderCopy class to make sure we atomic copy to smb destination
                    // 1. delete dstFolder.new
                    // 2. copy srcFolder -> dstFolder.new
                    // 3. delete dstFolder.old
                    // 4. rename dstFolder -> dstFolder.old
                    // 5. rename dstFolder.new -> dstFolder
                    task = new XStoreFileOperationTask(
                        XStoreFileOperationTask.XStoreTaskType.CopyFromXStoreToSMB,
                        this.srcRootUri,
                        this.dstRootUri,
                        true,
                        0,
                        helper,
                        operationId);

                    task.FileCopyFlag = copyFlag;
                    task.IsRoot = true;
                    this.xstoreTaskPool.AddTaskToTaskQueue(task);
                    break;
                case XStoreFileOperationType.CopyFromSMBToXStore:
                    this.SafeCreateContainer(helper);
                    this.srcRootUri = XStoreCommon.FormatSMBUri(srcRootUri);
                    this.dstRootUri = XStoreCommon.FormatXStoreUri(destRootUri);

                    // We have assumption about atomic copy contents to xstore:
                    // 1. there is only one writer
                    // 2. we don't overwrite existing folders
                    // we need to generate a manifest and upload to the xstore to mark the copy is done
                    task = new XStoreFileOperationTask(
                        XStoreFileOperationTask.XStoreTaskType.CopyFromSMBToXStore,
                        this.srcRootUri,
                        this.dstRootUri,
                        true,
                        0,
                        helper);


                    task.FileCopyFlag = copyFlag;
                    task.IsRoot = true;
                    this.xstoreTaskPool.AddTaskToTaskQueue(task);
                    break;
                case XStoreFileOperationType.CopyFromXStoreToXStore:
                    this.srcRootUri = XStoreCommon.FormatXStoreUri(srcRootUri);
                    this.dstRootUri = XStoreCommon.FormatXStoreUri(destRootUri);

                    task = new XStoreFileOperationTask(
                        XStoreFileOperationTask.XStoreTaskType.CopyFromXStoreToXStore,
                        this.srcRootUri,
                        this.dstRootUri,
                        true,
                        0,
                        helper,
                        operationId);

                    task.FileCopyFlag = copyFlag;
                    task.IsRoot = true;

                    this.xstoreTaskPool.AddTaskToTaskQueue(task);
                    break;

                default:
                    throw new ArgumentException("Invalid XStoreFileOperations used to create a Copy job");
            }

            // Initialize file extension list
            if (filterExtensions != null)
            {
                foreach (string e in filterExtensions)
                {
                    string extension = e;
                    if (this.filterExtensions == null)
                    {
                        this.filterExtensions = new Dictionary<string, bool>(StringComparer.OrdinalIgnoreCase);
                    }

                    // normalize; FileInfo.Extension returns . + extension like ".pdb"
                    if (extension.Length > 0 && extension[0] != '.')
                    {
                        extension = '.' + extension;
                    }

                    if (!this.filterExtensions.ContainsKey(extension))
                    {
                        this.filterExtensions[extension] = true;
                    }
                }
            }

            if (filterFileNames != null)
            {
                this.filterFileNames = new HashSet<string>(filterFileNames, StringComparer.OrdinalIgnoreCase);
            }
            else
            {
                this.filterFileNames = new HashSet<string>();
            }

            this.ExecFileOperation(GetXStoreTaskTypeFromFileOperationType(operationType));
        }

        /// <summary>
        /// Remove method that blocks till whole directory is removed from xstore
        /// </summary>
        /// <param name="blobUri">Directory uri that needs to be removed</param>
        /// <param name="helper">The timeout helper object.</param>
        private void RemoveXStoreFolderImpl(string blobUri, TimeoutHelper helper)
        {
            this.srcRootUri = XStoreCommon.FormatXStoreUri(blobUri);
            this.dstRootUri = null;

            XStoreFileOperationTask task = new XStoreFileOperationTask(
                XStoreFileOperationTask.XStoreTaskType.RemoveFromXStore,
                this.srcRootUri,
                null,
                true,
                0,
                helper);


            task.IsRoot = true;
            this.xstoreTaskPool.AddTaskToTaskQueue(task);
            this.ExecFileOperation(XStoreFileOperationTask.XStoreTaskType.RemoveFromXStore);
        }

        /// <summary>
        /// Dispatch file operation tasks to the task queue
        /// </summary>
        private void DispatchFileOperationTasks()
        {
            // This is the main thread that does: create new worker if needed
            if (null == this.workerList)
            {
                this.workerList = new List<XStoreFileOperationsWorker>();
            }

            while (WaitHandle.WaitAny(this.syncEvents.EventArray) != 1)
            {
                // Add new worker if needed
                bool needMoreWorker = this.xstoreTaskPool.WorkItemsInProgress > this.workerList.Count;
                if (needMoreWorker && TryGetWorkers(1))
                {
                    this.AddXStoreFileOperationWorker();
                }
                else
                {
                    // we don't seem to need/(can) add new worker for now
                    // sleep to yield cpu and check again
                    if (!needMoreWorker)
                    {
                        // the woke item can rapidally increase;
                        // so sleep short time to check again
                        System.Threading.Thread.Sleep(500);
                    }
                    else
                    {
                        // no more worker available
                        // sleep longer as it doesn't seem to change soon
                        System.Threading.Thread.Sleep(5000);
                    }
                }
            }
        }

        /// <summary>
        /// End processing of remove xstore folders
        /// </summary>
        private void EndRemoveXStoreFolder()
        {
            int failedItems = 0;
            while (this.xstoreTaskPool.EndTaskQueue.Count > 0)
            {
                failedItems++;
                var task = this.xstoreTaskPool.EndTaskQueue.Dequeue();
                this.traceSource.WriteError(
                    ClassName,
                    "ERROR TASK: " + task);
            }

            if (failedItems > 0)
            {
                string errorMessage = string.Format(CultureInfo.CurrentCulture, "Failed to execute file operation on {0} items", failedItems);
                throw new InvalidDataException(errorMessage);
            }
        }

        /// <summary>
        /// End processing of copy (from smb to xstore)
        /// </summary>
        private void EndCopyFromSMBToXStore()
        {
            int failedItems = 0;
            XStoreFileOperationTask endCopyTask = null;

            while (this.xstoreTaskPool.EndTaskQueue.Count > 0)
            {
                var task = this.xstoreTaskPool.EndTaskQueue.Dequeue();
                if (!task.IsSucceeded)
                {
                    failedItems++;
                    this.traceSource.WriteError(
                        ClassName,
                        "ERROR TASK: " + task);
                }

                if (task.OpType == XStoreFileOperationTask.XStoreTaskType.EndCopyFromSMBToXStore)
                {
                    endCopyTask = task;
                }
            }

            if (failedItems > 0)
            {
                string errorMessage = string.Format(CultureInfo.CurrentCulture, "Failed to execute file operation on {0} items", failedItems);
                throw new InvalidDataException(errorMessage);
            }

            if (null != endCopyTask)
            {
                // the upload is succeeded, perform the endatomiccopy action -
                // upload the manifest
                this.xstoreTaskPool.AddTaskToTaskQueue(endCopyTask);
                this.ExecFileOperation(XStoreFileOperationTask.XStoreTaskType.EndCopyFromSMBToXStore);
            }
        }

        /// <summary>
        /// End processing of copy (from xstore to xstore)
        /// </summary>
        private void EndCopyFromXStoreToXStore()
        {
            int failedItems = 0;
            XStoreFileOperationTask endCopyTask = null;

            while (this.xstoreTaskPool.EndTaskQueue.Count > 0)
            {
                var task = this.xstoreTaskPool.EndTaskQueue.Dequeue();
                if (!task.IsSucceeded)
                {
                    failedItems++;
                    this.traceSource.WriteError(
                        ClassName,
                        "ERROR TASK: " + task);
                }

                if (task.OpType == XStoreFileOperationTask.XStoreTaskType.EndCopyFromXStoreToXStore)
                {
                    endCopyTask = task;
                }
            }

            if (failedItems > 0)
            {
                string errorMessage = string.Format(CultureInfo.CurrentCulture, "Failed to execute file operation on {0} items", failedItems);
                throw new InvalidDataException(errorMessage);
            }

            if (null != endCopyTask)
            {
                // the upload is succeeded, perform the endatomiccopy action -
                // upload the manifest
                this.xstoreTaskPool.AddTaskToTaskQueue(endCopyTask);
                this.ExecFileOperation(XStoreFileOperationTask.XStoreTaskType.EndCopyFromXStoreToXStore);
            }
        }

        /// <summary>
        /// End processing of copying (from xstore to smb)
        /// </summary>
        private void EndCopyFromXStoreToSMB()
        {
            int failedItems = 0;
            bool atomicCopy = false;
            XStoreFileOperationTask endAtomicCopyTask = null;

            foreach (var task in this.xstoreTaskPool.EndTaskQueue)
            {
                if (!task.IsSucceeded)
                {
                    failedItems++;
                    this.traceSource.WriteError(
                        ClassName,
                        "ERROR TASK: " + task);
                }

                if (task.OpType == XStoreFileOperationTask.XStoreTaskType.EndCopyFromXStoreToSMB)
                {
                    atomicCopy = true;
                    endAtomicCopyTask = task;
                }
            }

            if (failedItems > 0)
            {
                string errorMessage = string.Format(CultureInfo.CurrentCulture, "Failed to execute file operation on {0} items", failedItems);
                throw new InvalidDataException(errorMessage);
            }

            // the upload is succeeded,
            // 1) combine divided files
            while (this.xstoreTaskPool.EndTaskQueue.Count > 0)
            {
                var task = this.xstoreTaskPool.EndTaskQueue.Dequeue();
                if (task.OpType == XStoreFileOperationTask.XStoreTaskType.CombinePartialFiles)
                {
                    CombinePartialFiles(task);
                }
            }

            // 2) perform the end atomic copy task
            if (atomicCopy && null != endAtomicCopyTask)
            {
                this.xstoreTaskPool.AddTaskToTaskQueue(endAtomicCopyTask);
                this.ExecFileOperation(XStoreFileOperationTask.XStoreTaskType.EndCopyFromXStoreToSMB);
            }
        }

        /// <summary>
        /// Go through the end task queue and
        /// summerize all failed tasks. Throw exception
        /// if there is any failed task
        /// </summary>
        private void CheckFailedTasks()
        {
            int failedItems = 0;

            foreach (var task in this.xstoreTaskPool.EndTaskQueue)
            {
                if (!task.IsSucceeded)
                {
                    failedItems++;
                    this.traceSource.WriteError(
                        ClassName,
                        "ERROR TASK: " + task);
                }
            }

            if (failedItems > 0)
            {
                string errorMessage = string.Format(CultureInfo.CurrentCulture, "Failed to execute file operation on {0} items", failedItems);
                throw new InvalidDataException(errorMessage);
            }
        }

        /// <summary>
        /// Create a new worker and add it to the worker list
        /// </summary>
        private void AddXStoreFileOperationWorker()
        {
            if (null == this.workerList)
            {
                this.workerList = new List<XStoreFileOperationsWorker>();
            }

            // Use the connections string that we determined is good.
            this.GetCloudBlobClientConnection(); //// This will determine the right connection string if one exist.
            string connectionString = this.connectionStringProvider.GetConnectionString(this.usePrimaryConnectionString);

            XStoreFileOperationsWorker worker = new XStoreFileOperationsWorker(
                this.xstoreTaskPool,
                this.syncEvents,
                new XStoreParameters(connectionString, connectionString, this.storeParams.Container),
                this.filterExtensions,
                this.filterFileNames,
                this.workerList.Count);
            worker.Start();
            this.workerList.Add(worker);
        }
    }
}