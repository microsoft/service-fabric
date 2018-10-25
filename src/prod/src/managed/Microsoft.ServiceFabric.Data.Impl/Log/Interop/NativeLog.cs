// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.Log.Interop
{
    using System.Fabric.Interop;
    using System.Runtime.CompilerServices;
    using System.Runtime.InteropServices;
    //* Type mappings
    using KTL_LOG_ASN = System.UInt64;
    using BOOLEAN = System.SByte;
    using HRESULT = System.UInt32;
    using KTL_LOG_STREAM_ID = System.Guid;
    using KTL_LOG_STREAM_TYPE_ID = System.Guid;
    using KTL_LOG_ID = System.Guid;
    using KTL_DISK_ID = System.Guid;
    using EnumToken = System.UIntPtr;


    //** Interop and helper definitions for KPhysicalLogDll.dll/ libKPhysicalLog.so (see KPhysicalLog.h) APIs

    // NOTE: !!!!! DO NOT MAKE ANY CHANGE TO THESE TYPES, ORDERING, ... WITHOUT CORRESPONDING CHANGES TO KPhysicalLog_.IDL AND KPhysicalLog.h !!!!!
    internal static class NativeLog
    {
        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        public unsafe struct GuidStruct
        {
            public uint a; // +0
            public ushort b; // +4
            public ushort c; // +6
            public byte d; // +7
            public byte e; // +8
            public byte f; // +9
            public byte g; // +10
            public byte h; // +11
            public byte i; // +12
            public byte j; // +13
            public byte k; // +14

            public Guid FromBytes()
            {
                return new Guid((int)this.a, (short)this.b, (short)this.c, this.d, this.e, this.f, this.g, this.h, this.i, this.j, this.k);
            }
        }

        /// <summary>
        /// Interop enum eqv to KtlLogStream::RecordDisposition
        /// </summary>
        [Guid("8aafa011-ad17-413b-9769-cb3d121bf524")]
        internal enum KPHYSICAL_LOG_STREAM_RECORD_DISPOSITION : int
        {
            DispositionPending,
            DispositionPersisted,
            DispositionNone
        }

        /// <summary>
        /// Interop enum eqv to KtlLogStream::AsyncStreamNotificationContext::ThresholdTypeEnum
        /// </summary>
        [Guid("10c7e359-bd5b-4e9e-bab5-0442a7297af1")]
        internal enum KPHYSICAL_LOG_STREAM_NOTIFICATION_TYPE : int
        {
            LogSpaceUtilization
        }

        /// <summary>
        /// IKArray element type returned from IKIoBuffer.GetElements()
        /// </summary>
        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        internal unsafe struct KIOBUFFER_ELEMENT_DESCRIPTOR
        {
            public UInt32 Size;
            public byte* ElementBaseAddress;
        }

        /// <summary>
        /// IKArray element type returned from IKPhysicalLogStream.QueryRecords()
        /// </summary>
        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        internal struct KPHYSICAL_LOG_STREAM_RecordMetadata
        {
            public KTL_LOG_ASN Asn;
            public UInt64 Version;
            public KPHYSICAL_LOG_STREAM_RECORD_DISPOSITION Disposition;
            public UInt32 Size;
            public Int64 Debug_LsnValue;

            public KPHYSICAL_LOG_STREAM_RecordMetadata(
                KTL_LOG_ASN Asn,
                UInt64 Version,
                KPHYSICAL_LOG_STREAM_RECORD_DISPOSITION Disposition,
                UInt32 Size,
                Int64 Debug_LsnValue)
            {
                this.Asn = Asn;
                this.Version = Version;
                this.Disposition = Disposition;
                this.Size = Size;
                this.Debug_LsnValue = Debug_LsnValue;
            }

            public static bool operator ==(KPHYSICAL_LOG_STREAM_RecordMetadata Left, KPHYSICAL_LOG_STREAM_RecordMetadata Right)
            {
                return ((Left.Asn == Right.Asn) &&
                        (Left.Version == Right.Version) &&
                        (Left.Disposition == Right.Disposition) &&
                        (Left.Size == Right.Size) &&
                        (Left.Debug_LsnValue == Right.Debug_LsnValue));
            }

            public static bool operator !=(KPHYSICAL_LOG_STREAM_RecordMetadata Left, KPHYSICAL_LOG_STREAM_RecordMetadata Right)
            {
                return !(Left == Right);
            }

            public override bool Equals(object Other)
            {
                if (Other is KPHYSICAL_LOG_STREAM_RecordMetadata)
                    return (this == (KPHYSICAL_LOG_STREAM_RecordMetadata)Other);

                return false;
            }

            public override int GetHashCode()
            {
                return base.GetHashCode();
            }
        }


        //** Physical Log COM Interfaces (see KPhysicalLog.h) --- NOTE: Order of method defs must match 
        //                                                              .idl defs exactly

        /// <summary>
        /// COM wrapper type for KTL KBuffer instances
        /// </summary>
        [ComImport]
        [Guid("89ab6225-2cb4-48b4-be5c-04bd441c774f")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal unsafe interface IKBuffer
        {
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT GetBuffer([Out] out void* Result);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT QuerySize([Out] out UInt32 Result);
        }

        /// <summary>
        /// COM wrapper type for KTL KIoBufferElement instances
        /// </summary>
        [ComImport]
        [Guid("61f6b9bf-1d08-4d04-b97f-e645782e7981")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal unsafe interface IKIoBufferElement
        {
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT GetBuffer([Out] out void* Result);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT QuerySize([Out] out UInt32 Result);
        }

        /// <summary>
        /// COM wrapper type for KTL KIoBuffer instances
        /// </summary>
        [ComImport]
        [Guid("29a7c7a2-a96e-419b-b171-6263625b6219")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal unsafe interface IKIoBuffer
        {
            /// <summary>
            /// Eqv to KTL KIoBuffer::AddIoBufferElement()
            /// </summary>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT AddIoBufferElement([In, MarshalAs(UnmanagedType.Interface)] IKIoBufferElement Element);

            /// <summary>
            /// Eqv to KTL KIoBuffer::QueryNumberOfIoBufferElements()
            /// </summary>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT QueryNumberOfIoBufferElements([Out] out UInt32 Result);

            /// <summary>
            /// Eqv to KTL KIoBuffer::QuerySize()
            /// </summary>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT QuerySize([Out] out UInt32 Result);

            /// <summary>
            /// Eqv to KTL KIoBuffer::First()
            /// </summary>
            /// <param name="Result">Resulting token value representing the first underlying KTL KIoBufferElement</param>
            /// <returns>S_OK if there is a first element; S_FALSE if not</returns>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            [PreserveSig]
            HRESULT First([Out] out EnumToken Result);

            /// <summary>
            /// Eqv to KTL KIoBuffer::Next()
            /// </summary>
            /// <param name="Current"></param>
            /// <param name="Result">Resulting token value representing the next underlying KTL KIoBufferElement after Current</param>
            /// <returns>S_OK if there is a first element; S_FALSE if not</returns>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            [PreserveSig]
            HRESULT Next([In] EnumToken Current, [Out] out EnumToken Result);

            /// <summary>
            /// Eqv to KTL KIoBuffer::Clear()
            /// </summary>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT Clear();

            /// <summary>
            /// Eqv to KTL KIoBuffer::AddIoBuffer()
            /// </summary>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT AddIoBuffer([In, MarshalAs(UnmanagedType.Interface)] IKIoBuffer ToAdd);

            /// <summary>
            /// Eqv to KTL KIoBuffer::AddIoBufferReference()
            /// </summary>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT AddIoBufferReference(
                [In, MarshalAs(UnmanagedType.Interface)] IKIoBuffer SourceIoBuffer,
                [In] UInt32 SourceStartingOffset,
                [In] UInt32 Size);

            /// <summary>
            /// Retrieve the address of the native buffer base and its length for the KIoBufferElement represented 
            /// by a token returned from either First() or Next()
            /// </summary>
            /// <param name="ForToken">Token</param>
            /// <param name="Result">underlying buffer base address</param>
            /// <param name="ResultSize">Buffer size</param>
            /// <returns></returns>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT QueryElementInfo([In] EnumToken ForToken, [Out] out void* Result, [Out] out UInt32 ResultSize);

            /// <summary>
            /// Translate a token returned from First() or Next() into the corresponding IKIoBufferElement reference
            /// </summary>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT GetElement([In] EnumToken ForToken, [Out, MarshalAs(UnmanagedType.Interface)] out IKIoBufferElement Result);

            /// <summary>
            /// Retrieve an IKArray whos elements of type KIOBUFFER_ELEMENT_DESCRIPTOR describe each underlying KIoBufferElement 
            /// buffer. The base address of the KIOBUFFER_ELEMENT_DESCRIPTOR[NumberOfElements] can be retrieved via IKArray.GetArrayBase()
            /// </summary>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT GetElements([Out] out UInt32 NumberOfElements, [Out, MarshalAs(UnmanagedType.Interface)] out IKArray Result);
        }

        /// <summary>
        /// COM wrapper type for KTL KArray instances once element types are undefined
        /// </summary>
        [ComImport]
        [Guid("b5a6bf9e-8247-40ce-aa31-acf8b099bd1c")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal unsafe interface IKArray
        {
            /// <summary>
            /// Eqv to KTL KArray{}::Status()
            /// </summary>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            [PreserveSig]
            HRESULT GetStatus();

            /// <summary>
            /// Eqv to KTL KArray{}::GetCount()
            /// </summary>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT GetCount([Out] out UInt32 Result);

            /// <summary>
            /// Retrieve the base address of the underlying KArray{}'s element array of size GetCount()
            /// </summary>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT GetArrayBase([Out] out void* Result);

            /// <summary>
            /// Test support method eqv to KArray{Guid}.Append(Guid ToAdd)
            /// </summary>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT AppendGuid([In] ref Guid ToAdd);

            /// <summary>
            /// Test support method eqv to KArray{KPHYSICAL_LOG_STREAM_RecordMetadata}.Append(KPHYSICAL_LOG_STREAM_RecordMetadata ToAdd)
            /// </summary>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT AppendRecordMetadata([In] KPHYSICAL_LOG_STREAM_RecordMetadata ToAdd);
        }


        //* IKPhysicalLogStream
        /// <summary>
        /// Eqv COM wrapper for KTL KtlLogStream instances
        /// </summary>
        [ComImport]
        [Guid("094fda74-d14b-4316-abad-aca133dfcb22")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal unsafe interface IKPhysicalLogStream
        {
            /// <summary>
            /// Eqv to KTL KtlLogStream::IsFunctional()
            /// </summary>
            /// <returns>S_OK if functional</returns>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            [PreserveSig]
            HRESULT IsFunctional();

            /// <summary>
            /// Eqv to KTL KtlLogStream::QueryLogStreamId()
            /// </summary>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT QueryLogStreamId(
                [Out] out KTL_LOG_STREAM_ID Result);

            /// <summary>
            /// Eqv to KTL KtlLogStream::QueryReservedMetadataSize()
            /// </summary>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT QueryReservedMetadataSize([Out] out UInt32 Result);


            /// <summary>
            /// Eqv to KTL KtlLogStream::AsyncQueryRecordRangeContext::StartQueryRecordRange()
            /// </summary>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT BeginQueryRecordRange(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback Callback,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeCommon.IFabricAsyncOperationContext Context);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT EndQueryRecordRange(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext Context,
                [Out] out KTL_LOG_ASN LowestAsn,
                [Out] out KTL_LOG_ASN HighestAsn,
                [Out] out KTL_LOG_ASN LogTruncationAsn);


            /// <summary>
            /// Eqv to KTL KtlLogStream::AsyncWriteContext::StartWrite()
            /// </summary>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT BeginWrite(
                [In] KTL_LOG_ASN Asn,
                [In] UInt64 Version,
                [In] UInt32 MetadataSize,
                [In, MarshalAs(UnmanagedType.Interface)] IKIoBuffer MetadataBuffer,
                [In, MarshalAs(UnmanagedType.Interface)] IKIoBuffer IoPageAlignedBuffer,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback Callback,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeCommon.IFabricAsyncOperationContext Context);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT EndWrite(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext Context);


            /// <summary>
            /// Eqv to KTL KtlLogStream::AsyncReadContext::StartRead()
            /// </summary>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT BeginRead(
                [In] KTL_LOG_ASN Asn,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback Callback,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeCommon.IFabricAsyncOperationContext Context);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT EndRead(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext Context,
                [Out] out UInt64 ResultingVersion,
                [Out] out UInt32 ResultingMetadataSize,
                [Out, MarshalAs(UnmanagedType.Interface)] out IKIoBuffer ResultingMetadata,
                [Out, MarshalAs(UnmanagedType.Interface)] out IKIoBuffer ResultingIoPageAlignedBuffer);


            /// <summary>
            /// Eqv to KTL KtlLogStream::AsyncReadContext::StartReadContaining()
            /// </summary>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT BeginReadContaining(
                [In] KTL_LOG_ASN Asn,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback Callback,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeCommon.IFabricAsyncOperationContext Context);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT EndReadContaining(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext Context,
                [Out] out KTL_LOG_ASN ContainingAsn,
                [Out] out UInt64 ContainingVersion,
                [Out] out UInt32 ResultingMetadataSize,
                [Out, MarshalAs(UnmanagedType.Interface)] out IKIoBuffer ResultingMetadata,
                [Out, MarshalAs(UnmanagedType.Interface)] out IKIoBuffer ResultingIoPageAlignedBuffer);


            /// <summary>
            /// Eqv to KTL KtlLogStream::AsyncQueryRecordContext::StartQueryRecord()
            /// </summary>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT BeginQueryRecord(
                [In] KTL_LOG_ASN Asn,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback Callback,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeCommon.IFabricAsyncOperationContext Context);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT EndQueryRecord(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext Context,
                [Out] out UInt64 Version,
                [Out] out KPHYSICAL_LOG_STREAM_RECORD_DISPOSITION Disposition,
                [Out] out UInt32 RecordSize,
                [Out] out UInt64 DebugInfo1);


            /// <summary>
            /// Eqv to KTL KtlLogStream::AsyncQueryRecordContext::StartQueryRecords()
            /// </summary>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT BeginQueryRecords(
                [In] KTL_LOG_ASN LowestAsn,
                [In] KTL_LOG_ASN HighestAsn,
                [In, MarshalAs(UnmanagedType.Interface)] IKArray ResultArray, // KArray<KPHYSICAL_LOG_STREAM_RecordMetadata>
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback Callback,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeCommon.IFabricAsyncOperationContext Context);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT EndQueryRecords(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext Context);


            /// <summary>
            /// Eqv to KTL KtlLogStream::AsyncTruncateContext::Truncate()
            /// </summary>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT Truncate(
                [In] KTL_LOG_ASN TruncationPoint,
                [In] KTL_LOG_ASN PreferredTruncationPoint);


            /// <summary>
            /// Eqv to KTL KtlLogStream::AsyncStreamNotificationContext::StartWaitForNotification()
            /// </summary>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT BeginWaitForNotification(
                [In] KPHYSICAL_LOG_STREAM_NOTIFICATION_TYPE NotificationType,
                [In] UInt64 NotificationValue,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback Callback,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeCommon.IFabricAsyncOperationContext Context);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT EndWaitForNotification(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext Context);


            /// <summary>
            /// Eqv to KTL KtlLogStream::AsyncIoctlContext::StartIoctl()
            /// </summary>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT BeginIoctl(
                [In] UInt32 ControlCode,
                [In, MarshalAs(UnmanagedType.Interface)] IKBuffer InBuffer,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback Callback,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeCommon.IFabricAsyncOperationContext Context);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT EndIoctl(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext Context,
                [Out] out UInt32 Result,
                [Out, MarshalAs(UnmanagedType.Interface)] out IKBuffer OutBuffer);


            /// <summary>
            /// Eqv to KTL KtlLogStream::StartClose()
            /// </summary>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT BeginClose(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback Callback,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeCommon.IFabricAsyncOperationContext Context);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT EndClose(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext Context);
        }


        //* IKPhysicalLogStream2
        /// <summary>
        /// Eqv COM wrapper for KTL KtlLogStream instances
        /// </summary>
        [ComImport]
        [Guid("E40B452B-03F9-4b6e-8229-53C509B87F2F")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal unsafe interface IKPhysicalLogStream2 : IKPhysicalLogStream
        {
            /// <summary>
            /// Eqv to KTL KtlLogStream::IsFunctional()
            /// </summary>
            /// <returns>S_OK if functional</returns>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            [PreserveSig]
            new HRESULT IsFunctional();

            /// <summary>
            /// Eqv to KTL KtlLogStream::QueryLogStreamId()
            /// </summary>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new HRESULT QueryLogStreamId(
                [Out] out KTL_LOG_STREAM_ID Result);

            /// <summary>
            /// Eqv to KTL KtlLogStream::QueryReservedMetadataSize()
            /// </summary>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new HRESULT QueryReservedMetadataSize([Out] out UInt32 Result);


            /// <summary>
            /// Eqv to KTL KtlLogStream::AsyncQueryRecordRangeContext::StartQueryRecordRange()
            /// </summary>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new HRESULT BeginQueryRecordRange(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback Callback,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeCommon.IFabricAsyncOperationContext Context);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new HRESULT EndQueryRecordRange(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext Context,
                [Out] out KTL_LOG_ASN LowestAsn,
                [Out] out KTL_LOG_ASN HighestAsn,
                [Out] out KTL_LOG_ASN LogTruncationAsn);


            /// <summary>
            /// Eqv to KTL KtlLogStream::AsyncWriteContext::StartWrite()
            /// </summary>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new HRESULT BeginWrite(
                [In] KTL_LOG_ASN Asn,
                [In] UInt64 Version,
                [In] UInt32 MetadataSize,
                [In, MarshalAs(UnmanagedType.Interface)] IKIoBuffer MetadataBuffer,
                [In, MarshalAs(UnmanagedType.Interface)] IKIoBuffer IoPageAlignedBuffer,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback Callback,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeCommon.IFabricAsyncOperationContext Context);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new HRESULT EndWrite(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext Context);


            /// <summary>
            /// Eqv to KTL KtlLogStream::AsyncReadContext::StartRead()
            /// </summary>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new HRESULT BeginRead(
                [In] KTL_LOG_ASN Asn,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback Callback,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeCommon.IFabricAsyncOperationContext Context);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new HRESULT EndRead(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext Context,
                [Out] out UInt64 ResultingVersion,
                [Out] out UInt32 ResultingMetadataSize,
                [Out, MarshalAs(UnmanagedType.Interface)] out IKIoBuffer ResultingMetadata,
                [Out, MarshalAs(UnmanagedType.Interface)] out IKIoBuffer ResultingIoPageAlignedBuffer);


            /// <summary>
            /// Eqv to KTL KtlLogStream::AsyncReadContext::StartReadContaining()
            /// </summary>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new HRESULT BeginReadContaining(
                [In] KTL_LOG_ASN Asn,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback Callback,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeCommon.IFabricAsyncOperationContext Context);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new HRESULT EndReadContaining(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext Context,
                [Out] out KTL_LOG_ASN ContainingAsn,
                [Out] out UInt64 ContainingVersion,
                [Out] out UInt32 ResultingMetadataSize,
                [Out, MarshalAs(UnmanagedType.Interface)] out IKIoBuffer ResultingMetadata,
                [Out, MarshalAs(UnmanagedType.Interface)] out IKIoBuffer ResultingIoPageAlignedBuffer);


            /// <summary>
            /// Eqv to KTL KtlLogStream::AsyncQueryRecordContext::StartQueryRecord()
            /// </summary>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new HRESULT BeginQueryRecord(
                [In] KTL_LOG_ASN Asn,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback Callback,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeCommon.IFabricAsyncOperationContext Context);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new HRESULT EndQueryRecord(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext Context,
                [Out] out UInt64 Version,
                [Out] out KPHYSICAL_LOG_STREAM_RECORD_DISPOSITION Disposition,
                [Out] out UInt32 RecordSize,
                [Out] out UInt64 DebugInfo1);


            /// <summary>
            /// Eqv to KTL KtlLogStream::AsyncQueryRecordContext::StartQueryRecords()
            /// </summary>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new HRESULT BeginQueryRecords(
                [In] KTL_LOG_ASN LowestAsn,
                [In] KTL_LOG_ASN HighestAsn,
                [In, MarshalAs(UnmanagedType.Interface)] IKArray ResultArray, // KArray<KPHYSICAL_LOG_STREAM_RecordMetadata>
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback Callback,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeCommon.IFabricAsyncOperationContext Context);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new HRESULT EndQueryRecords(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext Context);


            /// <summary>
            /// Eqv to KTL KtlLogStream::AsyncTruncateContext::Truncate()
            /// </summary>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new HRESULT Truncate(
                [In] KTL_LOG_ASN TruncationPoint,
                [In] KTL_LOG_ASN PreferredTruncationPoint);


            /// <summary>
            /// Eqv to KTL KtlLogStream::AsyncStreamNotificationContext::StartWaitForNotification()
            /// </summary>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new HRESULT BeginWaitForNotification(
                [In] KPHYSICAL_LOG_STREAM_NOTIFICATION_TYPE NotificationType,
                [In] UInt64 NotificationValue,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback Callback,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeCommon.IFabricAsyncOperationContext Context);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new HRESULT EndWaitForNotification(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext Context);


            /// <summary>
            /// Eqv to KTL KtlLogStream::AsyncIoctlContext::StartIoctl()
            /// </summary>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new HRESULT BeginIoctl(
                [In] UInt32 ControlCode,
                [In, MarshalAs(UnmanagedType.Interface)] IKBuffer InBuffer,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback Callback,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeCommon.IFabricAsyncOperationContext Context);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new HRESULT EndIoctl(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext Context,
                [Out] out UInt32 Result,
                [Out, MarshalAs(UnmanagedType.Interface)] out IKBuffer OutBuffer);


            /// <summary>
            /// Eqv to KTL KtlLogStream::StartClose()
            /// </summary>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new HRESULT BeginClose(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback Callback,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeCommon.IFabricAsyncOperationContext Context);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new HRESULT EndClose(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext Context);

            // New methods            

            /// <summary>
            /// Eqv to KTL KtlLogStream::AsyncMultiRecordReadContext::StartRead()
            /// </summary>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT BeginMultiRecordRead(
                [In] KTL_LOG_ASN Asn,
                [In] UInt32 BytesToRead,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback Callback,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeCommon.IFabricAsyncOperationContext Context);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT EndMultiRecordRead(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext Context,
                [Out, MarshalAs(UnmanagedType.Interface)] out IKIoBuffer ResultingMetadata,
                [Out, MarshalAs(UnmanagedType.Interface)] out IKIoBuffer ResultingIoPageAlignedBuffer);
        }

        //* IKPhysicalLogContainer
        private const UInt32 IKPhysicalLogContainer_MaxAliasNameSize = 128; // Alias names are limited in size to 128 chars

        /// <summary>
        /// Eqv COM wrapper for KTL KtlLogContainer instances
        /// </summary>
        [ComImport]
        [Guid("1bf056c8-57ba-4fa1-b9c7-bd9c4783bf6a")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal unsafe interface IKPhysicalLogContainer
        {
            /// <summary>
            /// Eqv to KTL KtlLogContainer::IsFunctional()
            /// </summary>
            /// <returns>S_OK if functional</returns>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            [PreserveSig]
            HRESULT IsFunctional();


            /// <summary>
            /// Eqv to KTL KtlLogContainer::AsyncCreateLogStreamContext::StartCreateLogStream()
            /// </summary>
            /// <param name="LogStreamTypeId"></param>
            /// <param name="OptionalLogStreamAlias">Alias - limited to IKPhysicalLogContainer_MaxAliasNameSize</param>
            /// <param name="LogStreamId"></param>
            /// <param name="Path"></param>
            /// <param name="OptionalSecurityInfo"></param>
            /// <param name="MaximumStreamSize"></param>
            /// <param name="MaximumBlockSize"></param>
            /// <param name="CreationFlags"></param>
            /// <param name="Callback"></param>
            /// <param name="Context"></param>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT BeginCreateLogStream(
                [In] KTL_LOG_STREAM_ID LogStreamId,
                [In] KTL_LOG_STREAM_TYPE_ID LogStreamTypeId,
                [In] IntPtr OptionalLogStreamAlias,
                [In] IntPtr Path,
                [In, MarshalAs(UnmanagedType.Interface)] IKBuffer OptionalSecurityInfo,
                [In] Int64 MaximumStreamSize,
                [In] UInt32 MaximumBlockSize,
                [In] LogManager.LogCreationFlags CreationFlags,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback Callback,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeCommon.IFabricAsyncOperationContext Context);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT EndCreateLogStream(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext Context,
                [Out, MarshalAs(UnmanagedType.Interface)] out IKPhysicalLogStream Result);


            /// <summary>
            /// Eqv to KTL KtlLogContainer::AsyncDeleteLogStreamContext::StartDeleteLogStream()
            /// </summary>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT BeginDeleteLogStream(
                [In] KTL_LOG_STREAM_ID LogStreamId,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback Callback,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeCommon.IFabricAsyncOperationContext Context);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT EndDeleteLogStream(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext Context);


            /// <summary>
            /// Eqv to KTL KtlLogContainer::AsyncOpenLogStreamContext::StartOpenLogStream()
            /// </summary>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT BeginOpenLogStream(
                [In] KTL_LOG_STREAM_ID LogStreamId,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback Callback,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeCommon.IFabricAsyncOperationContext Context);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT EndOpenLogStream(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext Context,
                [Out, MarshalAs(UnmanagedType.Interface)] out IKPhysicalLogStream Result);


            /// <summary>
            /// Eqv to KTL KtlLogContainer::AsyncAssignAliasContext::StartAssignAlias()
            /// </summary>
            /// <param name="Alias">Alias - limited to IKPhysicalLogContainer_MaxAliasNameSize</param>
            /// <param name="LogStreamId"></param>
            /// <param name="Callback"></param>
            /// <param name="Context"></param>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT BeginAssignAlias(
                [In] IntPtr Alias,
                [In] KTL_LOG_STREAM_ID LogStreamId,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback Callback,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeCommon.IFabricAsyncOperationContext Context);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT EndAssignAlias(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext Context);


            /// <summary>
            /// Eqv to KTL KtlLogContainer::AsyncRemoveAliasContext::StartRemoveAlias()
            /// </summary>
            /// <param name="Alias">Alias - limited to IKPhysicalLogContainer_MaxAliasNameSize</param>
            /// <param name="Callback"></param>
            /// <param name="Context"></param>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT BeginRemoveAlias(
                [In] IntPtr Alias,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback Callback,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeCommon.IFabricAsyncOperationContext Context);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT EndRemoveAlias(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext Context);


            /// <summary>
            /// Eqv to KTL KtlLogContainer::AsyncResolveAliasContext::StartResolveAlias()
            /// </summary>
            /// <param name="Alias">Alias - limited to IKPhysicalLogContainer_MaxAliasNameSize</param>
            /// <param name="Callback"></param>
            /// <param name="Context"></param>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT BeginResolveAlias(
                [In] IntPtr Alias,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback Callback,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeCommon.IFabricAsyncOperationContext Context);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT EndResolveAlias(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext Context,
                [Out] out KTL_LOG_STREAM_ID Result);


            /// <summary>
            /// Eqv to KTL KtlLogContainer::StartClose()
            /// </summary>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT BeginClose(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback Callback,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeCommon.IFabricAsyncOperationContext Context);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT EndClose(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext Context);
        }


        //* IKPhysicalLogManager
        /// <summary>
        /// Eqv COM wrapper for KTL KtlLogManager instances
        /// </summary>
        [ComImport]
        [Guid("c63c9378-6018-4859-a91e-85c268435d99")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal unsafe interface IKPhysicalLogManager
        {
            /// <summary>
            /// Eqv to KTL KtlLogContainer::StartOpenLogManager()
            /// </summary>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT BeginOpen(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback Callback,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeCommon.IFabricAsyncOperationContext Context);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT EndOpen(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext Context);


            /// <summary>
            /// Eqv to KTL KtlLogContainer::StartCloseLogManager()
            /// </summary>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT BeginClose(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback Callback,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeCommon.IFabricAsyncOperationContext Context);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT EndClose(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext Context);


            /// <summary>
            /// Eqv to KTL KtlLogContainer::AsyncCreateLogContainer::StartCreateLogContainer()
            /// </summary>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT BeginCreateLogContainer(
                [In] IntPtr FullyQualifiedLogFilePath,
                [In] KTL_LOG_ID LogId,
                [In] IntPtr LogType,
                [In] Int64 LogSize,
                [In] UInt32 MaxAllowedStreams,
                [In] UInt32 MaxRecordSize,
                [In] LogManager.LogCreationFlags CreationFlags,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback Callback,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeCommon.IFabricAsyncOperationContext Context);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT EndCreateLogContainer(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext Context,
                [Out, MarshalAs(UnmanagedType.Interface)] out IKPhysicalLogContainer Result);


            /// <summary>
            /// Eqv to KTL KtlLogContainer::AsyncOpenLogContainer::StartOpenLogContainer()
            /// </summary>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT BeginOpenLogContainerById(
                [In] KTL_DISK_ID DiskId,
                [In] KTL_LOG_ID LogId,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback Callback,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeCommon.IFabricAsyncOperationContext Context);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT BeginOpenLogContainer(
                [In] IntPtr FullyQualifiedLogFilePath,
                [In] KTL_LOG_ID LogId,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback Callback,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeCommon.IFabricAsyncOperationContext Context);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT EndOpenLogContainer(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext Context,
                [Out, MarshalAs(UnmanagedType.Interface)] out IKPhysicalLogContainer Result);


            /// <summary>
            /// Eqv to KTL KtlLogContainer::AsyncDeleteLogContainer::StartDeleteLogContainer()
            /// </summary>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT BeginDeleteLogContainer(
                [In] IntPtr FullyQualifiedLogFilePath,
                [In] KTL_LOG_ID LogId,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback Callback,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeCommon.IFabricAsyncOperationContext Context);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT BeginDeleteLogContainerById(
                [In] KTL_DISK_ID DiskId,
                [In] KTL_LOG_ID LogId,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback Callback,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeCommon.IFabricAsyncOperationContext Context);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT EndDeleteLogContainer(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext Context);


            /// <summary>
            /// Eqv to KTL KtlLogContainer::AsyncEnumerateLogContainer::StartEnumerateLogContainer()
            /// </summary>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT BeginEnumerateLogContainers(
                [In] KTL_DISK_ID DiskId,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback Callback,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeCommon.IFabricAsyncOperationContext Context);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT EndEnumerateLogContainers(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext Context,
                [Out, MarshalAs(UnmanagedType.Interface)] out IKArray Result);


            /// <summary>
            /// Helper to convert a volume as described by a Path into its Volume ID Guid (Result)
            /// </summary>
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT BeginGetVolumeIdFromPath(
                [In] IntPtr Path,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback Callback,
                [Out, MarshalAs(UnmanagedType.Interface)] out NativeCommon.IFabricAsyncOperationContext Context);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            HRESULT EndGetVolumeIdFromPath(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext Context,
                [Out] out Guid Result);
        }


        //** DLL API
        /// <summary>
        /// Eqv to KTL KtlLogManager::Create()
        /// </summary>
