/*++

    (c) 2010 by Microsoft Corp. All Rights Reserved.

    InternalRvdLogger.h

    Description:
      Internal constant and log storage structure definitions.

    History:
      PengLi, Richhas    22-Oct-2010         Initial version.

--*/

#pragma once

#include <ktl.h>
#include <ktrace.h>
#include <kinstrumentop.h>

#include "../inc/KtlPhysicalLog.h"


// Common global (static) utility functions
class RvdLogUtility
{
public:
    static ULONG const      AsciiGuidLength = 38;

    static VOID
    //* Convert a GUID to an array of ascii characters in the form of:
    //      {12345678-1234-1234-1234-123456789012} - 38 bytes
    //
    GuidToAcsii(__in const GUID& Guid, __out_bcount(38) UCHAR* ResultArray);

    class AsciiGuid
    {
    public:
        AsciiGuid();
        AsciiGuid(__in const GUID& Guid);

        LPCSTR
        Get();

        VOID
        Set(__in const GUID& Guid);

    private:
        VOID
        Init();

    private:
        UCHAR           _Value[RvdLogUtility::AsciiGuidLength + 1];
    };

    class AsciiTwinGuid
    {
    public:
        AsciiTwinGuid();
        AsciiTwinGuid(__in const GUID& Guid0, __in const GUID& Guid1);

        LPCSTR
        Get();

        VOID
        Set(__in const GUID& Guid0, __in const GUID& Guid1);

    private:
        VOID
        Init();

    private:
        UCHAR           _Value[(RvdLogUtility::AsciiGuidLength * 2) + 2];
    };
};

// Strong id and scalar types used in the implementation
class LogFormatSignature : public KStrongType<LogFormatSignature, KGuid>
{
    K_STRONG_TYPE(LogFormatSignature, KGuid);
public:
    LogFormatSignature() {}
    ~LogFormatSignature() {}
};

#define NUMBER_SIGNATURE_ULONGS 4

class RvdLogConfig;

//** Global Rvd Log implmentation constants
// BUG, richhas, xxxxx, some of these definitions will need to become
// configuration variables.

//
// Version 1.1 - Added CreationFlags
//
class RvdDiskLogConstants
{
public:
    static USHORT const             MajorFormatVersion = 1;
    static USHORT const             MinorFormatVersion = 1;
    static USHORT const             SignatureULongs = NUMBER_SIGNATURE_ULONGS;
    static USHORT const             MaxCreationPathSize = 512;
    static ULONG const              BlockSize = 4 * 1024;
    static ULONG const              MasterBlockSize = BlockSize;

    // Directory and file name strings:
    //      Log file names have the following format: "Log{XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}.log"
    //                                                 0123456789012345678901234567890123456789012345
#if !defined(PLATFORM_UNIX)
    static __inline wchar_t const&  DirectoryName() {return *(L"\\RvdLog\\");}
    static __inline wchar_t const&  RawDirectoryName() {return *(L"RvdLog\\");}
#else
    static __inline wchar_t const&  DirectoryName() {return *(L"/RvdLog/");}
    static __inline wchar_t const&  RawDirectoryName() {return *(L"RvdLog/");}
#endif
    static __inline wchar_t const&  RawDirectoryNameStr() {return *(L"RvdLog");}
    static __inline wchar_t const&  NamePrefix()    {return *(L"Log");}
    static ULONG const              NamePrefixSize = 3;
    static __inline wchar_t const&  NameExtension() {return *(L".log");}
    static ULONG const              NameExtensionSize = 4;
    static ULONG const              LogFileNameLength = 45;
    static ULONG const              LogFileNameGuidOffset = 3;
    static ULONG const              LogFileNameExtensionOffset = 41;
    static ULONG const              LogFileNameGuidStrLength = LogFileNameExtensionOffset - LogFileNameGuidOffset;

    //** Well known KGuid values
    // Special Free StreamInfo StreamId GUID: {00000000-0000-0000-0000-000000000000}
    static RvdLogStreamId const&    FreeStreamInfoStreamId()
    {
        static GUID const freeStreamInfoStreamId = {0x00000000, 0x0000, 0x0000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
        return *(reinterpret_cast<RvdLogStreamId const*>(&freeStreamInfoStreamId));
    }

    // Special Invalid StreamId GUID: {00000000-0000-0000-0000-000000000001}
    static RvdLogStreamId const&    InvalidLogStreamId()
    {
        static GUID const invalidStreamId = {0x00000000, 0x0000, 0x0000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01}};
        return *(reinterpret_cast<RvdLogStreamId const*>(&invalidStreamId));
    }

    // Special StreamType used to indicate a stream is being deleted: {00000000-0000-0000-0000-000000000002}
    static RvdLogStreamType const&  DeletingStreamType()
    {
        static GUID const deletingStreamType = {0x00000000, 0x0000, 0x0000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02}};
        return *(reinterpret_cast<RvdLogStreamType const*>(&deletingStreamType));
    }

    // A valid log master block must have this GUID: {1F1B95F3-4563-4530-A26A-8E17436EDE58}
    static LogFormatSignature const&    LogFormatGuid()
    {
        static GUID const logFormatGuid = {0x1f1b95f3, 0x4563, 0x4530, {0xa2, 0x6a, 0x8e, 0x17, 0x43, 0x6e, 0xde, 0x58}};
        return *(reinterpret_cast<LogFormatSignature const*>(&logFormatGuid));
    }

    // All internal checkpoint records use this GUID: {C9DC46E5-B669-4a25-A9E2-0DFF3D95C163}
    // as their LogStreamId.
    static RvdLogStreamId const&    CheckpointStreamId()
    {
        static GUID const checkpointStreamId = {0xc9dc46e5, 0xb669, 0x4a25, {0xa9, 0xe2, 0xd, 0xff, 0x3d, 0x95, 0xc1, 0x63}};
        return *(reinterpret_cast<RvdLogStreamId const*>(&checkpointStreamId));
    }

    // All internal checkpoint records use this GUID: {4D92BFD4-169D-4cd7-8229-9E9BA0108D67}
    // as their LogStreamId.
    static RvdLogStreamType const&    CheckpointStreamType()
    {
        static GUID const checkpointStreamType = {0x4d92bfd4, 0x169d, 0x4cd7, {0x82, 0x29, 0x9e, 0x9b, 0xa0, 0x10, 0x8d, 0x67}};
        return *(reinterpret_cast<RvdLogStreamType const*>(&checkpointStreamType));
    }

    // Log records of this stream are valid but do not contain any real data.
    // They are used to fill holes in the log. GUID: {522A951E-C4CF-409b-A952-6D5C98660476}
    static RvdLogStreamId const&    DummyStreamId()
    {
        static GUID const dummyStreamId = {0x522a951e, 0xc4cf, 0x409b, {0xa9, 0x52, 0x6d, 0x5c, 0x98, 0x66, 0x4, 0x76}};
        return *(reinterpret_cast<RvdLogStreamId const*>(&dummyStreamId));
    }

    // Dummy stream type. GUID: {EBB58FCA-9D68-498b-A8C9-96C125B7A27}
    static __inline RvdLogStreamType const&    DummyStreamType()
    {
        static GUID const dummyStreamType = {0xebb58fca, 0x9d68, 0x498b, {0xa8, 0xc9, 0x96, 0xc1, 0x25, 0xb7, 0xa2, 0x71}};
        return *(reinterpret_cast<RvdLogStreamType const*>(&dummyStreamType));
    }

#if defined(PLATFORM_UNIX)
    //
    // RvdOnDiskLog uses the diskId and logId as keys for identifying the logs. As Linux does not have a
    // concept of a diskId, we define a hardcoded disk id used on linux platform.    
    //
    static __inline GUID const&    HardCodedVolumeGuid()
    {
        // {3D88CD76-DE60-43cc-A6DE-D45FFA85D728}
        static const GUID hardcodedVolumeGuid = 
            { 0x3d88cd76, 0xde60, 0x43cc, { 0xa6, 0xde, 0xd4, 0x5f, 0xfa, 0x85, 0xd7, 0x28 } };
        
        return hardcodedVolumeGuid;
    }
#endif

private:
    RvdDiskLogConstants();      // Disallow construction

public:
    //** Common utility functions

    static NTSTATUS
    BuildFullyQualifiedLogName(__in const KGuid& DiskId, __in const RvdLogId& LogId, __out KWString& FullyQualifiedLogName)
    // Build up _FullyQualifiedLogName from DiskId and LogId
    {
        FullyQualifiedLogName = L"";
        NTSTATUS status = FullyQualifiedLogName.Status();

        if (NT_SUCCESS(status))
        {
#if !defined(PLATFORM_UNIX)    
            status = KVolumeNamespace::CreateFullyQualifiedRootDirectoryName(
                DiskId,
                FullyQualifiedLogName);
#else
            FullyQualifiedLogName = L"";
            status = FullyQualifiedLogName.Status();
#endif

            if (NT_SUCCESS(status))
            {
                FullyQualifiedLogName += &DirectoryName();
                status = FullyQualifiedLogName.Status();
                if (NT_SUCCESS(status))
                {
                    FullyQualifiedLogName += &NamePrefix();
                    status = FullyQualifiedLogName.Status();
                    if (NT_SUCCESS(status))
                    {
                        FullyQualifiedLogName += LogId.Get();
                        status = FullyQualifiedLogName.Status();
                        if (NT_SUCCESS(status))
                        {
                            FullyQualifiedLogName += &NameExtension();
                            status = FullyQualifiedLogName.Status();
                        }
                    }
                }
            }
        }

        return status;
    }
    
    static ULONG __inline
    ToNumberOfBlocks(__in ULONG SizeInBytes)
    {
        return (SizeInBytes + (BlockSize - 1)) / BlockSize;
    }

    static ULONG __inline
    RoundUpToBlockBoundary(__in ULONG SizeInBytes)
    {
        return ToNumberOfBlocks(SizeInBytes) * BlockSize;
    }

    static BOOLEAN __inline
    IsValidLogFileName(__in KStringView& NameInQuestion, __out KGuid& LogFileId)
    {
        LogFileId.SetToNull();
        if ((NameInQuestion.Length() != LogFileNameLength)                                                  ||
            (NameInQuestion.SubString(0, NamePrefixSize).CompareNoCase(KStringView(&NamePrefix())) != 0)    ||
            (NameInQuestion.SubString(LogFileNameExtensionOffset, NameExtensionSize)
                .CompareNoCase(KStringView(&NameExtension())) != 0)                                         ||
            (!NameInQuestion.SubString(LogFileNameGuidOffset, LogFileNameGuidStrLength).ToGUID(LogFileId)))
        {
            return FALSE;
        }

        return TRUE;
    }

    static BOOLEAN __inline
    IsReservedDirectory(__in const KStringView& PathInQuestion)
    {
        KStringView             reservedDir((WCHAR*)&RvdDiskLogConstants::DirectoryName());
        return (reservedDir
                    .CompareNoCase(PathInQuestion
                        .LeftString(__min(reservedDir.Length(), (PathInQuestion.Length() / sizeof(WCHAR))))) == 0);
    }
};

//* Configuration class describing an RVD Log's geometric "constants"
class RvdLogOnDiskConfig
{
protected:
    static ULONG const      _DefaultMaxRecordSize = 16 * 1024 * 1024;

private:
    static LONGLONG const   _MinMaxRecordsPerFile = 16;
    static ULONG const      _DefaultMaxNumberOfStreams = 1024;
    static ULONG const      _DefaultMaxQueuedWriteDepth = _DefaultMaxRecordSize;
    static ULONG const      _DefaultMaxRecordsPerCheckpointInterval = 4;
    static ULONG const      _DefaultMaxRecordsPerStreamCheckpointInterval = 8;
    static ULONG const      _MinMaxRecordSize = RvdDiskLogConstants::BlockSize * 4;
    static ULONG const      _DefaultMaxRecordMetadataFactor = 4;        // 1/4 MaxRecordSize

public:
    __inline RvdLogOnDiskConfig();
    __inline void Set(__in RvdLogOnDiskConfig& Other);
    __inline RvdLogOnDiskConfig(__in RvdLogOnDiskConfig& Other);
    __inline ULONG GetMaxNumberOfStreams();
    __inline ULONG GetMaxRecordSize();
    __inline ULONG GetMaxQueuedWriteDepth();
    __inline BOOLEAN Is4KBoundary(__in ULONG ValueInQuestion);
    __inline ULONGLONG GetMinFileSize();
    __inline ULONGLONG GetCheckpointInterval();
    __inline ULONGLONG GetStreamCheckpointInterval();
    __inline BOOLEAN IsValidConfig();
    __inline BOOLEAN IsValidConfig(__in ULONGLONG FileSize);
    __inline ULONG GetMaxMetadataSize();
    __inline ULONG GetMaxIoBufferSize();
    __inline ULONG GetMaxUserMetadataSize();
    __inline ULONGLONG GetMinFreeSpace();
    __inline bool operator==(__in RvdLogOnDiskConfig& Other);
    __inline bool operator!=(__in RvdLogOnDiskConfig& Other);

public:
    static ULONG const      DefaultMaxRecordSize = _DefaultMaxRecordSize;
    static ULONG const      DefaultMaxNumberOfStreams = _DefaultMaxNumberOfStreams;

protected:
    ULONG   _MaxNumberOfStreams;
    ULONG   _MaxRecordSize;
    ULONG   _MaxQueuedWriteDepth;

private:
    ULONG   _Filler0;
};

class RvdLogConfig : public KObject<RvdLogConfig>, public RvdLogOnDiskConfig
{
public:
    __inline RvdLogConfig();
    __inline RvdLogConfig(__in RvdLogOnDiskConfig& OnDiskConfig);
    __inline RvdLogConfig(__in ULONG Streams, __in ULONG MaxRecordSize, __in ULONG MaxQueuedWriteDepth);
    __inline RvdLogConfig(__in ULONG Streams, __in ULONG MaxRecordSize = _DefaultMaxRecordSize);
    __inline ULONG const MaxUserRecordSize();

private:
    __inline static void Init(RvdLogConfig& This, ULONG Streams, ULONG MaxRecordSize, ULONG MaxQueuedWriteDepth);
};

//** Log File Structure Definitions
class RvdLogLsn : public KStrongType<RvdLogLsn, LONGLONG volatile>
{
    K_STRONG_TYPE(RvdLogLsn, LONGLONG volatile);

public:
    RvdLogLsn() : KStrongType<RvdLogLsn, LONGLONG volatile>(RvdLogLsn::Null()) {}

    // All valid log LSNs must be between Null and Max, exclusively.
    static RvdLogLsn const&
    Null()
    {
        static RvdLogLsn const nullLsn((LONGLONG)-1);
        return nullLsn;
    }

    static RvdLogLsn const&
    Min()
    {
        static RvdLogLsn const minLsn((LONGLONG)0);
        return minLsn;
    }

    static RvdLogLsn const&
    Max()
    {
        static RvdLogLsn const maxLsn(_I64_MAX);
        return maxLsn;
    }

    BOOLEAN
    IsNull()
    {
        return (*this) == Null();
    }

    __inline BOOLEAN
    SetIfLarger(__in LONGLONG newValue)
    {
        LONGLONG volatile   oldValue;
        while (newValue > (oldValue = _Value))
        {
            if (InterlockedCompareExchange64(&_Value, newValue, oldValue) == oldValue)
            {
                return TRUE;
            }
        }

        return FALSE;
    }

    __inline RvdLogLsn
    operator+=(__in const LONGLONG Right)
    {
        KAssert(Right > Null().Get());
        if (Right != 0)
        {
            while (!SetIfLarger(_Value + Right)) {};
        }
        return *this;
    }

    __inline RvdLogLsn
    operator+(__in const LONGLONG Right) const
    {
        return RvdLogLsn(_Value + Right);
    }

    __inline BOOLEAN
    SetIfLarger(__in RvdLogLsn& NewSuggestedLsn)
    {
        return SetIfLarger(NewSuggestedLsn.Get());
    }
};

// Valid record LSNs in a log stream are in the range of [LowestLsn, HighestLsn], inclusively.
// NextLsn is HighestLsn plus the size of the latest record in the stream.
//
// If there are no records in the stream, all three LSNs are identical,
// but may or may not be RvdLogLsn::Null().
//
// If there is only one record in the stream, LowestLsn is the same as HighestLsn,
// but smaller than NextLsn.
struct RvdLogStreamInformation
{
    RvdLogStreamId      LogStreamId;
    RvdLogStreamType    LogStreamType;
    RvdLogLsn           LowestLsn;      // Lower closed bound of the log stream LSN
    RvdLogLsn           HighestLsn;     // Higher closed bound of the log stream LSN
    RvdLogLsn           NextLsn;        // LSN immediately after the latest record

