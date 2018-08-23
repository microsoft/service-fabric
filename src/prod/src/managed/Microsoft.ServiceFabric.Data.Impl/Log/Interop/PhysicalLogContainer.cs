// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.Log.Interop
{
    using System.Fabric.Interop;
    using System.Security.AccessControl;
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


    ///
    /// IPhysicalLogContainer represents a KtlLogContainer one-for-one; which manages the log container in 
    /// terms of physical space, cache, etc.
    ///
    /// Once a client gets an SPtr to a KtlLogContainer object, the log is already mounted.
    /// It does not have to and cannot call mount(). Only the KtlLogManager
    /// can mount a physical log.
    ///
    internal interface IPhysicalLogContainer
    {
        /// <summary>
        /// Determine if container is in a functional state
        /// </summary>
        /// <returns>false if the underlying container is in a faulted state</returns>
        bool IsFunctional();

        /// <summary>
        /// Close the underlying KtlLogContainer instance
        /// </summary>
        /// <param name="Token">Used to cancel the close operation</param>
        /// <returns>Task peforming the open</returns>
        Task CloseAsync(CancellationToken Token);

        /// <summary>
        /// Create an underlying physical log stream within the context of a current Log Container. 
        /// </summary>
        /// <param name="PhysicalLogStreamId">Unique ID (GUID) associated with the created stream.</param>
        /// <param name="PhysicalLogStreamTypeId"></param>
        /// <param name="OptionalLogStreamAlias">A short textual name to access the created stream in the future.</param>
        /// <param name="OptionalPath">Fully qualified path of the stream's private storage file. If null, a default system location will be used.</param>
        /// <param name="OptionalSecurityInfo">Optional access control constraints associated with the created stream.</param>
        /// <param name="MaximumSize">Maximum stream content size.</param>
        /// <param name="MaximumBlockSize">Maximum unit of buffering and I/O associated with the created stream.</param>
        /// <param name="CreationFlags"></param>
        /// <param name="CancellationToken">Used to cancel the operation</param>
        /// <returns>A reference to the IPhysicalLogStream representing the created stream.</returns>
        Task<IPhysicalLogStream> CreatePhysicalLogStreamAsync(
            Guid PhysicalLogStreamId,
            Guid PhysicalLogStreamTypeId,
            string OptionalLogStreamAlias,
            string OptionalPath,
            FileSecurity OptionalSecurityInfo,
            Int64 MaximumSize,
            UInt32 MaximumBlockSize,
            LogManager.LogCreationFlags CreationFlags,
            CancellationToken CancellationToken);

        /// <summary>
        /// Open access to an underlying physical log stream within the context of a current Log Container. 
        /// </summary>
        /// <param name="PhysicalLogStreamId">Unique ID (GUID) associated with the stream when it was created.</param>
        /// <param name="Token">Used to cancel the operation</param>
        /// <returns>A reference to the IPhysicalLogStream representing the opened stream.</returns>
        Task<IPhysicalLogStream> OpenPhysicalLogStreamAsync(Guid PhysicalLogStreamId, CancellationToken Token);

        /// <summary>
        /// Remove an underlying physical log stream from a current Log Container. 
        /// </summary>
        /// <param name="PhysicalLogStreamId">Unique ID (GUID) associated with the stream when it was created.</param>
        /// <param name="Token">Used to cancel the operation</param>
        Task DeletePhysicalLogStreamAsync(Guid PhysicalLogStreamId, CancellationToken Token);

        /// <summary>
        /// Associate an short textual name with an existing physical log stream.
        /// </summary>
        /// <param name="ToPhysicalLogStreamId">Unique ID (GUID) associated with the stream when it was created.</param>
        /// <param name="Alias">A short textual name used to access the existing stream in the future.</param>
        /// <param name="Token">Used to cancel the operation</param>
        Task AssignAliasAsync(Guid ToPhysicalLogStreamId, string Alias, CancellationToken Token);

        /// <summary>
        /// Dis-associate an short textual name with an existing physical log stream.
        /// </summary>
        /// <param name="Alias">A short textual name used to access the existing stream in the future.</param>
        /// <param name="Token">Used to cancel the operation</param>
        Task RemoveAliasAsync(string Alias, CancellationToken Token);

        /// <summary>
        /// Retrieve the Unique ID (GUID) associated with the stream corresponding to a supplied textual alias.
        /// </summary>
        /// <param name="Alias">A short textual name associated with an existing stream.</param>
        /// <param name="Token">Used to cancel the operation</param>
        /// <returns>Unique ID (GUID) associated with the stream</returns>
        Task<Guid> ResolveAliasAsync(string Alias, CancellationToken Token);
    }

    internal unsafe class PhysicalLogContainer : IPhysicalLogContainer
    {
        private NativeLog.IKPhysicalLogContainer _NativeContainer;

        public
            PhysicalLogContainer(NativeLog.IKPhysicalLogContainer NativeContainer)
        {
            this._NativeContainer = NativeContainer;
        }

        //* IsFunctional
        public bool
            IsFunctional()
        {
            return this._NativeContainer.IsFunctional() == (UInt32) 0;
        }


        //* CreatePhysicalLogStream
        public Task<IPhysicalLogStream>
            CreatePhysicalLogStreamAsync(
            Guid PhysicalLogStreamId,
            Guid PhysicalLogStreamTypeId,
            string OptionalLogStreamAlias,
            string OptionalPath,
            FileSecurity OptionalSecurityInfo,
            Int64 MaximumSize,
            UInt32 MaximumBlockSize,
            LogManager.LogCreationFlags CreationFlags,
            CancellationToken Token)
        {
            return Utility.WrapNativeAsyncInvokeInMTA<IPhysicalLogStream>(
                (Callback) => this.CreatePhysicalLogStreamBeginWrapper(
                    PhysicalLogStreamId,
                    PhysicalLogStreamTypeId,
                    OptionalLogStreamAlias,
                    OptionalPath,
                    OptionalSecurityInfo,
                    MaximumSize,
                    MaximumBlockSize,
                    CreationFlags,
                    Callback),
                (Context) => this.CreatePhysicalLogStreamEndWrapper(Context),
                Token,
                "NativeLog.CreateLogicalLog");
        }

        private NativeCommon.IFabricAsyncOperationContext
            CreatePhysicalLogStreamBeginWrapper(
            Guid PhysicalLogStreamId,
            Guid PhysicalLogStreamTypeId,
            string OptionalLogStreamAlias,
            string OptionalPath,
            FileSecurity OptionalSecurityInfo,
            Int64 MaximumSize,
            UInt32 MaximumBlockSize,
            LogManager.LogCreationFlags CreationFlags,
            NativeCommon.IFabricAsyncOperationCallback Callback)
        {
            NativeLog.IKBuffer secInfo = null;

            if (OptionalSecurityInfo != null)
            {
                var secDesc = OptionalSecurityInfo.GetSecurityDescriptorBinaryForm();
                NativeLog.CreateKBuffer((UInt32) secDesc.GetLength(0), out secInfo);
            }

            NativeCommon.IFabricAsyncOperationContext context;

            using (var pin = new PinCollection())
            {
                this._NativeContainer.BeginCreateLogStream(
                    PhysicalLogStreamId,
                    PhysicalLogStreamTypeId,
                    pin.AddBlittable(OptionalLogStreamAlias),
                    // CONSIDER: what about NULL Alias ?
                    pin.AddBlittable(OptionalPath),
                    secInfo,
                    // CONSIDER: does native code need to AddRef() ?
                    MaximumSize,
                    MaximumBlockSize,
                    CreationFlags,
                    Callback,
                    out context);
            }

            return context;
        }

        private IPhysicalLogStream
            CreatePhysicalLogStreamEndWrapper(
            NativeCommon.IFabricAsyncOperationContext Context)
        {
            NativeLog.IKPhysicalLogStream result = null;

            this._NativeContainer.EndCreateLogStream(Context, out result);
            return new PhysicalLogStream(result);
        }


        //* Close
        public Task
            CloseAsync(CancellationToken Token)
        {
            return Utility.WrapNativeAsyncInvokeInMTA(this.CloseBeginWrapper, this.CloseEndWrapper, Token, "NativeLog.CloseContainer");
        }

        private NativeCommon.IFabricAsyncOperationContext
            CloseBeginWrapper(NativeCommon.IFabricAsyncOperationCallback Callback)
        {
            NativeCommon.IFabricAsyncOperationContext context;

            this._NativeContainer.BeginClose(Callback, out context);
            return context;
        }

        private void
            CloseEndWrapper(NativeCommon.IFabricAsyncOperationContext Context)
        {
            try
            {
                this._NativeContainer.EndClose(Context);
            }
            finally
            {
                this._NativeContainer = null;
            }
        }

        //* OpenLogicalLog
        public Task<IPhysicalLogStream>
            OpenPhysicalLogStreamAsync(Guid PhysicalLogStreamId, CancellationToken Token)
        {
            return Utility.WrapNativeAsyncInvokeInMTA<IPhysicalLogStream>(
                (Callback) => this.OpenPhysicalLogStreamBeginWrapper(PhysicalLogStreamId, Callback),
                (Context) => this.OpenPhysicalLogStreamEndWrapper(Context),
                Token,
                "NativeLog.OpenLogicalLog");
        }

        private NativeCommon.IFabricAsyncOperationContext
            OpenPhysicalLogStreamBeginWrapper(
            Guid PhysicalLogStreamId,
            NativeCommon.IFabricAsyncOperationCallback Callback)
        {
            NativeCommon.IFabricAsyncOperationContext context;

            this._NativeContainer.BeginOpenLogStream(PhysicalLogStreamId, Callback, out context);
            return context;
        }

        private IPhysicalLogStream
            OpenPhysicalLogStreamEndWrapper(NativeCommon.IFabricAsyncOperationContext Context)
        {
            NativeLog.IKPhysicalLogStream stream;

            this._NativeContainer.EndOpenLogStream(Context, out stream);
            return new PhysicalLogStream(stream);
        }


        //* DeleteLogicalLog
        public Task
            DeletePhysicalLogStreamAsync(Guid PhysicalLogStreamId, CancellationToken Token)
        {
            return Utility.WrapNativeAsyncInvokeInMTA(
                (Callback) => this.DeletePhysicalLogStreamBeginWrapper(PhysicalLogStreamId, Callback),
                (Context) => this.DeletePhysicalLogStreamEndWrapper(Context),
                Token,
                "NativeLog.DeleteLogicalLog");
        }

        private NativeCommon.IFabricAsyncOperationContext
            DeletePhysicalLogStreamBeginWrapper(
            Guid PhysicalLogStreamId,
            NativeCommon.IFabricAsyncOperationCallback Callback)
        {
            NativeCommon.IFabricAsyncOperationContext context;

            this._NativeContainer.BeginDeleteLogStream(PhysicalLogStreamId, Callback, out context);
            return context;
        }

        private void
            DeletePhysicalLogStreamEndWrapper(NativeCommon.IFabricAsyncOperationContext Context)
        {
            this._NativeContainer.EndDeleteLogStream(Context);
        }


        //* AssignAlias
        public Task
            AssignAliasAsync(Guid PhysicalLogStreamId, string Alias, CancellationToken Token)
        {
            return Utility.WrapNativeAsyncInvokeInMTA(
                (Callback) => this.AssignAliasBeginWrapper(PhysicalLogStreamId, Alias, Callback),
                (Context) => this.AssignAliasEndWrapper(Context),
                Token,
                "NativeLog.AssignAlias");
        }

        private NativeCommon.IFabricAsyncOperationContext
            AssignAliasBeginWrapper(
            Guid ToLogicalLogId,
            string Alias,
            NativeCommon.IFabricAsyncOperationCallback Callback)
        {
            NativeCommon.IFabricAsyncOperationContext context;

            using (var pin = new PinCollection())
            {
                this._NativeContainer.BeginAssignAlias(
                    pin.AddBlittable(Alias),
                    ToLogicalLogId,
                    Callback,
                    out context);
            }
            return context;
        }

        private void
            AssignAliasEndWrapper(NativeCommon.IFabricAsyncOperationContext Context)
        {
            this._NativeContainer.EndAssignAlias(Context);
        }


        //* RemoveAlias
        public Task
            RemoveAliasAsync(string Alias, CancellationToken Token)
        {
            return Utility.WrapNativeAsyncInvokeInMTA(
                (Callback) => this.RemoveAliasBeginWrapper(Alias, Callback),
                (Context) => this.RemoveAliasEndWrapper(Context),
                Token,
                "NativeLog.RemoveAlias");
        }

        private NativeCommon.IFabricAsyncOperationContext
            RemoveAliasBeginWrapper(
            string Alias,
            NativeCommon.IFabricAsyncOperationCallback Callback)
        {
            NativeCommon.IFabricAsyncOperationContext context;

            using (var pin = new PinCollection())
            {
                this._NativeContainer.BeginRemoveAlias(
                    pin.AddBlittable(Alias),
                    Callback,
                    out context);
            }

            return context;
        }

        private void
            RemoveAliasEndWrapper(NativeCommon.IFabricAsyncOperationContext Context)
        {
            this._NativeContainer.EndRemoveAlias(Context);
        }


        public Task<Guid>
            ResolveAliasAsync(string Alias, CancellationToken Token)
        {
            return Utility.WrapNativeAsyncInvokeInMTA<Guid>(
                (Callback) => this.ResolveAliasBeginWrapper(Alias, Callback),
                (Context) => this.ResolveAliasEndWrapper(Context),
                InteropExceptionTracePolicy.None,
                Token,
                "NativeLog.ResolveAlias");
        }

        private NativeCommon.IFabricAsyncOperationContext
            ResolveAliasBeginWrapper(
            string Alias,
            NativeCommon.IFabricAsyncOperationCallback Callback)
        {
            NativeCommon.IFabricAsyncOperationContext context;

            using (var pin = new PinCollection())
            {
                this._NativeContainer.BeginResolveAlias(
                    pin.AddBlittable(Alias),
                    Callback,
                    out context);
            }

            return context;
        }

        private Guid
            ResolveAliasEndWrapper(NativeCommon.IFabricAsyncOperationContext Context)
        {
            Guid physicalLogStreamId;

            this._NativeContainer.EndResolveAlias(Context, out physicalLogStreamId);
            return physicalLogStreamId;
        }
    }
}