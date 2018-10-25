// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define NTDEVICE_NAME_STRING      L"\\Device\\KtlLogger"
#define SYMBOLIC_NAME_STRING     L"\\DosDevices\\KtlLogger"
#define DEVICEFILE_NAME_STRING     L"\\\\.\\KtlLogger"

#define KTLLOGGER_DEVICE_TYPE 32768
#define KTLLOGGER_DEVICE_IOCTL_X (CTL_CODE(KTLLOGGER_DEVICE_TYPE,                          \
                                                     1,                                    \
                                                     METHOD_BUFFERED,                      \
                                                     FILE_READ_ACCESS | FILE_WRITE_ACCESS))

class RequestMarshaller : public KObject<RequestMarshaller>, public KShared<RequestMarshaller>
{
    K_FORCE_SHARED_WITH_INHERITANCE(RequestMarshaller);

    protected:        
        RequestMarshaller(
            __in ULONG AllocationTag
            );

    public:
        typedef enum 
        {
            NullObject, LogManager, LogContainer, LogStream
        } OBJECT_TYPE;

        static const PWCHAR KTLLOGGER_DEVICE_NAME;
        static const ULONG KTLLOGGER_DEVICE_IOCTL;
        
    protected:
        inline BOOLEAN IsValidObjectType(
            __in OBJECT_TYPE ObjectType
        )
        {
            if ((ObjectType != NullObject) &&
                (ObjectType != LogManager) &&
                (ObjectType != LogContainer) &&
                (ObjectType != LogStream))
            {
                return(FALSE);
            }
            return(TRUE);
        }
        
        //
        // Data structures and helper routines used for passing from user to kernel
        //
        static const ULONG REQUEST_VERSION_1 = 1;
        static const ULONG REQUEST_VERSION_2 = 2;
        static const ULONG REQUEST_VERSION_3 = 3;
        static const ULONG REQUEST_VERSION_4 = 4;
        static const ULONG REQUEST_VERSION_5 = 5;

        inline BOOLEAN IsValidRequestVersion(
            __in ULONG RequestVersion
            )
        {
            return ((RequestVersion == REQUEST_VERSION_1)
                || (RequestVersion == REQUEST_VERSION_2)
                || (RequestVersion == REQUEST_VERSION_3)
                || (RequestVersion == REQUEST_VERSION_5)) ? TRUE : FALSE;
        }

    public:
        typedef enum 
        {
            CreateObjectRequest, DeleteObjectRequest, ObjectMethodRequest
        } REQUEST_TYPE;

    protected:
        inline BOOLEAN IsValidRequestType(
            REQUEST_TYPE RequestType
            )
        {
            if ((RequestType != CreateObjectRequest) &&
                (RequestType != DeleteObjectRequest) &&
                (RequestType != ObjectMethodRequest))
            {
                return(FALSE);
            }
            return(TRUE);
        }

    public:
        typedef enum
        {
            ObjectMethodNone,
            
            CreateLogContainer, OpenLogContainer, DeleteLogContainer, EnumerateLogContainers,

            CreateLogStream, DeleteLogStream, OpenLogStream, AssignAlias, RemoveAlias, ResolveAlias, CloseContainer,

            QueryRecordRange, QueryRecord, QueryRecords, Write, Reservation, Read, Truncate, WriteMetadata, ReadMetadata, Ioctl, CloseStream, StreamNotification,

            // Introduced in REQUEST_VERSION_2
            MultiRecordRead,
                    
            // Introduced in REQUEST_VERSION_3
            ConfigureLogManager,

			// Introduced in REQUEST_VERSION_4
			EnumerateStreams,

            // Introduced in REQUEST_VERSION_5
            WriteAndReturnLogUsage
                    
        } OBJECT_METHOD;


#ifdef FORCE_REALLOC
        static const ULONG DefaultPreAllocRecordMetadataSize = 0x1000;    // 4K
        static const ULONG DefaultPreAllocMetadataSize = 0x1000;
        static const ULONG DefaultPreAllocDataSize = 0x1000;
#else
        static const ULONG DefaultPreAllocRecordMetadataSize = 0x4000;    // 16K
        static const ULONG DefaultPreAllocMetadataSize = 0x20000;         // 128K
        static const ULONG DefaultPreAllocDataSize = 0x100000;            // 1MB
#endif
        
        //
        // This specifies a NULL object
        //
        static const ULONGLONG OBJECTID_NULL = 0;
        
        typedef struct
        {
            OBJECT_TYPE ObjectType;
            ULONGLONG ObjectId;
        } OBJECT_TYPEANDID;

        //
        // Buffer Access methods
        //
        PVOID GetBuffer()
        {
            return(_Buffer);
        }