    BOOLEAN
    IsEmptyStream()
    {
        return HighestLsn == NextLsn;
    }

    RvdLogStreamInformation() {}
};

//** Common on-disk log record header.
//
// Each log record begins with this header, followed by IO buffer. The header is
// padded to the closest RvdDiskLogConstants::BlockSize boundary.
struct RvdLogRecordHeader
{
    RvdLogRecordHeader() {};

    union
    {
        #pragma warning(push)
        #pragma warning(disable:4201)   // C4201: nonstandard extension used : nameless struct/union
        struct
        {
            // LSN of this log record.
            RvdLogLsn   Lsn;

            // Any LSN less than HighestCompletedLsn have completed their log write
            // at the time of generating this log record header.
            RvdLogLsn   HighestCompletedLsn;

            // Lsn of the last internal checkpoint record.
            RvdLogLsn   LastCheckPointLsn;

            // Lsn of the previous log record in the same log stream.
            RvdLogLsn   PrevLsnInLogStream;
        };
        #pragma warning(pop)

        // NOTE: The purpose of the union is to support a two-phase chksum approach;
        //       first the entire header and meta-data is chksummed starting with LogId
        //       and then once Lsn assignments have occured the last chunck of the
        //       RvdLogRecordHeader (LsnChksumBlock) is then included in ThisBlockChecksum.
        UCHAR           LsnChksumBlock[4 * sizeof(RvdLogLsn)];
    };

    // Unique id of this log.
    // It is given when a log is created.
    RvdLogId            LogId;

    // Version of the on-disk log record data structure.
    USHORT              MajorVersion;
    USHORT              MinorVersion;

    ULONG               Reserved1;              // Pad to 8-byte boundary

    // Signature unique to each physical log.
    // It is created at log creation time.
    // All valid log records in the log must have this signature.
    ULONG               LogSignature[RvdDiskLogConstants::SignatureULongs];
    static const ULONG  LogSignatureSize = RvdDiskLogConstants::SignatureULongs * sizeof(ULONG);

    // Unique id of this log stream.
    RvdLogStreamId      LogStreamId;

    // Type of this log stream.
    RvdLogStreamType    LogStreamType;

    // On-disk size of this header in bytes, including MetaData.
    // ThisHeaderSize is always multiples of RvdDiskLogConstants::BlockSize.
    ULONG               ThisHeaderSize;

    // Size of the IO buffer contained in this log record.
    // IoBufferSize is always multiples of RvdDiskLogConstants::BlockSize.
    ULONG               IoBufferSize;

    // Size of MetaData in bytes that follow the RvdLogRecordHeader.
    ULONG               MetaDataSize;

    ULONG               Reserved2;              // Pad to 8-byte boundary

    // Checksum of this header, including MetaData. This value is formed (and validated) in two steps.
    // First the overall header (MetaDataSize + sizeof(RvdLogRecordHeader) - sizeof(LsnChksumBlock). Then
    // second, LsnChksumBlock is checkedsummed with the result from the phase.
    ULONGLONG           ThisBlockChecksum;

    //** Utility Functions
    VOID
    Initialize(
        __in RvdLogId const&         LogId1,
        __in_ecount(LogSignatureSize) ULONG* const            LogSignature1,
        __in RvdLogStreamId const&   LogStreamId1,
        __in RvdLogStreamType const& LogStreamType1,
        __in ULONG                   HeaderSize,
        __in ULONG                   IoBufferSize1,
        __in ULONG                   MetaDataSize1)
    {
        this->LogId = LogId1;
        KMemCpySafe(&this->LogSignature[0], sizeof(this->LogSignature), LogSignature1, LogSignatureSize);
        this->LogStreamId = LogStreamId1;
        this->LogStreamType = LogStreamType1;
        this->ThisHeaderSize = HeaderSize;
        this->IoBufferSize = IoBufferSize1;
        this->MetaDataSize = MetaDataSize1;
        this->MajorVersion = RvdDiskLogConstants::MajorFormatVersion;
        this->MinorVersion = RvdDiskLogConstants::MinorFormatVersion;
        this->ThisBlockChecksum = 0;
    }

    VOID
    InitializeLsnProperties(
        __in RvdLogLsn const    Lsn1,
        __in RvdLogLsn const    HighestCompletedLsn1,
        __in RvdLogLsn const    LastCheckPointLsn1,
        __in RvdLogLsn const    PrevLsnInLogStream1)
    {
        KAssert(Lsn1 > HighestCompletedLsn1);
        KAssert(Lsn1 > PrevLsnInLogStream1);
        KAssert(Lsn1 > LastCheckPointLsn1);

        this->Lsn = Lsn1;
        this->HighestCompletedLsn = HighestCompletedLsn1;
        this->LastCheckPointLsn = LastCheckPointLsn1;
        this->PrevLsnInLogStream = PrevLsnInLogStream1;
    }

    static ULONGLONG __inline
    ToFileAddress(__in RvdLogLsn Lsn, __in ULONGLONG LogFileLsnSpace)
    // Convert the given Lsn with in the provided LogFileLsnSpace into the mapped file offest.
    //
    {
        KAssert((Lsn >= RvdLogLsn::Min()) && (Lsn < RvdLogLsn::Max()));
        return (Lsn.Get() % LogFileLsnSpace) + RvdDiskLogConstants::BlockSize; // Allow for log header
    }

    static ULONGLONG __inline
    ToFileAddress(
        __in RvdLogLsn Lsn,
        __in ULONGLONG LogFileLsnSpace,
        __in ULONG const SizeToMap,
        __out ULONG& FirstSegmentSize)
    //
    // Map an LSN and length (from that LSN) -> file offset and length of a first segment
    //
    // Parameters:
    //      Lsn             - Starting location in LSN-space
    //      LogFileLsnSpace - Size of the LSN-space to map to
    //      SizeToMap       - Length of the LSN-space
    //
    // Returns:              - File Offset of a first segment of the LSN-space [Lsn, Lsn + SizeToMap)
    //      FirstSegmentSize - size of the returned File Offset of the first segment of the LSN-space [Lsn, Lsn + SizeToMap).
    //                         NOTE: if this value is < SizeToMap; then the file space is wrapped
    //
    //
    {
        KAssert((Lsn >= RvdLogLsn::Min()) && (Lsn < RvdLogLsn::Max()));
        KAssert(SizeToMap <= LogFileLsnSpace);

        ULONGLONG lsnSpaceOffset = Lsn.Get() % LogFileLsnSpace;
        ULONGLONG longFirstSegmentSize = ((lsnSpaceOffset + SizeToMap) > LogFileLsnSpace) ? (LogFileLsnSpace - lsnSpaceOffset)
                                                                                           : SizeToMap;
        KInvariant(longFirstSegmentSize <= MAXULONG);
        FirstSegmentSize = (ULONG)longFirstSegmentSize;
        return lsnSpaceOffset + RvdDiskLogConstants::BlockSize; // Allow for log header
    }

    static ULONGLONG __inline
    ToFileAddressEx(
        __in RvdLogLsn Lsn,
        __in ULONGLONG LogFileLsnSpace,
        __in ULONGLONG const SizeToMap,
        __out ULONGLONG& FirstSegmentSize)
    //
    // Map an LSN and length (from that LSN) -> file offset and length of a first segment
    //
    // Parameters:
    //      Lsn             - Starting location in LSN-space
    //      LogFileLsnSpace - Size of the LSN-space to map to
    //      SizeToMap       - Length of the LSN-space
    //
    // Returns:              - File Offset of a first segment of the LSN-space [Lsn, Lsn + SizeToMap)
    //      FirstSegmentSize - size of the returned File Offset of the first segment of the LSN-space [Lsn, Lsn + SizeToMap).
    //                         NOTE: if this value is < SizeToMap; then the file space is wrapped
    //
    //
    {
        KAssert((Lsn >= RvdLogLsn::Min()) && (Lsn < RvdLogLsn::Max()));
        KAssert(SizeToMap <= LogFileLsnSpace);

        ULONGLONG lsnSpaceOffset = Lsn.Get() % LogFileLsnSpace;
        FirstSegmentSize = ((lsnSpaceOffset + SizeToMap) > LogFileLsnSpace) ? (LogFileLsnSpace - lsnSpaceOffset)
                                                                            : SizeToMap;
        return lsnSpaceOffset + RvdDiskLogConstants::BlockSize; // Allow for log header
    }

    BOOLEAN
    IsBasicallyValid(
        __in RvdLogConfig& Config,
        __in ULONGLONG const CurrentFileOffset,
        __in RvdLogId& LogId1,
        __in ULONG (&LogSignature1)[RvdDiskLogConstants::SignatureULongs],
        __in ULONGLONG LogFileLsnSpace,
        __in_opt RvdLogLsn* const LsnToValidate = nullptr)
    {
        if ((this->MajorVersion != RvdDiskLogConstants::MajorFormatVersion)                                     ||
            (this->MinorVersion != RvdDiskLogConstants::MinorFormatVersion)                                     ||
            (this->LogId != LogId1)                                                                              ||
            (RtlCompareMemory(&this->LogSignature[0], &LogSignature1[0], RvdLogRecordHeader::LogSignatureSize)
                        !=  RvdLogRecordHeader::LogSignatureSize)                                               ||
            (RvdLogRecordHeader::ToFileAddress(this->Lsn, LogFileLsnSpace) != CurrentFileOffset)                ||
            (this->PrevLsnInLogStream >= this->Lsn)                                                             ||
            (this->LastCheckPointLsn >= this->Lsn)                                                              ||
            (this->HighestCompletedLsn >= this->Lsn)                                                            ||
            ((this->ThisHeaderSize % RvdDiskLogConstants::BlockSize) != 0)                                      ||
            ((this->IoBufferSize % RvdDiskLogConstants::BlockSize) != 0)                                        ||
            (this->MetaDataSize > this->ThisHeaderSize)                                                         ||
            ((this->ThisHeaderSize + this->IoBufferSize) > Config.GetMaxRecordSize())                           ||
            ((LsnToValidate != nullptr) && ((*LsnToValidate) != this->Lsn)))
        {
            return FALSE;
        }

        return TRUE;
    }

    NTSTATUS
    ChecksumIsValid(__in ULONG BufferSize)
    {
        ULONG       sizeToCheckSum = sizeof(RvdLogRecordHeader) + this->MetaDataSize;
        ULONGLONG   savedChksum = this->ThisBlockChecksum;

        if (sizeToCheckSum > BufferSize)
        {
            return K_STATUS_LOG_STRUCTURE_FAULT;
        }

        this->ThisBlockChecksum = 0;
        ULONGLONG   partialChksum = KChecksum::Crc64(
            ((UCHAR*)this) + sizeof(this->LsnChksumBlock),
            sizeToCheckSum - sizeof(this->LsnChksumBlock));
        this->ThisBlockChecksum = savedChksum;

        BOOLEAN b;
        b = (KChecksum::Crc64(this, sizeof(this->LsnChksumBlock), partialChksum) == savedChksum);
        return (b ? STATUS_SUCCESS : STATUS_CRC_ERROR);
    }
};


//** On-disk record header structure common to both all records in a user log stream
struct RvdLogUserStreamRecordHeader : public RvdLogRecordHeader
{
    enum                    Type : ULONG {UserRecord = 0, CheckPointRecord = 1};
    Type                    RecordType;
    ULONG                   Reserved1;

    // The current log stream truncation point.
    RvdLogAsn               TruncationPoint;

    // If this record is a copy of another previous record,
    // this field is the LSN of that previous record.
    // This is used for copy-forward.
    RvdLogLsn               CopyRecordLsn;

    //** Utility Functions
    VOID
    Initialize(
        __in Type                    RecordType1,
        __in RvdLogId const&         LogId1,
        __in_ecount(LogSignatureSize) ULONG* const            LogSignature1,
        __in RvdLogStreamId const&   LogStreamId1,
        __in RvdLogStreamType const& LogStreamType1,
        __in ULONG                   HeaderSize1,
        __in ULONG                   IoBufferSize1,
        __in ULONG                   MetaDataSize1,
        __in RvdLogAsn               TruncationPoint1)
    {
        RvdLogRecordHeader::Initialize(
            LogId1,
            LogSignature1,
            LogStreamId1,
            LogStreamType1,
            HeaderSize1,
            IoBufferSize1,
            MetaDataSize1);

        this->RecordType = RecordType1;
        this->CopyRecordLsn = RvdLogLsn::Null();
        this->TruncationPoint = TruncationPoint1;
        this->Reserved1 = 0;
    }
};

//** Common on-disk header structure of all user defined stream records.
//      RecordType == UserRecord
#pragma warning(push)
#pragma warning(disable : 4200)

struct RvdLogUserRecordHeader : public RvdLogUserStreamRecordHeader
{
    // Asn of this log record.
    RvdLogAsn           Asn;

    // Version of this log record.
    ULONGLONG           AsnVersion;

    UCHAR               UserMetaData[];

    //** Supporting Utility Functions
    ULONG __inline
    GetSizeOfUserMetaData()
    {
        KAssert(MetaDataSize >= (sizeof(RvdLogUserRecordHeader) - sizeof(RvdLogRecordHeader)));
        ULONG       result = MetaDataSize - (sizeof(RvdLogUserRecordHeader) - sizeof(RvdLogRecordHeader));
        return result;
    }

    ULONG static __inline
    ComputeMetaDataSize(__in ULONG ForUserMetaDataSize)
    {
        ULONG       result = ForUserMetaDataSize + (sizeof(RvdLogUserRecordHeader) - sizeof(RvdLogRecordHeader));
        return result;
    }

    static ULONG
    ComputeSizeOnDisk(__in ULONG MetaDataSize)
    //
    //  Compute size of header (including metadata) size on disk - rounded to next BlockSize
    //
    //  Returns: Size in bytes of the headers + metadata - always multiple of BlockSize
    {
        return RvdDiskLogConstants::RoundUpToBlockBoundary(MetaDataSize + sizeof(RvdLogUserRecordHeader));
    }

    static ULONG
    ComputePaddingSize(__in ULONG MetaDataSize)
    //
    //  Compute size of header padding at the end of the headers + metadata
    //
    //  Returns: Size in bytes of padding at the end of the header + metadata
    {
        return ComputeSizeOnDisk(MetaDataSize) - (MetaDataSize + sizeof(RvdLogUserRecordHeader));
    }
};
static_assert(
    sizeof(RvdLogUserRecordHeader) <= RvdDiskLogConstants::BlockSize,
    "RvdLogUserRecordHeader too large to fit in first block");

#pragma warning(pop)

// On-disk sub-structure array element for RvdLogCheckpointStreamRecords.
// It contains the mapping between <Asn, Version> and Lsn.
struct AsnLsnMappingEntry
{
    RvdLogAsn       RecordAsn;
    ULONGLONG       RecordAsnVersion;
    RvdLogLsn       RecordLsn;
    ULONG           IoBufferSizeHint;   // Useful when fetching the operation data from log disk.
    ULONG           Reserved1;

    AsnLsnMappingEntry() {};

    AsnLsnMappingEntry(__in RvdLogAsn Asn, __in ULONGLONG Version, __in RvdLogLsn Lsn, __in ULONG IoBufferSizeHint)
        :   RecordAsn(Asn),
            RecordAsnVersion(Version),
            RecordLsn(Lsn),
            IoBufferSizeHint(IoBufferSizeHint)
    {
    }
};

// On-disk sub-structure array element for RvdLogCheckpointStreamRecords.
//
// This structure is used to track the LSN of a record in a user log stream.
struct RvdLogLsnEntry
{
    RvdLogLsn       Lsn;                            // Lsn
    ULONG           HeaderAndMetaDataSize;
    ULONG           IoBufferSize;
};

//** On-disk structure of internal checkpoint records carried in a user stream.
//      RecordType == CheckPointRecord
#pragma warning(push)
#pragma warning(disable : 4200)

struct RvdLogUserStreamCheckPointRecordHeader : public RvdLogUserStreamRecordHeader
{
    ULONG                           NumberOfSegments;
    ULONG                           SegmentNumber;                  // 0 - (NumberOfSegments - 1)
    ULONG                           NumberOfAsnMappingEntries;
    ULONG                           NumberOfLsnEntries;
    AsnLsnMappingEntry              AsnLsnMappingArray[0];

    AsnLsnMappingEntry* const
    GetAsnLsnMappingArray()
    {
        return (&AsnLsnMappingArray[0]);
    }

    // Entries in this list are ordered by their LSNs.
    // They can be accessed like array elements via zero-based indexes.
    // The assumption is that the number of log records is small,
    // i.e., max ~O(1000) so we can afford to track them in memory.
    //RvdLogLsnEntry                LsnEntryArray[NumberOfLsnEntries];

    RvdLogLsnEntry* const
    GetLsnEntryArray()
    {
        return (RvdLogLsnEntry*)(&AsnLsnMappingArray[NumberOfAsnMappingEntries]);
    }

