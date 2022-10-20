// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.ReliableMessaging.Session
{
    using Microsoft.ServiceFabric.Data;
    using System.Collections;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Fabric.Interop;
    using System.Fabric.ReliableMessaging.Interop;
    using System.IO;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;
    using BOOLEAN = System.SByte;

    /// class to pin message data and create an IFabricOperationData structure for passing to native layers
    public class ReliableSessionMessage
    {
        /// <summary>
        /// </summary>
        // pinned content of the message
        private IOperationData originalMessage;

        private PinCollection pinCollection;
        private NativeRuntime.IFabricOperationData savedNativeMessageRef;

        internal ReliableSessionMessage(IOperationData messageData)
        {
            this.originalMessage = messageData;
        }

        internal NativeRuntime.IFabricOperationData Pin(NativeReliableMessaging.IFabricMessageDataFactory messageDataFactory)
        {
            if (null != Interlocked.CompareExchange(ref this.pinCollection, new PinCollection(), null))
            {
                //
                // We do not allow the same ReliableSessionMessage object to be pinned concurrently.
                //
                throw new InvalidOperationException();
            }

            var sizes = this.originalMessage.Select(element => (uint) element.Count).ToArray();

            var bufferCount = (uint) sizes.Length;

            if (bufferCount == 0)
            {
                return null;
            }

            // pin the IOperationData buffers
            var pinnedBuffers = this.originalMessage.Select(element => this.pinCollection.AddBlittable(element.Array)).ToArray();

            NativeRuntime.IFabricOperationData messageData = null;

            using (PinBlittable pinSizes = new PinBlittable(sizes), pinBuffers = new PinBlittable(pinnedBuffers))
            {
                messageData = messageDataFactory.CreateMessageData(
                    bufferCount,
                    pinSizes.AddrOfPinnedObject(),
                    pinBuffers.AddrOfPinnedObject());
            }

            this.savedNativeMessageRef = messageData;

            return messageData;
        }

        internal void Unpin()
        {
            // AppTrace.TraceSource.WriteNoise("ReliableSessionMessage.Unpin");
            this.pinCollection.Dispose();
            this.savedNativeMessageRef = null;
            Interlocked.Exchange(ref this.pinCollection, null);
        }
    }

    internal sealed class OutboundMessage : IOperationData
    {
        private byte[] headerBuffer;
        private IOperationData data;

        public OutboundMessage(IOperationData data)
        {
            this.data = data;

            var sizes = data.Select(elem => elem.Count).ToArray();
            var segmentCount = sizes.Length;
            this.headerBuffer = new byte[sizeof(Int32) + segmentCount*sizeof(Int32)];
            var ms = new MemoryStream(this.headerBuffer);
            ms.Write(BitConverter.GetBytes(segmentCount), 0, sizeof(Int32));
            foreach (var i in sizes)
            {
                ms.Write(BitConverter.GetBytes(i), 0, sizeof(Int32));
            }
        }

        public IEnumerator<ArraySegment<byte>> GetEnumerator()
        {
            yield return new ArraySegment<byte>(this.headerBuffer);

            foreach (var segment in this.data)
            {
                yield return segment;
            }

            yield break;
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return this.GetEnumerator();
        }
    }


    internal sealed class SegmentStructureIterator : IEnumerable<Int32>
    {
        private byte[] data;
        private int segmentCount;

        public SegmentStructureIterator(int segmentCount, byte[] data)
        {
            this.data = data;
            this.segmentCount = segmentCount;
        }

        public IEnumerator<Int32> GetEnumerator()
        {
            for (var i = 0; i < this.segmentCount; i++)
            {
                var next = BitConverter.ToInt32(this.data, i*sizeof(Int32));
                yield return next;
            }

            yield break;
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return this.GetEnumerator();
        }
    }

    internal sealed class ChunkMaker
    {
        private IOperationData dataSource;
        private IEnumerator<ArraySegment<byte>> dataEnumerator;
        private ArraySegment<byte> currentSegment;
        private byte[] currentSegmentArray;
        private int currentSegmentOffset;
        private int currentSegmentCount;
        private int currentSegmentBytesRemaining;

        public ChunkMaker(IOperationData data)
        {
            // we only use this in scenarios like inbound messages where data cannot be null
            Debug.Assert(data != null);

            this.dataSource = data;
            this.dataEnumerator = this.dataSource.GetEnumerator();
            this.AdvanceEnumerator();
        }

        private void AdvanceEnumerator()
        {
            var success = this.dataEnumerator.MoveNext();
            if (success)
            {
                this.currentSegment = this.dataEnumerator.Current;
                this.currentSegmentArray = this.currentSegment.Array;
                this.currentSegmentOffset = this.currentSegment.Offset;
                this.currentSegmentCount = this.currentSegment.Count;
                this.currentSegmentBytesRemaining = this.currentSegmentCount;
            }
            else
            {
                this.currentSegmentArray = null;
                this.currentSegmentOffset = 0;
                this.currentSegmentCount = 0;
                this.currentSegmentBytesRemaining = 0;
            }
        }

        public byte[] GetNextChunk(int size)
        {
            var chunk = new byte[size];
            var ms = new MemoryStream(chunk);
            var bytesToWriteRemaining = size;
            while (bytesToWriteRemaining > 0 && this.currentSegmentArray != null)
            {
                var bytesToWrite = bytesToWriteRemaining > this.currentSegmentBytesRemaining ? this.currentSegmentBytesRemaining : bytesToWriteRemaining;
                ms.Write(this.currentSegmentArray, this.currentSegmentOffset, bytesToWrite);
                bytesToWriteRemaining = bytesToWriteRemaining - bytesToWrite;
                if (bytesToWrite == this.currentSegmentBytesRemaining)
                {
                    this.AdvanceEnumerator();
                }
                else
                {
                    this.currentSegmentOffset = this.currentSegmentOffset + bytesToWrite;
                    this.currentSegmentBytesRemaining = this.currentSegmentBytesRemaining - bytesToWrite;
                }
            }

            if (bytesToWriteRemaining == 0)
            {
                return chunk;
            }
            else
            {
                return null;
            }
        }
    }


    internal sealed class InboundMessage : IOperationData
    {
        private ChunkMaker dataChunks;
        private Int32 segmentCount;
        private SegmentStructureIterator sizesHeader;

        public InboundMessage(IOperationData message)
        {
            this.dataChunks = new ChunkMaker(message);

            var segmentCountBuffer = this.dataChunks.GetNextChunk(sizeof(Int32));
            this.segmentCount = BitConverter.ToInt32(segmentCountBuffer, 0);

            var sizesHeaderBuffer = this.dataChunks.GetNextChunk(this.segmentCount*sizeof(Int32));
            Debug.Assert(sizesHeaderBuffer != null);
            this.sizesHeader = new SegmentStructureIterator(this.segmentCount, sizesHeaderBuffer);
        }

        public IEnumerator<ArraySegment<byte>> GetEnumerator()
        {
            foreach (var size in this.sizesHeader)
            {
                var segment = this.dataChunks.GetNextChunk(size);
                Debug.Assert(segment != null);
                yield return new ArraySegment<byte>(segment);
            }

            yield break;
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return this.GetEnumerator();
        }
    }

    internal sealed class NativeReliableInboundSessionCallback : NativeReliableMessaging.IFabricReliableInboundSessionCallback
    {
        private IReliableInboundSessionCallback managedCallback;

        public NativeReliableInboundSessionCallback(IReliableInboundSessionCallback realCallback)
        {
            this.managedCallback = realCallback;
        }

        private unsafe long Int64KeyValue(IntPtr int64KeyRef)
        {
            return *(long*) int64KeyRef.ToPointer();
        }

        public int AcceptInboundSession(
            NativeReliableMessaging.IFabricReliableSessionPartitionIdentity partitionId,
            NativeReliableMessaging.IFabricReliableSession session)
        {
            var offeredSession = ReliableMessagingSession.FromNative(session);

            var nativeSvcName = partitionId.getServiceName();
            var svcInstance = NativeTypes.FromNativeUri(nativeSvcName);

            var result = false;

            var partitionKeyType = partitionId.getPartitionKeyType();
            var partitionKey = partitionId.getPartitionKey();

            if (partitionKeyType == NativeTypes.FABRIC_PARTITION_KEY_TYPE.FABRIC_PARTITION_KEY_TYPE_NONE)
            {
                result = this.managedCallback.AcceptInboundSession(svcInstance, offeredSession);
            }
            else if (partitionKeyType == NativeTypes.FABRIC_PARTITION_KEY_TYPE.FABRIC_PARTITION_KEY_TYPE_INT64)
            {
                var int64KeyRange = IntegerPartitionKeyRangeFactory.FromNative(partitionKey);
                result = this.managedCallback.AcceptInboundSession(svcInstance, int64KeyRange, offeredSession);
            }
            else if (partitionKeyType == NativeTypes.FABRIC_PARTITION_KEY_TYPE.FABRIC_PARTITION_KEY_TYPE_STRING)
            {
                result = this.managedCallback.AcceptInboundSession(svcInstance, NativeTypes.FromNativeString(partitionKey), offeredSession);
            }

            // TODO: if the key is an unknown type should be invalif arg
            // TODO: this whole boolean return API needs update
            if (result) return 1;
            else return 0;
        }
    }

    internal sealed class NativeReliableSessionAbortedCallback : NativeReliableMessaging.IFabricReliableSessionAbortedCallback
    {
        private IReliableSessionAbortedCallback managedCallback;

        public NativeReliableSessionAbortedCallback(IReliableSessionAbortedCallback realCallback)
        {
            this.managedCallback = realCallback;
        }

        private unsafe long Int64KeyValue(IntPtr int64KeyRef)
        {
            return *(long*) int64KeyRef.ToPointer();
        }

        public void SessionAbortedByPartner(
            BOOLEAN isInbound,
            NativeReliableMessaging.IFabricReliableSessionPartitionIdentity partitionId,
            NativeReliableMessaging.IFabricReliableSession session)
        {
            var closingSession = ReliableMessagingSession.FromNative(session);

            var nativeSvcName = partitionId.getServiceName();
            var svcInstance = NativeTypes.FromNativeUri(nativeSvcName);

            var partitionKeyType = partitionId.getPartitionKeyType();
            var partitionKey = partitionId.getPartitionKey();

            if (partitionKeyType == NativeTypes.FABRIC_PARTITION_KEY_TYPE.FABRIC_PARTITION_KEY_TYPE_NONE)
            {
                this.managedCallback.SessionAbortedByPartner(NativeTypes.FromBOOLEAN(isInbound), svcInstance, closingSession);
            }
            else if (partitionKeyType == NativeTypes.FABRIC_PARTITION_KEY_TYPE.FABRIC_PARTITION_KEY_TYPE_INT64)
            {
                var int64KeyRange = IntegerPartitionKeyRangeFactory.FromNative(partitionKey);
                this.managedCallback.SessionAbortedByPartner(NativeTypes.FromBOOLEAN(isInbound), svcInstance, int64KeyRange, closingSession);
            }
            else if (partitionKeyType == NativeTypes.FABRIC_PARTITION_KEY_TYPE.FABRIC_PARTITION_KEY_TYPE_STRING)
            {
                this.managedCallback.SessionAbortedByPartner(
                    NativeTypes.FromBOOLEAN(isInbound),
                    svcInstance,
                    NativeTypes.FromNativeString(partitionKey),
                    closingSession);
            }
        }
    }

    /// <summary>
    /// 
    /// </summary>
    public sealed class ReliableSessionManager : IReliableSessionManager
    {
        // native reliable session interface pointer.
        private NativeReliableMessaging.IFabricReliableSessionManager nativeSessionManager;

        /// <summary>
        /// Initialize and start the session manager service with owner information as parameter(s)
        /// Three overloads for singleton, numbered and named partition types
        /// </summary>
        /// <param name="replicaId"></param>
        /// <param name="ownerServiceInstanceName">The name of the owner service instance for the session</param>
        /// <param name="partitionId"></param>
        public ReliableSessionManager(Guid partitionId, long replicaId, Uri ownerServiceInstanceName)
        {
            // AppTrace.TraceSource.WriteNoise("ReliableMessaging.NativeReliableMessaging.FabricCreateReliableSession");

            this.SessionManagerConstructorHelper(
                partitionId,
                replicaId,
                ownerServiceInstanceName,
                NativeTypes.FABRIC_PARTITION_KEY_TYPE.FABRIC_PARTITION_KEY_TYPE_NONE,
                null);
        }

        /// <summary>
        /// overload for named partition owner
        /// </summary>
        /// <param name="replicaId"></param>
        /// <param name="ownerServiceInstanceName">The name of the owner service instance for the session</param>
        /// <param name="partitionId"></param>
        /// <param name="partitionKey"></param>
        public ReliableSessionManager(Guid partitionId, long replicaId, Uri ownerServiceInstanceName, string partitionKey)
        {
            // AppTrace.TraceSource.WriteNoise("ReliableMessaging.NativeReliableMessaging.FabricCreateReliableSession");

            this.SessionManagerConstructorHelper(
                partitionId,
                replicaId,
                ownerServiceInstanceName,
                NativeTypes.FABRIC_PARTITION_KEY_TYPE.FABRIC_PARTITION_KEY_TYPE_STRING,
                partitionKey);
        }


        /// <summary>
        /// overload for numbered partition owner
        /// </summary>
        /// <param name="replicaId"></param>
        /// <param name="ownerServiceInstanceName">The name of the owner service instance for the session</param>
        /// <param name="partitionId"></param>
        /// <param name="partitionKey"></param>
        public ReliableSessionManager(Guid partitionId, long replicaId, Uri ownerServiceInstanceName, IntegerPartitionKeyRange partitionKey)
        {
            // AppTrace.TraceSource.WriteNoise("ReliableMessaging.NativeReliableMessaging.FabricCreateReliableSession");

            this.SessionManagerConstructorHelper(
                partitionId,
                replicaId,
                ownerServiceInstanceName,
                NativeTypes.FABRIC_PARTITION_KEY_TYPE.FABRIC_PARTITION_KEY_TYPE_INT64,
                partitionKey);
        }


        private void SessionManagerConstructorHelper(
            Guid partitionId,
            long replicaId,
            Uri ownerServiceInstanceName,
            NativeTypes.FABRIC_PARTITION_KEY_TYPE keyType,
            object partitionKey)
        {
            // AppTrace.TraceSource.WriteNoise("ReliableMessaging.NativeReliableMessaging.FabricCreateReliableSession");

            // Marshal the serviceInstance URI
            // TODO: Call FreeHGlobal when done with this native string

            partitionKey = PartitionKeyHelpers.NormalizeManagerPartitionKey(partitionKey);

            using (var pin = new PinCollection())
            {
                this.nativeSessionManager = NativeReliableMessaging.CreateReliableSessionManager(
                    pin.AddBlittable(HostServicePartitionInfoFactory.ToNativeValue(partitionId, replicaId)),
                    pin.AddObject(ownerServiceInstanceName),
                    keyType,
                    pin.AddBlittable(partitionKey)
                    );
            }

            Debug.Assert(null != this.nativeSessionManager);
        }


        /// <summary>
        /// Start the session manager
        /// </summary>
        public Task OpenAsync(IReliableSessionAbortedCallback sessionClosedCallback, CancellationToken cancellationToken)
        {
            return
                Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.OpenBeginWrapper(sessionClosedCallback, callback),
                    this.OpenEndWrapper,
                    cancellationToken,
                    "NativeReliableSession.Open");
        }

        private NativeCommon.IFabricAsyncOperationContext OpenBeginWrapper(
            IReliableSessionAbortedCallback sessionClosedCallback, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            // AppTrace.TraceSource.WriteNoise("ReliableMessaging.NativeReliableSession.BeginOpen");
            // TODO: exception handling

            var nativeSessionAbortedCallback = new NativeReliableSessionAbortedCallback(sessionClosedCallback);

            return this.nativeSessionManager.BeginOpen(nativeSessionAbortedCallback, callback);
        }

        private void OpenEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            // AppTrace.TraceSource.WriteNoise("ReliableMessaging.NativeReliableSession.EndOpen");
            // TODO: exception handling
            this.nativeSessionManager.EndOpen(context);
        }


        /// <summary>
        /// Create the session
        /// Three overloads for singleton, numbered and named partition types
        /// </summary>
        /// <returns> The created session</returns>
        public Task<IReliableMessagingSession> CreateOutboundSessionAsync(
            Uri targetServiceInstanceName, string targetEndpoint, CancellationToken cancellationToken)
        {
            return this.CreateOutboundSessionAsyncHelper(
                targetServiceInstanceName,
                NativeTypes.FABRIC_PARTITION_KEY_TYPE.FABRIC_PARTITION_KEY_TYPE_NONE,
                null,
                targetEndpoint,
                cancellationToken);
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="targetServiceInstanceName"></param>
        /// <param name="partitionKey"></param>
        /// <param name="targetEndpoint"></param>
        /// <param name="cancellationToken"></param>
        /// <returns></returns>
        public Task<IReliableMessagingSession> CreateOutboundSessionAsync(
            Uri targetServiceInstanceName, long partitionKey, string targetEndpoint, CancellationToken cancellationToken)
        {
            return this.CreateOutboundSessionAsyncHelper(
                targetServiceInstanceName,
                NativeTypes.FABRIC_PARTITION_KEY_TYPE.FABRIC_PARTITION_KEY_TYPE_INT64,
                partitionKey,
                targetEndpoint,
                cancellationToken);
        }


        /// <summary>
        /// 
        /// </summary>
        /// <param name="targetServiceInstanceName"></param>
        /// <param name="partitionKey"></param>
        /// <param name="targetEndpoint"></param>
        /// <param name="cancellationToken"></param>
        /// <returns></returns>
        public Task<IReliableMessagingSession> CreateOutboundSessionAsync(
            Uri targetServiceInstanceName, string partitionKey, string targetEndpoint, CancellationToken cancellationToken)
        {
            return this.CreateOutboundSessionAsyncHelper(
                targetServiceInstanceName,
                NativeTypes.FABRIC_PARTITION_KEY_TYPE.FABRIC_PARTITION_KEY_TYPE_STRING,
                partitionKey,
                targetEndpoint,
                cancellationToken);
        }


        private NativeCommon.IFabricAsyncOperationContext CreateOutboundSessionAsyncBeginWrapper(
            Uri ownerServiceInstanceName,
            NativeTypes.FABRIC_PARTITION_KEY_TYPE keyType,
            object partitionKey,
            string targetEndpoint,
            NativeCommon.IFabricAsyncOperationCallback callback)
        {
            // AppTrace.TraceSource.WriteNoise("ReliableMessaging.NativeReliableMessaging.FabricCreateReliableSession");

            partitionKey = PartitionKeyHelpers.NormalizeManagerPartitionKey(partitionKey);

            using (var pin = new PinCollection())
            {
                return this.nativeSessionManager.BeginCreateOutboundSession(
                    pin.AddObject(ownerServiceInstanceName),
                    keyType,
                    pin.AddBlittable(partitionKey),
                    pin.AddBlittable(targetEndpoint),
                    callback);
            }
        }

        private IReliableMessagingSession CreateOutboundSessionAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            return ReliableMessagingSession.FromNative(this.nativeSessionManager.EndCreateOutboundSession(context));
        }

        private Task<IReliableMessagingSession> CreateOutboundSessionAsyncHelper(
            Uri targetServiceInstanceName,
            NativeTypes.FABRIC_PARTITION_KEY_TYPE keyType,
            object partitionKey,
            string targetEndpoint,
            CancellationToken cancellationToken)
        {
            return Utility.WrapNativeAsyncInvokeInMTA<IReliableMessagingSession>(
                (callback) => this.CreateOutboundSessionAsyncBeginWrapper(targetServiceInstanceName, keyType, partitionKey, targetEndpoint, callback),
                this.CreateOutboundSessionAsyncEndWrapper,
                cancellationToken,
                "ReliableSessionManager.CreateOutboundSession");
        }

        /// <summary>
        /// Register willingness to accept inbound sessions, with permission granted through
        ///    the IReliableInboundSessionCallback parameter supplied
        /// Temporarily missing the IReliableInboundSessionCallback input parameter
        ///    until KTL async compatible synchronous callback pattern is frigured out
        /// </summary>
        /// <returns>the endpoint (FQDN and port) at which the session is listening</returns>
        public Task<string> RegisterInboundSessionCallbackAsync(IReliableInboundSessionCallback callback, CancellationToken token)
        {
            return this.RegisterInboundSessionCallbackAsyncHelper(callback, token);
        }


        private NativeCommon.IFabricAsyncOperationContext RegisterInboundSessionCallbackAsyncBeginWrapper(
            IReliableInboundSessionCallback sessionCallback,
            NativeCommon.IFabricAsyncOperationCallback callback)
        {
            // AppTrace.TraceSource.WriteNoise("ReliableMessaging.NativeReliableMessaging.FabricCreateReliableSession");

            var nativeSessionCallback = new NativeReliableInboundSessionCallback(sessionCallback);

            return this.nativeSessionManager.BeginRegisterInboundSessionCallback(
                nativeSessionCallback,
                callback);
        }

        private string RegisterInboundSessionCallbackAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            var stringResult = this.nativeSessionManager.EndRegisterInboundSessionCallback(context);
            return NativeTypes.FromNativeString(stringResult.get_String());
        }

        private Task<string> RegisterInboundSessionCallbackAsyncHelper(
            IReliableInboundSessionCallback sessionCallback,
            CancellationToken cancellationToken)
        {
            return Utility.WrapNativeAsyncInvokeInMTA<string>(
                (callback) => this.RegisterInboundSessionCallbackAsyncBeginWrapper(sessionCallback, callback),
                this.RegisterInboundSessionCallbackAsyncEndWrapper,
                cancellationToken,
                "ReliableSessionManager.CreateOutboundSession");
        }


        /// <summary>
        /// Revoke willingness to accept inbound sessions
        /// </summary>
        /// <returns></returns>
        public Task UnregisterInboundSessionCallbackAsync(CancellationToken token)
        {
            return this.UnregisterInboundSessionCallbackAsyncHelper(token);
        }


        private NativeCommon.IFabricAsyncOperationContext UnregisterInboundSessionCallbackAsyncBeginWrapper(
            NativeCommon.IFabricAsyncOperationCallback callback)
        {
            // AppTrace.TraceSource.WriteNoise("ReliableMessaging.NativeReliableMessaging.FabricCreateReliableSession");

            return this.nativeSessionManager.BeginUnregisterInboundSessionCallback(callback);
        }

        private void UnregisterInboundSessionCallbackAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            this.nativeSessionManager.EndUnregisterInboundSessionCallback(context);
        }

        private Task UnregisterInboundSessionCallbackAsyncHelper(
            CancellationToken cancellationToken)
        {
            return Utility.WrapNativeAsyncInvokeInMTA(
                this.UnregisterInboundSessionCallbackAsyncBeginWrapper,
                this.UnregisterInboundSessionCallbackAsyncEndWrapper,
                cancellationToken,
                "ReliableSessionManager.CreateOutboundSession");
        }


        // <summary>
        // Abrupt shutdown of the session manager and its resources
        // </summary>   
        //public void Abort()
        //{
        //    Utility.WrapNativeSyncInvoke(this.nativeSessionManager.Abort, "ReliableSessionManager.Abort");
        //}

        /// <summary>
        /// Graceful shutdown of the session manager and its resources
        /// </summary>
        /// <returns></returns>
        public Task CloseAsync(CancellationToken token)
        {
            return this.CloseAsyncHelper(token);
        }


        private NativeCommon.IFabricAsyncOperationContext CloseAsyncBeginWrapper(NativeCommon.IFabricAsyncOperationCallback callback)
        {
            // AppTrace.TraceSource.WriteNoise("ReliableMessaging.NativeReliableMessaging.FabricCreateReliableSession");

            return this.nativeSessionManager.BeginClose(callback);
        }

        private void CloseAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            this.nativeSessionManager.EndClose(context);
        }

        private Task CloseAsyncHelper(
            CancellationToken cancellationToken)
        {
            return Utility.WrapNativeAsyncInvokeInMTA(
                this.CloseAsyncBeginWrapper,
                this.CloseAsyncEndWrapper,
                cancellationToken,
                "ReliableSessionManager.CreateOutboundSession");
        }
    }


    /// <summary>
    /// Describes statistics exposed by the btree. These will be used to report load metrics.
    /// </summary>
    public sealed class SessionDataSnapshotFactory
    {
        /// <summary>
        /// Populates session data snapshot from native snapshot
        /// </summary>
        /// <param name="sessionDataSnapshotIntPtr">Native stats to use.</param>
        /// <returns></returns>
        internal static unsafe SessionDataSnapshot FromNative(IntPtr sessionDataSnapshotIntPtr)
        {
            Debug.Assert(IntPtr.Zero != sessionDataSnapshotIntPtr);
            var nativeSnapshot = *((NativeReliableMessaging.SESSION_DATA_SNAPSHOT*) sessionDataSnapshotIntPtr);
            return FromNative(nativeSnapshot);
        }

        /// <summary>
        /// Populates this struct from native stats.
        /// </summary>
        /// <param name="nativeSnapshot"></param>
        /// <returns></returns>
        internal static SessionDataSnapshot FromNative(NativeReliableMessaging.SESSION_DATA_SNAPSHOT nativeSnapshot)
        {
            var snapshot = new SessionDataSnapshot
            {
                InboundMessageQuota = nativeSnapshot.InboundMessageQuota,
                InboundMessageCount = nativeSnapshot.InboundMessageCount,
                SendOperationQuota = nativeSnapshot.SendOperationQuota,
                SendOperationCount = nativeSnapshot.SendOperationCount,
                ReceiveOperationQuota = nativeSnapshot.ReceiveOperationQuota,
                ReceiveOperationCount = nativeSnapshot.ReceiveOperationCount,
                IsOutbound = NativeTypes.FromBOOLEAN(nativeSnapshot.IsOutbound),
                SessionId = nativeSnapshot.SessionId,
                NormalCloseOccurred = NativeTypes.FromBOOLEAN(nativeSnapshot.NormalCloseOccurred),
                IsOpenForSend = NativeTypes.FromBOOLEAN(nativeSnapshot.IsOpenForSend),
                IsOpenForReceive = NativeTypes.FromBOOLEAN(nativeSnapshot.IsOpenForReceive),
                LastOutboundSequenceNumber = nativeSnapshot.LastOutboundSequenceNumber,
                FinalInboundSequenceNumberInSession = nativeSnapshot.FinalInboundSequenceNumberInSession
            };

            return snapshot;
        }
    }


    /// <summary>
    /// 
    /// </summary>
    public sealed class HostServicePartitionInfoFactory
    {
        internal static NativeReliableMessaging.HOST_SERVICE_PARTITION_INFO ToNativeValue(Guid partitionId, long replicaId)
        {
            var nativePartitionInfo = new NativeReliableMessaging.HOST_SERVICE_PARTITION_INFO
            {
                ReplicaId = replicaId,
                PartitionId = partitionId
            };

            return nativePartitionInfo;
        }
    }


    /// <summary>
    /// Describes statistics exposed by the btree. These will be used to report load metrics.
    /// </summary>
    public sealed class IntegerPartitionKeyRangeFactory
    {
        /// <summary>
        /// Populates session data snapshot from native snapshot
        /// </summary>
        /// <param name="IntegerPartitionKeyRangeIntPtr"></param>
        /// <returns></returns>
        internal static unsafe IntegerPartitionKeyRange FromNative(IntPtr IntegerPartitionKeyRangeIntPtr)
        {
            Debug.Assert(IntPtr.Zero != IntegerPartitionKeyRangeIntPtr);
            var nativeKeyRange = *((NativeReliableMessaging.INTEGER_PARTITION_KEY_RANGE*) IntegerPartitionKeyRangeIntPtr);
            return FromNative(nativeKeyRange);
        }

        /// <summary>
        /// Populates this struct from native stats.
        /// </summary>
        /// <param name="nativeKeyRange"></param>
        /// <returns></returns>
        internal static IntegerPartitionKeyRange FromNative(NativeReliableMessaging.INTEGER_PARTITION_KEY_RANGE nativeKeyRange)
        {
            var keyRange = new IntegerPartitionKeyRange
            {
                IntegerKeyLow = nativeKeyRange.IntegerKeyLow,
                IntegerKeyHigh = nativeKeyRange.IntegerKeyHigh
            };

            return keyRange;
        }
    }

    internal class PartitionKeyHelpers
    {
        internal static object NormalizeManagerPartitionKey(object partitionKey)
        {
            if (partitionKey == null || partitionKey is string)
            {
                return partitionKey;
            }

            var key = partitionKey as IntegerPartitionKeyRange;
            if (key != null)
            {
                var range = key;
                var nativeRange = new NativeReliableMessaging.INTEGER_PARTITION_KEY_RANGE
                {
                    IntegerKeyLow = range.IntegerKeyLow,
                    IntegerKeyHigh = range.IntegerKeyHigh
                };
                return nativeRange;
            }

            if (partitionKey is long)
            {
                var singleNumber = (long) partitionKey;
                var nativeRange = new NativeReliableMessaging.INTEGER_PARTITION_KEY_RANGE
                {
                    IntegerKeyLow = singleNumber,
                    IntegerKeyHigh = singleNumber
                };
                return nativeRange;
            }

            throw new ArgumentException(SR.Error_PartitionKey_Format_Unsupported);
        }
    }


    /// <summary>
    /// 
    /// </summary>
    public sealed class ReliableMessagingSession : IReliableMessagingSession
    {
        // native reliable session interface pointer.
        private readonly NativeReliableMessaging.IFabricReliableSession nativeReliableSession;
        private readonly NativeReliableMessaging.IFabricMessageDataFactory nativeMessageDataFactory;

        internal static ReliableMessagingSession FromNative(
            NativeReliableMessaging.IFabricReliableSession nativeSession)
        {
            Debug.Assert(null != nativeSession);

            // AppTrace.TraceSource.WriteNoise("ReliableMessagingSession.FromNative");

            return new ReliableMessagingSession(nativeSession);
        }

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="nativeSession"></param>
        internal ReliableMessagingSession(
            NativeReliableMessaging.IFabricReliableSession nativeSession)
        {
            // AppTrace.TraceSource.WriteNoise("ReliableMessaging.NativeReliableSession.FabricCreateReliableSession");

            // Initialize native reliable session interface pointer.
            // TODO: exception handling
            this.nativeReliableSession = nativeSession;
            Debug.Assert(null != this.nativeReliableSession);
            this.nativeMessageDataFactory = (NativeReliableMessaging.IFabricMessageDataFactory) nativeSession;
        }


        /// <summary>
        /// Start the session
        /// </summary>
        /// <returns>Throws exception if the session cannot be started because the name cannot be resolved 
        /// or a connection cannot be established</returns>
        public Task OpenAsync(CancellationToken cancellationToken)
        {
            return
                Utility.WrapNativeAsyncInvokeInMTA(
                    this.OpenBeginWrapper,
                    this.OpenEndWrapper,
                    cancellationToken,
                    "NativeReliableSession.Open");
        }

        private NativeCommon.IFabricAsyncOperationContext OpenBeginWrapper(NativeCommon.IFabricAsyncOperationCallback callback)
        {
            // AppTrace.TraceSource.WriteNoise("ReliableMessaging.NativeReliableSession.BeginOpen");
            // TODO: exception handling
            return this.nativeReliableSession.BeginOpen(callback);
        }

        private void OpenEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            // AppTrace.TraceSource.WriteNoise("ReliableMessaging.NativeReliableSession.EndOpen");
            // TODO: exception handling
            this.nativeReliableSession.EndOpen(context);
        }


        /// <summary>
        /// Stop the session
        /// </summary>
        /// <returns>Throws exception if the session cannot be closed gracefully </returns>
        public Task CloseAsync(CancellationToken cancellationToken)
        {
            return
                Utility.WrapNativeAsyncInvokeInMTA(
                    this.CloseBeginWrapper,
                    this.CloseEndWrapper,
                    cancellationToken,
                    "NativeReliableSession.Close");
        }

        private NativeCommon.IFabricAsyncOperationContext CloseBeginWrapper(NativeCommon.IFabricAsyncOperationCallback callback)
        {
            // AppTrace.TraceSource.WriteNoise("ReliableMessaging.NativeReliableSession.BeginClose");
            // TODO: exception handling
            return this.nativeReliableSession.BeginClose(callback);
        }

        private void CloseEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            // AppTrace.TraceSource.WriteNoise("ReliableMessaging.NativeReliableSession.EndClose");
            // TODO: exception handling
            this.nativeReliableSession.EndClose(context);
        }

        /// <summary>
        /// Abrupt shutdown of the session protocol and connections and other resources
        /// </summary>   
        public void Abort()
        {
            // AppTrace.TraceSource.WriteNoise("ReliableMessaging.NativeReliableSession.Abort");
            // TODO: exception handling
            Utility.WrapNativeSyncInvoke(this.nativeReliableSession.Abort, "NativeReliableSession.Abort");
        }


        /// <summary>
        /// May be delayed locally if outbound buffer is full (throttling case)
        /// Returns to continuation when the message has been acknowledged as received by the target
        /// </summary>
        /// <param name="message"></param>
        /// <param name="cancellationToken"></param>
        public Task SendAsync(IOperationData message, CancellationToken cancellationToken)
        {
            if (message == null)
            {
                // TODO: do we support null messages, e.g., for pinging purposes?
                throw new ArgumentException(SR.Error_ReliableSession_NullArgument_SendAsync);
            }

            var sendMessage = new ReliableSessionMessage(new OutboundMessage(message));

            //TODO: what if sendMessage is null?
            return Utility.WrapNativeAsyncInvokeInMTA(
                (callback) => this.SendBeginWrapper(sendMessage, callback),
                (context) => this.SendEndWrapper(sendMessage, context),
                cancellationToken,
                "ReliableSession.SendAsync");
        }

        private NativeCommon.IFabricAsyncOperationContext SendBeginWrapper(
            ReliableSessionMessage sendMessage, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            try
            {
                var nativeSendMessage = sendMessage.Pin(this.nativeMessageDataFactory);

                // TODO: do we support empty messages?
                if (nativeSendMessage == null) throw new ArgumentException(SR.Error_ReliableSession_EmptyMessage);
                // AppTrace.TraceSource.WriteNoise("NativeReliableSession.BeginSend");
                return this.nativeReliableSession.BeginSend(nativeSendMessage, callback);
            }
            catch
            {
                sendMessage.Unpin();
                throw;
            }
        }

        private void SendEndWrapper(ReliableSessionMessage sendMessage, NativeCommon.IFabricAsyncOperationContext context)
        {
            try
            {
                // AppTrace.TraceSource.WriteNoise("NativeReliableSession.EndSend");
                this.nativeReliableSession.EndSend(context);
            }
            finally
            {
                sendMessage.Unpin();
            }
        }

        /// ReceiveAsync waits for messages TryReceiveAsync does not
        /// <summary>
        /// Async, return to continuation when inbound message available, wait for messages to come in if necessary
        /// </summary>
        /// <returns></returns>
        public Task<IOperationData> ReceiveAsync(CancellationToken cancellationToken)
        {
            return Utility.WrapNativeAsyncInvokeInMTA<IOperationData>(
                (callback) => this.ReceiveBeginWrapper(true, callback),
                this.ReceiveEndWrapper,
                cancellationToken,
                "ReliableSession.ReceiveAsync");
        }

        /// <summary>
        /// Same as ReceiveAsync, except do not wait for messages to come in, return null IOperationData if there are no messages waiting to be delivered
        /// </summary>
        /// <returns></returns>
        public Task<IOperationData> TryReceiveAsync(CancellationToken cancellationToken)
        {
            return Utility.WrapNativeAsyncInvokeInMTA<IOperationData>(
                (callback) => this.ReceiveBeginWrapper(false, callback),
                this.ReceiveEndWrapper,
                cancellationToken,
                "ReliableSession.ReceiveAsync");
        }

        private NativeCommon.IFabricAsyncOperationContext ReceiveBeginWrapper(bool waitForMessages, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            // AppTrace.TraceSource.WriteNoise("NativeReliableSession.BeginReceive");
            return this.nativeReliableSession.BeginReceive(NativeTypes.ToBOOLEAN(waitForMessages), callback);
        }

        private IOperationData ReceiveEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            NativeRuntime.IFabricOperationData nativeReceiveMessage = null;
            // AppTrace.TraceSource.WriteNoise("NativeReliableSession.EndReceive");
            nativeReceiveMessage = this.nativeReliableSession.EndReceive(context);
            IOperationData result = new ReadOnlyOperationData(nativeReceiveMessage);
            return new InboundMessage(result);
        }

        /// <summary>
        /// Retrieves session data snapshot
        /// </summary>
        /// 
        public SessionDataSnapshot GetDataSnapshot()
        {
            Debug.Assert(null != this.nativeReliableSession);
            return
                SessionDataSnapshotFactory.FromNative(
                    Utility.WrapNativeSyncInvoke<IntPtr>(this.nativeReliableSession.GetSessionDataSnapshot, "ReliableSession.DataSnapshot"));
        }
    }
}