#if DotNetCoreClrLinux
        [DllImport("libKPhysicalLog.so", EntryPoint = "KCreatePhysicalLogManager", PreserveSig = false)]
#else
        [DllImport("KPhysicalLogDll.dll", EntryPoint = "KCreatePhysicalLogManager", PreserveSig = false)]
#endif
        internal static extern HRESULT CreateLogManager([Out, MarshalAs(UnmanagedType.Interface)] out IKPhysicalLogManager Result);

#if DotNetCoreClrLinux
        [DllImport("libKPhysicalLog.so", EntryPoint = "KCreatePhysicalLogManagerInproc", PreserveSig = false)]
#else
        [DllImport("KPhysicalLogDll.dll", EntryPoint = "KCreatePhysicalLogManagerInproc", PreserveSig = false)]
#endif
        internal static extern HRESULT CreateLogManagerInproc([Out, MarshalAs(UnmanagedType.Interface)] out IKPhysicalLogManager Result);

        /// <summary>
        /// Eqv to KTL KBuffer::Create()
        /// </summary>
#if DotNetCoreClrLinux
        [DllImport("libKPhysicalLog.so", EntryPoint = "KCreateKBuffer", PreserveSig = false)]
#else
        [DllImport("KPhysicalLogDll.dll", EntryPoint = "KCreateKBuffer", PreserveSig = false)]