    //** Supporting Utility Functions
    static ULONG
    ComputeMaxSizeOnDisk(__in RvdLogConfig& Config)
    {
        return Config.GetMaxMetadataSize();
    }

    //static ULONG const MaxSinglePartRecordSize = RvdDiskLogConstants::MaxMetadataSize;
    static ULONG __inline
    GetMaxSinglePartRecordSize(__in RvdLogConfig& Config)
    {
        return Config.GetMaxMetadataSize();
    };

    //static ULONG const MaxMultiPartRecordSize = MaxSinglePartRecordSize / 2;
    static ULONG __inline
    GetMaxMultiPartRecordSize(__in RvdLogConfig& Config)
    {
        return Config.GetMaxMetadataSize() / 2;
    };

    static ULONG
    //
    //  Compute size of record size on disk - rounded to next BlockSize
    //
    //  Returns:
    //          Size in bytes of all records (headers + metadata) - always multiple of BlockSize
    //          NumberOfRecords - Number of records needed to hold contents
    //
    ComputeSizeOnDisk(
        __in RvdLogConfig& Config,
        __in ULONG NumberOfAsnMappingEntries,
        __in ULONG NumberOfLsnEntries,
        __in ULONG& NumberOfRecords)
    {
        ULONG recordSize = (sizeof(RvdLogUserStreamCheckPointRecordHeader)) +
                           (sizeof(AsnLsnMappingEntry) * NumberOfAsnMappingEntries) +
                           (sizeof(RvdLogLsnEntry) * NumberOfLsnEntries);

        if (recordSize <= GetMaxSinglePartRecordSize(Config))
        {
            NumberOfRecords = 1;
            return RvdDiskLogConstants::RoundUpToBlockBoundary(recordSize);
        }

        // A single log record can't contain all the indicated entries; must calculate a
        // Multi-segment record layout: a series of MaxMultiPartRecordSize (max size) records.
        ULONG const perRecSpace = GetMaxMultiPartRecordSize(Config) - sizeof(RvdLogUserStreamCheckPointRecordHeader);
        ULONG const maxAsnEntriesPerRec = perRecSpace / sizeof(AsnLsnMappingEntry);
        ULONG const maxLsnEntriesPerRec = perRecSpace / sizeof(RvdLogLsnEntry);

        ULONG const fullRecsForAsnEntries = NumberOfAsnMappingEntries / maxAsnEntriesPerRec;
        ULONG const numberOfAsnsInPartialRec = NumberOfAsnMappingEntries % maxAsnEntriesPerRec;
        ULONG const freeSpaceInPartialRecAfterAsns = perRecSpace - (numberOfAsnsInPartialRec * sizeof(AsnLsnMappingEntry));
        ULONG const numberOfLsnsInPartialRec = freeSpaceInPartialRecAfterAsns / sizeof(RvdLogLsnEntry);
        ULONG const numberOfLsnsBeyondPartialRec = NumberOfLsnEntries - numberOfLsnsInPartialRec;
        ULONG const fullRecsForRemainingLsnEntries = numberOfLsnsBeyondPartialRec / maxLsnEntriesPerRec;
        ULONG const numberOfLsnEntriesInLastRec = NumberOfLsnEntries -
                                                  numberOfLsnsInPartialRec -
                                                  (fullRecsForRemainingLsnEntries * maxLsnEntriesPerRec);

        ULONG       totalRecords    = fullRecsForAsnEntries + fullRecsForRemainingLsnEntries;
        ULONGLONG   totalSize       = totalRecords * GetMaxMultiPartRecordSize(Config);

        if (numberOfAsnsInPartialRec + numberOfLsnsInPartialRec > 0)
        {
            totalSize += RvdDiskLogConstants::RoundUpToBlockBoundary(
                                (numberOfAsnsInPartialRec * sizeof(AsnLsnMappingEntry)) +
                                (numberOfLsnsInPartialRec * sizeof(RvdLogLsnEntry)) +
                                sizeof(RvdLogUserStreamCheckPointRecordHeader));
            totalRecords++;
        }

        if (numberOfLsnEntriesInLastRec > 0)
        {
            totalSize += RvdDiskLogConstants::RoundUpToBlockBoundary(
                                (numberOfLsnEntriesInLastRec * sizeof(RvdLogLsnEntry)) +
                                sizeof(RvdLogUserStreamCheckPointRecordHeader));
            totalRecords++;
        }

        KAssert((totalSize % RvdDiskLogConstants::BlockSize) == 0);
        KInvariant(totalSize <= MAXULONG);

        NumberOfRecords = totalRecords;
        return (ULONG)totalSize;
    }

    static ULONG
    ComputeRecordPaddingSize(__in RvdLogConfig& Config, __in ULONG NumberOfAsnMappingEntries, __in ULONG NumberOfLsnEntries)
    //
    //  Compute size of header padding at the end of the headers
    //
    //  Returns: Size in bytes of padding at the end of the header
    {
        ULONG recordSize = (sizeof(RvdLogUserStreamCheckPointRecordHeader)) +
                           (sizeof(AsnLsnMappingEntry) * NumberOfAsnMappingEntries) +
                           (sizeof(RvdLogLsnEntry) * NumberOfLsnEntries);

        ULONG records = 0;

        recordSize = ComputeSizeOnDisk(Config, NumberOfAsnMappingEntries, NumberOfLsnEntries, records) - recordSize;
        KInvariant(records == 1);

        return recordSize;
    }
};
#pragma warning(pop)


//** On-disk and in-memory record structure for checkpoints protecting the
//   physical log file as a whole. These records are stored in the dedicated
//   checkpoint stream:
//      LogStreamId == RvdDiskLogConstants::CheckpointStreamId
#pragma warning(push)
#pragma warning(disable : 4200)

struct RvdLogPhysicalCheckpointRecord : public RvdLogRecordHeader
{
    ULONG                   NumberOfLogStreams;
    ULONG                   Reserved1;
    RvdLogStreamInformation LogStreamArray[0];

    //** Supporting Utility Functions
    static ULONG
    ComputeSizeOnDisk(__in ULONG NumberOfLogStreams)
    //
    //  Compute size of header size on disk - rounded to next BlockSize
    //
    //  Returns: Size in bytes of the headers + metadata - always multiple of BlockSize
    {
        ULONG recordSize = (sizeof(RvdLogPhysicalCheckpointRecord)) +
                           (sizeof(RvdLogStreamInformation) * NumberOfLogStreams);

        return RvdDiskLogConstants::RoundUpToBlockBoundary(recordSize);
    }

    static ULONG
    ComputePaddingSize(__in ULONG NumberOfLogStreams)
    //
    //  Compute size of header padding at the end of the headers
    //
    //  Returns: Size in bytes of padding at the end of the header
    {
        ULONG recordSize = (sizeof(RvdLogPhysicalCheckpointRecord)) +
                           (sizeof(RvdLogStreamInformation) * NumberOfLogStreams);

        return ComputeSizeOnDisk(NumberOfLogStreams) - recordSize;
    }

    static ULONG
    ComputeMaxSizeOnDisk(__in RvdLogConfig& Config)
    {
        return ComputeSizeOnDisk(Config.GetMaxNumberOfStreams());
    }

    static ULONG
    ComputeMaxSizeOnDisk(__in RvdLogOnDiskConfig* Config)
    {
        return ComputeSizeOnDisk(Config->GetMaxNumberOfStreams());
    }

};
#pragma warning(pop)


//** On disk log file configuration structure all-inline implementation
__inline
RvdLogOnDiskConfig::RvdLogOnDiskConfig()
    :   _MaxNumberOfStreams(_DefaultMaxNumberOfStreams),
        _MaxRecordSize(_DefaultMaxRecordSize),
        _MaxQueuedWriteDepth(_DefaultMaxQueuedWriteDepth),
        _Filler0(0)
{
    KAssert(IsValidConfig());
}

__inline void
RvdLogOnDiskConfig::Set(__in RvdLogOnDiskConfig& Other)
{
    _MaxNumberOfStreams = Other._MaxNumberOfStreams;
    _MaxRecordSize = Other._MaxRecordSize;
    _MaxQueuedWriteDepth = Other._MaxQueuedWriteDepth;
    _Filler0 = Other._Filler0;
    KInvariant(IsValidConfig());
}

__inline
RvdLogOnDiskConfig::RvdLogOnDiskConfig(__in RvdLogOnDiskConfig& Other)
{
    Set(Other);
}

__inline BOOLEAN
RvdLogOnDiskConfig::Is4KBoundary(__in ULONG ValueInQuestion)
{
    return (ValueInQuestion & 0xFFF) == 0;
}

__inline BOOLEAN
RvdLogOnDiskConfig::IsValidConfig()
{
    // MaxQueuedWriteDepth guarentees that no more than that amount of stream data outstanding in write queues is pending
    // at any time so that the recovery logic can make this assumption when scanning backwards trying to find the last stable
    // (continous) region in LSN space. Thus no single record larger than MaxQueuedWriteDepth can be written.
    //
    // This condition also guarantees that if the log is full or wraps, there is at least one checkpoint log record.

    return  (_MaxRecordSize >= _MinMaxRecordSize) &&
        (_MaxRecordSize <= _MaxQueuedWriteDepth) &&
        Is4KBoundary(_MaxRecordSize) &&
        Is4KBoundary(_MaxQueuedWriteDepth) &&
        (GetCheckpointInterval() < GetMinFileSize()) &&
        (_MaxNumberOfStreams >= 2) &&
        (_Filler0 == 0) &&
        (GetCheckpointInterval() <= MAXLONGLONG) &&
        (GetStreamCheckpointInterval() <= MAXLONGLONG) &&
        (GetCheckpointInterval() >= _MaxQueuedWriteDepth) &&
        (GetStreamCheckpointInterval() >= _MaxQueuedWriteDepth) &&
        (RvdLogPhysicalCheckpointRecord::ComputeMaxSizeOnDisk(this) <= _MaxQueuedWriteDepth);
}

__inline BOOLEAN
RvdLogOnDiskConfig::IsValidConfig(__in ULONGLONG FileSize)
{
    return IsValidConfig() && (FileSize >= GetMinFileSize());
}

__inline ULONG
RvdLogOnDiskConfig::GetMaxNumberOfStreams()
{
    return _MaxNumberOfStreams;
}

__inline ULONG
RvdLogOnDiskConfig::GetMaxRecordSize()
{
    return _MaxRecordSize;
}

__inline ULONG
RvdLogOnDiskConfig::GetMaxQueuedWriteDepth()
{
    return _MaxQueuedWriteDepth;
}

__inline ULONGLONG
RvdLogOnDiskConfig::GetMinFileSize()
{
    return _MinMaxRecordsPerFile * _MaxRecordSize;
}

__inline ULONGLONG
RvdLogOnDiskConfig::GetCheckpointInterval()
{
    return (ULONGLONG)_MaxRecordSize * _DefaultMaxRecordsPerCheckpointInterval;
}

__inline ULONGLONG
RvdLogOnDiskConfig::GetStreamCheckpointInterval()
{
    return (ULONGLONG)_MaxRecordSize * _DefaultMaxRecordsPerStreamCheckpointInterval;
}

// Max allowed metadata and IO buffer size of a log record.
__inline ULONG
RvdLogOnDiskConfig::GetMaxMetadataSize()
{
    return _MaxRecordSize / _DefaultMaxRecordMetadataFactor;
}

__inline ULONG
RvdLogOnDiskConfig::GetMaxIoBufferSize()
{
    return _MaxRecordSize - RvdDiskLogConstants::BlockSize;
}

// Max allowed user record limits
__inline ULONG
RvdLogOnDiskConfig::GetMaxUserMetadataSize()
{
    return GetMaxMetadataSize() - RvdDiskLogConstants::BlockSize;
}

// Minimal reserved free space for copy-forward.
__inline ULONGLONG
RvdLogOnDiskConfig::GetMinFreeSpace()
{
    return (LONGLONG)_MaxRecordSize * 2;
}

__inline bool
RvdLogOnDiskConfig::operator==(__in RvdLogOnDiskConfig& Other)
{
    return  (_MaxRecordSize == Other._MaxRecordSize)                &&
            (_MaxQueuedWriteDepth == Other._MaxQueuedWriteDepth)    &&
            (_MaxNumberOfStreams == Other._MaxNumberOfStreams)      &&
            (_Filler0 == 0);
}

__inline bool
RvdLogOnDiskConfig::operator!=(__in RvdLogOnDiskConfig& Other)
{
    return !(*this == Other);
}


//** In-memory version of RvdLogOnDiskConfig - w/soft error detection and indication via KObject::Status()
__inline void
RvdLogConfig::Init(__in RvdLogConfig& This, __in ULONG Streams, __in ULONG MaxRecordSize, __in ULONG MaxQueuedWriteDepth)
{
    This._MaxNumberOfStreams = Streams;
    This._MaxRecordSize = MaxRecordSize;
    This._MaxQueuedWriteDepth = MaxQueuedWriteDepth;

    if (!This.IsValidConfig())
    {
        This.SetConstructorStatus(STATUS_INVALID_PARAMETER);
    }
}

_inline
RvdLogConfig::RvdLogConfig()
{
    KAssert(NT_SUCCESS(Status()));
}

_inline
RvdLogConfig::RvdLogConfig(__in RvdLogOnDiskConfig& OnDiskConfig)
{
    Init(
        *this,
        OnDiskConfig.GetMaxNumberOfStreams(),
        OnDiskConfig.GetMaxRecordSize(),
        OnDiskConfig.GetMaxQueuedWriteDepth());
}

_inline
RvdLogConfig::RvdLogConfig(__in ULONG Streams, __in ULONG MaxRecordSize, __in ULONG MaxQueuedWriteDepth)
{
    Init(*this, Streams, MaxRecordSize, MaxQueuedWriteDepth);
}

_inline
RvdLogConfig::RvdLogConfig(__in ULONG Streams, __in ULONG MaxRecordSize)
{
    Init(*this, Streams, MaxRecordSize, MaxRecordSize);
}

__inline ULONG const
RvdLogConfig::MaxUserRecordSize()
{
    // The maximum user record size must consider the possibility of both a physical
    // and a stream level checkpoint records
    return _MaxRecordSize                       // Overall physical record limit
            - RvdDiskLogConstants::BlockSize    // The worst case header overhead
            - RvdLogUserStreamCheckPointRecordHeader::ComputeMaxSizeOnDisk(*this)
            - RvdLogPhysicalCheckpointRecord::ComputeMaxSizeOnDisk(*this);
}


//** On-disk master block data structure. It is padded to
// one RvdDiskLogConstants::BlockSize on disk.
//
// There are two master blocks per log container.
// One is at the beginning of the log container file,
// another is at the end.
struct RvdLogMasterBlock
{
    // Version of the on-disk log record data structure.
    USHORT              MajorVersion;
    USHORT              MinorVersion;

    // Reserved.
    ULONG               Reserved1;

    // All logs must have this GUID.
    LogFormatSignature  LogFormatGuid;

    // Unique id of this log.
    // It is given when a log is created.
    RvdLogId            LogId;

    // Signature unique to each physical log. It is created at log creation time.
    // All valid log records in the log must have this signature.
    ULONG               LogSignature[RvdDiskLogConstants::SignatureULongs];

    // Size of the log container file in bytes.
    ULONGLONG           LogFileSize;

    // Location in logfile (should be either 0 or LogFileSize - MasterBlockSize)
    ULONGLONG           MasterBlockLocation;

    // Logfile configured geometry
    RvdLogOnDiskConfig  GeometryConfig;

    // LogFile creation path: empty if created (only) in the default location
    //      NOTE: If this directory does not contain a hardlink to the log file,
    //            the file will be deleted from the system by removing all hardlinks.
    WCHAR               CreationDirectory[RvdDiskLogConstants::MaxCreationPathSize];        // zero-term
    static const ULONG  CreationDirectoryMaxSize = RvdDiskLogConstants::MaxCreationPathSize * sizeof(WCHAR);

    // LogFile type is limited to 32 WCHAR
    WCHAR               LogType[RvdLogManager::AsyncCreateLog::MaxLogTypeLength];
    static const ULONG  LogTypeMaxSize = RvdLogManager::AsyncCreateLog::MaxLogTypeLength * sizeof(WCHAR);

    // Checksum of this header.
    ULONGLONG           ThisBlockChecksum;

    // Creation flags
    DWORD                CreationFlags;

