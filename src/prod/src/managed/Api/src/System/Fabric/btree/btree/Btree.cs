// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.Btree
{
    using System;
    using System.Collections.Generic;
    using System.Runtime.InteropServices;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Globalization;
    using System.Diagnostics;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Data.Btree.Interop;
    using System.Fabric.Data.Common;
    using System.Fabric.Data.Replicator;
    using BOOLEAN = System.SByte;
    using FABRIC_ATOMIC_GROUP_ID = System.Int64;
    using FABRIC_INSTANCE_ID = System.Int64;
    using FABRIC_PARTITION_ID = System.Guid;
    using FABRIC_REPLICA_ID = System.Int64;
    using FABRIC_SEQUENCE_NUMBER = System.Int64;
    using FABRIC_TRANSACTION_ID = System.Guid;

    /// <summary>
    /// Data types supported as btree key type.
    /// </summary>
    public enum DataType : int
    {
        Invalid = NativeBtree.FABRIC_BTREE_DATA_TYPE.FABRIC_BTREE_DATA_TYPE_INVALID,
        Binary = NativeBtree.FABRIC_BTREE_DATA_TYPE.FABRIC_BTREE_DATA_TYPE_BINARY,
        Byte = NativeBtree.FABRIC_BTREE_DATA_TYPE.FABRIC_BTREE_DATA_TYPE_BYTE,
        Char = NativeBtree.FABRIC_BTREE_DATA_TYPE.FABRIC_BTREE_DATA_TYPE_WCHAR,
        DateTime = NativeBtree.FABRIC_BTREE_DATA_TYPE.FABRIC_BTREE_DATA_TYPE_DATETIME,
        TimeSpan = NativeBtree.FABRIC_BTREE_DATA_TYPE.FABRIC_BTREE_DATA_TYPE_TIMESPAN,
        Int16 = NativeBtree.FABRIC_BTREE_DATA_TYPE.FABRIC_BTREE_DATA_TYPE_SHORT,
        UInt16 = NativeBtree.FABRIC_BTREE_DATA_TYPE.FABRIC_BTREE_DATA_TYPE_USHORT,
        Int32 = NativeBtree.FABRIC_BTREE_DATA_TYPE.FABRIC_BTREE_DATA_TYPE_INT,
        UInt32 = NativeBtree.FABRIC_BTREE_DATA_TYPE.FABRIC_BTREE_DATA_TYPE_UINT,
        Int64 = NativeBtree.FABRIC_BTREE_DATA_TYPE.FABRIC_BTREE_DATA_TYPE_LONGLONG,
        UInt64 = NativeBtree.FABRIC_BTREE_DATA_TYPE.FABRIC_BTREE_DATA_TYPE_ULONGLONG,
        Single = NativeBtree.FABRIC_BTREE_DATA_TYPE.FABRIC_BTREE_DATA_TYPE_SINGLE,
        Double = NativeBtree.FABRIC_BTREE_DATA_TYPE.FABRIC_BTREE_DATA_TYPE_DOUBLE,
        Guid = NativeBtree.FABRIC_BTREE_DATA_TYPE.FABRIC_BTREE_DATA_TYPE_GUID,
        String = NativeBtree.FABRIC_BTREE_DATA_TYPE.FABRIC_BTREE_DATA_TYPE_WSTRING
    }


    /// <summary>
    /// List of operations currently supported by the btree.
    /// </summary>
    public enum BtreeOperationType
    {
        Invalid = NativeBtree.FABRIC_BTREE_OPERATION_TYPE.FABRIC_BTREE_OPERATION_INVALID,
        Insert = NativeBtree.FABRIC_BTREE_OPERATION_TYPE.FABRIC_BTREE_OPERATION_INSERT,
        Delete = NativeBtree.FABRIC_BTREE_OPERATION_TYPE.FABRIC_BTREE_OPERATION_DELETE,
        Update = NativeBtree.FABRIC_BTREE_OPERATION_TYPE.FABRIC_BTREE_OPERATION_UPDATE,
        PartialUpdate = NativeBtree.FABRIC_BTREE_OPERATION_TYPE.FABRIC_BTREE_OPERATION_PARTIAL_UPDATE,
        Erase = NativeBtree.FABRIC_BTREE_OPERATION_TYPE.FABRIC_BTREE_OPERATION_ERASE
    }

    /// <summary>
    /// Describes storage parameters for the underlying btree object.
    /// </summary>
    public sealed class BtreeStorageConfigurationDescription
    {
        /// <summary>
        /// Path to local log-structured store file, in the cas eof a persisted btree.
        /// </summary>
        public string Path;

        /// <summary>
        /// Total amount of storage the btree may use.
        /// </summary>
        public uint MaximumStorageInMB;

        /// <summary>
        /// True, if the btree is volatile (in-memory only). Peristed otherwise.
        /// </summary>
        public bool IsVolatile;

        /// <summary>
        /// Memory limit for the btree.
        /// </summary>
        public uint MaximumMemoryInMB;

        /// <summary>
        /// Page size for the btree. The page is elastic, this value specifies max size. 
        /// When max size is reached, the btree will split the page.
        /// </summary>
        public uint MaximumPageSizeInKB;

        /// <summary>
        /// True, if data is stored inline in btree pages. Pointers are stored otherwise.
        /// </summary>
        public bool StoreDataInline;

        /// <summary>
        /// Number of CAS failed comparisons before giving up.
        /// </summary>
        public uint RetriesBeforeTimeout;
    }

    /// <summary>
    /// Describes configuration parameters for the btree.
    /// </summary>
    public sealed class BtreeConfigurationDescription
    {
        /// <summary>
        /// Partition id of the partition hosting this btree.
        /// </summary>
        public Guid PartitionId;

        /// <summary>
        /// Replica id of the replica hosting this btree.
        /// </summary>
        public long ReplicaId;

        /// <summary>
        /// Storage configuration.
        /// </summary>
        public BtreeStorageConfigurationDescription StorageConfiguration;

        /// <summary>
        /// Describes how keys are being compared.
        /// </summary>
        public KeyComparisonDescription KeyComparison;

        /// <summary>
        /// Used for pinning this struct for interop.
        /// </summary>
        PinCollection pinCollection = new PinCollection();

        /// <summary>
        /// Prepares this struct for interop and returns address of top pinned object.
        /// </summary>
        internal IntPtr Pin()
        {
            if (null == this.StorageConfiguration || null == this.KeyComparison)
            {
                throw new InvalidOperationException();
            }

            NativeBtree.BTREE_CONFIGURATION_DESCRIPTION nativeConfiguration = new NativeBtree.BTREE_CONFIGURATION_DESCRIPTION();
            nativeConfiguration.PartitionId = this.PartitionId;
            nativeConfiguration.ReplicaId = this.ReplicaId;

            NativeBtree.BTREE_STORAGE_CONFIGURATION_DESCRIPTION nativeStorageConfiguration = new NativeBtree.BTREE_STORAGE_CONFIGURATION_DESCRIPTION();
            nativeStorageConfiguration.Path = string.IsNullOrEmpty(this.StorageConfiguration.Path) ? IntPtr.Zero : this.pinCollection.AddBlittable(this.StorageConfiguration.Path);
            nativeStorageConfiguration.MaximumStorageInMB = this.StorageConfiguration.MaximumStorageInMB;
            nativeStorageConfiguration.IsVolatile = NativeTypes.ToBOOLEAN(this.StorageConfiguration.IsVolatile);
            nativeStorageConfiguration.MaximumMemoryInMB = this.StorageConfiguration.MaximumMemoryInMB;
            nativeStorageConfiguration.MaximumPageSizeInKB = this.StorageConfiguration.MaximumPageSizeInKB;
            nativeStorageConfiguration.StoreDataInline = NativeTypes.ToBOOLEAN(this.StorageConfiguration.StoreDataInline);
            nativeStorageConfiguration.RetriesBeforeTimeout = this.StorageConfiguration.RetriesBeforeTimeout;
            nativeStorageConfiguration.Reserved = IntPtr.Zero;
            
            NativeBtree.BTREE_KEY_COMPARISON_DESCRIPTION nativeKeyComparison = new NativeBtree.BTREE_KEY_COMPARISON_DESCRIPTION();
            nativeKeyComparison.KeyDataType = (NativeBtree.FABRIC_BTREE_DATA_TYPE)this.KeyComparison.KeyDataType;
            if (null != this.KeyComparison.CultureInfo)
            {
                nativeKeyComparison.LCID = this.KeyComparison.CultureInfo.LCID;
            }
            else
            {
                nativeKeyComparison.LCID = CultureInfo.InvariantCulture.LCID;
            }
            nativeKeyComparison.MaximumKeySizeInBytes = this.KeyComparison.MaximumKeySizeInBytes;
            nativeKeyComparison.IsFixedLengthKey = NativeTypes.ToBOOLEAN(this.KeyComparison.IsFixedLengthKey);
            nativeKeyComparison.Reserved = IntPtr.Zero;

            nativeConfiguration.StorageConfiguration = this.pinCollection.AddBlittable(nativeStorageConfiguration);
            nativeConfiguration.KeyComparison = this.pinCollection.AddBlittable(nativeKeyComparison);
            
            nativeConfiguration.Reserved = IntPtr.Zero;

            return this.pinCollection.AddBlittable(nativeConfiguration);
        }

        /// <summary>
        /// Unpins this struct after interop has completed.
        /// </summary>
        internal void Unpin()
        {
            this.pinCollection.Dispose();
        }
    }

    /// <summary>
    /// Descrobes statistics exposed by the btree. These will be used to report load metrics.
    /// </summary>
    public sealed class BtreeStatistics
    {
        /// <summary>
        /// Populates this struct from native stats.
        /// </summary>
        /// <param name="btreeStatsIntPtr">Native stats to use.</param>
        /// <returns></returns>
        internal static unsafe BtreeStatistics FromNative(IntPtr btreeStatsIntPtr)
        {
            Debug.Assert(IntPtr.Zero != btreeStatsIntPtr);
            NativeBtree.BTREE_STATISTICS nativeStats = *((NativeBtree.BTREE_STATISTICS*)btreeStatsIntPtr);
            return BtreeStatistics.FromNative(nativeStats);
        }

        /// <summary>
        /// Populates this struct from native stats.
        /// </summary>
        /// <param name="nativeBtreeStats">Native stats to use.</param>
        /// <returns></returns>
        internal static unsafe BtreeStatistics FromNative(NativeBtree.BTREE_STATISTICS nativeBtreeStats)
        {
            BtreeStatistics stats = new BtreeStatistics();
            stats.MemoryUsageInBytes = nativeBtreeStats.MemoryUsageInBytes;
            stats.StorageUsageInBytes = nativeBtreeStats.StorageUsageInBytes;
            stats.PageCount = nativeBtreeStats.PageCount;
            stats.RecordCount = nativeBtreeStats.RecordCount;
            Debug.Assert(0 <= stats.MemoryUsageInBytes && 0 <= stats.StorageUsageInBytes && 0 <= stats.PageCount && 0 <= stats.RecordCount);
            return stats;
        }

        /// <summary>
        /// Size of RAM currently used by the btree.
        /// </summary>
        public int MemoryUsageInBytes;

        /// <summary>
        /// Size of storage used by the btree.
        /// </summary>
        public int StorageUsageInBytes;

        /// <summary>
        /// Current btree pages count.
        /// </summary>
        public int PageCount;

        /// <summary>
        /// Current btree record count.
        /// </summary>
        public int RecordCount;
    }

    /// <summary>
    /// Binary representation of a btree key.
    /// </summary>
    public interface IBtreeKey
    {
        /// <summary>
        /// Bytes representing the key. May be null.
        /// </summary>
        byte[] Bytes { get; }
    }

    /// <summary>
    /// Binary representation of a btree value.
    /// </summary>
    public interface IBtreeValue
    {
        /// <summary>
        /// Bytes representing the value. May be null.
        /// </summary>
        byte[] Bytes { get; }
    }

    /// <summary>
    /// Describes the current position in a btree scan.
    /// </summary>
    public interface IBtreeScanPosition
    {
        /// <summary>
        /// Current key.
        /// </summary>
        IBtreeKey Key { get; }

        /// <summary>
        /// Current value.
        /// </summary>
        IBtreeValue Value { get; }
    }

    /// <summary>
    /// Scan position class.
    /// </summary>
    sealed class BtreeScanPosition : IBtreeScanPosition
    {
        /// <summary>
        /// Constructor.
        /// </summary>
        /// <param name="key">Current key.</param>
        /// <param name="value">Current value.</param>
        public BtreeScanPosition(IBtreeKey key, IBtreeValue value)
        {
            this.key = key;
            this.value = value;
        }

        #region Instance Members

        /// <summary>
        /// Current key.
        /// </summary>
        IBtreeKey key;

        /// <summary>
        /// Current value.
        /// </summary>
        IBtreeValue value;

        #endregion

        /// <summary>
        /// Returns the current key in the scan position.
        /// </summary>
        public IBtreeKey Key
        {
            get { return this.key; }
        }

        /// <summary>
        /// Returns the current value in the scan position.
        /// </summary>
        public IBtreeValue Value
        {
            get { return this.value; }
        }
    }

    /// <summary>
    /// Btree scan functionality.
    /// </summary>
    public interface IBtreeScan
    {
        /// <summary>
        /// Open a key-ordered range scan on the index. Returns the key before the first key in the scan.
        /// </summary>
        /// <param name="beginKey">Range start key.</param>
        /// <param name="endKey">Range end key.</param>
        /// <param name="keyPrefixMatch">Prefix match key.</param>
        /// <param name="isValueNeeded">True, if values will be returned in the scan position while advancing the scan.</param>
        /// <param name="cancellationToken">Propagates notification that operation should be canceled.</param>
        /// <returns></returns>
        Task<IBtreeKey> OpenAsync(IBtreeKey beginKey, IBtreeKey endKey, IBtreeKey keyPrefixMatch, bool isValueNeeded, CancellationToken cancellationToken);

        /// <summary>
        /// Advances the scan. Returns the current btree scan position.
        /// </summary>
        /// <param name="cancellationToken">Propagates notification that operation should be canceled.</param>
        /// <returns></returns>
        Task<IBtreeScanPosition> MoveNextAsync(CancellationToken cancellationToken);

        /// <summary>
        /// Peek at the next key in the scan.
        /// </summary>
        /// <param name="cancellationToken">Propagates notification that operation should be canceled.</param>
        /// <returns></returns>
        Task<IBtreeKey> PeekNextKeyAsync(CancellationToken cancellationToken);

        /// <summary>
        /// Resets the scan. 
        /// </summary>
        /// <param name="cancellationToken">Propagates notification that operation should be canceled.</param>
        /// <returns></returns>
        Task ResetAsync(CancellationToken cancellationToken);

        /// <summary>
        /// Closes the scan.
        /// </summary>
        /// <param name="cancellationToken">Propagates notification that operation should be canceled.</param>
        /// <returns></returns>
        Task CloseAsync(CancellationToken cancellationToken);
    }

    /// <summary>
    /// Btree functionality.
    /// </summary>
    sealed class BtreeScan<K, V, KBC, VBC> : IBtreeScan
        where KBC : IKeyBitConverter<K>, new()
        where VBC : IValueBitConverter<V>, new()
    {
        /// <summary>
        /// Constructor.
        /// </summary>
        /// <param name="nativeBtreeScan">Native scan that backs this object.</param>
        public BtreeScan(NativeBtree.IFabricBtreeScan nativeBtreeScan)
        {
            this.nativeBtreeScan = nativeBtreeScan;
        }

        #region Instance Members

        /// <summary>
        // Native btree scan interface pointer.
        /// </summary>
        readonly NativeBtree.IFabricBtreeScan nativeBtreeScan;

        #endregion

        #region IBtreeScan Members

        /// <summary>
        /// Open a key-ordered range scan on the index. Returns the key before the first key in the scan.
        /// </summary>
        /// <param name="beginKey">Range start key.</param>
        /// <param name="endKey">Range end key.</param>
        /// <param name="keyPrefixMatch">Prefix match key.</param>
        /// <param name="isValueNeeded">True, if values will be returned in the scan position while advancing the scan.</param>
        /// <param name="cancellationToken">Propagates notification that operation should be canceled.</param>
        /// <returns></returns>
        public Task<IBtreeKey> OpenAsync(IBtreeKey beginKey, IBtreeKey endKey, IBtreeKey keyPrefixMatch, bool isValueNeeded, CancellationToken cancellationToken)
        {
            BtreeKey<K, KBC> btreeBeginKey = new BtreeKey<K, KBC>(beginKey.Bytes);
            BtreeKey<K, KBC> btreeEndKey = new BtreeKey<K, KBC>(endKey.Bytes);
            BtreeKey<K, KBC> btreePrefixKey = new BtreeKey<K, KBC>(keyPrefixMatch.Bytes);
            return Utility.WrapNativeAsyncInvoke<IBtreeKey>(
                (callback) => this.OpenBeginWrapper(btreeBeginKey, btreeEndKey, btreePrefixKey, isValueNeeded, callback),
                (context) => this.OpenEndWrapper(btreeBeginKey, btreeEndKey, btreePrefixKey, context),
                cancellationToken,
                "BtreeScan.Open");
        }

        NativeCommon.IFabricAsyncOperationContext OpenBeginWrapper(BtreeKey<K, KBC> beginKey, BtreeKey<K, KBC> endKey, BtreeKey<K, KBC> keyPrefixMatch, bool isValueNeeded, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            NativeBtree.IFabricBtreeKey nativeBeginKey = null;
            NativeBtree.IFabricBtreeKey nativeEndKey = null;
            NativeBtree.IFabricBtreeKey nativePrefixKey = null;
            try
            {
                nativeBeginKey = (null != beginKey) ? beginKey.Pin() : null;
                nativeEndKey = (null != endKey) ? endKey.Pin() : null;
                nativePrefixKey = (null != keyPrefixMatch) ? keyPrefixMatch.Pin() : null;
                AppTrace.TraceSource.WriteNoise("Btree.NativeBtreeScan.BeginOpen");
                return this.nativeBtreeScan.BeginOpen(nativeBeginKey, nativeEndKey, nativePrefixKey, NativeTypes.ToBOOLEAN(isValueNeeded), callback);
            }
            catch 
            {
                if (null != beginKey)
                {
                    beginKey.Unpin();
                }

                if (null != endKey)
                {
                    endKey.Unpin();
                }

                if (null != keyPrefixMatch)
                {
                    keyPrefixMatch.Unpin();
                }

                throw;
            }
        }

        IBtreeKey OpenEndWrapper(BtreeKey<K, KBC> beginKey, BtreeKey<K, KBC> endKey, BtreeKey<K, KBC> keyPrefixMatch, NativeCommon.IFabricAsyncOperationContext context)
        {
            NativeBtree.IFabricBtreeKey nativeKey;
            try
            {
                AppTrace.TraceSource.WriteNoise("Btree.NativeBtreeScan.EndOpen");
                nativeKey = this.nativeBtreeScan.EndOpen(context);
            }
            catch
            {
                throw;
            }
            finally
            {
                if (null != beginKey)
                {
                    beginKey.Unpin();
                }

                if (null != endKey)
                {
                    endKey.Unpin();
                }

                if (null != keyPrefixMatch)
                {
                    keyPrefixMatch.Unpin();
                }
            }
            return BtreeKey<K, KBC>.CreateFromNative(nativeKey);
        }

        /// <summary>
        /// Advances the scan. Returns the current btree scan position.
        /// </summary>
        /// <param name="cancellationToken">Propagates notification that operation should be canceled.</param>
        /// <returns></returns>
        public Task<IBtreeScanPosition> MoveNextAsync(CancellationToken cancellationToken)
        {
            return Utility.WrapNativeAsyncInvoke<IBtreeScanPosition>(
                this.MoveNextBeginWrapper,
                this.MoveNextEndWrapper,
                cancellationToken,
                "BtreeScan.MoveNext");
        }

        NativeCommon.IFabricAsyncOperationContext MoveNextBeginWrapper(NativeCommon.IFabricAsyncOperationCallback callback)
        {
            AppTrace.TraceSource.WriteNoise("Btree.NativeBtreeScan.BeginMoveNext");
            return this.nativeBtreeScan.BeginMoveNext(callback);
        }

        IBtreeScanPosition MoveNextEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            AppTrace.TraceSource.WriteNoise("Btree.NativeBtreeScan.EndMoveNext");
            NativeBtree.IFabricBtreeScanPosition nativeScanPosition = this.nativeBtreeScan.EndMoveNext(context);
            return new BtreeScanPosition(
                BtreeKey<K, KBC>.CreateFromNative(nativeScanPosition.GetKey()), 
                BtreeValue<V, VBC>.CreateFromNative(nativeScanPosition.GetValue()));
        }

        /// <summary>
        /// Peek at the next key in the scan.
        /// </summary>
        /// <param name="cancellationToken">Propagates notification that operation should be canceled.</param>
        /// <returns></returns>
        public Task<IBtreeKey> PeekNextKeyAsync(CancellationToken cancellationToken)
        {
            return Utility.WrapNativeAsyncInvoke<IBtreeKey>(
                this.PeekNextKeyBeginWrapper,
                this.PeekNextKeyEndWrapper,
                cancellationToken,
                "BtreeScan.PeekNextKey");
        }

        NativeCommon.IFabricAsyncOperationContext PeekNextKeyBeginWrapper(NativeCommon.IFabricAsyncOperationCallback callback)
        {
            AppTrace.TraceSource.WriteNoise("Btree.NativeBtreeScan.BeginPeekNextKey");
            return this.nativeBtreeScan.BeginPeekNextKey(callback);
        }

        IBtreeKey PeekNextKeyEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            AppTrace.TraceSource.WriteNoise("Btree.NativeBtreeScan.EndPeekNextKey");
            NativeBtree.IFabricBtreeKey nativeKey = this.nativeBtreeScan.EndPeekNextKey(context);
            return BtreeKey<K, KBC>.CreateFromNative(nativeKey);
        }

        /// <summary>
        /// Resets the scan. 
        /// </summary>
        /// <param name="cancellationToken">Propagates notification that operation should be canceled.</param>
        /// <returns></returns>
        public Task ResetAsync(CancellationToken cancellationToken)
        {
            return Utility.WrapNativeAsyncInvoke(
                this.ResetBeginWrapper,
                this.ResetEndWrapper,
                cancellationToken,
                "BtreeScan.Reset");
        }

        NativeCommon.IFabricAsyncOperationContext ResetBeginWrapper(NativeCommon.IFabricAsyncOperationCallback callback)
        {
            AppTrace.TraceSource.WriteNoise("Btree.NativeBtreeScan.BeginReset");
            return this.nativeBtreeScan.BeginReset(callback);
        }

        void ResetEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            AppTrace.TraceSource.WriteNoise("Btree.NativeBtreeScan.EndReset");
            this.nativeBtreeScan.EndReset(context);
        }

        /// <summary>
        /// Closes the scan. Scan is unusable after this call completes.
        /// </summary>
        /// <param name="cancellationToken">Propagates notification that operation should be canceled.</param>
        /// <returns></returns>
        public Task CloseAsync(CancellationToken cancellationToken)
        {
            return Utility.WrapNativeAsyncInvoke(
                this.CloseBeginWrapper,
                this.CloseEndWrapper,
                cancellationToken,
                "BtreeScan.Close");
        }

        NativeCommon.IFabricAsyncOperationContext CloseBeginWrapper(NativeCommon.IFabricAsyncOperationCallback callback)
        {
            AppTrace.TraceSource.WriteNoise("Btree.NativeBtreeScan.BeginClose");
            return this.nativeBtreeScan.BeginClose(callback);
        }

        void CloseEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            AppTrace.TraceSource.WriteNoise("Btree.NativeBtreeScan.EndClose");
            this.nativeBtreeScan.EndClose(context);
        }

        #endregion
    }

    /// <summary>
    /// Btree operation.
    /// </summary>
    public interface IBtreeOperation
    {
        /// <summary>
        /// Btree operation type.
        /// </summary>
        BtreeOperationType OperationType { get; }

        /// <summary>
        /// Key part of this operation.
        /// </summary>
        IBtreeKey Key { get; }

        /// <summary>
        /// Value part of this operation.
        /// </summary>
        IBtreeValue Value { get; }

        /// <summary>
        /// Conditional value part of this operation. May be null.
        /// </summary>
        IBtreeValue ConditionalValue { get; }

        /// <summary>
        /// Conditional offset part of this operation. May be zero.
        /// </summary>
        uint ConditionalOffset { get; }

        /// <summary>
        /// Partial update offset part of this operation. May be zero.
        /// </summary>
        uint PartialUpdateOffset { get; }
    }

    /// <summary>
    /// Btree operation functionality.
    /// </summary>
    sealed class BtreeOperation : IBtreeOperation
    {
        /// <summary>
        /// Constructor.
        /// </summary>
        /// <param name="operationType">Operation type.</param>
        /// <param name="key">Operation key.</param>
        /// <param name="value">Operation value.</param>
        /// <param name="conditionalValue">Operation conditional value. May be null.</param>
        /// <param name="conditionalOffset">Operation conditional offset. May be zero.</param>
        /// <param name="partialUpdateOffset">Operation partial update offset. May be zero.</param>
        public BtreeOperation(
            BtreeOperationType operationType, 
            IBtreeKey key, 
            IBtreeValue value, 
            IBtreeValue conditionalValue, 
            uint conditionalOffset, 
            uint partialUpdateOffset)
        {
            this.operationType = operationType;
            this.key = key;
            this.value = value;
            this.conditionalValue = conditionalValue;
            this.conditionalOffset = conditionalOffset;
            this.partialUpdateOffset = partialUpdateOffset;
        }

        #region Instance Members

        /// <summary>
        /// 
        /// </summary>
        BtreeOperationType operationType = BtreeOperationType.Invalid;

        /// <summary>
        /// 
        /// </summary>
        IBtreeKey key;

        /// <summary>
        /// 
        /// </summary>
        IBtreeValue value;

        /// <summary>
        /// 
        /// </summary>
        IBtreeValue conditionalValue;

        /// <summary>
        /// 
        /// </summary>
        uint conditionalOffset;

        /// <summary>
        /// 
        /// </summary>
        uint partialUpdateOffset;

        #endregion

        #region IBtreeOperation Members

        /// <summary>
        /// Btree operation type.
        /// </summary>
        public BtreeOperationType OperationType
        {
            get { return this.operationType; }
        }

        /// <summary>
        /// Key part of this operation.
        /// </summary>
        public IBtreeKey Key
        {
            get { return this.key; }
        }

        /// <summary>
        /// Value part of this operation.
        /// </summary>
        public IBtreeValue Value
        {
            get { return this.value; }
        }

        /// <summary>
        /// Conditional value part of this operation. May be null.
        /// </summary>
        public IBtreeValue ConditionalValue
        {
            get { return this.conditionalValue; }
        }

        /// <summary>
        /// Conditional offset part of this operation. May be zero.
        /// </summary>
        public uint ConditionalOffset
        {
            get { return this.conditionalOffset; }
        }

        /// <summary>
        /// Partial update offset part of this operation. May be zero.
        /// </summary>
        public uint PartialUpdateOffset
        {
            get { return this.partialUpdateOffset; }
        }

        #endregion
    }

    /// <summary>
    /// Btree public interface.
    /// </summary>
    public interface IBtree
    {
        /// <summary>
        /// Retrieves btree statistics.
        /// </summary>
        BtreeStatistics Statistics { get; }

        /// <summary>
        /// Opens the btree.
        /// </summary>
        /// <param name="configuration">Btree configuration.</param>
        /// <param name="storageExists">True if storage should already exist.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task OpenAsync(BtreeConfigurationDescription configuration, bool storageExists, CancellationToken cancellationToken);

        /// <summary>
        /// Insert a new key and value into the btree.
        /// </summary>
        /// <param name="key">Key to insert.</param>
        /// <param name="value">Value to insert.</param>
        /// <param name="sequenceNumber">Operation sequence number.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task<IRedoUndoInformation> InsertAsync(
            IBtreeKey key,
            IBtreeValue value,
            long sequenceNumber,
            CancellationToken cancellationToken);

        /// <summary>
        /// Insert a new key and value into the btree. If the key exists, the existent value is returned.
        /// </summary>
        /// <param name="key">Key to insert.</param>
        /// <param name="value">Value to insert.</param>
        /// <param name="sequenceNumber">Operation sequence number.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task<Tuple<IBtreeValue, IRedoUndoInformation>> InsertWithOutputAsync(
            IBtreeKey key,
            IBtreeValue value,
            long sequenceNumber,
            CancellationToken cancellationToken);

        /// <summary>
        /// Inserts a new key and value, or updates the existent key with the new value.
        /// </summary>
        /// <param name="key">Key to insert/update.</param>
        /// <param name="value">Value to insert/update.</param>
        /// <param name="sequenceNumber">Operation sequence number.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task<Tuple<IBtreeValue, IRedoUndoInformation>> UpsertAsync(
            IBtreeKey key,
            IBtreeValue value,
            long sequenceNumber,
            CancellationToken token);

        /// <summary>
        /// Update the value for a given key.
        /// </summary>
        /// <param name="key">Key to update.</param>
        /// <param name="value">Value to update.</param>
        /// <param name="sequenceNumber">Operation sequence number.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task<IRedoUndoInformation> UpdateAsync(
            IBtreeKey key,
            IBtreeValue value,
            long sequenceNumber,
            CancellationToken cancellationToken);

        /// <summary>
        /// Update the value for a given key. Returns the old value.
        /// </summary>
        /// <param name="key">Key to update.</param>
        /// <param name="value">New value to update.</param>
        /// <param name="sequenceNumber">Operation sequence number.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task<Tuple<IBtreeValue, IRedoUndoInformation>> UpdateWithOutputAsync(
            IBtreeKey key,
            IBtreeValue value,
            long sequenceNumber,
            CancellationToken cancellationToken);

        /// <summary>
        /// Conditionally updates a given key.
        /// </summary>
        /// <param name="key">Key to update.</param>
        /// <param name="value">value to update.</param>
        /// <param name="conditionalCheckValueOffset">Offset for conditional value check.</param>
        /// <param name="conditionalCheckValue">Value to use for the conditional check.</param>
        /// <param name="sequenceNumber">Operation sequence number.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task<Tuple<bool, IRedoUndoInformation>> ConditionalUpdateAsync(
            IBtreeKey key, 
            IBtreeValue value, 
            uint conditionalCheckValueOffset, 
            IBtreeValue conditionalCheckValue, 
            long sequenceNumber,
            CancellationToken cancellationToken);

        /// <summary>
        /// Partially update a value for a given key.
        /// </summary>
        /// <param name="key">Key to update.</param>
        /// <param name="partialUpdateOffset">Partial update offset.</param>
        /// <param name="partialValue">Partial value to be used for update.</param>
        /// <param name="conditionalCheckValueOffset">Conditional check offset.</param>
        /// <param name="conditionalCheckValue">Conditional check value used for the update.</param>
        /// <param name="sequenceNumber">Operation sequence number.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task<Tuple<bool, IRedoUndoInformation>> ConditionalPartialUpdateAsync(
            IBtreeKey key,
            uint partialUpdateOffset,
            IBtreeValue partialValue,
            uint conditionalCheckValueOffset,
            IBtreeValue conditionalCheckValue,
            long sequenceNumber,
            CancellationToken cancellationToken);

        /// <summary>
        /// Deletes a key and a value from the btree. Returns true if the delete has been performed.
        /// </summary>
        /// <param name="key">Key to delete.</param>
        /// <param name="sequenceNumber">Operation sequence number.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task<Tuple<bool, IRedoUndoInformation>> DeleteAsync(
            IBtreeKey key,
            long sequenceNumber,
            CancellationToken cancellationToken);

        /// <summary>
        /// Deletes a given key from the btree.
        /// </summary>
        /// <param name="key">Key to delete.</param>
        /// <param name="sequenceNumber">Operation sequence number.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task<Tuple<IBtreeValue, IRedoUndoInformation>> DeleteWithOutputAsync(IBtreeKey key, long sequenceNumber, CancellationToken cancellationToken);

        /// <summary>
        /// Conditionally deletes a key from the btree. Returns true if the delete has been performed.
        /// </summary>
        /// <param name="key">Key to delete.</param>
        /// <param name="conditionalCheckValueOffset">Conditional check offset.</param>
        /// <param name="conditionalCheckValue">Conditional value to be used for the delete.</param>
        /// <param name="sequenceNumber">Operation sequence number.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task<Tuple<bool, IRedoUndoInformation>> ConditionalDeleteAsync(
            IBtreeKey key,
            uint conditionalCheckValueOffset,
            IBtreeValue conditionalCheckValue,
            long sequenceNumber,
            CancellationToken cancellationToken);

        /// <summary>
        /// Seek/read the value for a given key.
        /// </summary>
        /// <param name="key">Key to read value for.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task<IBtreeValue> SeekAsync(IBtreeKey key, CancellationToken cancellationToken);

        /// <summary>
        /// Seek/read the a partial value for a given key. 
        /// </summary>
        /// <param name="key">Key to read.</param>
        /// <param name="partialSeekValueOffset">Value offset to read at.</param>
        /// <param name="partialSeekValueSize">Value size to read.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task<IBtreeValue> PartialSeekAsync(
            IBtreeKey key,
            uint partialSeekValueOffset,
            uint partialSeekValueSize,
            CancellationToken cancellationToken);

        /// <summary>
        /// Atomically drop all data from the btree.
        /// </summary>
        /// <param name="sequenceNumber">Operation sequence number.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task<Tuple<bool, IRedoUndoInformation>> EraseAsync(long sequenceNumber, CancellationToken cancellationToken);

        /// <summary>
        /// Decodes and optionally reconstructs the btree operation encapsulated in the given operation data.
        /// </summary>
        /// <param name="sequenceNumber">Operation sequence number.</param>
        /// <param name="operation">Buffers encoding the btree operation.</param>
        /// <param name="decodeOnly">True if only decode is needed and operation is not actually performed.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task<IBtreeOperation> ApplyWithOutputAsync(long sequenceNumber, IOperationData operation, bool decodeOnly, CancellationToken cancellationToken);

        /// <summary>
        /// Applies copy bytes to a newly constructed btree.
        /// </summary>
        /// <param name="copyStreamData">Bytes to apply.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task ApplyCopyDataAsync(IOperationData copyStreamData, CancellationToken cancellationToken);

        /// <summary>
        /// Checkpoints the btree up to the given sequence number. This operation is mandatory.
        /// </summary>
        /// <param name="sequenceNumber">Operation sequence number.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task CheckpointAsync(long sequenceNumber, CancellationToken cancellationToken);

        /// <summary>
        /// Informs the btree that operations up to the given sequence number are not stable.
        /// </summary>
        /// <param name="sequenceNumber">Operation sequence number.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task<long> OnOperationStableAsync(long sequenceNumber, CancellationToken cancellationToken);

        /// <summary>
        /// Returns a stream that represents the state to be copied out of this btree into another btree.
        /// </summary>
        /// <param name="upToSequenceNumber">Sequence number used to copy btree data.</param>
        /// <returns></returns>
        IOperationDataStreamEx GetCopyState(long upToSequenceNumber);

        /// <summary>
        /// Creates a btree scan object.
        /// </summary>
        /// <returns></returns>
        IBtreeScan CreateScan();

        /// <summary>
        /// Closes the btree.
        /// </summary>
        /// <param name="eraseStorage">True, if the storage associated with the btree is to be deallocated.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task CloseAsync(bool eraseStorage, CancellationToken cancellationToken);

        /// <summary>
        /// Aborts new operations on the btree.
        /// </summary>
        void Abort();

        /// <summary>
        /// Returns the last committed sequence number in the btree.
        /// </summary>
        /// <returns></returns>
        long GetLastCommittedSequenceNumber();
    }

    /// <summary>
    /// Btree key functionality.
    /// </summary>
    sealed class BtreeKey<K, KBC> : IBtreeKey, NativeBtree.IFabricBtreeKey where KBC : IKeyBitConverter<K>, new()
    {
        #region Instance Members

        /// <summary>
        /// 
        /// </summary>
        KBC keyBitConverter = new KBC();

        #endregion

        /// <summary>
        /// 
        /// </summary>
        /// <param name="key"></param>
        public BtreeKey(K key)
        {
            if (null != key)
            {
                this.key = keyBitConverter.ToByteArray(key);
            }
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="bytes"></param>
        public BtreeKey(byte[] bytes)
        {
            this.key = bytes;
        }

        /// <summary>
        /// 
        /// </summary>
        public byte[] Bytes
        {
            get { return this.key; }
        }

        /// <summary>
        /// 
        /// </summary>
        public K Key
        {
            get
            {
                return this.keyBitConverter.FromByteArray(this.key);
            }
        }

        /// <summary>
        /// 
        /// </summary>
        byte[] key;

        /// <summary>
        /// 
        /// </summary>
        IntPtr keyPinned = IntPtr.Zero;

        /// <summary>
        /// 
        /// </summary>
        /// <param name="count"></param>
        /// <returns></returns>
        public IntPtr GetBytes(out UInt32 count)
        {
            if (null == this.key)
            {
                count = 0;
                return IntPtr.Zero;
            }
            else
            {
                Debug.Assert(IntPtr.Zero != this.keyPinned);
                count = (uint)this.key.Length;
                return this.keyPinned;
            }
        }

        /// <summary>
        /// 
        /// </summary>
        PinCollection pinCollection;

        /// <summary>
        /// 
        /// </summary>
        internal NativeBtree.IFabricBtreeKey Pin()
        {
            if (null != Interlocked.CompareExchange(ref this.pinCollection, new PinCollection(), null))
            {
                //
                // We do not allow the same BtreeKey object to be pinned concurrently.
                //
                throw new InvalidOperationException();
            }
            if (null != this.key)
            {
                this.keyPinned = this.pinCollection.AddBlittable(this.key);
            }
            NativeBtree.IFabricBtreeKey nativeKey = (NativeBtree.IFabricBtreeKey)this;
            return nativeKey;
        }

        /// <summary>
        /// 
        /// </summary>
        internal void Unpin()
        {
            this.pinCollection.Dispose();
            this.keyPinned = IntPtr.Zero;
            Interlocked.Exchange(ref this.pinCollection, null);
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="nativeKey"></param>
        /// <returns></returns>
        internal static BtreeKey<K, KBC> CreateFromNative(NativeBtree.IFabricBtreeKey nativeKey)
        {
            Debug.Assert(null != nativeKey);

            UInt32 count;
            IntPtr buffer = nativeKey.GetBytes(out count);
            return BtreeKey<K, KBC>.CreateFromNative(count, buffer);
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="count"></param>
        /// <param name="buffer"></param>
        /// <returns></returns>
        static BtreeKey<K, KBC> CreateFromNative(UInt32 count, IntPtr buffer)
        {
            if (0 == count)
            {
                return new BtreeKey<K, KBC>(null);
            }
            if (IntPtr.Zero == buffer)
            {
                throw new ArgumentNullException("buffer");
            }
            return BtreeKey<K, KBC>.CreateFromNativeInternal(count, buffer);
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="count"></param>
        /// <param name="buffer"></param>
        /// <returns></returns>
        static unsafe BtreeKey<K, KBC> CreateFromNativeInternal(UInt32 count, IntPtr buffer)
        {
            return new BtreeKey<K, KBC>(NativeTypes.FromNativeBytes(buffer, count));
        }
    }

    /// <summary>
    /// 
    /// </summary>
    sealed class BtreeValue<V, VBC> : IBtreeValue, NativeBtree.IFabricBtreeValue where VBC : IValueBitConverter<V>, new()
    {
        #region Instance Members

        /// <summary>
        /// 
        /// </summary>
        private VBC valueBitConverter = new VBC();

        #endregion
        
        /// <summary>
        /// 
        /// </summary>
        /// <param name="value"></param>
        public BtreeValue(V value)
        {
            this.value = this.valueBitConverter.ToByteArray(value);
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="bytes"></param>
        public BtreeValue(byte[] bytes)
        {
            this.value = bytes;
        }

        /// <summary>
        /// 
        /// </summary>
        public byte[] Bytes
        {
            get 
            { 
                return this.value; 
            }
        }

        /// <summary>
        /// 
        /// </summary>
        public V Value
        {
            get
            {
                return this.valueBitConverter.FromByteArray(this.value);
            }
        }

        /// <summary>
        /// 
        /// </summary>
        byte[] value;

        /// <summary>
        /// 
        /// </summary>
        IntPtr valuePinned = IntPtr.Zero;

        /// <summary>
        /// 
        /// </summary>
        /// <param name="count"></param>
        /// <returns></returns>
        public IntPtr GetBytes(out UInt32 count)
        {
            if (null == this.value)
            {
                count = 0;
                return IntPtr.Zero;
            }
            else
            {
                Debug.Assert(IntPtr.Zero != this.valuePinned);
                count = (uint)this.value.Length;
                return this.valuePinned;
            }
        }

        /// <summary>
        /// 
        /// </summary>
        PinCollection pinCollection;

        /// <summary>
        /// 
        /// </summary>
        internal NativeBtree.IFabricBtreeValue Pin()
        {
            if (null != Interlocked.CompareExchange(ref this.pinCollection, new PinCollection(), null))
            {
                //
                // We do not allow the same BtreeValue object to be pinned concurrently.
                //
                throw new InvalidOperationException();
            }
            if (null != this.value)
            {
                this.valuePinned = this.pinCollection.AddBlittable(this.value);
            }
            NativeBtree.IFabricBtreeValue nativeValue = (NativeBtree.IFabricBtreeValue)this;
            return nativeValue;
        }

        /// <summary>
        /// 
        /// </summary>
        internal void Unpin()
        {
            this.pinCollection.Dispose();
            this.valuePinned = IntPtr.Zero;
            Interlocked.Exchange(ref this.pinCollection, null);
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="nativeValue"></param>
        /// <returns></returns>
        internal static BtreeValue<V, VBC> CreateFromNative(NativeBtree.IFabricBtreeValue nativeValue)
        {
            Debug.Assert(null != nativeValue);

            UInt32 count;
            IntPtr buffer = nativeValue.GetBytes(out count);
            return BtreeValue<V, VBC>.CreateFromNative(count, buffer);
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="count"></param>
        /// <param name="buffer"></param>
        /// <returns></returns>
        static BtreeValue<V, VBC> CreateFromNative(UInt32 count, IntPtr buffer)
        {
            if (0 == count)
            {
                return new BtreeValue<V, VBC>(null);
            }
            if (IntPtr.Zero == buffer)
            {
                throw new ArgumentNullException("buffer");
            }
            return BtreeValue<V, VBC>.CreateFromNativeInternal(count, buffer);
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="count"></param>
        /// <param name="buffer"></param>
        /// <returns></returns>
        static unsafe BtreeValue<V, VBC> CreateFromNativeInternal(UInt32 count, IntPtr buffer)
        {
            AppTrace.TraceSource.WriteNoise("BtreeValue.CreateFromNative", "{0}", "non-null");
            return new BtreeValue<V, VBC>(NativeTypes.FromNativeBytes(buffer, count));
        }
    }

    /// <summary>
    /// 
    /// </summary>
    sealed class BtreeOperationDataStream : IOperationDataStreamEx
    {
        #region Instance member
        //
        // Native btree operation stream interface pointer.
        // 
        readonly NativeRuntime.IFabricOperationDataStream nativeOperationStream;
        #endregion

        /// <summary>
        /// 
        /// </summary>
        /// <param name="nativeOperationStream"></param>
        public BtreeOperationDataStream(NativeRuntime.IFabricOperationDataStream nativeOperationStream)
        {
            this.nativeOperationStream = nativeOperationStream;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="cancellationToken"></param>
        /// <returns></returns>
        public Task<IOperationData> GetNextAsync(CancellationToken cancellationToken)
        {
            return Utility.WrapNativeAsyncInvoke<IOperationData>(
                (callback) => this.GetNextBeginWrapper(callback), 
                (callback) => this.GetNextEndWrapper(callback), 
                cancellationToken, 
                "BtreeOperationDataStream.GetNext");
        }

        NativeCommon.IFabricAsyncOperationContext GetNextBeginWrapper(NativeCommon.IFabricAsyncOperationCallback callback)
        {
            AppTrace.TraceSource.WriteNoise("Btree.NativeOperationStream.BeginGetNext");
            return this.nativeOperationStream.BeginGetNext(callback);
        }

        IOperationData GetNextEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            AppTrace.TraceSource.WriteNoise("Btree.NativeOperationStream.EndGetNext");
            NativeRuntime.IFabricOperationData nativeOperationData = this.nativeOperationStream.EndGetNext(context);
            return new ReadOnlyOperationData(nativeOperationData);
        }
    }

    /// <summary>
    /// Btree functionality.
    /// </summary>
    /// <typeparam name="K">Key type.</typeparam>
    /// <typeparam name="V">Value type.</typeparam>
    /// <typeparam name="KBC">Key bit converter.</typeparam>
    /// <typeparam name="VBC">Value type converter.</typeparam>
    sealed class Btree<K, V, KBC, VBC> : IBtree
        where KBC : IKeyBitConverter<K>, new()
        where VBC : IValueBitConverter<V>, new()
    {
        #region Instance Members

        //
        // Native btree interface pointer.
        // 
        readonly NativeBtree.IFabricBtree nativeBtree;

        #endregion

        /// <summary>
        /// Constructor.
        /// </summary>
        public Btree()
        {
            AppTrace.TraceSource.WriteNoise("Btree.NativeBtree.CreateBtree");
            //
            // Initialize native btree interface pointer.
            // 
            this.nativeBtree = NativeBtree.CreateBtree();
            Debug.Assert(null != this.nativeBtree);
        }

        /// <summary>
        /// Retrieves btree statistics.
        /// </summary>
        public BtreeStatistics Statistics
        {
            get 
            {
                AppTrace.TraceSource.WriteNoise("Btree.NativeBtree.GetStatistics");
                Debug.Assert(null != this.nativeBtree);
                return BtreeStatistics.FromNative(Utility.WrapNativeSyncInvoke<IntPtr>(this.nativeBtree.GetStatistics, "Btree.GetStatistics"));
            }
        }

        /// <summary>
        /// Opens the btree.
        /// </summary>
        /// <param name="configuration">Btree configuration.</param>
        /// <param name="storageExists">True if storage should already exist.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        public Task OpenAsync(BtreeConfigurationDescription configuration, bool storageExists, CancellationToken cancellationToken)
        {
            //
            // Check arguments.
            //
            if (null == configuration || null == configuration.StorageConfiguration || null == configuration.KeyComparison)
            {
                throw new ArgumentNullException("configuration");
            }
            return Utility.WrapNativeAsyncInvoke(
                (callback) => this.OpenBeginWrapper(configuration, storageExists, callback),
                (context) => this.OpenEndWrapper(configuration, context),
                cancellationToken,
                "Btree.Open");
        }

        NativeCommon.IFabricAsyncOperationContext OpenBeginWrapper(BtreeConfigurationDescription configuration, bool storageExists, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            try
            {
                IntPtr nativeConfiguration = (null != configuration) ? configuration.Pin() : IntPtr.Zero;
                AppTrace.TraceSource.WriteNoise("Btree.NativeBtree.BeginOpen");
                return this.nativeBtree.BeginOpen(nativeConfiguration, NativeTypes.ToBOOLEAN(storageExists), callback);
            }
            catch
            {
                if (null != configuration) configuration.Unpin();
                throw;
            }
        }

        void OpenEndWrapper(BtreeConfigurationDescription configuration, NativeCommon.IFabricAsyncOperationContext context)
        {
            try
            {
                AppTrace.TraceSource.WriteNoise("Btree.NativeBtree.EndOpen");
                this.nativeBtree.EndOpen(context);
            }
            catch
            {
                throw;
            }
            finally
            {
                if (null != configuration) configuration.Unpin();
            }
        }

        /// <summary>
        /// Insert a new key and value into the btree.
        /// </summary>
        /// <param name="key">Key to insert.</param>
        /// <param name="value">Value to insert.</param>
        /// <param name="sequenceNumber">Operation sequence number.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        public Task<IRedoUndoInformation> InsertAsync(IBtreeKey key, IBtreeValue value, long sequenceNumber, CancellationToken cancellationToken)
        {
            BtreeKey<K, KBC> btreeKey = new BtreeKey<K, KBC>(key.Bytes);
            BtreeValue<V, VBC> btreeValue = new BtreeValue<V, VBC>(value.Bytes);
            return Utility.WrapNativeAsyncInvoke<IRedoUndoInformation>(
                (callback) => this.InsertBeginWrapper(btreeKey, btreeValue, sequenceNumber, callback),
                (context) => this.InsertEndWrapper(btreeKey, btreeValue, context),
                cancellationToken,
                "Btree.Insert");
        }

        NativeCommon.IFabricAsyncOperationContext InsertBeginWrapper(BtreeKey<K, KBC> key, BtreeValue<V, VBC> value, long sequenceNumber, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            NativeBtree.IFabricBtreeKey nativeKey = null;
            NativeBtree.IFabricBtreeValue nativeValue = null;
            try
            {
                nativeKey = (null != key) ? key.Pin() : null;
                nativeValue = (null != value) ? value.Pin() : null;
                AppTrace.TraceSource.WriteNoise("Btree.NativeBtree.BeginInsert");
                return this.nativeBtree.BeginInsert(nativeKey, nativeValue, sequenceNumber, callback);
            }
            catch 
            {
                if (null != key) key.Unpin();
                if (null != value) value.Unpin();
                throw;
            }
        }

        IRedoUndoInformation InsertEndWrapper(BtreeKey<K, KBC> key, BtreeValue<V, VBC> value, NativeCommon.IFabricAsyncOperationContext context)
        {
            NativeRuntime.IFabricOperationData nativeRedo;
            NativeRuntime.IFabricOperationData nativeUndo;
            try
            {
                AppTrace.TraceSource.WriteNoise("Btree.NativeBtree.EndInsert");
                this.nativeBtree.EndInsert(context, out nativeRedo, out nativeUndo);
            }
            catch 
            {
                throw;
            }
            finally
            {
                if (null != key) key.Unpin();
                if (null != value) value.Unpin();
            }
            return new RedoUndoInformation(new ReadOnlyOperationData(nativeRedo), new ReadOnlyOperationData(nativeUndo));
        }

        /// <summary>
        /// Insert a new key and value into the btree. If the key exists, the existent value is returned.
        /// </summary>
        /// <param name="key">Key to insert.</param>
        /// <param name="value">Value to insert.</param>
        /// <param name="sequenceNumber">Operation sequence number.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        public Task<Tuple<IBtreeValue, IRedoUndoInformation>> InsertWithOutputAsync(IBtreeKey key, IBtreeValue value, long sequenceNumber, CancellationToken cancellationToken)
        {
            BtreeKey<K, KBC> btreeKey = new BtreeKey<K, KBC>(key.Bytes);
            BtreeValue<V, VBC> btreeValue = new BtreeValue<V, VBC>(value.Bytes);
            return Utility.WrapNativeAsyncInvoke<Tuple<IBtreeValue, IRedoUndoInformation>>(
                (callback) => this.InsertWithOutputBeginWrapper(btreeKey, btreeValue, sequenceNumber, callback),
                (context) => this.InsertWithOutputEndWrapper(btreeKey, btreeValue, context),
                cancellationToken,
                "Btree.InsertWithOutput");
        }

        NativeCommon.IFabricAsyncOperationContext InsertWithOutputBeginWrapper(BtreeKey<K, KBC> key, BtreeValue<V, VBC> value, long sequenceNumber, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            NativeBtree.IFabricBtreeKey nativeKey = null;
            NativeBtree.IFabricBtreeValue nativeValue = null;
            try
            {
                nativeKey = (null != key) ? key.Pin() : null;
                nativeValue = (null != value) ? value.Pin() : null;
                AppTrace.TraceSource.WriteNoise("Btree.NativeBtree.BeginInsertWithOutput");
                return this.nativeBtree.BeginInsertWithOutput(nativeKey, nativeValue, sequenceNumber, callback);
            }
            catch 
            {
                if (null != key) key.Unpin();
                if (null != value) value.Unpin();
                throw;
            }
        }

        Tuple<IBtreeValue, IRedoUndoInformation> InsertWithOutputEndWrapper(BtreeKey<K, KBC> key, BtreeValue<V, VBC> value, NativeCommon.IFabricAsyncOperationContext context)
        {
            NativeRuntime.IFabricOperationData nativeRedo;
            NativeRuntime.IFabricOperationData nativeUndo;
            NativeBtree.IFabricBtreeValue nativeValue;
            try
            {
                AppTrace.TraceSource.WriteNoise("Btree.NativeBtree.EndInsertWithOutput");
                nativeValue = this.nativeBtree.EndInsertWithOutput(context, out nativeRedo, out nativeUndo);
            }
            catch 
            {
                throw;
            }
            finally
            {
                if (null != key) key.Unpin();
                if (null != value) value.Unpin();
            }
            return new Tuple<IBtreeValue, IRedoUndoInformation>(
                BtreeValue<V, VBC>.CreateFromNative(nativeValue), 
                new RedoUndoInformation(new ReadOnlyOperationData(nativeRedo), new ReadOnlyOperationData(nativeUndo)));
        }

        /// <summary>
        /// Inserts a new key and value, or updates the existent key with the new value.
        /// </summary>
        /// <param name="key">Key to insert/update.</param>
        /// <param name="value">Value to insert/update.</param>
        /// <param name="sequenceNumber">Operation sequence number.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        public Task<Tuple<IBtreeValue, IRedoUndoInformation>> UpsertAsync(IBtreeKey key, IBtreeValue value, long sequenceNumber, CancellationToken cancellationToken)
        {
            BtreeKey<K, KBC> btreeKey = new BtreeKey<K, KBC>(key.Bytes);
            BtreeValue<V, VBC> btreeValue = new BtreeValue<V, VBC>(value.Bytes);
            return Utility.WrapNativeAsyncInvoke<Tuple<IBtreeValue, IRedoUndoInformation>>(
                (callback) => this.UpsertBeginWrapper(btreeKey, btreeValue, sequenceNumber, callback),
                (context) => this.UpsertEndWrapper(btreeKey, btreeValue, context),
                cancellationToken,
                "Btree.Upsert");
        }

        NativeCommon.IFabricAsyncOperationContext UpsertBeginWrapper(BtreeKey<K, KBC> key, BtreeValue<V, VBC> value, long sequenceNumber, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            NativeBtree.IFabricBtreeKey nativeKey = null;
            NativeBtree.IFabricBtreeValue nativeValue = null;
            try
            {
                nativeKey = (null != key) ? key.Pin() : null;
                nativeValue = (null != value) ? value.Pin() : null;
                AppTrace.TraceSource.WriteNoise("Btree.NativeBtree.BeginUpsert");
                return this.nativeBtree.BeginUpsert(nativeKey, nativeValue, sequenceNumber, callback);
            }
            catch 
            {
                if (null != key) key.Unpin();
                if (null != value) value.Unpin();
                throw;
            }
        }

        Tuple<IBtreeValue, IRedoUndoInformation> UpsertEndWrapper(BtreeKey<K, KBC> key, BtreeValue<V, VBC> value, NativeCommon.IFabricAsyncOperationContext context)
        {
            NativeRuntime.IFabricOperationData nativeRedo;
            NativeRuntime.IFabricOperationData nativeUndo;
            NativeBtree.IFabricBtreeValue nativeValue;
            try
            {
                AppTrace.TraceSource.WriteNoise("Btree.NativeBtree.EndUpsert");
                nativeValue = this.nativeBtree.EndUpsert(context, out nativeRedo, out nativeUndo);
            }
            catch 
            {
                throw;
            }
            finally
            {
                if (null != key) key.Unpin();
                if (null != value) value.Unpin();
            }
            return new Tuple<IBtreeValue, IRedoUndoInformation>(
                BtreeValue<V, VBC>.CreateFromNative(nativeValue), 
                new RedoUndoInformation(new ReadOnlyOperationData(nativeRedo), new ReadOnlyOperationData(nativeUndo)));
        }

        /// <summary>
        /// Update the value for a given key.
        /// </summary>
        /// <param name="key">Key to update.</param>
        /// <param name="value">Value to update.</param>
        /// <param name="sequenceNumber">Operation sequence number.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        public Task<IRedoUndoInformation> UpdateAsync(IBtreeKey key, IBtreeValue value, long sequenceNumber, CancellationToken cancellationToken)
        {
            BtreeKey<K, KBC> btreeKey = new BtreeKey<K, KBC>(key.Bytes);
            BtreeValue<V, VBC> btreeValue = new BtreeValue<V, VBC>(value.Bytes);
            return Utility.WrapNativeAsyncInvoke<IRedoUndoInformation>(
                (callback) => this.UpdateBeginWrapper(btreeKey, btreeValue, sequenceNumber, callback),
                (context) => this.UpdateEndWrapper(btreeKey, btreeValue, context),
                cancellationToken,
                "Btree.Update");
        }

        NativeCommon.IFabricAsyncOperationContext UpdateBeginWrapper(BtreeKey<K, KBC> key, BtreeValue<V, VBC> value, long sequenceNumber, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            NativeBtree.IFabricBtreeKey nativeKey = null;
            NativeBtree.IFabricBtreeValue nativeValue = null;
            try
            {
                nativeKey = (null != key) ? key.Pin() : null;
                nativeValue = (null != value) ? value.Pin() : null;
                AppTrace.TraceSource.WriteNoise("Btree.NativeBtree.BeginUpdate");
                return this.nativeBtree.BeginUpdate(nativeKey, nativeValue, sequenceNumber, callback);
            }
            catch 
            {
                if (null != key) key.Unpin();
                if (null != value) value.Unpin();
                throw;
            }
        }

        IRedoUndoInformation UpdateEndWrapper(BtreeKey<K, KBC> key, BtreeValue<V, VBC> value, NativeCommon.IFabricAsyncOperationContext context)
        {
            NativeRuntime.IFabricOperationData nativeRedo;
            NativeRuntime.IFabricOperationData nativeUndo;
            try
            {
                AppTrace.TraceSource.WriteNoise("Btree.NativeBtree.EndUpdate");
                this.nativeBtree.EndUpdate(context, out nativeRedo, out nativeUndo);
            }
            catch 
            {
                throw;
            }
            finally
            {
                if (null != key) key.Unpin();
                if (null != value) value.Unpin();
            }
            return new RedoUndoInformation(new ReadOnlyOperationData(nativeRedo), new ReadOnlyOperationData(nativeUndo));
        }

        /// <summary>
        /// Update the value for a given key. Returns the old value.
        /// </summary>
        /// <param name="key">Key to update.</param>
        /// <param name="value">New value to update.</param>
        /// <param name="sequenceNumber">Operation sequence number.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        public Task<Tuple<IBtreeValue, IRedoUndoInformation>> UpdateWithOutputAsync(IBtreeKey key, IBtreeValue value, long sequenceNumber, CancellationToken cancellationToken)
        {
            BtreeKey<K, KBC> btreeKey = new BtreeKey<K, KBC>(key.Bytes);
            BtreeValue<V, VBC> btreeValue = new BtreeValue<V, VBC>(value.Bytes);
            return Utility.WrapNativeAsyncInvoke<Tuple<IBtreeValue, IRedoUndoInformation>>(
                (callback) => this.UpdateWithOutputBeginWrapper(btreeKey, btreeValue, sequenceNumber, callback),
                (context) => this.UpdateWithOutputEndWrapper(btreeKey, btreeValue, context),
                cancellationToken,
                "Btree.UpdateWithOutput");
        }

        NativeCommon.IFabricAsyncOperationContext UpdateWithOutputBeginWrapper(BtreeKey<K, KBC> key, BtreeValue<V, VBC> value, long sequenceNumber, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            NativeBtree.IFabricBtreeKey nativeKey = null;
            NativeBtree.IFabricBtreeValue nativeValue = null;
            try
            {
                nativeKey = (null != key) ? key.Pin() : null;
                nativeValue = (null != value) ? value.Pin() : null;
                AppTrace.TraceSource.WriteNoise("Btree.NativeBtree.BeginUpdateWithOutput");
                return this.nativeBtree.BeginUpdateWithOutput(nativeKey, nativeValue, sequenceNumber, callback);
            }
            catch 
            {
                if (null != key) key.Unpin();
                if (null != value) value.Unpin();
                throw;
            }
        }

        Tuple<IBtreeValue, IRedoUndoInformation> UpdateWithOutputEndWrapper(BtreeKey<K, KBC> key, BtreeValue<V, VBC> value, NativeCommon.IFabricAsyncOperationContext context)
        {
            NativeRuntime.IFabricOperationData nativeRedo = null;
            NativeRuntime.IFabricOperationData nativeUndo = null;
            NativeBtree.IFabricBtreeValue nativeValue = null;
            try
            {
                AppTrace.TraceSource.WriteNoise("Btree.NativeBtree.EndUpdateWithOutput");
                try
                {
                    nativeValue = this.nativeBtree.EndUpdateWithOutput(context, out nativeRedo, out nativeUndo);
                }
                catch (COMException comEx)
                {
                    unchecked
                    {
                        if (comEx.ErrorCode == (int)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_KEY_NOT_FOUND)
                        {
                            throw new KeyNotFoundException();
                        }
                    }
                }
            }
            catch
            {
                throw;
            }
            finally
            {
                if (null != key) key.Unpin();
                if (null != value) value.Unpin();
            }
            return new Tuple<IBtreeValue, IRedoUndoInformation>(
                BtreeValue<V, VBC>.CreateFromNative(nativeValue), 
                new RedoUndoInformation(new ReadOnlyOperationData(nativeRedo), new ReadOnlyOperationData(nativeUndo)));
        }

        /// <summary>
        /// Conditionally updates a given key.
        /// </summary>
        /// <param name="key">Key to update.</param>
        /// <param name="value">value to update.</param>
        /// <param name="conditionalCheckValueOffset">Offset for conditional value check.</param>
        /// <param name="conditionalCheckValue">Value to use for the conditional check.</param>
        /// <param name="sequenceNumber">Operation sequence number.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        public Task<Tuple<bool, IRedoUndoInformation>> ConditionalUpdateAsync(IBtreeKey key, IBtreeValue value, uint conditionalCheckValueOffset, IBtreeValue conditionalCheckValue, long sequenceNumber, CancellationToken cancellationToken)
        {
            BtreeKey<K, KBC> btreeKey = new BtreeKey<K, KBC>(key.Bytes);
            BtreeValue<V, VBC> btreeValue = new BtreeValue<V, VBC>(value.Bytes);
            BtreeValue<V, VBC> btreeConditionalValue = new BtreeValue<V, VBC>(conditionalCheckValue.Bytes);
            return Utility.WrapNativeAsyncInvoke<Tuple<bool, IRedoUndoInformation>>(
                (callback) => this.ConditionalUpdateBeginWrapper(btreeKey, btreeValue, conditionalCheckValueOffset, btreeConditionalValue, sequenceNumber, callback),
                (context) => this.ConditionalUpdateEndWrapper(btreeKey, btreeValue, btreeConditionalValue, context),
                cancellationToken,
                "Btree.ConditionalUpdate");
        }

        NativeCommon.IFabricAsyncOperationContext ConditionalUpdateBeginWrapper(BtreeKey<K, KBC> key, BtreeValue<V, VBC> value, uint conditionalCheckValueOffset, BtreeValue<V, VBC> conditionalCheckValue, long sequenceNumber, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            NativeBtree.IFabricBtreeKey nativeKey = null;
            NativeBtree.IFabricBtreeValue nativeValue = null;
            NativeBtree.IFabricBtreeValue nativeConditionalValue = null;
            try
            {
                nativeKey = (null != key) ? key.Pin() : null;
                nativeValue = (null != value) ? value.Pin() : null;
                nativeConditionalValue = (null != conditionalCheckValue) ? conditionalCheckValue.Pin() : null;
                AppTrace.TraceSource.WriteNoise("Btree.NativeBtree.BeginConditionalUpdate");
                return this.nativeBtree.BeginConditionalUpdate(nativeKey, nativeValue, conditionalCheckValueOffset, nativeConditionalValue, sequenceNumber, callback);
            }
            catch 
            {
                if (null != key) key.Unpin();
                if (null != value) value.Unpin();
                if (null != conditionalCheckValue) conditionalCheckValue.Unpin();
                throw;
            }
        }

        Tuple<bool, IRedoUndoInformation> ConditionalUpdateEndWrapper(BtreeKey<K, KBC> key, BtreeValue<V, VBC> value, BtreeValue<V, VBC> conditionalCheckValue, NativeCommon.IFabricAsyncOperationContext context)
        {
            NativeRuntime.IFabricOperationData nativeRedo;
            NativeRuntime.IFabricOperationData nativeUndo;
            BOOLEAN result;
            try
            {
                AppTrace.TraceSource.WriteNoise("Btree.NativeBtree.EndConditionalUpdate");
                result = this.nativeBtree.EndConditionalUpdate(context, out nativeRedo, out nativeUndo);
            }
            catch 
            {
                throw;
            }
            finally
            {
                if (null != key) key.Unpin();
                if (null != value) value.Unpin();
                if (null != conditionalCheckValue) conditionalCheckValue.Unpin();
            }
            return new Tuple<bool, IRedoUndoInformation>(
                NativeTypes.FromBOOLEAN(result), 
                new RedoUndoInformation(new ReadOnlyOperationData(nativeRedo), new ReadOnlyOperationData(nativeUndo)));
        }

        /// <summary>
        /// Partially update a value for a given key.
        /// </summary>
        /// <param name="key">Key to update.</param>
        /// <param name="partialUpdateOffset">Partial update offset.</param>
        /// <param name="partialValue">Partial value to be used for update.</param>
        /// <param name="conditionalCheckValueOffset">Conditional check offset.</param>
        /// <param name="conditionalCheckValue">Conditional check value used for the update.</param>
        /// <param name="sequenceNumber">Operation sequence number.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        public Task<Tuple<bool, IRedoUndoInformation>> ConditionalPartialUpdateAsync(
            IBtreeKey key,
            uint partialUpdateOffset,
            IBtreeValue partialValue,
            uint conditionalCheckValueOffset,
            IBtreeValue conditionalCheckValue,
            long sequenceNumber,
            CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        /// <summary>
        /// Deletes a key and a value from the btree. Returns true if the delete has been performed.
        /// </summary>
        /// <param name="key">Key to delete.</param>
        /// <param name="sequenceNumber">Operation sequence number.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        public Task<Tuple<bool, IRedoUndoInformation>> DeleteAsync(IBtreeKey key, long sequenceNumber, CancellationToken cancellationToken)
        {
            BtreeKey<K, KBC> btreeKey = new BtreeKey<K, KBC>(key.Bytes);
            return Utility.WrapNativeAsyncInvoke<Tuple<bool, IRedoUndoInformation>>(
                (callback) => this.DeleteBeginWrapper(btreeKey, sequenceNumber, callback),
                (context) => this.DeleteEndWrapper(btreeKey, context),
                cancellationToken,
                "Btree.Delete");
        }

        NativeCommon.IFabricAsyncOperationContext DeleteBeginWrapper(BtreeKey<K, KBC> key, long sequenceNumber, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            NativeBtree.IFabricBtreeKey nativeKey = null;
            try
            {
                nativeKey = (null != key) ? key.Pin() : null;
                AppTrace.TraceSource.WriteNoise("Btree.NativeBtree.BeginDelete");
                return this.nativeBtree.BeginDelete(nativeKey, sequenceNumber, callback);
            }
            catch 
            {
                if (null != key) key.Unpin();
                throw;
            }
        }

        Tuple<bool, IRedoUndoInformation> DeleteEndWrapper(BtreeKey<K, KBC> key, NativeCommon.IFabricAsyncOperationContext context)
        {
            NativeRuntime.IFabricOperationData nativeRedo = null;
            NativeRuntime.IFabricOperationData nativeUndo = null;
            BOOLEAN result = 0;
            try
            {
                AppTrace.TraceSource.WriteNoise("Btree.NativeBtree.EndDelete");
                try
                {
                    result = this.nativeBtree.EndDelete(context, out nativeRedo, out nativeUndo);
                }
                catch (COMException comEx)
                {
                    unchecked
                    {
                        if (comEx.ErrorCode == (int)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_KEY_NOT_FOUND)
                        {
                            throw new KeyNotFoundException();
                        }
                    }
                }
            }
            catch
            {
                throw;
            }
            finally
            {
                if (null != key) key.Unpin();
            }
            return new Tuple<bool, IRedoUndoInformation>(
                NativeTypes.FromBOOLEAN(result),
                new RedoUndoInformation(new ReadOnlyOperationData(nativeRedo), new ReadOnlyOperationData(nativeUndo)));
        }

        /// <summary>
        /// Conditionally deletes a key from the btree. Returns true if the delete has been performed.
        /// </summary>
        /// <param name="key">Key to delete.</param>
        /// <param name="conditionalCheckValueOffset">Conditional check offset.</param>
        /// <param name="conditionalCheckValue">Conditional value to be used for the delete.</param>
        /// <param name="sequenceNumber">Operation sequence number.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        public Task<Tuple<bool, IRedoUndoInformation>> ConditionalDeleteAsync(IBtreeKey key, uint conditionalCheckValueOffset, IBtreeValue conditionalCheckValue, long sequenceNumber, CancellationToken cancellationToken)
        {
            BtreeKey<K, KBC> btreeKey = new BtreeKey<K, KBC>(key.Bytes);
            BtreeValue<V, VBC> btreeConditionalValue = new BtreeValue<V, VBC>(conditionalCheckValue.Bytes);
            return Utility.WrapNativeAsyncInvoke<Tuple<bool, IRedoUndoInformation>>(
                (callback) => this.ConditionalDeleteBeginWrapper(btreeKey, conditionalCheckValueOffset, btreeConditionalValue, sequenceNumber, callback),
                (context) => this.ConditionalDeleteEndWrapper(btreeKey, btreeConditionalValue, context),
                cancellationToken,
                "Btree.ConditionalDelete");
        }

        NativeCommon.IFabricAsyncOperationContext ConditionalDeleteBeginWrapper(BtreeKey<K, KBC> key, uint conditionalCheckValueOffset, BtreeValue<V, VBC> conditionalCheckValue, long sequenceNumber, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            NativeBtree.IFabricBtreeKey nativeKey = null;
            NativeBtree.IFabricBtreeValue nativeConditionalValue = null;
            try
            {
                nativeKey = (null != key) ? key.Pin() : null;
                nativeConditionalValue = (null != conditionalCheckValue) ? conditionalCheckValue.Pin() : null;
                AppTrace.TraceSource.WriteNoise("Btree.NativeBtree.BeginConditionalDelete");
                return this.nativeBtree.BeginConditionalDelete(nativeKey, conditionalCheckValueOffset, nativeConditionalValue, sequenceNumber, callback);
            }
            catch 
            {
                if (null != key) key.Unpin();
                if (null != conditionalCheckValue) conditionalCheckValue.Unpin();
                throw;
            }
        }

        Tuple<bool, IRedoUndoInformation> ConditionalDeleteEndWrapper(BtreeKey<K, KBC> key, BtreeValue<V, VBC> conditionalCheckValue, NativeCommon.IFabricAsyncOperationContext context)
        {
            NativeRuntime.IFabricOperationData nativeRedo = null;
            NativeRuntime.IFabricOperationData nativeUndo = null;
            BOOLEAN result = 0;
            try
            {
                AppTrace.TraceSource.WriteNoise("Btree.NativeBtree.EndConditionalDelete");
                try
                {
                    result = this.nativeBtree.EndConditionalDelete(context, out nativeRedo, out nativeUndo);
                }
                catch (COMException comEx)
                {
                    unchecked
                    {
                        if (comEx.ErrorCode == (int)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_KEY_NOT_FOUND)
                        {
                            throw new KeyNotFoundException();
                        }
                    }
                }
            }
            catch 
            {
                throw;
            }
            finally
            {
                if (null != key) key.Unpin();
                if (null != conditionalCheckValue) conditionalCheckValue.Unpin();
            }
            return new Tuple<bool, IRedoUndoInformation>(
                NativeTypes.FromBOOLEAN(result), 
                new RedoUndoInformation(new ReadOnlyOperationData(nativeRedo), new ReadOnlyOperationData(nativeUndo)));
        }

        /// <summary>
        /// Deletes a given key from the btree. Returns the value of that key.
        /// </summary>
        /// <param name="key">Key to delete.</param>
        /// <param name="sequenceNumber">Operation sequence number.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        public Task<Tuple<IBtreeValue, IRedoUndoInformation>> DeleteWithOutputAsync(IBtreeKey key, long sequenceNumber, CancellationToken cancellationToken)
        {
            BtreeKey<K, KBC> btreeKey = new BtreeKey<K, KBC>(key.Bytes);
            return Utility.WrapNativeAsyncInvoke<Tuple<IBtreeValue, IRedoUndoInformation>>(
                (callback) => this.DeleteWithOutputBeginWrapper(btreeKey, sequenceNumber, callback),
                (context) => this.DeleteWithOutputEndWrapper(btreeKey, context),
                cancellationToken,
                "Btree.DeleteWithOutput");
        }

        NativeCommon.IFabricAsyncOperationContext DeleteWithOutputBeginWrapper(BtreeKey<K, KBC> key, long sequenceNumber, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            NativeBtree.IFabricBtreeKey nativeKey = null;
            try
            {
                nativeKey = (null != key) ? key.Pin() : null;
                AppTrace.TraceSource.WriteNoise("Btree.NativeBtree.BeginDeleteWithOutput");
                return this.nativeBtree.BeginDeleteWithOutput(nativeKey, sequenceNumber, callback);
            }
            catch 
            {
                if (null != key) key.Unpin();
                throw;
            }
        }

        Tuple<IBtreeValue, IRedoUndoInformation> DeleteWithOutputEndWrapper(BtreeKey<K, KBC> key, NativeCommon.IFabricAsyncOperationContext context)
        {
            NativeRuntime.IFabricOperationData nativeRedo;
            NativeRuntime.IFabricOperationData nativeUndo;
            NativeBtree.IFabricBtreeValue nativeValue;
            try
            {
                AppTrace.TraceSource.WriteNoise("Btree.NativeBtree.EndDeleteWithOutput");
                nativeValue = this.nativeBtree.EndDeleteWithOutput(context, out nativeRedo, out nativeUndo);
            }
            catch 
            {
                throw;
            }
            finally
            {
                if (null != key) key.Unpin();
            }
            return new Tuple<IBtreeValue, IRedoUndoInformation>(
                BtreeValue<V, VBC>.CreateFromNative(nativeValue), 
                new RedoUndoInformation(new ReadOnlyOperationData(nativeRedo), new ReadOnlyOperationData(nativeUndo)));
        }

        /// <summary>
        /// Seek/read the value for a given key.
        /// </summary>
        /// <param name="key">Key to read value for.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        public Task<IBtreeValue> SeekAsync(IBtreeKey key, CancellationToken cancellationToken)
        {
            BtreeKey<K, KBC> btreeKey = new BtreeKey<K, KBC>(key.Bytes);
            return Utility.WrapNativeAsyncInvoke<IBtreeValue>(
                (callback) => this.SeekBeginWrapper(btreeKey, callback),
                (context) => this.SeekEndWrapper(btreeKey, context),
                cancellationToken,
                "Btree.Seek");
        }

        NativeCommon.IFabricAsyncOperationContext SeekBeginWrapper(BtreeKey<K, KBC> key, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            NativeBtree.IFabricBtreeKey nativeKey = null;
            try
            {
                nativeKey = (null != key) ? key.Pin() : null;
                AppTrace.TraceSource.WriteNoise("Btree.NativeBtree.BeginSeek");
                return this.nativeBtree.BeginSeek(nativeKey, callback);
            }
            catch 
            {
                if (null != key) key.Unpin();
                throw;
            }
        }

        IBtreeValue SeekEndWrapper(BtreeKey<K, KBC> key, NativeCommon.IFabricAsyncOperationContext context)
        {
            NativeBtree.IFabricBtreeValue nativeValue = null;
            try
            {
                AppTrace.TraceSource.WriteNoise("Btree.NativeBtree.EndSeek");
                try
                {
                    nativeValue = this.nativeBtree.EndSeek(context);
                }
                catch (COMException comEx)
                {
                    unchecked
                    {
                        if (comEx.ErrorCode == (int)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_KEY_NOT_FOUND)
                        {
                            throw new KeyNotFoundException();
                        }
                    }
                }
            }
            catch 
            {
                throw;
            }
            finally
            {
                if (null != key) key.Unpin();
            }
            return BtreeValue<V, VBC>.CreateFromNative(nativeValue);
        }

        /// <summary>
        /// Seek/read the a partial value for a given key. 
        /// </summary>
        /// <param name="key">Key to read.</param>
        /// <param name="partialSeekValueOffset">Value offset to read at.</param>
        /// <param name="partialSeekValueSize">Value size to read.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        public Task<IBtreeValue> PartialSeekAsync(IBtreeKey key, uint partialSeekValueOffset, uint partialSeekValueSize, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        /// <summary>
        /// Atomically drop all data from the btree. Returns true if erase was performed.
        /// </summary>
        /// <param name="sequenceNumber">Operation sequence number.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        public Task<Tuple<bool, IRedoUndoInformation>> EraseAsync(long sequenceNumber, CancellationToken cancellationToken)
        {
            return Utility.WrapNativeAsyncInvoke<Tuple<bool, IRedoUndoInformation>>(
                (callback) => this.EraseBeginWrapper(sequenceNumber, callback),
                (context) => this.EraseEndWrapper(context),
                cancellationToken,
                "Btree.Erase");
        }

        NativeCommon.IFabricAsyncOperationContext EraseBeginWrapper(long sequenceNumber, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            AppTrace.TraceSource.WriteNoise("Btree.NativeBtree.BeginErase");
            return this.nativeBtree.BeginErase(sequenceNumber, callback);
        }

        Tuple<bool, IRedoUndoInformation> EraseEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            NativeRuntime.IFabricOperationData nativeRedo;
            AppTrace.TraceSource.WriteNoise("Btree.NativeBtree.EndErase");
            BOOLEAN result = this.nativeBtree.EndErase(context, out nativeRedo);
            return new Tuple<bool, IRedoUndoInformation>(
                NativeTypes.FromBOOLEAN(result), 
                new RedoUndoInformation(new ReadOnlyOperationData(nativeRedo), null));
        }

        /// <summary>
        /// Checkpoints the btree up to the given sequence number. This operation is mandatory.
        /// </summary>
        /// <param name="sequenceNumber">Operation sequence number.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        public Task CheckpointAsync(long sequenceNumber, CancellationToken cancellationToken)
        {
            return Utility.WrapNativeAsyncInvoke(
                (callback) => this.CheckpointBeginWrapper(sequenceNumber, callback),
                this.CheckpointEndWrapper,
                cancellationToken,
                "Btree.Checkpoint");
        }

        NativeCommon.IFabricAsyncOperationContext CheckpointBeginWrapper(long sequenceNumber, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            AppTrace.TraceSource.WriteNoise("Btree.NativeBtree.BeginCheckpoint");
            return this.nativeBtree.BeginCheckpoint(sequenceNumber, callback);
        }

        void CheckpointEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            AppTrace.TraceSource.WriteNoise("Btree.NativeBtree.EndCheckpoint");
            this.nativeBtree.EndCheckpoint(context);
        }

        /// <summary>
        /// Informs the btree that operations up to the given sequence number are not stable.
        /// </summary>
        /// <param name="sequenceNumber">Operation sequence number.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        public Task<long> OnOperationStableAsync(long sequenceNumber, CancellationToken cancellationToken)
        {
            return Utility.WrapNativeAsyncInvoke<long>(
                (callback) => this.OnOperationStableBeginWrapper(sequenceNumber, callback),
                this.OnOperationStableEndWrapper,
                cancellationToken,
                "Btree.OperationStable");
        }

        NativeCommon.IFabricAsyncOperationContext OnOperationStableBeginWrapper(long sequenceNumber, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            AppTrace.TraceSource.WriteNoise("Btree.NativeBtree.BeginOperationStable");
            return this.nativeBtree.BeginOperationStable(sequenceNumber, callback);
        }

        long OnOperationStableEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            AppTrace.TraceSource.WriteNoise("Btree.NativeBtree.EndOperationStable");
            return this.nativeBtree.EndOperationStable(context);
        }

        /// <summary>
        /// Applies copy bytes to a newly constructed btree.
        /// </summary>
        /// <param name="copyStreamData">Bytes to apply.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        public Task ApplyCopyDataAsync(IOperationData copyStreamData, CancellationToken cancellationToken)
        {
            ReadOnlyOperationData operationData = copyStreamData as ReadOnlyOperationData;
            Debug.Assert(null != operationData);
            return Utility.WrapNativeAsyncInvoke(
                (callback) => this.ApplyCopyDataBeginWrapper(operationData, callback),
                this.ApplyCopyDataEndWrapper,
                cancellationToken,
                "Btree.ApplyCopyData");
        }

        NativeCommon.IFabricAsyncOperationContext ApplyCopyDataBeginWrapper(ReadOnlyOperationData operationData, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            NativeRuntime.IFabricOperationData nativeOperationData = operationData.NativeOperationData;
            AppTrace.TraceSource.WriteNoise("Btree.NativeBtree.BeginApplyCopyData");
            return this.nativeBtree.BeginApplyCopyData(nativeOperationData, callback);
        }

        void ApplyCopyDataEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            AppTrace.TraceSource.WriteNoise("Btree.NativeBtree.EndApplyWithOutput");
            this.nativeBtree.EndApplyWithOutput(context);
        }

        /// <summary>
        /// Returns a stream that represents the state to be copied out of this btree into another btree.
        /// </summary>
        /// <param name="upToSequenceNumber">Sequence number used to copy btree data.</param>
        /// <returns></returns>
        public IOperationDataStreamEx GetCopyState(long upToSequenceNumber)
        {
            NativeRuntime.IFabricOperationDataStream nativeOperationStream =
                Utility.WrapNativeSyncInvoke<NativeRuntime.IFabricOperationDataStream>(() => this.nativeBtree.GetCopyState(upToSequenceNumber), "Btree.GetCopyState");
            
            return (null == nativeOperationStream) ? null : new BtreeOperationDataStream(nativeOperationStream);
        }

        /// <summary>
        /// Decodes and optionally reconstructs the btree operation encapsulated in the given operation data.
        /// </summary>
        /// <param name="sequenceNumber">Operation sequence number.</param>
        /// <param name="operation">Buffers encoding the btree operation.</param>
        /// <param name="decodeOnly">True if only decode is needed and operation is not actually performed.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        public Task<IBtreeOperation> ApplyWithOutputAsync(long sequenceNumber, IOperationData operation, bool decodeOnly, CancellationToken cancellationToken)
        {
            ReadOnlyOperationData operationData = operation as ReadOnlyOperationData;
            Debug.Assert(null != operationData);
            return Utility.WrapNativeAsyncInvoke<IBtreeOperation>(
                (callback) => this.ApplyWithOutputBeginWrapper(sequenceNumber, operationData, decodeOnly, callback),
                this.ApplyWithOutputEndWrapper,
                cancellationToken,
                "Btree.ApplyWithOutput");
        }

        NativeCommon.IFabricAsyncOperationContext ApplyWithOutputBeginWrapper(long sequenceNumber, ReadOnlyOperationData operationData, bool decodeOnly, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            NativeRuntime.IFabricOperationData nativeOperationData = operationData.NativeOperationData;
            AppTrace.TraceSource.WriteNoise("Btree.NativeBtree.BeginApplyWithOutput");
            return this.nativeBtree.BeginApplyWithOutput(sequenceNumber, nativeOperationData, NativeTypes.ToBOOLEAN(decodeOnly), callback);
        }

        IBtreeOperation ApplyWithOutputEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            NativeBtree.IFabricBtreeOperation nativeBtreeOperation;
            AppTrace.TraceSource.WriteNoise("Btree.NativeBtree.EndApplyWithOutput");
            nativeBtreeOperation = this.nativeBtree.EndApplyWithOutput(context);

            AppTrace.TraceSource.WriteNoise("NativeBtreeOperation.GetKey");
            NativeBtree.IFabricBtreeKey nativeKey = nativeBtreeOperation.GetKey();

            AppTrace.TraceSource.WriteNoise("NativeBtreeOperation.GetValue");
            NativeBtree.IFabricBtreeValue nativeValue = nativeBtreeOperation.GetValue();

            AppTrace.TraceSource.WriteNoise("NativeBtreeOperation.GetConditionalValue");
            NativeBtree.IFabricBtreeValue nativeConditionalValue = nativeBtreeOperation.GetConditionalValue();

            AppTrace.TraceSource.WriteNoise("NativeBtreeOperation.GetConditionalOffset");
            uint conditionalOffset = nativeBtreeOperation.GetConditionalOffset();

            AppTrace.TraceSource.WriteNoise("NativeBtreeOperation.GetPartialUpdateOffset");
            uint partialUpdateOffset = nativeBtreeOperation.GetPartialUpdateOffset();
       
            return new BtreeOperation(
                (BtreeOperationType)nativeBtreeOperation.GetOperationType(), 
                BtreeKey<K, KBC>.CreateFromNative(nativeKey), 
                BtreeValue<V, VBC>.CreateFromNative(nativeValue), 
                BtreeValue<V, VBC>.CreateFromNative(nativeConditionalValue),
                conditionalOffset,
                partialUpdateOffset);
        }

        /// <summary>
        /// Creates a btree scan object.
        /// </summary>
        /// <returns></returns>
        public IBtreeScan CreateScan()
        {
            AppTrace.TraceSource.WriteNoise("Btree.NativeBtree.CreateScan");
            return new BtreeScan<K, V, KBC, VBC>(Utility.WrapNativeSyncInvoke<NativeBtree.IFabricBtreeScan>(this.nativeBtree.CreateScan, "Btree.CreateScan"));
        }

        /// <summary>
        /// Closes the btree.
        /// </summary>
        /// <param name="eraseStorage">True, if the storage associated with the btree is to be deallocated.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        public Task CloseAsync(bool eraseStorage, CancellationToken cancellationToken)
        {
            return Utility.WrapNativeAsyncInvoke(
                (callback) => this.CloseBeginWrapper(eraseStorage, callback),
                this.CloseEndWrapper,
                cancellationToken,
                "Btree.Close");
        }

        NativeCommon.IFabricAsyncOperationContext CloseBeginWrapper(bool eraseStorage, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            AppTrace.TraceSource.WriteNoise("Btree.NativeBtree.BeginClose");
            return this.nativeBtree.BeginClose(NativeTypes.ToBOOLEAN(eraseStorage), callback);
        }

        void CloseEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            AppTrace.TraceSource.WriteNoise("Btree.NativeBtree.EndClose");
            this.nativeBtree.EndClose(context);
        }

        /// <summary>
        /// Aborts new operations on the btree.
        /// </summary>
        public void Abort()
        {
            AppTrace.TraceSource.WriteNoise("Btree.NativeBtree.Abort");
            Utility.WrapNativeSyncInvoke(this.nativeBtree.Abort, "Btree.Abort");
        }

        /// <summary>
        /// Returns the last committed sequence number in the btree.
        /// </summary>
        /// <returns></returns>
        public long GetLastCommittedSequenceNumber()
        {
            AppTrace.TraceSource.WriteNoise("Btree.NativeBtree.GetLastCommittedSequenceNumber");
            return Utility.WrapNativeSyncInvoke<long>(this.nativeBtree.GetLastCommittedSequenceNumber, "Btree.GetLastCommittedSequenceNumber");
        }
    }
}