#endif
        internal static extern HRESULT CreateKBuffer([In] UInt32 Size, [Out, MarshalAs(UnmanagedType.Interface)] out IKBuffer Result);

        /// <summary>
        /// Eqv to KTL KIoBufferElement::CreateNew()
        /// </summary>
#if DotNetCoreClrLinux
        [DllImport("libKPhysicalLog.so", EntryPoint = "KCreateKIoBufferElement", PreserveSig = false)]
#else
        [DllImport("KPhysicalLogDll.dll", EntryPoint = "KCreateKIoBufferElement", PreserveSig = false)]
#endif
        internal static extern HRESULT CreateKIoBufferElement([In] UInt32 Size, [Out, MarshalAs(UnmanagedType.Interface)] out IKIoBufferElement Result);

        /// <summary>
        /// Eqv to KTL KIoBuffer::CreateEmpty()
        /// </summary>
#if DotNetCoreClrLinux
        [DllImport("libKPhysicalLog.so", EntryPoint = "KCreateEmptyKIoBuffer", PreserveSig = false)]
#else
        [DllImport("KPhysicalLogDll.dll", EntryPoint = "KCreateEmptyKIoBuffer", PreserveSig = false)]
#endif
        internal static extern HRESULT CreateEmptyKIoBuffer([Out, MarshalAs(UnmanagedType.Interface)] out IKIoBuffer Result);

        /// <summary>
        /// Eqv to KTL KIoBuffer::CreateSimple()
        /// </summary>