    //** Utility functions
    NTSTATUS
    Validate(
        __in RvdLogId& LogId1,
        __in ULONGLONG LogFileSize1,
        __in ULONGLONG MasterBlockLocation1,
        __in_opt ULONG(* const LogSignature1)[RvdDiskLogConstants::SignatureULongs] = nullptr)
    {
        if ((this->MajorVersion != RvdDiskLogConstants::MajorFormatVersion) ||
            (this->MinorVersion != RvdDiskLogConstants::MinorFormatVersion) ||
            (this->LogFormatGuid != RvdDiskLogConstants::LogFormatGuid())   ||
            (this->LogId != LogId1)                                         ||
            (this->LogFileSize != LogFileSize1)                             ||
            (this->MasterBlockLocation != MasterBlockLocation1)             ||
            (!this->GeometryConfig.IsValidConfig(this->LogFileSize)))
        {
            return K_STATUS_LOG_STRUCTURE_FAULT;
        }

        if (LogSignature1 != nullptr)
        {
            if (RtlCompareMemory(LogSignature1, &this->LogSignature[0], sizeof(this->LogSignature)) != sizeof(this->LogSignature))
            {
                return K_STATUS_LOG_STRUCTURE_FAULT;
            }
        }

        ULONGLONG savedChksum = this->ThisBlockChecksum;
        this->ThisBlockChecksum = 0;
        BOOLEAN checksumWorked = (KChecksum::Crc64(this, RvdDiskLogConstants::MasterBlockSize, 0) == savedChksum);
        this->ThisBlockChecksum = savedChksum;

        return (checksumWorked) ? STATUS_SUCCESS : K_STATUS_LOG_STRUCTURE_FAULT;
    }
};

static_assert(
    sizeof(RvdLogMasterBlock) <= RvdDiskLogConstants::MasterBlockSize,
    "RvdLogMasterBlock too large");


//** Convenient typedefs and forward defs
class RvdLogStreamImp;
class LogStreamDescriptor;
class AsyncDeleteLogStreamContextImp;
typedef KSharedPtr<KWeakRef<RvdLogStreamImp>> StreamWRef;

//** Templated Helper for indexing KWeakRefTypes
template<typename KeyType, class WeakRefType>
struct KeyedWRef
{
    RvdLogId                            Key;
    KSharedPtr<KWeakRef<WeakRefType>>   WRef;

    KeyedWRef() {}
    ~KeyedWRef() {}

    KeyedWRef<KeyType, WeakRefType>& operator=(__in KeyedWRef<KeyType, WeakRefType>&& Source)
    {
        this->Key = Source.Key;
        this->WRef = Ktl::Move(Source.WRef);
        return *this;
    }

    KeyedWRef<KeyType, WeakRefType>& operator=(__in KeyedWRef<KeyType, WeakRefType>& Source)
    {
        this->Key = Source.Key;
        this->WRef = Source.WRef;
        return *this;
    }

private:
    KeyedWRef(KeyedWRef<KeyType, WeakRefType>& Source);
    KeyedWRef(KeyedWRef<KeyType, WeakRefType>&& Source);
};


//** RvdLogLsnEntryTracker
class RvdLogLsnEntryTracker
{
public:
    RvdLogLsnEntryTracker(__in KAllocator& Allocator);
    ~RvdLogLsnEntryTracker();

    VOID
    Clear();

    //* This routine adds a new LSN to the tracking array. The new LSN must be higher than all previous LSNs.
    //  Arguments:
    //        Lsn                     - Supplies a pointer to the LSN.
    //        HeaderSize              - Supplies size of the record header (incuding meta data).
    //        IoBufferSize            - Supplies size of the record IO buffer.
    //
    //  Return Value:
    //      STATUS_SUCCESS
    //      STATUS_INSUFFICIENT_RESOURCES
    //
    NTSTATUS
    AddHigherLsnRecord(__in RvdLogLsn Lsn, __in ULONG HeaderSize, __in ULONG IoBufferSize);

    // Returns: Highest Lsn Removed
    RvdLogLsn
    RemoveHighestLsnRecord();

    //* This routine adds a new LSN to the tracking array. The new LSN must be lower than all previous LSNs.
    //  Arguments:
    //        Lsn                     - Supplies a pointer to the LSN.
    //        HeaderSize              - Supplies size of the record header (incuding meta data).
    //        IoBufferSize            - Supplies size of the record IO buffer.
    //
    //  Return Value:
    //      STATUS_SUCCESS
    //      STATUS_INSUFFICIENT_RESOURCES
    //
    NTSTATUS
    AddLowerLsnRecord(__in RvdLogLsn Lsn, __in ULONG HeaderSize, __in ULONG IoBufferSize);

    //* This routine truncates all LSNs up to NewLowestLsn. Any LSN lower than NewLowestLsn is
    //  removed from the tracking array.
    //
    //  Arguments:
    //       NewLowestLsn    - Supplies a reference to the new lowest LSN.
    //
    VOID
    Truncate(__in RvdLogLsn NewLowestLsn);

    //* This routine returns the number of log records in the tracking array.
    //
    ULONG
    QueryNumberOfRecords();

    //* This routine gives caller the ability to read each LSN via an index, like an array.
    //  Valid indexes are from zero up to QueryNumberOfRecords() - 1. One can use this routine
    //  to access individual LSNs, or to enumerate all LSNs.
    //
    //  Arguments:
    //      LogRecordIndex  - Supplies index of the record in the tracking array.
    //      Lsn             - Returns LSN of the record.
    //      HeaderSize      - Optionally returns header (including meta data) size of the record.
    //      IoBufferSize    - Optionally returns IO buffer size of the record.
    //
    //  Return Value:
    //      STATUS_SUCCESS
    //      STATUS_INVALID_PARAMETER
    //
    NTSTATUS
    QueryRecord(
        __in ULONG              LogRecordIndex,
        __out RvdLogLsn&        Lsn,
        __out_opt ULONG* const  HeaderAndMetadataSize,
        __out_opt ULONG* const  IoBufferSize);

    //* This routine returns the total size of all log records in the tracking array.
    //
    LONGLONG
    QueryTotalSize();

    //* This routine returns a copy of all log record LSN entries. It is more efficient than
    //  enumerating all records and copying them one by one.
    //
    //  Arguments:
    //      NumberOfEntries - On input, supplies the number of elements in LsnArray.
    //                        On output, returns the number of LSN entries copied
    //                        into LsnArray.
    //      LsnArray        - Returns the LSN entries. Enteries are returned in increasing
    //                        LSN order.
    //
    //  Return Value:
    //      STATUS_SUCCESS
    //      STATUS_INVALID_PARAMETER    - NumberOfEntries = # entries in table
    //
    NTSTATUS
    GetAllRecordLsns(
        __inout ULONG* const                            NumberOfEntries,
        __out_ecount(*NumberOfEntries) RvdLogLsnEntry*  ResultingLsnEntries);

    //* This routine returns a copy of all log record LSN entries into a supplied KIoBuffer; extending
    //  that KIoBuffer as needed.
    //
    //  Arguments:
    //      SegmentSize     - Size that limits the number of entries before PreableSize bytes are skipped.
    //      PreambleSize    - Number of bytes to skip at the front of each segment (except for the 1st).
    //      IoBuffer        - KIoBuffer[IoBufferOffset on entry] == RvdLogLsnEntry[NumberOfEntries] on
    //                        return. If multiple segments are generated (NumberOfSegments > 1), 1)  end of
    //                        segments are padded as needed such that a returned entry is not split across
    //                        4k page boundary AND PreambleSize space is skipped at the start of each segment
    //                        after the first.
    //
    //      IoBufferOffset      - Logical starting into IoBuffer that the first RvdLogLsnEntry will be stored.
    //      TotalNumberOfEntries- Returns the number of LSN entries copied into IoBuffer.
    //      NumberOfEntriesIn1stSegment - Returns number of entries stored in 1st segment at IoBufferOffset (if any)
    //      NumberOfSegments            - Returns total number of segments needed to store the results
    //
    //  Return Value:
    //      STATUS_SUCCESS
    //      STATUS_INSUFFICIENT_RESOURCES
    //
    NTSTATUS
    GetAllRecordLsnsIntoIoBuffer(
        __in ULONG              SegmentSize,
        __in ULONG              PreambleSize,
        __in KIoBuffer&         IoBuffer,
        __inout ULONG&          IoBufferOffset,
        __out ULONG&            TotalNumberOfEntries,
        __out ULONG&            NumberOfEntriesIn1stSegment,
        __out ULONG&            NumberOfSegments);

    //* This routine, if successful, guarentees the next two calls to AddHigherLsnRecord()
    //  will succeed.
    //
    //  Return Value:
    //      STATUS_SUCCESS
    //      STATUS_INSUFFICIENT_RESOURCES
    //
    NTSTATUS
    GuarenteeAddTwoHigherRecords();

    //* Load the contents of an empty RvdLogLsnEntryTracker from another provided instance.
    //
    //
    //  Return Value:
    //      ....
    //
    //  NOTE: the contents of Src are cleared.
    //
    NTSTATUS
    Load(
        __in RvdLogLsnEntryTracker& Src,
        __in_opt RvdLogLsn *const LowestLsn = nullptr,
        __in_opt RvdLogLsn *const HighestLsn = nullptr);

private:

    // BUG, richhas, xxxxxx, Consider making RvdLogLsnEntryBlock a KNodeListShared<> derivation
    struct RvdLogLsnEntryBlock
    {
        static const ULONG          _NumberOfEntriesPerBlock = 1024;
        KListEntry                  _BlockLinks;
        RvdLogLsnEntry              _Entries[_NumberOfEntriesPerBlock];
    };
    // BUG, richhas, xxxxxx, Consider constraining _NumberOfEntriesPerBlock so that sizeof(RvdLogLsnEntryBlock) is
    //                       <= sizeof(BlkSize). This, combined with the willingness to waste some disk space because
    //                       of empty/truncated entries in the first/last blks, would make production of a KIoBuffer
    //                       in GetAllRecordLsnsIntoIoBuffer() simpler. This would also require additional metadata
    //                       about the returned table (offset to first RvdLogLsnEntry) to be carried in cp records.

private:
    RvdLogLsnEntryTracker();

    __inline ULONG
    ComputeTotalRecordSizeOnDisk(__in ULONG HeaderAndMetaDataSize, __in ULONG IoBufferSize)
    {
        HeaderAndMetaDataSize = RvdDiskLogConstants::RoundUpToBlockBoundary(HeaderAndMetaDataSize);
        KAssert(((IoBufferSize + HeaderAndMetaDataSize) % RvdDiskLogConstants::BlockSize) == 0);

        return IoBufferSize + HeaderAndMetaDataSize;
    }

    __inline VOID
    UnsafeAddRecord(
        __in RvdLogLsnEntry& Entry,
        __in RvdLogLsn Lsn,
        __in ULONG HeaderAndMetaDataSize,
        __in ULONG IoBufferSize)
    {
        Entry.Lsn = Lsn;
        Entry.IoBufferSize = IoBufferSize;
        Entry.HeaderAndMetaDataSize = HeaderAndMetaDataSize;
        _NumberOfEntires++;
        _TotalSize += ComputeTotalRecordSizeOnDisk(HeaderAndMetaDataSize, IoBufferSize);
    }

    NTSTATUS
    UnsafeAllocateRvdLogLsnEntryBlock(__out RvdLogLsnEntryBlock*& Result);


private:
    KSpinLock                       _ThisLock;
    KAllocator&                     _Allocator;
    KNodeList<RvdLogLsnEntryBlock>  _Blocks;                // Lowest LSNs at Head; highest at Tail
    ULONG                           _HeadBlockFirstEntryIx;
    ULONG                           _TailBlockLastEntryIx;
    ULONG                           _NumberOfEntires;
    ULONGLONG                       _TotalSize;
    RvdLogLsn                       _LowestLsn;
    RvdLogLsn                       _HighestLsn;
    RvdLogLsnEntryBlock*            _ReservedBlock;
};

//** RvdAsnIndex
class RvdAsnIndexEntry : public KShared<RvdAsnIndexEntry>
{
    K_FORCE_SHARED(RvdAsnIndexEntry);

public:
    static LONG
    Comparator(__in RvdAsnIndexEntry& Left, __in RvdAsnIndexEntry& Right);

    static ULONG
    GetLinksOffset();

    RvdAsnIndexEntry(__in RvdLogAsn Asn, __in ULONGLONG Version, __in ULONG IoBufferSize);
    RvdAsnIndexEntry(
        __in RvdLogAsn Asn,
        __in ULONGLONG Version,
        __in ULONG IoBufferSize,
        __in RvdLogStream::RecordDisposition Disposition,
        __in RvdLogLsn Lsn);

    __inline RvdLogAsn const    GetAsn()            { return _MappingEntry.RecordAsn; }
    __inline RvdLogLsn const    GetLsn()            { return _MappingEntry.RecordLsn; }
    __inline ULONGLONG const    GetVersion()        { return _MappingEntry.RecordAsnVersion; }
    __inline RvdLogStream::RecordDisposition
                                GetDisposition()    { return _Disposition; }
    __inline ULONG              GetIoBufferSizeHint()
                                                    { return _MappingEntry.IoBufferSizeHint; }
    __inline BOOLEAN            IsIndexedByAsn()    { return _AsnIndexEntryLinks.IsInserted(); }
    __inline VOID               SetLowestLsnOfHigherASNs(__in RvdLogLsn Value)   { _LowestLsnOfHigherASNs = Value; }
    __inline RvdLogLsn          GetLowestLsnOfHigherASNs()                  { return _LowestLsnOfHigherASNs; }

#if DBG
    ULONGLONG          DebugContext;
#endif

public:
    class SavedState
    {
    public:

        SavedState();

        _inline RvdLogStream::RecordDisposition
        GetDisposition()
        {
            return _Disposition;
        }

        _inline
        AsnLsnMappingEntry&
        GetMappingEntry()
        {
            return _MappingEntry;
        }

        _inline
        RvdLogLsn
        GetLowestLsnOfHigherASNs()
        {
            return _LowestLsnOfHigherASNs;
        }

    private:
        friend class RvdAsnIndexEntry;
        RvdLogStream::RecordDisposition _Disposition;
        AsnLsnMappingEntry              _MappingEntry;
        RvdLogLsn                       _LowestLsnOfHigherASNs;
    };

private:
    friend class RvdAsnIndex;

    __inline VOID SetAsn(__in RvdLogAsn Asn)            { _MappingEntry.RecordAsn = Asn; }

    VOID
    Save(__out SavedState& To);

    VOID
    Restore(__in SavedState& From);

private:
    KTableEntry                     _AsnIndexEntryLinks;
    RvdLogStream::RecordDisposition _Disposition;
    AsnLsnMappingEntry              _MappingEntry;
    RvdLogLsn                       _LowestLsnOfHigherASNs;
};

class RvdAsnIndex : public KObject<RvdAsnIndex>
{
public:
    RvdAsnIndex(__in KAllocator& Allocator);
    ~RvdAsnIndex();

    VOID
    Clear();

    //  Either add a new RvdAsnIndexEntry or update an existing entry with a newer version
    //
    //  Parameters:
    //      IndexEntry          - New Entry to be added if no current entry OR information to update an existing
    //                            entry. If IndexEntry is used (added), it is add ref'd
    //
    //      ResultIndexEntry    - Receives the results of the operation:
    //                                  nullptr     - duplicate entry - status of STATUS_OBJECT_NAME_COLLISION returned.
    //                                  ~nullptr    - Either the address of RvdAsnIndexEntry iif a new entry was added
    //                                                OR the address of the existing (and updated) RvdAsnIndexEntry. If
    //                                                IndexEntry is used (ResultIndexEntry == IndexEntry), ownership of
    //                                                the memory backing IndexEntry is transfered from the caller.
    //      PreviousIndexEntryWasStored - Set TRUE on exit iif the a newer version was updated; FALSE if a new version was added
    //
    //      PreviousIndexEntryValue - Has a copy of the previous value iif PreviousIndexEntryWasStored is returned TRUE
    //
    //      DuplicateRecordAsnVersion - Returns version number of the duplicate record ASN
    //
    //  Returns:
    //      STATUS_SUCCESS               - Entry was added or updated - see ResultIndexEntry
    //      STATUS_OBJECT_NAME_COLLISION - Duplicate or attempt to update with an old version
    //
    NTSTATUS
    AddOrUpdate(
        __in RvdAsnIndexEntry::SPtr& IndexEntry,
        __out RvdAsnIndexEntry::SPtr& ResultIndexEntry,
        __out RvdAsnIndexEntry::SavedState& PreviousIndexEntryValue,
        __out BOOLEAN& PreviousIndexEntryWasStored,
        __out ULONGLONG& DuplicateRecordAsnVersion);

    //  Restore an RvdAsnIndexEntry to the supplied previous state
    //
    //  Parameters:
    //      IndexEntry          - Entry to be restored
    //
    //      ForVersion          - Only to the restoration if IndexEntry's version is this value
    //
    //      EntryStateToRestore - Value to be restored.
    //
    VOID
    Restore(
        __in RvdAsnIndexEntry::SPtr& IndexEntry,
        __in ULONGLONG ForVersion,
        __in RvdAsnIndexEntry::SavedState& EntryStateToRestore);