        ULONG GetWriteSize()
        {
            return(_WriteIndex);
        }

        ULONG GetBufferSize()
        {
            return(_MaxIndex);
        }

        //
        // This is the format of the request as it is passed through
        // the buffered IOCTL
        //
        typedef struct 
        {
            REQUEST_TYPE RequestType;
            ULONG Version;
            OBJECT_TYPEANDID ObjectTypeAndId;
            ULONGLONG RequestId;
            NTSTATUS FinalStatus;
            ULONG SizeNeeded;
            char Data[1];
        } REQUEST_BUFFER;

    protected:
        __inline ULONG AlignmentNeeded(
            __in ULONG Value,
            __in ULONG Alignment
        )
        {
            return(((Value + (Alignment-1)) & (~(Alignment-1))) - Value);
        }

        static const ULONG BaseReturnBufferSize = FIELD_OFFSET(REQUEST_BUFFER, Data);
        static const ULONG ObjectReturnBufferSize =
                          (BaseReturnBufferSize) +
                          (((BaseReturnBufferSize + (sizeof(LONGLONG)-1)) & (~(sizeof(LONGLONG)-1))) - BaseReturnBufferSize) +
                          sizeof(OBJECT_TYPEANDID);
        
        //
        // Methods to write to the buffer
        //
        NTSTATUS SetWriteIndex(
            __in ULONG WriteIndex
        );

        NTSTATUS WriteBytes(
            __in_bcount(ByteCount) PUCHAR Bytes,
            __in ULONG ByteCount,
            __in ULONG Alignment
        );

    public:
        
        template <typename T>
        NTSTATUS
        WriteData(T Data)
        {
            return(WriteBytes((PUCHAR)(&Data),
                              sizeof(T),
                              sizeof(T)));           
        }
        
        NTSTATUS WritePVOID(
            __in PVOID Pvoid
        )
        {
            ULONGLONG u = (ULONGLONG)Pvoid;
            
            return(WriteBytes((PUCHAR)(&u),
                              sizeof(ULONGLONG),
                              sizeof(ULONGLONG)));           
        }
        
        NTSTATUS WriteKBuffer(
            __in KBuffer::SPtr& Buffer
        );

        NTSTATUS WriteGuid(
            __in KGuid& Guid
        )
        {
            return(WriteBytes((PUCHAR)(&Guid),
                              sizeof(GUID),
                              sizeof(ULONG)));
        }
        
        NTSTATUS WriteKtlLogContainerId(
            __in KtlLogContainerId& Id
        )
        {
            return(WriteBytes((PUCHAR)(&Id),
                              sizeof(KtlLogContainerId),
                              sizeof(ULONG)));
        }

        NTSTATUS
        RequestMarshaller::WriteKtlLogContainerIdArray(
        #ifdef USE_RVDLOGGER_OBJECTS
            __in KArray<RvdLogId>& Array
        #else
            __in KArray<KtlLogContainerId>& Array
        #endif
        );      
        
        NTSTATUS WriteKtlLogStreamId(
            __in KtlLogStreamId& Id
        )
        {
            return(WriteBytes((PUCHAR)(&Id),
                              sizeof(KtlLogStreamId),
                              sizeof(ULONG)));
        }
        
        NTSTATUS
        RequestMarshaller::WriteKtlLogStreamIdArray(
        #ifdef USE_RVDLOGGER_OBJECTS
            __in KArray<RvdLogStreamId>& Array
        #else
            __in KArray<KtlLogStreamId>& Array
        #endif
        );      
        
        NTSTATUS WriteKtlLogStreamType(
            __in KtlLogStreamType& Type
        )
        {
            return(WriteBytes((PUCHAR)(&Type),
                              sizeof(KtlLogStreamType),
                              sizeof(ULONG)));
        }
        
        NTSTATUS WriteRecordMetadata(
#ifdef USE_RVDLOGGER_OBJECTS
            __in RvdLogStream::RecordMetadata& Record
#else
            __in RecordMetadata& Record     
#endif
        );
        
        NTSTATUS WriteRecordMetadataArray(
#ifdef USE_RVDLOGGER_OBJECTS
            __in KArray<RvdLogStream::RecordMetadata>& Array
#else
            __in KArray<RecordMetadata>& Array
#endif
        );
        
        NTSTATUS WriteString(
            __in KString::CSPtr String
            );
        
        //
        // Methods to read from the buffer
        //
    protected:
        
        NTSTATUS SetReadIndex(
            __in ULONG ReadIndex
        );

        NTSTATUS ReadBytes(
            __in_bcount(ByteCount) PUCHAR Bytes,
            __in ULONG ByteCount,
            __in ULONG Alignment
        );