#if DotNetCoreClrLinux
        [DllImport("libKPhysicalLog.so", EntryPoint = "KCreateSimpleKIoBuffer", PreserveSig = false)]
#else
        [DllImport("KPhysicalLogDll.dll", EntryPoint = "KCreateSimpleKIoBuffer", PreserveSig = false)]
#endif
        internal static extern HRESULT CreateSimpleKIoBuffer([In] UInt32 Size, [Out, MarshalAs(UnmanagedType.Interface)] out IKIoBuffer Result);

        /// <summary>
        /// Eqv to KTL new KSharedType{KArray{Guid}}()
        /// </summary>
#if DotNetCoreClrLinux
        [DllImport("libKPhysicalLog.so", EntryPoint = "KCreateGuidIKArray", PreserveSig = false)]
#else
        [DllImport("KPhysicalLogDll.dll", EntryPoint = "KCreateGuidIKArray", PreserveSig = false)]
#endif
        internal static extern HRESULT CreateGuidIKArray([In] UInt32 InitialSize, [Out, MarshalAs(UnmanagedType.Interface)] out IKArray Result);

        /// <summary>
        /// Eqv to KTL new KSharedType{KArray{KPHYSICAL_LOG_STREAM_RecordMetadata}}()
        /// </summary>
#if DotNetCoreClrLinux
        [DllImport("libKPhysicalLog.so", EntryPoint = "KCreateLogRecordMetadataIKArray", PreserveSig = false)]