    //*
    // Retrieve the smallest LowestLsnOfHigherASNs value from all of the records
    //
    RvdLogLsn
        GetLowestLsnOfHigherASNs(__in RvdLogLsn HighestKnownLsn);

    //*
    // Remove IndexEntry from _Table iif ForVersion matches IndexEntry current version
    //
    void
        TryRemove(
        __in RvdAsnIndexEntry::SPtr& IndexEntry,
        __in ULONGLONG ForVersion);

    //*
    // Remove IndexEntry from _Table iif ForVersion matches IndexEntry current version
    //
    NTSTATUS
        TryRemoveForDelete(
        __in RvdLogAsn Asn,
        __in ULONGLONG ForVersion,
        __out RvdLogLsn& TruncatableLsn);

    //*
    //  Returns:    TRUE iif the RvdAsnIndexEntry is for the supplied version - ForVersion. If TRUE, the Disposition
    //              has been updated to NewDisposition.
    BOOLEAN
    UpdateDisposition(
        __in RvdAsnIndexEntry::SPtr& IndexEntry,
        __in ULONGLONG ForVersion,
        __in RvdLogStream::RecordDisposition NewDisposition);

    //*
    //  Returns:    TRUE iif the RvdAsnIndexEntry is for the supplied version - ForVersion. If TRUE, the Disposition and
    //              Lsn have been updated to NewDisposition and NewLsn.
    BOOLEAN
    UpdateLsnAndDisposition(
        __in RvdAsnIndexEntry::SPtr& IndexEntry,
        __in ULONGLONG ForVersion,
        __in RvdLogStream::RecordDisposition NewDisposition,
        __in RvdLogLsn NewLsn);

    //* Truncate all RvdAsnIndexEntrys up to (but not including) UpToAsn. Return the highest truncated
    //  LSN. If all RvdAsnIndexEntry are truncated and HighestKnownLsn is higher than any of those
    //  truncated entries, return HighestKnownLsn
    //
    RvdLogLsn
    Truncate(__in RvdLogAsn UpToAsn, __in RvdLogLsn HighestKnownLsn);

    //* This routine returns a copy of all AsnLsnMappingEntry(s) into a supplied KIoBuffer in a segmented
    //  fashion; extending that KIoBuffer as needed. Each segment will be the same size (SegmentSize) and
    //  will have a preamble (skipped) of PreableSize at the start of all but the first segment - it is
    //  assumed that the caller will consider any first preamble needs in the initial IoBufferOffset
    //  passed. Only record metadata w/Dispositions of Pending or Persisted are returned.
    //
    //  NOTE: Entries will not be split across a segment boundary - unused space
    //
    //  Arguments:
    //      SegmentSize     - Size that limits the number of entries before PreableSize bytes are skipped.
    //      PreambleSize    - Number of bytes to skip at the front of each segment (except for the first).
    //      IoBuffer        - KIoBuffer - Contents below initial IoBufferOffset value are not disturbed. On
    //                                    return IoBuffer has NumberOfEntries copied in the specified
    //                                    segmented format: [IoBufferOffset][Entries][pad][[Preabmle][Entries][pad]]...
    //      IoBufferOffset  - Logical starting point in IoBuffer that the first AsnLsnMappingEntry will be stored.
    //      NumberOfEntries - Returns the number of LSN entries copied into IoBuffer.
    //      TotalSegments   - Returns the total number of segments that AsnLsnMappingEntry were stored in.
    //
    //  Return Value:
    //      STATUS_SUCCESS  - Worked; else not
    //
    NTSTATUS
    GetAllEntriesIntoIoBuffer(
        __in KAllocator&        Allocator,
        __in ULONG              SegmentSize,
        __in ULONG              PreambleSize,
        __inout KIoBuffer&      IoBuffer,
        __inout ULONG&          IoBufferOffset,
        __out ULONG&            NumberOfEntries,
        __out ULONG&            TotalSegments);

    //* This routine returns a range of RvdAsnIndexEntrys into a supplied KArray<RvdLogStream::RecordMetadata>
    //
    //  Arguments:
    //      LowestAsn - Supplies the lowest Asn of the record metadata to be returned.
    //
    //      HighestAsn - Supplies the highest Asn of the record metadata to be returned.
    //
    //      ResultsArray - Supplies the KArray<RecordMetadata> that the results of this
    //                     method will be returned.
    //
    //  Return Value:
    //      STATUS_SUCCESS  - Worked; else not
    //
    NTSTATUS
    GetEntries(
        __in RvdLogAsn LowestAsn,
        __in RvdLogAsn HighestAsn,
        __in KArray<RvdLogStream::RecordMetadata>& ResultsArray);

    VOID
    GetAsnRange(__out RvdLogAsn& LowestAsn, __out RvdLogAsn& HighestAsn, __in RvdLogAsn& HighestAsnObserved);

    VOID
    GetAsnInformation(
        __in RvdLogAsn                          Asn,
        __out ULONGLONG&                        Version,
        __out RvdLogStream::RecordDisposition&  RecordDisposition,
        __out RvdLogLsn&                        CurrentLsn,
        __out ULONG&                            IoBufferSizeHint);

    VOID
    GetPreviousAsnInformation(
        __inout RvdLogAsn&                      Asn,
        __out ULONGLONG&                        Version,
        __out RvdLogStream::RecordDisposition&  RecordDisposition,
        __out RvdLogLsn&                        CurrentLsn,
        __out ULONG&                            IoBufferSizeHint);

    VOID
    GetNextAsnInformation(
        __inout RvdLogAsn&                      Asn,
        __out ULONGLONG&                        Version,
        __out RvdLogStream::RecordDisposition&  RecordDisposition,
        __out RvdLogLsn&                        CurrentLsn,
        __out ULONG&                            IoBufferSizeHint);

    VOID
    GetNextAsnInformationFromNearest(
        __inout RvdLogAsn&                      Asn,
        __out ULONGLONG&                        Version,
        __out RvdLogStream::RecordDisposition&  RecordDisposition,
        __out RvdLogLsn&                        CurrentLsn,
        __out ULONG&                            IoBufferSizeHint);

    VOID
    GetPreviousAsnInformationFromNearest(
        __inout RvdLogAsn&                      Asn,
        __out ULONGLONG&                        Version,
        __out RvdLogStream::RecordDisposition&  RecordDisposition,
        __out RvdLogLsn&                        CurrentLsn,
        __out ULONG&                            IoBufferSizeHint);

    VOID
    GetContainingAsnInformation(
        __inout RvdLogAsn&                      Asn,
        __out ULONGLONG&                        Version,
        __out RvdLogStream::RecordDisposition&  RecordDisposition,
        __out RvdLogLsn&                        CurrentLsn,
        __out ULONG&                            IoBufferSizeHint);

    VOID
    GetNextFromSpecificAsnInformation(
        __inout RvdLogAsn&                      Asn,
        __out ULONGLONG&                        Version,
        __out RvdLogStream::RecordDisposition&  RecordDisposition,
        __out RvdLogLsn&                        CurrentLsn,
        __out ULONG&                            IoBufferSizeHint);

    VOID
    GetPreviousFromSpecificAsnInformation(
        __inout RvdLogAsn&                      Asn,
        __out ULONGLONG&                        Version,
        __out RvdLogStream::RecordDisposition&  RecordDisposition,
        __out RvdLogLsn&                        CurrentLsn,
        __out ULONG&                            IoBufferSizeHint);

    BOOLEAN
    CheckForRecordsWithHigherVersion(
        __in RvdLogAsn Asn,
        __in ULONGLONG Version,
        __out ULONGLONG& HigherVersion);

    RvdAsnIndexEntry::SPtr
    UnsafeFirst();

    RvdAsnIndexEntry::SPtr
    UnsafeLast();

    RvdAsnIndexEntry::SPtr
    UnsafeNext(__in RvdAsnIndexEntry::SPtr& Current);

    RvdAsnIndexEntry::SPtr
    UnsafePrev(__in RvdAsnIndexEntry::SPtr& Current);

    ULONG _inline
    GetNumberOfEntries()
    {
        return _Table.Count();
    }

    NTSTATUS
    Load( __in RvdAsnIndex& Src);

    BOOLEAN
    Validate();

private:
    //
    // Updates IndexEntry's _LowestLsnOfHigherASNs.
    //
    // The update will either capture IndexEntry's assigned LSN is there are no higher RvdAsnIndexEntrys,
    // OR it will be set to the next higher RvdAsnIndexEntry's _LowestLsnOfHigherASN. This latter condition
    // arises because log stream writes can be issued (or at least be processed) out of ASN order and as
    // such the ASN order of the written records can't be assumed to match the ordering from an LSN spatial
    // assignment point of view.
    VOID
    UnsafeUpdateLowestLsnOfHigherASNs(__in RvdAsnIndexEntry& IndexEntry);

    NTSTATUS
    UnsafeAddOrUpdate(
        __in RvdAsnIndexEntry::SPtr& IndexEntry,
        __out RvdAsnIndexEntry::SPtr& ResultIndexEntry,
        __out RvdAsnIndexEntry::SavedState& PreviousIndexEntryValue,
        __out BOOLEAN& PreviousIndexEntryWasStored,
        __out ULONGLONG& DuplicateRecordAsnVersion);

    _inline VOID
    UnsafeAccountForNewItem(__in RvdLogStream::RecordDisposition Disposition)
    {
        _CountsOfDispositions[Disposition]++;
        KAssert((_CountsOfDispositions[RvdLogStream::RecordDisposition::eDispositionPending] +
                 _CountsOfDispositions[RvdLogStream::RecordDisposition::eDispositionNone] +
                 _CountsOfDispositions[RvdLogStream::RecordDisposition::eDispositionPersisted]) == _Table.Count());
    }

    _inline VOID
    UnsafeAccountForRemovedItem(__in RvdLogStream::RecordDisposition Disposition)
    {
        _CountsOfDispositions[Disposition]--;
        KAssert((_CountsOfDispositions[RvdLogStream::RecordDisposition::eDispositionPending] +
                 _CountsOfDispositions[RvdLogStream::RecordDisposition::eDispositionNone] +
                 _CountsOfDispositions[RvdLogStream::RecordDisposition::eDispositionPersisted]) == _Table.Count());
    }

    _inline VOID
    UnsafeAccountForChangedDisposition(
        __in RvdLogStream::RecordDisposition OldDisposition,
        __in RvdLogStream::RecordDisposition NewDisposition)
    {
        _CountsOfDispositions[OldDisposition]--;
        _CountsOfDispositions[NewDisposition]++;
        KAssert((_CountsOfDispositions[RvdLogStream::RecordDisposition::eDispositionPending] +
                 _CountsOfDispositions[RvdLogStream::RecordDisposition::eDispositionNone] +
                 _CountsOfDispositions[RvdLogStream::RecordDisposition::eDispositionPersisted]) == _Table.Count());
    }

    static _inline BOOLEAN
    UnsafeIsAllocatedLsnDisposition(__in RvdLogStream::RecordDisposition Disposition)
    {
        return (Disposition == RvdLogStream::RecordDisposition::eDispositionPending) ||
               (Disposition == RvdLogStream::RecordDisposition::eDispositionPersisted);
    }

private:
    KSpinLock                       _ThisLock;
    KNodeTable<RvdAsnIndexEntry>    _Table;
    RvdAsnIndexEntry::SPtr          _LookupKey;



    // State to track count of types of RecordDispositions. A count Pending + Persisted (vs. None) is all that's needed.
    // This is being done this way for perf and simplicity.
    static_assert(
        ((RvdLogStream::RecordDisposition::eDispositionNone > RvdLogStream::RecordDisposition::eDispositionPending)  &&
         (RvdLogStream::RecordDisposition::eDispositionNone > RvdLogStream::RecordDisposition::eDispositionPersisted) &&
         (RvdLogStream::RecordDisposition::eDispositionNone > 0)),
         "Relative RvdLogStream::RecordDisposition values incorrect");

    ULONG                           _CountsOfDispositions[RvdLogStream::RecordDisposition::eDispositionNone + 1];
};

