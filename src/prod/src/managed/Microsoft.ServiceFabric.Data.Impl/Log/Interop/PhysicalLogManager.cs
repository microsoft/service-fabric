// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.Log.Interop
{
    using System.Fabric.Interop;
    using System.Threading;
    using System.Threading.Tasks;
//* Type mappings
    using KTL_LOG_ASN = System.UInt64;
    using BOOLEAN = System.SByte;
    using HRESULT = System.UInt32;
    using KTL_LOG_STREAM_ID = System.Guid;
    using KTL_LOG_STREAM_TYPE_ID = System.Guid;
    using KTL_LOG_ID = System.Guid;
    using KTL_DISK_ID = System.Guid;
    using EnumToken = System.UIntPtr;


    /// <summary>
    /// An IPhysicalLogManager interface isntance is related one-to-one to a KtlLogManager instance 
    /// (see KtlLogger.h). It is not a public interface but rather used to implement the public
    /// ILogManager aspects of the logical logger API.
    /// </summary>
    internal interface IPhysicalLogManager
    {
        /// <summary>
        /// Open the underlying KtlLogManager instance
        /// </summary>
        /// <param name="Token">Used to cancel the open operation</param>
        /// <returns>Task peforming the open</returns>
        Task OpenAsync(CancellationToken Token);

        /// <summary>
        /// Close the underlying KtlLogManager instance
        /// </summary>
        /// <param name="Token">Used to cancel the close operation</param>
        /// <returns>Task peforming the open</returns>
        Task CloseAsync(CancellationToken Token);

        /// <summary>
        /// Asynchronously create a new log container
        /// </summary>
        /// <param name="FullyQualifiedLogFilePath">Supplies fully qualified pathname to the log file - must be in the form of "\??\c:\..."</param>
        /// <param name="PhysicalLogId">Supplies ID of the log</param>
        /// <param name="PhysicalLogSize">Supplies the min desired size of the log container in bytes</param>
        /// <param name="MaxAllowedStreams">Supplies the maximum number of streams that can be created in the log container. Use 0 for default.</param>
        /// <param name="MaxRecordSize">Supplies the maximum size of a record that can be written to a stream in
        ///                            the log container. Use 0 if default is desired.</param>
        /// <param name="CreationFlags"></param>
        /// <param name="Token">Used to cancel the operation</param>
        /// <returns>Reference to the created (and open) container</returns>
        Task<IPhysicalLogContainer> CreateContainerAsync(
            string FullyQualifiedLogFilePath,
            Guid PhysicalLogId,
            Int64 PhysicalLogSize,
            UInt32 MaxAllowedStreams,
            UInt32 MaxRecordSize,
            LogManager.LogCreationFlags CreationFlags,
            CancellationToken Token);

        /// <summary>
        /// Open an existing log container
        /// </summary>
        /// <param name="FullyQualifiedLogFilePath">Supplies fully qualified pathname to the log container file</param>
        /// <param name="LogId">Supplies the unique ID of the log container used when created</param>
        /// <param name="Token">Used to cancel the operation</param>
        /// <returns>Reference to the opened container</returns>
        Task<IPhysicalLogContainer> OpenLogContainerAsync(string FullyQualifiedLogFilePath, Guid LogId, CancellationToken Token);

        /// <summary>
        /// Open an existing log container using the containing disk volume ID
        /// </summary>
        /// <param name="DiskId">Supplies volume GUID on which the log container resides</param>
        /// <param name="LogId">Supplies the unique ID of the log container used when created</param>
        /// <param name="Token">Used to cancel the operation</param>
        /// <returns>Reference to the opened container</returns>
        Task<IPhysicalLogContainer> OpenLogContainerAsync(Guid DiskId, Guid LogId, CancellationToken Token);

        /// <summary>
        /// Remove an existing log container
        /// </summary>
        /// <param name="FullyQualifiedLogFilePath">Supplies fully qualified pathname to the log container file</param>
        /// <param name="LogId">Supplies the unique ID of the log container used when created</param>
        /// <param name="Token">Used to cancel the operation</param>
        /// <returns>Delete operation's Task</returns>
        Task DeleteLogContainerAsync(string FullyQualifiedLogFilePath, Guid LogId, CancellationToken Token);

        /// <summary>
        /// Remove an existing log container using the containing disk volume ID
        /// </summary>
        /// <param name="DiskId">Supplies volume GUID on which the log container resides</param>
        /// <param name="LogId">Supplies the unique ID of the log container used when created</param>
        /// <param name="Token">Used to cancel the operation</param>
        /// <returns>Delete operation's Task</returns>
        Task DeleteLogContainerAsync(Guid DiskId, Guid LogId, CancellationToken Token);

        /// <summary>
        /// Enumerate all Log container IDs on a volume
        /// </summary>
        /// <param name="DiskId">Volume ID</param>
        /// <param name="Token">Used to cancel the operation</param>
        /// <returns>The requested Log container IDs</returns>
        Task<Guid[]> EnumerateLogIdsAsync(Guid DiskId, CancellationToken Token);

        /// <summary>
        /// Obtain a volume's unique ID given a fully qualified file system path 
        /// </summary>
        /// <param name="Path">Fully qualified file system path</param>
        /// <param name="Token">Used to cancel the operation</param>
        /// <returns>Requested volume ID</returns>
        Task<Guid> GetVolumeIdFromPathAsync(string Path, CancellationToken Token);
    }

    /// <summary>
    /// Managed implementation of KtlLogManager
    /// </summary>
    internal class PhysicalLogManager : IPhysicalLogManager
    {
        private NativeLog.IKPhysicalLogManager _NativeManager;
        private LogManager.LoggerType _LoggerType = LogManager.LoggerType.Unknown;

        public
            PhysicalLogManager(LogManager.LoggerType LoggerType)
        {
            switch (LoggerType)
            {
                case LogManager.LoggerType.Inproc:
                {
                    NativeLog.CreateLogManagerInproc(out this._NativeManager);
                    _LoggerType = LogManager.LoggerType.Inproc;
                    break;
                }

                case LogManager.LoggerType.Driver:
                {
                    NativeLog.CreateLogManager(out this._NativeManager);
                    _LoggerType = LogManager.LoggerType.Driver;
                    break;
                }

                default:
                {
                    throw new InvalidOperationException("Invalid LogManager.LoggerType");
                }
            }
        }

        public
            LogManager.LoggerType GetLoggerType()
        {
            return _LoggerType;
        }

        //* Open
        public Task
            OpenAsync(CancellationToken Token)
        {
            return Utility.WrapNativeAsyncInvokeInMTA(this.OpenBeginWrapper, this.OpenEndWrapper, Token, "NativeLog.Open");
        }

        private NativeCommon.IFabricAsyncOperationContext
            OpenBeginWrapper(NativeCommon.IFabricAsyncOperationCallback Callback)
        {
            NativeCommon.IFabricAsyncOperationContext context;

            this._NativeManager.BeginOpen(Callback, out context);
            return context;
        }

        private void
            OpenEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            this._NativeManager.EndOpen(context);
        }


        //* Close
        public Task
            CloseAsync(CancellationToken Token)
        {
            return Utility.WrapNativeAsyncInvokeInMTA(this.CloseBeginWrapper, this.CloseEndWrapper, Token, "NativeLog.Close");
        }

        private NativeCommon.IFabricAsyncOperationContext
            CloseBeginWrapper(NativeCommon.IFabricAsyncOperationCallback Callback)
        {
            NativeCommon.IFabricAsyncOperationContext context;

            this._NativeManager.BeginClose(Callback, out context);
            return context;
        }

        private void
            CloseEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            try
            {
                this._NativeManager.EndClose(context);
            }
            finally
            {
                this._NativeManager = null;
            }
        }


        //* CreateContainer
        public Task<IPhysicalLogContainer>
            CreateContainerAsync(
            string FullyQualifiedLogFilePath,
            Guid PhysicalLogId,
            Int64 PhysicalLogSize,
            UInt32 MaxAllowedStreams,
            UInt32 MaxRecordSize,
            LogManager.LogCreationFlags CreationFlags,
            CancellationToken Token)
        {
            return Utility.WrapNativeAsyncInvokeInMTA<IPhysicalLogContainer>(
                (Callback) => this.CreateContainerBeginWrapper(
                    FullyQualifiedLogFilePath,
                    PhysicalLogId,
                    PhysicalLogSize,
                    MaxAllowedStreams,
                    MaxRecordSize,
                    CreationFlags,
                    Callback),
                (Context) => this.CreateContainerEndWrapper(Context),
                Token,
                "NativeLog.CreateContainer");
        }

        private NativeCommon.IFabricAsyncOperationContext
            CreateContainerBeginWrapper(
            string FullyQualifiedLogFilePath,
            Guid PhysicalLogId,
            Int64 PhysicalLogSize,
            UInt32 MaxAllowedStreams,
            UInt32 MaxRecordSize,
            LogManager.LogCreationFlags CreationFlags,
            NativeCommon.IFabricAsyncOperationCallback Callback)
        {
            NativeCommon.IFabricAsyncOperationContext context;

            using (var pin = new PinCollection())
            {
                this._NativeManager.BeginCreateLogContainer(
                    pin.AddBlittable(FullyQualifiedLogFilePath),
                    PhysicalLogId,
                    pin.AddBlittable(""),
                    PhysicalLogSize,
                    MaxAllowedStreams,
                    MaxRecordSize,
                    CreationFlags,
                    Callback,
                    out context);
            }

            return context;
        }

        private IPhysicalLogContainer
            CreateContainerEndWrapper(NativeCommon.IFabricAsyncOperationContext Context)
        {
            NativeLog.IKPhysicalLogContainer result;

            this._NativeManager.EndCreateLogContainer(Context, out result);
            return new PhysicalLogContainer(result);
        }


        //* OpenContainer
        public Task<IPhysicalLogContainer>
            OpenLogContainerAsync(
            string FullyQualifiedLogFilePath,
            Guid PhysicalLogId,
            CancellationToken Token)
        {
            return Utility.WrapNativeAsyncInvokeInMTA<IPhysicalLogContainer>(
                (Callback) => this.OpenContainerBeginWrapper(FullyQualifiedLogFilePath, PhysicalLogId, Callback),
                (Context) => this.OpenContainerEndWrapper(Context),
                Token,
                "NativeLog.OpenContainer");
        }

        private NativeCommon.IFabricAsyncOperationContext
            OpenContainerBeginWrapper(
            string FullyQualifiedLogFilePath,
            Guid PhysicalLogId,
            NativeCommon.IFabricAsyncOperationCallback Callback)
        {
            NativeCommon.IFabricAsyncOperationContext context;

            using (var pin = new PinCollection())
            {
                this._NativeManager.BeginOpenLogContainer(
                    pin.AddBlittable(FullyQualifiedLogFilePath),
                    PhysicalLogId,
                    Callback,
                    out context);
            }

            return context;
        }

        public Task<IPhysicalLogContainer>
            OpenLogContainerAsync(Guid DiskId, Guid LogId, CancellationToken Token)
        {
            return Utility.WrapNativeAsyncInvokeInMTA<IPhysicalLogContainer>(
                (Callback) => this.OpenContainerByIdBeginWrapper(DiskId, LogId, Callback),
                (Context) => this.OpenContainerEndWrapper(Context),
                Token,
                "NativeLog.OpenContainerById");
        }

        private NativeCommon.IFabricAsyncOperationContext
            OpenContainerByIdBeginWrapper(
            Guid DiskId,
            Guid LogId,
            NativeCommon.IFabricAsyncOperationCallback Callback)
        {
            NativeCommon.IFabricAsyncOperationContext context;

            this._NativeManager.BeginOpenLogContainerById(
                DiskId,
                LogId,
                Callback,
                out context);

            return context;
        }

        private IPhysicalLogContainer
            OpenContainerEndWrapper(NativeCommon.IFabricAsyncOperationContext Context)
        {
            NativeLog.IKPhysicalLogContainer result;

            this._NativeManager.EndOpenLogContainer(Context, out result);
            return new PhysicalLogContainer(result);
        }


        //* DeleteContainer
        public Task
            DeleteLogContainerAsync(string FullyQualifiedLogFilePath, Guid LogId, CancellationToken Token)
        {
            return Utility.WrapNativeAsyncInvokeInMTA(
                (Callback) => this.DeleteContainerBeginWrapper(FullyQualifiedLogFilePath, LogId, Callback),
                (Context) => this.DeleteContainerEndWrapper(Context),
                Token,
                "NativeLog.DeleteContainer");
        }

        private NativeCommon.IFabricAsyncOperationContext
            DeleteContainerBeginWrapper(
            string FullyQualifiedLogFilePath,
            Guid PhysicalLogId,
            NativeCommon.IFabricAsyncOperationCallback Callback)
        {
            NativeCommon.IFabricAsyncOperationContext context;

            using (var pin = new PinCollection())
            {
                this._NativeManager.BeginDeleteLogContainer(
                    pin.AddBlittable(FullyQualifiedLogFilePath),
                    PhysicalLogId,
                    Callback,
                    out context);
            }

            return context;
        }

        public Task
            DeleteLogContainerAsync(Guid DiskId, Guid LogId, CancellationToken Token)
        {
            return Utility.WrapNativeAsyncInvokeInMTA(
                (Callback) => this.DeleteContainerByIdBeginWrapper(DiskId, LogId, Callback),
                (Context) => this.DeleteContainerEndWrapper(Context),
                Token,
                "NativeLog.DeleteContainerById");
        }

        private NativeCommon.IFabricAsyncOperationContext
            DeleteContainerByIdBeginWrapper(
            Guid DiskId,
            Guid LogId,
            NativeCommon.IFabricAsyncOperationCallback Callback)
        {
            NativeCommon.IFabricAsyncOperationContext context;

            this._NativeManager.BeginDeleteLogContainerById(
                DiskId,
                LogId,
                Callback,
                out context);

            return context;
        }

        private void
            DeleteContainerEndWrapper(NativeCommon.IFabricAsyncOperationContext Context)
        {
            this._NativeManager.EndDeleteLogContainer(Context);
        }


        //* EnumerateLogIds
        public Task<Guid[]>
            EnumerateLogIdsAsync(Guid DiskId, CancellationToken Token)
        {
            return Utility.WrapNativeAsyncInvokeInMTA<Guid[]>(
                (Callback) => this.EnumerateLogIdsBeginWrapper(DiskId, Callback),
                (Context) => this.EnumerateLogIdsEndWrapper(Context),
                Token,
                "NativeLog.EnumerateLogIds");
        }

        private NativeCommon.IFabricAsyncOperationContext
            EnumerateLogIdsBeginWrapper(
            Guid DiskId,
            NativeCommon.IFabricAsyncOperationCallback Callback)
        {
            NativeCommon.IFabricAsyncOperationContext context;

            this._NativeManager.BeginEnumerateLogContainers(DiskId, Callback, out context);
            return context;
        }

        private unsafe Guid[]
            EnumerateLogIdsEndWrapper(NativeCommon.IFabricAsyncOperationContext Context)
        {
            NativeLog.IKArray results;
            UInt32 arraySize;
            void* arrayBase;

            this._NativeManager.EndEnumerateLogContainers(Context, out results);
            results.GetCount(out arraySize);
            results.GetArrayBase(out arrayBase);

            var result = new Guid[arraySize];
            for (var ix = 0; ix < arraySize; ix++)
            {
                result[ix] = ((NativeLog.GuidStruct*) arrayBase)[ix].FromBytes();
            }

            return result;
        }


        //* GetVolumeIdFromPath
        public Task<Guid>
            GetVolumeIdFromPathAsync(string Path, CancellationToken Token)
        {
            return Utility.WrapNativeAsyncInvokeInMTA<Guid>(
                (Callback) => this.GetVolumeIdFromPathBeginWrapper(Path, Callback),
                (Context) => this.GetVolumeIdFromPathEndWrapper(Context),
                Token,
                "NativeLog.GetVolumeIdFromPath");
        }

        private NativeCommon.IFabricAsyncOperationContext
            GetVolumeIdFromPathBeginWrapper(
            string Path,
            NativeCommon.IFabricAsyncOperationCallback Callback)
        {
            NativeCommon.IFabricAsyncOperationContext context;

            using (var pin = new PinCollection())
            {
                this._NativeManager.BeginGetVolumeIdFromPath(
                    pin.AddBlittable(Path),
                    Callback,
                    out context);
            }

            return context;
        }

        private Guid
            GetVolumeIdFromPathEndWrapper(NativeCommon.IFabricAsyncOperationContext Context)
        {
            Guid result;

            this._NativeManager.EndGetVolumeIdFromPath(Context, out result);
            return result;
        }
    }
}