#else
        [DllImport("KPhysicalLogDll.dll", EntryPoint = "KCreateLogRecordMetadataIKArray", PreserveSig = false)]
#endif
        internal static extern HRESULT CreateLogRecordMetadataIKArray([In] UInt32 InitialSize, [Out, MarshalAs(UnmanagedType.Interface)] out IKArray Result);

        /// <summary>
        /// Eqv to memcpy()  - the intrinsic
        /// </summary>
#if DotNetCoreClrLinux
        [DllImport("libKPhysicalLog.so", EntryPoint = "KCopyMemory", PreserveSig = false)]
#else
        [DllImport("KPhysicalLogDll.dll", EntryPoint = "KCopyMemory", PreserveSig = false)]
#endif
        internal static extern unsafe HRESULT CopyMemory([In] void* Soruce, [In] void* Dest, [In] UInt32 Size); // CONSIDER: Does this need pinning ?

        /// <summary>
        /// Eqv to KChecksum::Crc64()
        /// </summary>
#if DotNetCoreClrLinux
        [DllImport("libKPhysicalLog.so", EntryPoint = "KCrc64", PreserveSig = true)]
#else
        [DllImport("KPhysicalLogDll.dll", EntryPoint = "KCrc64", PreserveSig = true)]
#endif
        internal static extern unsafe UInt64 KCrc64([In] void* Source, [In] UInt32 Size, [In] UInt64 InitialCrc); // CONSIDER: Does this need pinning ?

        /// <summary>
        /// Query build information
        /// </summary>
#if DotNetCoreClrLinux
        [DllImport("libKPhysicalLog.so", EntryPoint = "QueryUserBuildInformation", PreserveSig = true)]
#else
        [DllImport("KPhysicalLogDll.dll", EntryPoint = "QueryUserBuildInformation", PreserveSig = true)]
#endif
        internal static extern unsafe UInt64 QueryUserBuildInformation([Out] out UInt32 BuildNumber, [Out] out byte IsFreeBuild);

        // CONSIDER: Does this need pinning ?
    }
}