//** RvdLogManager implementation class
class RvdLogManagerImp : public RvdLogManager
{
    K_FORCE_SHARED(RvdLogManagerImp);

public:
    NTSTATUS
    Activate(
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

    VOID
    Deactivate();

    NTSTATUS
    CreateAsyncEnumerateLogsContext(__out AsyncEnumerateLogs::SPtr& Context);
    
    NTSTATUS
    CreateAsyncCreateLogContext(__out AsyncCreateLog::SPtr& Context);

    NTSTATUS
    CreateAsyncOpenLogContext(__out AsyncOpenLog::SPtr& Context);

    NTSTATUS
    CreateAsyncQueryLogIdContext(__out AsyncQueryLogId::SPtr& Context);

    NTSTATUS
    CreateAsyncDeleteLogContext(__out AsyncDeleteLog::SPtr& Context);

    NTSTATUS
    RegisterVerificationCallback(
        __in const RvdLogStreamType& LogStreamType,
        __in VerificationCallback Callback);

    VerificationCallback
    UnRegisterVerificationCallback(__in const RvdLogStreamType& LogStreamType);

    VerificationCallback
    QueryVerificationCallback(__in const RvdLogStreamType& LogStreamType);

    // forward define strongly typed DiskLog derivation of RvdLog
    class RvdOnDiskLog;

private:
    // Forward define private async operation implementations
    class AsyncOpenLogImp;
    class AsyncQueryLogIdImp;
    class AsyncEnumerateLogsImp;
    class AsyncCreateLogImp;
    class AsyncDeleteLogImp;
    class DiskRecoveryOp;

    // Friendly relations
    friend NTSTATUS RvdLogManager::Create(
        __in ULONG AllocationTag,
        __in KAllocator& Allocator,
        __out RvdLogManager::SPtr& Result);

    RvdLogManagerImp(__in ULONG MaxNumberOfLogsHint);

    VOID
    OnStart();

    VOID
    OnCancel();

    BOOLEAN __inline
    TryAcquireActivity();

    VOID __inline
    ReleaseActivity();

    NTSTATUS
    FindOrCreateLog(
        __in BOOLEAN const MustCreate,
        __in KGuid& DiskId,
        __in RvdLogId& LogId,
        __in KWString& OptionalLoggerPath,
        __out KSharedPtr<RvdOnDiskLog>& ResultRvdLog);

    NTSTATUS                // Returns STATUS_PENDING iif async recovery is taking place
    TryDiskRecovery(
        __in KGuid DiskId,
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

    VOID
    DiskLogDestructed();

    enum ActiveState : ULONG
    {
        Inactive,
        Active,
        Deactivating
    };

    // RvdOnDiskLog weak ref index entry
    typedef KSharedPtr<KWeakRef<RvdOnDiskLog>> DiskLogWRef;
    typedef KeyedWRef<RvdLogId, RvdOnDiskLog>  KeyedDiskLogWRef;

    struct DiskRecoveryGate
    {
        KGuid                   _DiskId;
        KAsyncEvent             _Event;         // Starts as an auto-reset but is switched to manual once a disk has
                                                // been recovered
    };

private:
    // This count control the overall life time of an RvdLogManagerImp. It MUST be 0 when Activate() is called;
    // Activate() increments it to reflect there is a activity - the instance being active. Deactivate causes
    // this value to be decremented. Any operations being performed on a RvdLogManagerImp will cause this value
    // to be incremented by one for the duration of the activity. All RvdLog making reference to an RvdLogManagerImp
    // also are reflected in this count; which is decremented when such and RvdLog instance destructs. At any point
    // when this value is dec'd to zero, Complete() is called.
    volatile LONG                       _ActivityCount;
    volatile BOOLEAN                    _IsActive;

    KSemaphore                          _Lock;      // Protects:
        KArray<KeyedDiskLogWRef>                _OnDiskLogObjectArray;
        KHashTable<GUID, VerificationCallback>  _CallbackArray;
        KArray<KUniquePtr<DiskRecoveryGate>>    _DiskRecoveryGates;
};

BOOLEAN __inline
RvdLogManagerImp::TryAcquireActivity()
//
// Try and acquire the right to use this instance. Once _ActivityCount reaches zero, no
// such right will be granted as the instance is in a deactivating state.
//
// Returns: FALSE - iif _ActivityCount has been latched at zero - only another Activate()
//                  will allow TRUE to be returned beyond that point.
//          TRUE  - _ActivityCount has been incremented safely and zero was not touched
{
    LONG value;
    while (((value = _ActivityCount) > 0) && _IsActive)
    {
        LONG newValue = value + 1;
        if (_InterlockedCompareExchange(&_ActivityCount, newValue, value) == value)
        {
            // We did a safe interlocked inc of _ActivityCount
            return TRUE;
        }
    }

    return FALSE;
}

VOID __inline
RvdLogManagerImp::ReleaseActivity()
//
// Release the right of usage acquired via TryAcquireActivity().
//
// Completes operation cycle when _ActivityCount goes to zero.
//
{
    KAssert(_ActivityCount > 0);
    if (_InterlockedDecrement(&_ActivityCount) == 0)
    {
        // API is locked and there is no other activity in this instance - complete the
        // operation cycle started with Activate()
        KInvariant(Complete(STATUS_SUCCESS));
    }
}


//** Class to contain a snapshot the entire metadata state for a Log; used in unit tests.
//   Generated by LogValidator and RvdLogManagerImp::RvdOnDiskLog classes.

class LogState : public KObject<LogState>, public KShared<LogState>
{
    K_FORCE_SHARED(LogState);

public:
    static NTSTATUS
    Create(__in ULONG AllocationTag, __in KAllocator& Allocator, __out LogState::SPtr& Result);

    class StreamDescriptor
    {
    public:
        RvdLogStreamInformation     _Info;
        RvdAsnIndex*                _AsnIndex;
        RvdLogLsnEntryTracker*      _LsnIndex;
        RvdLogAsn                   _TruncationPoint;

    public:
        StreamDescriptor()
            :   _AsnIndex(nullptr),
                _LsnIndex(nullptr)
        {}

        ~StreamDescriptor()
        {
            // _Info is not deleted because it references _StreamInfos
            _delete(_AsnIndex);
            _delete(_LsnIndex);
        }
    };

public:
    RvdLogConfig                _Config;
    ULONG                       _LogSignature[RvdDiskLogConstants::SignatureULongs];
    static const ULONG          LogSignatureSize = RvdDiskLogConstants::SignatureULongs * sizeof(ULONG);
    ULONGLONG                   _LogFileSize;
    ULONGLONG                   _LogFileLsnSpace;           // total available LSN-space within _LogFileSize
    ULONGLONG                   _MaxCheckPointLogRecordSize;
    ULONGLONG                   _LogFileFreeSpace;          // Current available free space in the LOG

    RvdLogLsn                   _LowestLsn;                 // All physical space mapped below this may be reclaimed
    RvdLogLsn                   _NextLsnToWrite;            // aka HighestLsn; active space = [_LowestLsn, _NextLsnToWrite)
    RvdLogLsn                   _HighestCompletedLsn;       // _HighestCompletedLsn < _NextLsnToWrite
    RvdLogLsn                   _HighestCheckpointLsn;

    ULONG                       _NumberOfStreams;
    KArray<StreamDescriptor>    _StreamDescs;
    WCHAR                       _CreationDirectory[RvdDiskLogConstants::MaxCreationPathSize];
    WCHAR                       _LogType[RvdLogManager::AsyncCreateLog::MaxLogTypeLength];
};


//** RvdLogStreamImp class
class RvdLogStreamImp : public RvdLogStream, public KWeakRefType<RvdLogStreamImp>
{
    K_FORCE_SHARED(RvdLogStreamImp);

public:
    LONGLONG
    QueryLogStreamUsage() override ;

    VOID
    QueryLogStreamType(__out RvdLogStreamType& LogStreamType) override;

    VOID
    QueryLogStreamId(__out RvdLogStreamId& LogStreamId) override;

    NTSTATUS
    QueryRecordRange(
        __out_opt RvdLogAsn* const LowestAsn,
        __out_opt RvdLogAsn* const HighestAsn,
        __out_opt RvdLogAsn* const LogTruncationAsn) override;

    ULONGLONG
    QueryCurrentReservation() override
    {
        return _ReservationHeld;
    }

    NTSTATUS
    CreateAsyncWriteContext(__out RvdLogStream::AsyncWriteContext::SPtr& Context) override;

    NTSTATUS
    CreateAsyncReadContext(__out RvdLogStream::AsyncReadContext::SPtr& Context) override;

    NTSTATUS
    CreateUpdateReservationContext(__out RvdLogStream::AsyncReservationContext::SPtr& Context) override;

    NTSTATUS
    QueryRecord(
        __in RvdLogAsn RecordAsn,
        __out_opt ULONGLONG* const Version,
        __out_opt RecordDisposition* const Disposition,
        __out_opt ULONG* const IoBufferSize,
        __out_opt ULONGLONG* const DebugInfo1) override;

    NTSTATUS
    QueryRecord(
        __inout RvdLogAsn& RecordAsn,
        __in RvdLogStream::AsyncReadContext::ReadType Type,
        __out_opt ULONGLONG* const Version,
        __out_opt RecordDisposition* const Disposition,
        __out_opt ULONG* const IoBufferSize,
        __out_opt ULONGLONG* const DebugInfo1) override;

    NTSTATUS
    QueryRecords(
        __in RvdLogAsn LowestAsn,
        __in RvdLogAsn HighestAsn,
        __in KArray<RecordMetadata>& ResultsArray) override;

    NTSTATUS
    DeleteRecord(
        __in RvdLogAsn RecordAsn,
        __in ULONGLONG Version) override;

    VOID
    Truncate(__in RvdLogAsn TruncationPoint, __in RvdLogAsn PreferredTruncationPoint) override;

    VOID
    TruncateBelowVersion(__in RvdLogAsn TruncationPoint, ULONGLONG Version) override;

    NTSTATUS
    SetTruncationCompletionEvent(__in_opt KAsyncEvent* const EventToSignal) override;
        
    //** Test support API

#define GetRvdLogStreamActivityId() ((KActivityId)_LogStreamDescriptor.Info.LogStreamId.Get().Data1)
                
    VOID
    GetPhysicalExtent(__out RvdLogLsn& Lowest, __out RvdLogLsn& Highest);

    LogStreamDescriptor&
    UnsafeGetLogStreamDescriptor()
    {
        return _LogStreamDescriptor;
    }

    //** API supporting RvdLogManagerImp::RvdOnDiskLog
    RvdLogStreamImp(
        __in RvdLogManagerImp::RvdOnDiskLog& Log,
        __in LogStreamDescriptor& StreamDesc,
        __in ULONG UniqueInstanceId);

    LogStreamDescriptor& GetLogStreamDescriptor() { return _LogStreamDescriptor; };

    // Forward define the private imp classes for RvdLogStream::AsyncWriteContext
    // and RvdLogStream::AsyncReadContext abstractions
    class AsyncWriteStream;
    class AsyncReadStream;
    class AsyncReserveStream;

    typedef KDelegate<VOID()> StreamDestructionCallback;

    VOID
    SetStreamDestructionCallback(__in_opt const StreamDestructionCallback& Callback)
    {
        if (Callback)
        {
            KAssert(!_DestructionCallback);
        }

        _DestructionCallback = Callback;
    }

private:

    VOID
        InitiateTruncationWrite(
        );

    VOID
    OnTruncationComplete(
        __in_opt KAsyncContextBase* const Parent,
        __in KAsyncContextBase&       CompletingSubOp);

private:
    // BUG, richhas, xxxxx, revisit what state needs to be carried here - e.g. does _StreamId need to be here given
    //                      _StreamInfo is here? Fix this once the deletion logic/approach is reworked.
    KSharedPtr<RvdLogManagerImp::RvdOnDiskLog>  _Log;
    LogStreamDescriptor&                        _LogStreamDescriptor;
    ULONG const                                 _ThisInstanceId;
    StreamDestructionCallback                   _DestructionCallback;   // !null means there is a pending delete or create
                                                                        // to be continued by the dtor - _State MUST == Deleted
    ULONGLONG                                   _ReservationHeld;
    KSharedPtr<AsyncWriteStream>                _AutoCloseReserveOp;

    KAsyncContextBase::CompletionCallback       _TruncationCompletion;
    KSpinLock                                   _ThisLock;
    KAsyncEvent*                                _TruncationCompletionEvent;
};

//** LogStreamDescriptor - all streams carried within a log are represented by an
//   instance of this class - no matter if that stream is open for access by the
//   log user or not.
class LogStreamDescriptor : public KObject<LogStreamDescriptor>
{
public:
    KListEntry                  Links;
    KAsyncLock                  OpenCloseDeleteLock;
    // BUG, richhas, xxxxx, consider only tracking Deleting here and move the opened indication
    //                      into the underlying RvdLogStreamImp. Background thinking: if Deleting
    //                      is indicated here, that is a one way state in that the Stream marked
    //                      as such WILL be deleted at some point from _Streams and all open/create
    //                      /delete ops finding such a mark will fail. This means that an Opened
    //                      indication will be carried in the related RvdLogStreamImp and that will
    //                      protected by the OpenCloseDeleteLock here.
    //                          - Pull SerialNumber
    //                          - Pull StreamClosed()
    //
    enum OpenState
    {
        Closed,
        Opened,
        Deleting
    };

    OpenState                       State;
    ULONG                           SerialNumber;       // Incremented everytime a new StreamImp
                                                        // is created for a given Stream obj
    RvdLogStreamInformation&        Info;
    RvdAsnIndex                     AsnIndex;
    RvdLogLsnEntryTracker           LsnIndex;
    StreamWRef                      WRef;               // WeakRef to (optional) API stream component if this stream is
                                                        // in use by a user

    KSharedPtr<RvdLogStreamImp::AsyncWriteStream> CheckPointRecordWriteOp;      // Each Stream can have 1 outstanding CP write op

    LONG volatile                   StreamTruncateWriteOpsRequested;            // > 0 means StreamTruncateWriteOp is active
        KSharedPtr<RvdLogStreamImp::AsyncWriteStream> StreamTruncateWriteOp;    // Each Stream can have 1 outstanding truncate op

    // Logical Stream Level Asn and Lsn Cursors - NOTE: Physical Log file stream related limits are carried in Info and are
    // tightly maintained by the physical stream write path logic - see RvdLofStreamImp::AsyncWriteStream
    RvdLogAsn                       MaxAsnWritten;
    RvdLogAsn                       TruncationPoint;
    RvdLogLsn                       HighestLsn;
    RvdLogLsn                       LastCheckPointLsn;
    RvdLogLsn                       LsnTruncationPointForStream;

    KInstrumentedComponent::SPtr    _InstrumentedComponent; 
public:
    LogStreamDescriptor(__in KAllocator& Allocator, __in RvdLogStreamInformation& StreamInfo);

    NTSTATUS
    BuildStreamCheckPointRecordTables(
        __in KAllocator& Allocator,
        __in KIoBuffer& RecordBuffer,
        __in RvdLogConfig& Config,
        __out ULONG& SizeWrittenInRecordBuffer);

    BOOLEAN
    Truncate(__in RvdLogAsn UpToAsn);    // returns TRUE if UpToAsn caused LsnTruncationPointForStream to advance

    VOID
    GetAsnRange(__out RvdLogAsn& LowestAsn, __out RvdLogAsn& HighestAsn);

private:
    LogStreamDescriptor();
};


//** Disk-based Log derivation of the RvdLog abstraction
class RvdLogManagerImp::RvdOnDiskLog : public RvdLog, public KWeakRefType<RvdOnDiskLog>
{
    K_FORCE_SHARED(RvdOnDiskLog);

public:
    //
    // This is the size that is used for the readahead cache in
    // KBlockDevice. Any reads less than this size will read this size
    // and cache the whole block in case a subsequent read is done.
    // Setting to 0 disables any readahead behavior
    //
    static const ULONG ReadAheadSize = 0;

    const WCHAR* LogContainerText = L"LogContainer";

    VOID
    QueryLogType(__out KWString& LogType);

    VOID
    QueryLogId(__out KGuid& DiskId, __out RvdLogId& LogId);

    VOID
    QuerySpaceInformation(__out_opt ULONGLONG* const TotalSpace, __out_opt ULONGLONG* const FreeSpace);

    ULONGLONG QueryReservedSpace() override ;

    VOID
    QueryCacheSize(__out_opt LONGLONG* const ReadCacheSizeLimit, __out_opt LONGLONG* const ReadCacheUsage);

    ULONGLONG
    QueryCurrentReservation() override
    {
        return _LogFileReservedSpace;
    }

    ULONG
    QueryMaxAllowedStreams() override
    {
        return _Config.GetMaxNumberOfStreams();
    }

    ULONG
    QueryMaxRecordSize() override
    {
        return _Config.GetMaxRecordSize();
    }

    ULONG
    QueryMaxUserRecordSize() override
    {
        return _Config.GetMaxUserMetadataSize();
    }

    ULONG
    QueryUserRecordSystemMetadataOverhead() override
    {
        return sizeof(RvdLogUserRecordHeader);
    }

    VOID
    QueryLsnRangeInformation(
        __out LONGLONG& LowestLsn,
        __out LONGLONG& HighestLsn,
        __out RvdLogStreamId& LowestLsnStreamId
        ) override ;
    
    VOID
    SetCacheSize(__in LONGLONG CacheSizeLimit);

    NTSTATUS
    CreateAsyncCreateLogStreamContext(__out AsyncCreateLogStreamContext::SPtr& Context);

    NTSTATUS
    CreateAsyncDeleteLogStreamContext(__out AsyncDeleteLogStreamContext::SPtr& Context);

    NTSTATUS
    CreateAsyncOpenLogStreamContext(__out AsyncOpenLogStreamContext::SPtr& Context);

    NTSTATUS
    UnsafeSnapLogState(__out LogState::SPtr& Result, __in ULONG const AllocationTag, __in KAllocator& Allocator);

    NTSTATUS
    PopulateLogState(__in LogState::SPtr& State);

    //** Test support API
    BOOLEAN
    IsStreamIdValid(__in RvdLogStreamId StreamId);

    BOOLEAN
    GetStreamState(
        __in RvdLogStreamId StreamId,
        __out_opt BOOLEAN * const IsOpen = nullptr,
        __out_opt BOOLEAN * const IsClosed = nullptr,
        __out_opt BOOLEAN * const IsDeleted = nullptr);

    BOOLEAN
    IsLogFlushed();

    VOID
    GetPhysicalExtent(__out LONGLONG& Lowest, __out LONGLONG& NextLsn);

    ULONGLONG
    GetAvailableLsnSpace()
    {
        return _LogFileLsnSpace;
    }

    ULONGLONG
    RemainingLsnSpaceToEOF();

    ULONG
    GetNumberOfStreams();

    ULONG
    GetStreams(__in ULONG MaxResults, __in RvdLogStreamId* Streams);

    ULONGLONG
    GetQuotaGateState();

    KBlockDevice::SPtr
    GetBlockDevice()
    {
        return _BlockDevice;
    }

    VOID
    SetBlockDevice(__in KBlockDevice& BlockDevice)
    {
        _BlockDevice = &BlockDevice;
    }

    RvdLogConfig&
    GetConfig()
    {
        return _Config;
    }

    //** API supporting RvdLogManagerImp
    class AsyncMountLog : public KAsyncContextBase
    {
        K_FORCE_SHARED(AsyncMountLog);

    public:
        AsyncMountLog(__in RvdLogManagerImp::RvdOnDiskLog::SPtr Owner);

        //
        // This method asynchronously mounts a log.
        //
        // Mounting a log involves building any neccessary in-memory data structures,
        // and running crash recovery. It should only be invoked by the owning RvdLogManager.
        //
        // Parameters:
        //  ParentAsyncContext  - Supplies an optional parent async context.
        //  CallbackPtr         - Supplies an optional callback delegate to be notified
        //                        when the physical log is mounted.
        //
        // Completion status:
        //  STATUS_SUCCESS                  - The operation completed successfully.
        //  STATUS_NOT_FOUND                - The speficied log does not exist.
        //  STATUS_CRC_ERROR                - The log file failed CRC verification.
        //  K_STATUS_LOG_STRUCTURE_FAULT    - The log file's structure is incorrect
        //  K_STATUS_LOG_INVALID_LOGID      - The log file's structure's log id is incorrect
        //
        VOID
        StartMount(
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

    private:
        VOID
        OnStart();

        VOID
        DoComplete(__in NTSTATUS Status);

        VOID
        GateAcquireComplete(__in BOOLEAN IsAcquired, __in KAsyncLock& LockAttempted);

        VOID
        LogFileCheckCreationFlags(
            __in_opt KAsyncContextBase* const Parent,
            __in KAsyncContextBase& CompletingSubOp);

        VOID
        LogFileReadMasterBlock(
            __in_opt KAsyncContextBase* const Parent,
            __in KAsyncContextBase& CompletingSubOp);

        VOID
        LogFileOpenComplete(
            __in_opt KAsyncContextBase* const Parent,
            __in KAsyncContextBase& CompletingSubOp);

        VOID
        RecoveryComplete(
            __in_opt KAsyncContextBase* const Parent,
            __in KAsyncContextBase& CompletingSubOp);

    private:
        class Recovery;

    private:
        KIoBuffer::SPtr             _IoBuffer;
        RvdOnDiskLog::SPtr          _Owner;
        KSharedPtr<Recovery>        _RecoveryState;
    };

    NTSTATUS
    CreateAsyncMountLog(__out AsyncMountLog::SPtr& Context);

    class AsyncCreateLog : public KAsyncContextBase
    {
        K_FORCE_SHARED(AsyncCreateLog);

    public:
        AsyncCreateLog(RvdLogManagerImp::RvdOnDiskLog::SPtr Owner);

        // This method asynchronously creates and mounts a log.
        //
        // This involves creation of an empty log structure on disk, building any
        // neccessary in-memory data structures. It should only be invoked by the
        // owning RvdLogManager.
        //
        // Parameters:
        //  ParentAsyncContext  - Supplies an optional parent async context.
        //  CallbackPtr         - Supplies an optional callback delegate to be notified
        //                        when the physical log is mounted.
        //
        // Completion status:
        //  STATUS_SUCCESS      - The operation completed successfully.
        //  STATUS_NOT_FOUND    - The speficied log does not exist.
        //  STATUS_CRC_ERROR    - The log file failed CRC verification.
        //
        VOID
        StartCreateLog(
            __in RvdLogConfig& Config,
            __in KWString& LogType,
            __in LONGLONG const LogSize,
            __in KStringView& OptionalLogFileLocation,
            __in DWORD CreationFlags,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

    private:
        VOID
        OnStart();

        VOID
        GateAcquireComplete(__in BOOLEAN IsAcquired, __in KAsyncLock& LockAttempted);

        VOID
        DoComplete(__in NTSTATUS Status);

        VOID
        DoCompleteWithDelete(__in NTSTATUS Status);

        VOID
        LogDirCreateComplete(
            __in_opt KAsyncContextBase* const Parent,
            __in KAsyncContextBase& CompletingSubOp);

        VOID
        OnLogDirCreateComplete();

        VOID
        LogFileCreateComplete(
            __in_opt KAsyncContextBase* const Parent,
            __in KAsyncContextBase& CompletingSubOp);

        VOID
        HeaderWriteComplete(
            __in_opt KAsyncContextBase* const Parent,
            __in KAsyncContextBase& CompletingSubOp);

        VOID
        TrailerWriteComplete(
            __in_opt KAsyncContextBase* const Parent,
            __in KAsyncContextBase& CompletingSubOp);

        VOID
        LogFileDeleteComplete(
            __in_opt KAsyncContextBase* const Parent,
            __in KAsyncContextBase& CompletingSubOp);

    private:
        RvdOnDiskLog::SPtr  _Owner;
        KWString*           _LogType;
        LONGLONG            _LogSize;
        RvdLogConfig        _ConfigParams;
        KStringView*        _OptionalLogFileLocation;
        KIoBuffer::SPtr     _MasterBlkIoBuffer;
        NTSTATUS            _DoCompleteWithDeleteStatus;
        DWORD               _CreationFlags;
    };

    NTSTATUS
    CreateAsyncCreateLog(__out AsyncCreateLog::SPtr& Context);

    class AsyncDeleteLog : public KAsyncContextBase
    {
        K_FORCE_SHARED(AsyncDeleteLog);

    public:
        AsyncDeleteLog(__in RvdLogManagerImp::RvdOnDiskLog::SPtr Owner);

        //
        // This method asynchronously deletes a log.
        //
        // Parameters:
        //  ParentAsyncContext  - Supplies an optional parent async context.
        //  CallbackPtr         - Supplies an optional callback delegate to be notified
        //                        when the physical log is mounted.
        //
        // Completion status:
        //  STATUS_SUCCESS      - The operation completed successfully.
        //  STATUS_NOT_FOUND    - The speficied log does not exist.
        //  K_STATUS_FILE_BUSY  - The log file is already opened.
        //
        VOID
        StartDelete(
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

    private:
        VOID
        OnStart();

        VOID
        DoComplete(__in NTSTATUS Status);

        VOID
        GateAcquireComplete(__in BOOLEAN IsAcquired, __in KAsyncLock& LockAttempted);

        VOID
        LogFileDeleteComplete(
            __in_opt KAsyncContextBase* const Parent,
            __in KAsyncContextBase& CompletingSubOp);

    private:
        RvdOnDiskLog::SPtr          _Owner;
    };

    NTSTATUS
    CreateAsyncDeleteLog(__out AsyncDeleteLog::SPtr& Context);

    HRESULT
    SetShutdownEvent(__in_opt KAsyncEvent* const EventToSignal) override;

    RvdOnDiskLog(
        __in RvdLogManagerImp* const LogManager,
        __in KGuid& DiskId,
        __in RvdLogId& LogId,
        __in KWString& FullyQualifiedLogName);

private:
    friend class RvdLogStreamImp;

    // Private API supporting the friend Async*LogStreamContextImps:
    friend class AsyncCreateLogStreamContextImp;
    friend class AsyncOpenLogStreamContextImp;
    friend class AsyncDeleteLogStreamContextImp;

    NTSTATUS
    CompleteActivation(__in RvdLogConfig& Config);

    NTSTATUS
    FindOrCreateStream(
        __in BOOLEAN const MustCreate,
        __in const RvdLogStreamId& LogStreamId,
        __in const RvdLogStreamType& LogStreamType,
        __out KSharedPtr<RvdLogStreamImp>& ResultStream,
        __out LogStreamDescriptor*& ResultStreamDesc);

    ULONG
    UnsafeAllocateStreamInfo();

    VOID
    UnsafeDeallocateStreamInfo(__inout RvdLogStreamInformation& EntryToFree);

    NTSTATUS
    UnsafeAllocateLogStreamDescriptor(
        __in const RvdLogStreamId& LogStreamId,
        __in const RvdLogStreamType& LogStreamType,
        __out LogStreamDescriptor*& Result);

    ULONG
    SnapSafeCopyOfStreamsTable(__out RvdLogStreamInformation* DestLocation);

    NTSTATUS
    StartDeleteStream(
        __in RvdLogStreamId& LogStreamId,
        __out KSharedPtr<RvdLogStreamImp>& ResultStreamSPtr,
        __out LogStreamDescriptor*& ResultStream);

    VOID
    FinishDeleteStream(__in LogStreamDescriptor* const LogStreamDescriptor);

    VOID
    StreamClosed(__in LogStreamDescriptor& LogStreamDescriptor, __in ULONG StreamImpInstanceId);

    RvdLogLsn
    ComputeLowestUsedLsn();

    VOID
        ComputeUnusedFileRanges(
        __out ULONGLONG& FromRange1,
        __out ULONGLONG& ToRange1,
        __out ULONGLONG& FromRange2,
        __out ULONGLONG& ToRange2
        );

    NTSTATUS
    EnqueueStreamWriteOp(__in RvdLogStreamImp::AsyncWriteStream& WriteOpToEnqueue);

    VOID
    IncomingStreamWriteDequeueCallback(
        __in_opt KAsyncContextBase* const ParentAsync,
        __in KAsyncContextBase& CompletingSubOp);

    VOID
    SuspendIncomingStreamWriteDequeuing();

    VOID
    ResumeIncomingStreamWriteDequeuing();

    NTSTATUS
    EnqueueCompletedStreamWriteOp(__in RvdLogStreamImp::AsyncWriteStream& WriteOpToEnqueue);

    BOOLEAN
    IsIncomingStreamWriteOpQueueDispatched()
    {
        return _IncomingStreamWriteOpQueueRefCount > 0;
    }

    VOID
    CompletingStreamWriteLsnOrderedGateCallback(
        __in_opt KAsyncContextBase* const ParentAsync,
        __in KAsyncContextBase& CompletingSubOp);

    // LSN/File Space Related Utility Functions
    ULONGLONG
    ToFileAddress(__in RvdLogLsn Lsn)
    {
        return RvdLogRecordHeader::ToFileAddress(Lsn, _LogFileLsnSpace);
    }

    ULONGLONG
    ToFileAddress(
        __in RvdLogLsn Lsn,
        __in ULONG const SizeToMap,
        __out ULONG& FirstSegmentSize)
    //
    // Map an LSN and length (from that LSN) -> file offset and length of a first segment
    //
    // Parameters:
    //      Lsn         - Starting location in LSN-space
    //      SizeToMap   - Length of the LSN-space
    //
    // Returns:              - File Offset of a first segment of the LSN-space [Lsn, Lsn + SizeToMap)
    //      FirstSegmentSize - size of the returned File Offset of the first segment of the LSN-space [Lsn, Lsn + SizeToMap).
    //                         NOTE: if this value is < SizeToMap; then the file space is wrapped
    //
    //
    {
        return RvdLogRecordHeader::ToFileAddress(Lsn, _LogFileLsnSpace, SizeToMap, FirstSegmentSize);
    }

private:
    KSharedPtr<RvdLogManagerImp>    _LogManager;
    KGuid                           _DiskId;
    RvdLogId                        _LogId;
    KWString                        _FullyQualifiedLogName;
    DWORD                           _CreationFlags;
    BOOLEAN                         _IsLogFailed;
    RvdLogConfig                    _Config;
    KAsyncEvent*                    _ShutdownEvent;
    WCHAR                           _LogType[RvdLogManager::AsyncCreateLog::MaxLogTypeLength];

    KAsyncLock                      _CreateOpenDeleteGate;      // Used to serialize all log open/close/del ops
    BOOLEAN                         _IsOpen;
        KBlockDevice::SPtr                  _BlockDevice;               // must be !nullptr if _IsOpen
        ULONG                               _LogSignature[RvdDiskLogConstants::SignatureULongs];
        __declspec(align(8)) ULONGLONG      _LogFileSize;
        __declspec(align(8)) ULONGLONG      _LogFileLsnSpace;           // total available LSN-space within _LogFileSize
        ULONGLONG                           _LogFileMinFreeSpace;
        ULONGLONG                           _MaxCheckPointLogRecordSize;
        LogStreamDescriptor*                _CheckPointStream;

        KSharedPtr<RvdLogStreamImp::AsyncWriteStream>
                                            _PhysicalCheckPointWriteOp;   // Should only exist iif there is at least 1 user stream opened

    KSpinLock                           _ThisLock;      // Protects:
        KNodeList<LogStreamDescriptor>  _Streams;       // BUG, richhas, xxxxx, consider a better data structure aimed at
                                                        //                      1000's of streams per log
                                                        // BUG, richhas, xxxxx, consider using KSharedNodeList
        KArray<RvdLogStreamInformation> _LogStreamInfoArray;
        ULONG                           _FreeLogStreamInfoCount;
        ULONG                           _LastUsedLogStreamInfoIx;       // Only valid iif _FreeLogStreamInfoCount < MaxNumberOfStreams

    KBuffer::SPtr                       _EmptyBuffer;
    KIoBuffer::SPtr                     _EmptyIoBuffer;

    typedef KAsyncQueue<RvdLogStreamImp::AsyncWriteStream> AsyncWriteStreamQueue;
    typedef KAsyncOrderedGate<RvdLogStreamImp::AsyncWriteStream, RvdLogLsn> AsyncWriteStreamGate;

    KSharedPtr<KQuotaGate>                  _StreamWriteQuotaGate;

    KSharedPtr<AsyncWriteStreamQueue>       _IncomingStreamWriteOpQueue;
    // The following are protected by _IncomingStreamWriteOpQueue:
        __declspec(align(8)) ULONGLONG      _LogFileFreeSpace;          // Current available free space in the LOG
        __declspec(align(8)) ULONGLONG      _LogFileReservedSpace;      // Current reserve (against free space) in the LOG
        __declspec(align(8)) RvdLogLsn      _LowestLsn;                 // All physical space mapped below this may be reclaimed
        __declspec(align(8)) RvdLogLsn      _NextLsnToWrite;            // aka HighestLsn; active space = [_LowestLsn, _NextLsnToWrite)
                             LONG volatile  _IncomingStreamWriteOpQueueRefCount;    // FALSE or > FALSE

    KSharedPtr<AsyncWriteStreamQueue::DequeueOperation> _IncomingStreamWriteDequeueOp;

    KSharedPtr<AsyncWriteStreamGate>        _CompletingStreamWriteLsnOrderedGate;
        // The following is protected by _CompletingStreamWriteLsnOrderedGate:
        __declspec(align(8)) RvdLogLsn      _HighestCompletedLsn;       // _HighestCompletedLsn < _NextLsnToWrite
        BOOLEAN                             _CompletingStreamWriteLsnOrderedGateDispatched;

    KAsyncContextBase::CompletionCallback   _CompletingStreamWriteLsnOrderedGateCallback;
    KAsyncContextBase::CompletionCallback   _IncomingStreamWriteDequeueCallback;

public:
    // State supporting debugging
    LONG volatile                           _CountOfOutstandingWrites;
    BOOLEAN                                 _DebugDisableAutoCheckpointing;
    BOOLEAN                                 _DebugDisableTruncateCheckpointing;
    BOOLEAN                                 _DebugDisableAssertOnLogFull;
    BOOLEAN                                 _DebugDisableHighestCompletedLsnUpdates;
};


//** RvdLogStreamImp::AsyncWriteStream class.
//   This class is used for four different kinds of "writes" (really log mutation):
//      1) Normal user initiated LogStream writes
//      2) Generation of internal writes to the dedicated CheckPoint Stream,
//      3) Writes of logical checkpoint records into a user log stream,
//      4) Log truncation operations
//      5) Log reservations
//
// BUG, richhas, xxxxxx, todo: refactor this class once the QuotaGate
//                             has been put into place.
//
class RvdLogStreamImp::AsyncWriteStream : public RvdLogStream::AsyncWriteContext
{
    K_FORCE_SHARED(AsyncWriteStream);

public:
    //** UserWriteOp specific API
    VOID
    StartWrite(
        __in RvdLogAsn RecordAsn,
        __in ULONGLONG Version,
        __in const KBuffer::SPtr& MetaDataBuffer,
        __in const KIoBuffer::SPtr& IoBuffer,
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override;

    VOID
    StartWrite(
        __in BOOLEAN LowPriorityIO,
        __in RvdLogAsn RecordAsn,
        __in ULONGLONG Version,
        __in const KBuffer::SPtr& MetaDataBuffer,
        __in const KIoBuffer::SPtr& IoBuffer,
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override;

    VOID
    StartReservedWrite(
        __in ULONGLONG ReserveToUse,
        __in RvdLogAsn RecordAsn,
        __in ULONGLONG Version,
        __in const KBuffer::SPtr& MetaDataBuffer,
        __in const KIoBuffer::SPtr& IoBuffer,
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override;

    VOID
    StartReservedWrite(
        __in ULONGLONG ReserveToUse,
        __in RvdLogAsn RecordAsn,
        __in ULONGLONG Version,
        __in const KBuffer::SPtr& MetaDataBuffer,
        __in const KIoBuffer::SPtr& IoBuffer,
        __out ULONGLONG& LogSize,
        __out ULONGLONG& LogSpaceRemaining,
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override;

    VOID
        StartReservedWrite(
        __in BOOLEAN LowPriorityIO,
        __in ULONGLONG ReserveToUse,
        __in RvdLogAsn RecordAsn,
        __in ULONGLONG Version,
        __in const KBuffer::SPtr& MetaDataBuffer,
        __in const KIoBuffer::SPtr& IoBuffer,
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override;


    VOID
    StartForcedCheckpointWrite(
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

    //** TruncateWriteOp specific API
    VOID
    StartTruncation(
        __in BOOLEAN ForceTruncation,
        __in RvdLogLsn LsnTruncationPointForStream,
        __in RvdLogManagerImp::RvdOnDiskLog::SPtr& Log,             // This is passed because TruncateWriteOps don't keep
        __in_opt KAsyncContextBase *const ParentAsyncContext,       // a sptr ref to its Log while its ts not an active operation.
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

    //** UpdateReservationOp specific API
    VOID
    StartUpdateReservation(
        __in LONGLONG DeltaReserve,                                 // Amount to adjust the current reserve by
        __in_opt KAsyncContextBase *const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

    //** Methods required by KAsyncOrderedGate<>
    RvdLogLsn
    GetAsyncOrderedGateKey();

    RvdLogLsn
    GetNextAsyncOrderedGateKey();

    //** Test support methods
    BOOLEAN
    IoWrappedEOF()
    {
        return _WriteWrappedEOF;
    }

private:
    friend class RvdLogManagerImp::RvdOnDiskLog;
    friend class RvdLogStreamImp;
    friend class AsyncCreateLogStreamContextImp;
    friend class AsyncDeleteLogStreamContextImp;
    friend NTSTATUS RvdLogStreamImp::CreateAsyncWriteContext(__out RvdLogStream::AsyncWriteContext::SPtr&);

    //*** Extended API to support special write forms
    enum Type
    {
        UserWriteOp,
        LogicalCheckPointWriteOp,
        LogicalCheckPointSegmentWriteOp,
        PhysicalCheckPointWriteOp,
        TruncateWriteOp,
        UpdateReservationOp
    };

    //** UpdateReservationOp specific API
    static NTSTATUS CreateUpdateReservationAsyncWriteContext(
        __in KAllocator& Allocator,
        __in RvdLogStreamImp& Owner,
        __out KSharedPtr<AsyncWriteStream>& Context);

    static NTSTATUS CreateUpdateReservationAsyncWriteContext(
        __in KAllocator& Allocator,
        __in RvdLogManagerImp::RvdOnDiskLog& Log,
        __in LogStreamDescriptor& LogStreamDescriptor,
        __out KSharedPtr<AsyncWriteStream>& Context);

    VOID
    OnStartUpdateReservation();

    NTSTATUS
    OnContinueUpdateReservation();

    //** UserWriteOp specific API
    AsyncWriteStream(__in RvdLogStreamImp* const Owner);
    AsyncWriteStream(__in RvdLogManagerImp::RvdOnDiskLog& Log, __in LogStreamDescriptor& ForStreamDesc);

    VOID
    UserWriteOpCtorInit(__in RvdLogManagerImp::RvdOnDiskLog& Log);

    //** TruncateWriteOp specific API
    static NTSTATUS CreateTruncationAsyncWriteContext(
        __in KAllocator& Allocator,
        __in RvdLogManagerImp::RvdOnDiskLog& Log,
        __in LogStreamDescriptor& ForStreamDesc,
        __out KSharedPtr<AsyncWriteStream>& Context);

    //** PhysicalCheckPointWriteOp specific API
    static NTSTATUS CreateCheckpointStreamAsyncWriteContext(
        __in KAllocator& Allocator,
        __in RvdLogManagerImp::RvdOnDiskLog& Log,
        __out KSharedPtr<AsyncWriteStream>& Context);

    AsyncWriteStream(__in RvdLogManagerImp::RvdOnDiskLog& Log);

    VOID
    PrepareForPhysicalCheckPointWrite(
        __in RvdLogManagerImp::RvdOnDiskLog& Log,
        __in RvdLogLsn WriteAt,
        __in RvdLogLsn HighestCompletedLsn,
        __in RvdLogLsn LastCheckPointStreamLsn,
        __in RvdLogLsn PreviousLsnInStream,
        __out ULONGLONG& ResultingLsnSpace,
        __out RvdLogLsn& HighestLsnGenerated);

    VOID
    CancelPrepareForPhysicalCheckPointWrite();

    VOID
    StartPhysicalCheckPointWrite(
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

    //** LogicalCheckPointWriteOp specific API
    static NTSTATUS CreateCheckpointRecordAsyncWriteContext(
        __in KAllocator& Allocator,
        __in RvdLogManagerImp::RvdOnDiskLog& Log,
        __in LogStreamDescriptor& ForStreamDesc,
        __out KSharedPtr<AsyncWriteStream>& Context);

    NTSTATUS
    PrepareForCheckPointRecordWrite(
        __in RvdLogManagerImp::RvdOnDiskLog& Log,
        __in RvdLogLsn WriteAt,
        __in RvdLogLsn HighestCompletedLsn,
        __in RvdLogLsn LastCheckPointStreamLsn,
        __in RvdLogLsn PreviousLsnInStream,
        __out ULONG& ResultingLsnSpace,
        __out RvdLogLsn& HighestRecordLsnGenerated);

    VOID
    CancelPrepareForCheckPointRecordWrite();

    VOID
    StartCheckPointRecordWrite(
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

    //** LogicalCheckPointSegmentWriteOp specific API
    static NTSTATUS CreateCheckpointAsyncSegmentWriteContext(
        __in KAllocator& Allocator,
        __in RvdLogManagerImp::RvdOnDiskLog& Log,
        __in LogStreamDescriptor& ForStreamDesc,
        __out KSharedPtr<AsyncWriteStream>& Context);

    VOID
    StartCheckPointRecordSegmentWrite(
        __in RvdLogManagerImp::RvdOnDiskLog& Log,
        __in RvdLogLsn WriteAt,
        __in KIoBuffer::SPtr BaseIoBuffer,
        __in ULONG BaseIoBufferOffset,
        __in ULONG SizeToWrite,
        __in RvdLogLsn HighestCompletedLsn,
        __in RvdLogLsn LastCheckPointStreamLsn,
        __in RvdLogLsn PreviousLsnInStream,
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

    //** Common imp methods
    AsyncWriteStream(
        __in RvdLogManagerImp::RvdOnDiskLog& Log,
        __in LogStreamDescriptor& StreamDesc,
        __in AsyncWriteStream::Type InstanceType);

    NTSTATUS
    CommonCtorInit(__in RvdLogManagerImp::RvdOnDiskLog& Log);

    VOID
    OnStart() override;

    VOID
    OnCompleted() override;

    VOID
    DoCommonComplete(__in NTSTATUS Status);

    VOID
    RelieveAsyncActivity();

    VOID
    QuantaAcquiredCompletion(__in_opt KAsyncContextBase* const Parent, __in KAsyncContextBase& CompletingSubOp);

    NTSTATUS
    ContinueRecordWrite();

    NTSTATUS
    ComputePhysicalWritePlan(
        __in KIoBuffer& CompleteRecordBuffer,
        __out ULONGLONG& FirstSegmentFileOffset,
        __out ULONGLONG& SecondSegmentFileOffset);

    VOID
    LogFileWriteCompletion(__in_opt KAsyncContextBase* const Parent, __in KAsyncContextBase& CompletingSubOp);

    NTSTATUS
    ContinueRecordWriteCompletion();

    VOID
    CommonRecordWriteCompletion();

    //** AsyncWriteStream::Type specific imp
    //*     UserWriteOp specific
    VOID
    OnStartUserRecordWrite();

    VOID
    OnCompletedUserRecordWrite();

    VOID
    DoUserRecordWriteComplete(__in NTSTATUS Status);

    VOID
    OnUserRecordWriteQuantaAcquired();

    NTSTATUS
    OnContinueUserRecordWrite();

    NTSTATUS
    OnUserRecordWriteCompletion();

    static VOID
    SubCheckpointWriteCompletion(
        __in_opt KAsyncContextBase* const Parent,
        __in KAsyncContextBase& CompletingSubOp);

    VOID
    ForcedCheckpointWriteCompletion(
        __in_opt KAsyncContextBase* const Parent,
        __in KAsyncContextBase& CompletingSubOp);

    //*     TruncateWriteOp specific
    VOID
    OnStartTruncation();

    VOID
    OnCompletedTruncation();

    VOID
    OnTruncationQuantaAcquired();

    VOID
    DoTruncationComplete(__in NTSTATUS Status);

    NTSTATUS
    OnContinueTruncation();

    VOID
    TruncateTrimComplete(
        __in_opt KAsyncContextBase* const Parent,
        __in KAsyncContextBase& CompletingSubOp);

    VOID
    ReleaseTruncateActivity();

    VOID
    TruncateCheckPointWriteComplete(
        __in_opt KAsyncContextBase* const Parent,
        __in KAsyncContextBase& CompletingSubOp);

    VOID
    TruncationCompleted();

    //*     LogicalCheckPointWriteOp specific
    VOID
    OnCheckPointRecordWriteStart();

    VOID
    DoCheckPointRecordWriteComplete(__in NTSTATUS Status);

    VOID
    OnCheckpointSegmentWriteCompletion(
        __in_opt KAsyncContextBase* const Parent,
        __in KAsyncContextBase& CompletingSubOp);

    VOID
    ScheduleNextSegmentWrite(__in AsyncWriteStream& SegWriteOp);

    //*     LogicalCheckPointSegmentWriteOp specific
    VOID
    OnCheckPointRecordSegmentWriteStart();

    VOID
    DoCheckPointRecordSegmentWriteComplete(__in NTSTATUS Status);

    NTSTATUS
    OnCheckPointRecordSegmentWriteCompletion();

    //*     PhysicalCheckPointWriteOp specific
    VOID
    OnStartPhysicalCheckPointWrite();

    VOID
    DoPhysicalCheckPointWriteComplete(__in NTSTATUS Status);

    NTSTATUS
    OnPhysicalCheckPointWriteCompletion();


    //** Common checkpoint-related imp methods
    NTSTATUS
    CommonCheckpointCtorInit();

    VOID
    CompleteHeaderAndStartWrites(__inout KIoBuffer& FullRecordBuffer, __in ULONG FullRecordSize);

private:
    KListEntry                              _WriteQueueLinks;

    KInstrumentedOperation                  _InstrumentedOperation;
    
    // Common "const" state
    AsyncWriteStream::Type const            _Type;
    KAsyncContextBase::SPtr                 _WriteOp0;              // Two are needed when log wrapping occurs
    KAsyncContextBase::SPtr                 _WriteOp1;
    KIoBuffer::SPtr                         _WorkingIoBuffer0;      // Two are needed when log wrapping occurs
    KIoBuffer::SPtr                         _WorkingIoBuffer1;
    CompletionCallback                      _WriteCompletion;
    RvdLogManagerImp::RvdOnDiskLog::SPtr    _Log;                   // The following reference validity is maintained
        LogStreamDescriptor&                _StreamDesc;            // because these are sub-structs of RvdOnDiskLog
        RvdLogStreamInformation&            _StreamInfo;
    KQuotaGate::AcquireContext::SPtr        _QuantaAcquireOp;
    CompletionCallback                      _QuantaAcquiredCompletion;
    ULONGLONG                               _QuantaAcquired;

    // UserWriteOp and UpdateReservationOp specific "const" state:
    RvdLogStreamImp::SPtr                   _OwningStream;  // can be null - used only to keep alive a parent Stream
                                                            // API object - not used for internal delete stream related
                                                            // write ops

    // UserWriteOp specific "const" state:
    RvdAsnIndex* const                      _AsnIndex;      // Protected by _Log
    KIoBuffer::SPtr                         _CombinedUserRecordIoBuffer;
    CompletionCallback                      _SubCheckpointWriteCompletion;
    CompletionCallback                      _ForcedCheckpointWriteCompletion;
    BOOLEAN                                 _WriteWrappedEOF;

    // Common "const" state for PhysicalCheckPointWriteOp and LogicalCheckPointWriteOp:
    KIoBuffer::SPtr                         _CommonCheckpointRecordIoBuffer;

    // PhysicalCheckPointWriteOp specific "const" state:
    KIoBufferElement::SPtr                  _PhysicalCheckpointRecordBuffer;
        RvdLogPhysicalCheckpointRecord*     _PhysicalCheckpointRecord;

    // LogicalCheckPointWriteOp specific "const" state:
    AsyncWriteStream::SPtr                  _CheckpointSegmentWriteOp0;
    AsyncWriteStream::SPtr                  _CheckpointSegmentWriteOp1;
    BOOLEAN                                 _IsInputOpQueueSuspended;
    CompletionCallback                      _OnCheckpointSegmentWriteCompletion;


    // LogicalCheckPointSegmentWriteOp specific "const" state:
    KIoBufferView::SPtr                     _SegmentView;

    // TruncateWriteOp specific "const" state:
    AsyncWriteStream::SPtr                  _TruncateCheckPointOp;
    CompletionCallback                      _TruncateCheckPointWriteCompletion;

    CompletionCallback                      _TruncateTrimCompletion;


    // Per operation parameters - UpdateReservationOp:
    LONGLONG                                _ReservationDelta;

    // Per operation parameters - UserWriteOp:
    BOOLEAN                                 _ForcedCheckpointWrite;
        AsyncWriteStream::SPtr              _DedicatedForcedCheckpointWriteOp;

    RvdLogAsn                               _RecordAsn;
    ULONGLONG                               _Version;
    KBuffer::SPtr                           _MetaDataBuffer;
    KIoBuffer::SPtr                         _IoBuffer;
    ULONGLONG                               _ReserveToUse;
    BOOLEAN                                 _LowPriorityIO;
    ULONG                                   _WriteSize;
    ULONGLONG*                              _CurrentLogSize;
    ULONGLONG*                              _CurrentLogSpaceRemaining;

    // Per operation parameters - TruncateWriteOp:
    RvdLogLsn                               _LsnTruncationPointForStream;
    BOOLEAN                                 _ForceTruncation;

    ULONGLONG                               _TrimFrom1;
    ULONGLONG                               _TrimTo1;
    ULONGLONG                               _TrimFrom2;
    ULONGLONG                               _TrimTo2;

    // Per operation parameters - Common to LogicalCheckPointWriteOp, LogicalCheckPointSegmentWriteOp,
    //                            and PhysicalCheckPointWriteOp:
    RvdLogLsn                               _CheckPointHighestCompletedLsn;
    RvdLogLsn                               _CheckPointLastCheckPointStreamLsn;
    RvdLogLsn                               _CheckPointPreviousLsnInStream;

    // Per operation parameters - LogicalCheckPointSegmentWriteOp:
    KIoBuffer::SPtr                         _CheckpointSegmentWriteIoBuffer;
    ULONG                                   _CheckpointSegmentWriteOffset;

    // Common per operation working state
    RvdLogLsn                               _AssignedLsn;
    ULONG                                   _SizeToWrite;
    LONG volatile                           _AsyncActivitiesLeft;
    LONG volatile                           _WriteOpFailureCount;

    // UserWriteOp per operation working state
    RvdAsnIndexEntry::SPtr                  _AsnIndexEntry;
    RvdAsnIndexEntry::SavedState            _SavedAsnIndexEntry;
    BOOLEAN                                 _IsSavedAsnIndexEntryValid;
    BOOLEAN                                 _TryDeleteAsnIndexEntryOnCompletion;

    // LogicalCheckPointWriteOp per operation working state
    ULONG                                   _SegmentsToWrite;
    ULONG                                   _SegmentsWritten;
    ULONG                                   _NextSegmentToWrite;

public:
    // Test support
    BOOLEAN                                 _WroteMultiSegmentStreamCheckpoint;

    BOOLEAN                                 _ForceStreamCheckpoint;

    // TEMP
    ULONGLONG                               _DebugFileWriteOffset0;
    ULONG                                   _DebugFileWriteLength0;
    ULONGLONG                               _DebugFileWriteOffset1;
    ULONG                                   _DebugFileWriteLength1;
};

//** RvdLogStreamImp::AsyncReserveStream imp
class RvdLogStreamImp::AsyncReserveStream : public RvdLogStream::AsyncReservationContext
{
    K_FORCE_SHARED(AsyncReserveStream);

public:
    VOID
    StartUpdateReservation(
        __in LONGLONG Delta,
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override;

    AsyncReserveStream(__in RvdLogStreamImp& Owner);

private:
    VOID OnReserveUpdated(__in_opt KAsyncContextBase* const Parent, __in KAsyncContextBase& SubOp);
    VOID OnStart() override;

private:
    RvdLogStreamImp::SPtr                   _Owner;
    RvdLogStreamImp::AsyncWriteStream::SPtr _BackingOp;
    LONGLONG                                _Delta;
};


//** AsyncParseLogFilename
class AsyncParseLogFilename : public KAsyncContextBase
{
    K_FORCE_SHARED(AsyncParseLogFilename);

public:
    static NTSTATUS
    Create(
        __in ULONG AllocationTag,
        __in KAllocator& Allocator,
        __out AsyncParseLogFilename::SPtr& Result);

    VOID
    StartParseFilename(
        __in KWString& Filename,
        __out KGuid& DiskId,
        __out KWString& RootPath,
        __out KWString& RelativePath,
        __out RvdLogId& LogId,
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

private:
    VOID OnStart() override;
    VOID OnCompleted() override;

    VOID
    OnQueryStableVolumeId(
        __in_opt KAsyncContextBase* const Parent,
        __in KAsyncContextBase& CompletingSubOp);
    VOID
    OnStartContinue();
    
    VOID
    OnLogFileAliasOpenComplete(
        __in_opt KAsyncContextBase* const Parent,
        __in KAsyncContextBase& CompletingSubOp);

    VOID
    OnLogFileHeaderReadComplete(
        __in_opt KAsyncContextBase* const Parent,
        __in KAsyncContextBase& CompletingSubOp);
    
    //
    // Parameters
    //
    KWString _Filename;
    KGuid* _DiskId;
    KWString* _RelativePath;
    KWString* _RootPath;
    RvdLogId* _LogId;
    
    //
    // Work variables
    //
    KBlockFile::SPtr                        _LogFileHandle;
    KIoBuffer::SPtr                         _LogFileHeaderBuffer;
    RvdLogMasterBlock*                      _LogFileHeader;
};

