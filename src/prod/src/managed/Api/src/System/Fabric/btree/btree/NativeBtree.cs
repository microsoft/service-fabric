// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.Btree.Interop
{
    using System;
    using System.Diagnostics.CodeAnalysis;
    using System.Runtime.CompilerServices;
    using System.Runtime.InteropServices;
    using System.Fabric.Interop;
    using BOOLEAN = System.SByte;
    using FABRIC_ATOMIC_GROUP_ID = System.Int64;
    using FABRIC_INSTANCE_ID = System.Int64;
    using FABRIC_PARTITION_ID = System.Guid;
    using FABRIC_REPLICA_ID = System.Int64;
    using FABRIC_SEQUENCE_NUMBER = System.Int64;
    using FABRIC_TRANSACTION_ID = System.Guid;

    internal static class NativeBtree
    {
        //----------------------------------------------------------------------
        // Constants.
        //----------------------------------------------------------------------
        //
        public const uint BtreeMaxKeySize = 4096; // bytes
        public const uint BtreeMaxPageSize = 128; // kilobytes

        //----------------------------------------------------------------------
        // Enumerations
        //----------------------------------------------------------------------
        //

        [Guid("e2d0ab74-f307-473d-8f99-20b387a240b8")]
        internal enum FABRIC_BTREE_DATA_TYPE : int
        {
            FABRIC_BTREE_DATA_TYPE_INVALID,
            FABRIC_BTREE_DATA_TYPE_BINARY,
            FABRIC_BTREE_DATA_TYPE_BYTE,
            FABRIC_BTREE_DATA_TYPE_WCHAR,
            FABRIC_BTREE_DATA_TYPE_DATETIME,
            FABRIC_BTREE_DATA_TYPE_TIMESPAN,
            FABRIC_BTREE_DATA_TYPE_SHORT,
            FABRIC_BTREE_DATA_TYPE_USHORT,
            FABRIC_BTREE_DATA_TYPE_INT,
            FABRIC_BTREE_DATA_TYPE_UINT,
            FABRIC_BTREE_DATA_TYPE_LONGLONG,
            FABRIC_BTREE_DATA_TYPE_ULONGLONG,
            FABRIC_BTREE_DATA_TYPE_SINGLE,
            FABRIC_BTREE_DATA_TYPE_DOUBLE,
            FABRIC_BTREE_DATA_TYPE_GUID,
            FABRIC_BTREE_DATA_TYPE_WSTRING
        }

        [Guid("5284f24f-f62f-4c33-894a-421d5fa35860")]
        internal enum FABRIC_BTREE_OPERATION_TYPE : int
        {
            FABRIC_BTREE_OPERATION_INVALID,
            FABRIC_BTREE_OPERATION_INSERT,
            FABRIC_BTREE_OPERATION_DELETE,
            FABRIC_BTREE_OPERATION_UPDATE,
            FABRIC_BTREE_OPERATION_PARTIAL_UPDATE,
            FABRIC_BTREE_OPERATION_ERASE      
        }

        //----------------------------------------------------------------------
        // Structures
        //----------------------------------------------------------------------
        //

        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        internal struct BTREE_KEY_COMPARISON_DESCRIPTION
        {
            public FABRIC_BTREE_DATA_TYPE KeyDataType;
            public int LCID;
            public uint MaximumKeySizeInBytes;
            public BOOLEAN IsFixedLengthKey;
            public IntPtr Reserved;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        internal struct BTREE_STORAGE_CONFIGURATION_DESCRIPTION
        {
            public IntPtr Path;
            public uint MaximumStorageInMB;
            public BOOLEAN IsVolatile;
            public uint MaximumMemoryInMB;
            public uint MaximumPageSizeInKB;
            public BOOLEAN StoreDataInline;
            public uint RetriesBeforeTimeout;    
            public IntPtr Reserved;
        }
        
        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        internal struct BTREE_CONFIGURATION_DESCRIPTION
        {
            public FABRIC_PARTITION_ID PartitionId;
            public FABRIC_REPLICA_ID ReplicaId;
            public IntPtr StorageConfiguration;
            public IntPtr KeyComparison;              
            public IntPtr Reserved;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        internal struct BTREE_STATISTICS
        {
            public int MemoryUsageInBytes;
            public int StorageUsageInBytes;
            public int PageCount;
            public int RecordCount;       
            public IntPtr Reserved;
        }

        //----------------------------------------------------------------------
        // Interfaces
        //----------------------------------------------------------------------
        //

        [ComImport]
        [Guid("7cb23eac-731c-4b91-a95c-d84050c1ecc5")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricBtreeKey
        {
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            IntPtr GetBytes(
                [Out] out UInt32 count);
        }

        [ComImport]
        [Guid("9f5b60af-3d9d-4d45-9094-b0b3c8188dc5")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricBtreeValue
        {
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            IntPtr GetBytes(
                [Out] out UInt32 count);
        }
        
        [ComImport]
        [Guid("1a3cc156-c5bd-41f8-b9bf-270d63559318")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricBtreeOperation
        {
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            FABRIC_BTREE_OPERATION_TYPE GetOperationType();

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeBtree.IFabricBtreeKey GetKey();

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeBtree.IFabricBtreeValue GetValue();

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeBtree.IFabricBtreeValue GetConditionalValue();

            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            uint GetConditionalOffset();

            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            uint GetPartialUpdateOffset();
        }

        [ComImport]
        [Guid("ab74fcdb-f2e1-45a8-870a-31b3f8e50f96")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricBtreeScanPosition
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeBtree.IFabricBtreeKey GetKey();

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeBtree.IFabricBtreeValue GetValue();
        }
        
        [ComImport]
        [Guid("404c7900-99ed-49ee-9388-5063c185b741")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricBtreeScan
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]        
            NativeCommon.IFabricAsyncOperationContext BeginOpen(
                [In, MarshalAs(UnmanagedType.Interface)] NativeBtree.IFabricBtreeKey beginKey,
                [In, MarshalAs(UnmanagedType.Interface)] NativeBtree.IFabricBtreeKey endKey,
                [In, MarshalAs(UnmanagedType.Interface)] NativeBtree.IFabricBtreeKey keyPrefixMatch,
                [In] BOOLEAN isValueNeeded,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeBtree.IFabricBtreeKey EndOpen(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]        
            NativeCommon.IFabricAsyncOperationContext BeginClose(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            void EndClose(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]        
            NativeCommon.IFabricAsyncOperationContext BeginReset(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback); 

            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)] 
            void EndReset(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]        
            NativeCommon.IFabricAsyncOperationContext BeginMoveNext(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeBtree.IFabricBtreeScanPosition EndMoveNext(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]        
            NativeCommon.IFabricAsyncOperationContext BeginPeekNextKey(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeBtree.IFabricBtreeKey EndPeekNextKey(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }
        
        [ComImport]
        [Guid("16dbb404-3127-4c8e-995b-3b3d9a8ea099")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricBtree
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeCommon.IFabricAsyncOperationContext BeginOpen(
                [In] IntPtr configuration,
                [In] BOOLEAN storageExists,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            void EndOpen(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeCommon.IFabricAsyncOperationContext BeginClose(
                [In] BOOLEAN eraseStorage,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            void EndClose(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            void Abort();

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeCommon.IFabricAsyncOperationContext BeginInsert(
                [In, MarshalAs(UnmanagedType.Interface)] NativeBtree.IFabricBtreeKey key,
                [In, MarshalAs(UnmanagedType.Interface)] NativeBtree.IFabricBtreeValue value,
                [In] FABRIC_SEQUENCE_NUMBER osn,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            void EndInsert(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeRuntime.IFabricOperationData redoBytes,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeRuntime.IFabricOperationData undoBytes);

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeCommon.IFabricAsyncOperationContext BeginInsertWithOutput(
                [In, MarshalAs(UnmanagedType.Interface)] NativeBtree.IFabricBtreeKey key,
                [In, MarshalAs(UnmanagedType.Interface)] NativeBtree.IFabricBtreeValue value,
                [In] FABRIC_SEQUENCE_NUMBER osn,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeBtree.IFabricBtreeValue EndInsertWithOutput(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeRuntime.IFabricOperationData redoBytes,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeRuntime.IFabricOperationData undoBytes);

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeCommon.IFabricAsyncOperationContext BeginUpsert(
                [In, MarshalAs(UnmanagedType.Interface)] NativeBtree.IFabricBtreeKey key,
                [In, MarshalAs(UnmanagedType.Interface)] NativeBtree.IFabricBtreeValue value,
                [In] FABRIC_SEQUENCE_NUMBER osn,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeBtree.IFabricBtreeValue EndUpsert(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeRuntime.IFabricOperationData redoBytes,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeRuntime.IFabricOperationData undoBytes);

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeCommon.IFabricAsyncOperationContext BeginUpdate(
                [In, MarshalAs(UnmanagedType.Interface)] NativeBtree.IFabricBtreeKey key,
                [In, MarshalAs(UnmanagedType.Interface)] NativeBtree.IFabricBtreeValue value,
                [In] FABRIC_SEQUENCE_NUMBER osn,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            void EndUpdate(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeRuntime.IFabricOperationData redoBytes,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeRuntime.IFabricOperationData undoBytes);

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeCommon.IFabricAsyncOperationContext BeginConditionalUpdate(
                [In, MarshalAs(UnmanagedType.Interface)] NativeBtree.IFabricBtreeKey key,
                [In, MarshalAs(UnmanagedType.Interface)] NativeBtree.IFabricBtreeValue value,
                [In] uint conditionalCheckOffset,
                [In, MarshalAs(UnmanagedType.Interface)] NativeBtree.IFabricBtreeValue conditionalCheckValue,
                [In] FABRIC_SEQUENCE_NUMBER osn,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            BOOLEAN EndConditionalUpdate(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeRuntime.IFabricOperationData redoBytes,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeRuntime.IFabricOperationData undoBytes);

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeCommon.IFabricAsyncOperationContext BeginUpdateWithOutput(
                [In, MarshalAs(UnmanagedType.Interface)] NativeBtree.IFabricBtreeKey key,
                [In, MarshalAs(UnmanagedType.Interface)] NativeBtree.IFabricBtreeValue value,
                [In] FABRIC_SEQUENCE_NUMBER osn,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeBtree.IFabricBtreeValue EndUpdateWithOutput(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeRuntime.IFabricOperationData redoBytes,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeRuntime.IFabricOperationData undoBytes);

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeCommon.IFabricAsyncOperationContext BeginConditionalPartialUpdate(
                [In, MarshalAs(UnmanagedType.Interface)] NativeBtree.IFabricBtreeKey key,
                [In] uint partialUpdateOffset,
                [In, MarshalAs(UnmanagedType.Interface)] NativeBtree.IFabricBtreeValue partialValue,        
                [In] uint conditionalCheckOffset,
                [In, MarshalAs(UnmanagedType.Interface)] NativeBtree.IFabricBtreeValue conditionalCheckValue,
                [In] FABRIC_SEQUENCE_NUMBER osn,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            BOOLEAN EndConditionalPartialUpdate(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeRuntime.IFabricOperationData redoBytes,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeRuntime.IFabricOperationData undoBytes);

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeCommon.IFabricAsyncOperationContext BeginDelete(
                [In, MarshalAs(UnmanagedType.Interface)] NativeBtree.IFabricBtreeKey key,
                [In] FABRIC_SEQUENCE_NUMBER osn,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            BOOLEAN EndDelete(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeRuntime.IFabricOperationData redoBytes,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeRuntime.IFabricOperationData undoBytes);

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeCommon.IFabricAsyncOperationContext BeginConditionalDelete(
                [In, MarshalAs(UnmanagedType.Interface)] NativeBtree.IFabricBtreeKey key, 
                [In] uint conditionalCheckOffset,
                [In, MarshalAs(UnmanagedType.Interface)] NativeBtree.IFabricBtreeValue conditionalCheckValue,
                [In] FABRIC_SEQUENCE_NUMBER osn,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            BOOLEAN EndConditionalDelete(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeRuntime.IFabricOperationData redoBytes,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeRuntime.IFabricOperationData undoBytes);

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeCommon.IFabricAsyncOperationContext BeginDeleteWithOutput(
                [In, MarshalAs(UnmanagedType.Interface)] NativeBtree.IFabricBtreeKey key,
                [In] FABRIC_SEQUENCE_NUMBER osn,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeBtree.IFabricBtreeValue EndDeleteWithOutput(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeRuntime.IFabricOperationData redoBytes,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeRuntime.IFabricOperationData undoBytes);

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeCommon.IFabricAsyncOperationContext BeginSeek(
                [In, MarshalAs(UnmanagedType.Interface)] NativeBtree.IFabricBtreeKey key,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeBtree.IFabricBtreeValue EndSeek(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeCommon.IFabricAsyncOperationContext BeginPartialSeek(
                [In, MarshalAs(UnmanagedType.Interface)] NativeBtree.IFabricBtreeKey key,
                [In] uint partialSeekOffset,
                [In] uint partialSeekLength,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeBtree.IFabricBtreeValue EndPartialSeek(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeCommon.IFabricAsyncOperationContext BeginErase(
                [In] FABRIC_SEQUENCE_NUMBER osn,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            BOOLEAN EndErase(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeRuntime.IFabricOperationData redoBytes);

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeBtree.IFabricBtreeScan CreateScan();

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeCommon.IFabricAsyncOperationContext BeginOperationStable(
                [In] FABRIC_SEQUENCE_NUMBER stableOsn,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            FABRIC_SEQUENCE_NUMBER EndOperationStable(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeCommon.IFabricAsyncOperationContext BeginCheckpoint(
                [In] FABRIC_SEQUENCE_NUMBER checkpointOsn,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            void EndCheckpoint(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            IntPtr GetStatistics();

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeCommon.IFabricAsyncOperationContext BeginApplyWithOutput(
                [In] FABRIC_SEQUENCE_NUMBER osn,
                [In, MarshalAs(UnmanagedType.Interface)] NativeRuntime.IFabricOperationData operation,
                [In] BOOLEAN decodeOnly,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeBtree.IFabricBtreeOperation EndApplyWithOutput(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeRuntime.IFabricOperationDataStream GetCopyState(
                [In] FABRIC_SEQUENCE_NUMBER upToOsn);

            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            FABRIC_SEQUENCE_NUMBER GetLastCommittedSequenceNumber();

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeCommon.IFabricAsyncOperationContext BeginApplyCopyData(
                [In, MarshalAs(UnmanagedType.Interface)] NativeRuntime.IFabricOperationData copyStreamData,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            void EndApplyCopyData(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        //----------------------------------------------------------------------
        // Entry Point
        //----------------------------------------------------------------------
        //

        [DllImport("FabricData.dll", EntryPoint = "FabricCreateBtree", PreserveSig = false)]
        [return: MarshalAs(UnmanagedType.Interface)]
        public static extern NativeBtree.IFabricBtree CreateBtree();
    }
}