    public:
        template <typename T>
        NTSTATUS
        ReadData(T& Data)
        {
            return(ReadBytes((PUCHAR)(&Data),
                              sizeof(T),
                              sizeof(T)));           
        }

        NTSTATUS ReadObjectTypeAndId(                              
            __out OBJECT_TYPEANDID& ObjectTypeAndId 
        );
                
        NTSTATUS ReadKtlLogContainerId(
            __out KtlLogContainerId& Id
        )
        {
            return(ReadBytes((PUCHAR)(&Id),
                             sizeof(KtlLogContainerId),
                             sizeof(ULONG)));
        }

        NTSTATUS
        RequestMarshaller::ReadKtlLogContainerIdArray(
        #ifdef USE_RVDLOGGER_OBJECTS
            __in KArray<RvdLogId>& Array
        #else
            __in KArray<KtlLogContainerId>& Array
        #endif
        );
        
        NTSTATUS ReadKtlLogStreamId(
            __out KtlLogStreamId& Id
        )
        {
            return(ReadBytes((PUCHAR)(&Id),
                             sizeof(KtlLogStreamId),
                             sizeof(ULONG)));
        }       
        
        NTSTATUS
        RequestMarshaller::ReadKtlLogStreamIdArray(
        #ifdef USE_RVDLOGGER_OBJECTS
            __in KArray<RvdLogStreamId>& Array
        #else
            __in KArray<KtlLogStreamId>& Array
        #endif
        );
        
        
        NTSTATUS ReadKtlLogStreamType(
            __out KtlLogStreamType& Type
        )
        {
            return(ReadBytes((PUCHAR)(&Type),
                             sizeof(KtlLogStreamType),
                             sizeof(ULONG)));
        }       
        
        
        NTSTATUS ReadReadType(
            __out RvdLogStream::AsyncReadContext::ReadType& ReadType
        )
        {
            ReadType = RvdLogStream::AsyncReadContext::ReadTypeNotSpecified;
            return(ReadBytes((PUCHAR)(&ReadType),
                              sizeof(RvdLogStream::AsyncReadContext::ReadType),
                              sizeof(RvdLogStream::AsyncReadContext::ReadType)));
        }

        NTSTATUS ReadKBuffer(
            __out KBuffer::SPtr& Buffer
        );
        
        NTSTATUS ReadRecordDisposition(
#ifdef USE_RVDLOGGER_OBJECTS
            __out RvdLogStream::RecordDisposition& Disposition
#else
            __out KtlLogStream::RecordDisposition& Disposition
#endif
        )
        {
            return(ReadBytes((PUCHAR)(&Disposition),
#ifdef USE_RVDLOGGER_OBJECTS
                             sizeof(RvdLogStream::RecordDisposition),
                             sizeof(RvdLogStream::RecordDisposition)));
#else
                             sizeof(KtlLogStream::RecordDisposition),
                             sizeof(KtlLogStream::RecordDisposition)));
#endif
        }       

        NTSTATUS ReadRecordMetadata(
#ifdef USE_RVDLOGGER_OBJECTS
            __in RvdLogStream::RecordMetadata& Record
#else
            __in RecordMetadata& Record     
#endif
        );
        
        NTSTATUS ReadRecordMetadataArray(
#ifdef USE_RVDLOGGER_OBJECTS
            __in KArray<RvdLogStream::RecordMetadata>& Array
#else
            __in KArray<RecordMetadata>& Array
#endif
        );
        
        NTSTATUS ReadGuid(
            __out KGuid& Guid
        )
        {
            return(ReadBytes((PUCHAR)(&Guid),
                             sizeof(GUID),
                             sizeof(ULONG)));
        }
        
        NTSTATUS ReadString(
            __out KString::CSPtr& String
        );
        
        NTSTATUS ReadString(
            __out KString::SPtr& String
        );

        NTSTATUS ReadPVOID(
            __out PVOID& Pvoid
        )
        {
            NTSTATUS status;
            
            ULONGLONG u = 0;
            status = ReadBytes((PUCHAR)(&u),
                             sizeof(ULONGLONG),
                             sizeof(ULONGLONG));
            if (NT_SUCCESS(status))
            {
                Pvoid = (PVOID)(u);
            }
            
            return(status);
        }

    protected:
        PUCHAR _Buffer;
        ULONG _MaxIndex;
        ULONG _WriteIndex;
        ULONG _ReadIndex;
        BOOLEAN _ExtendBuffer;
        ULONGLONG _LogSize;
        ULONGLONG _LogSpaceRemaining;

        KBuffer::SPtr _KBuffer;
        ULONG _AllocationTag;     
};
