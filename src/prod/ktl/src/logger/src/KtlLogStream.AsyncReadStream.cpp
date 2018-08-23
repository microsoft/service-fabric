/*++

    (c) 2010 by Microsoft Corp. All Rights Reserved.

    RvdLogStream.AsyncReadStream.cpp

    Description:
      RvdLogStream::AsyncReadStream implementation.

    History:
      PengLi/Richhas        22-Oct-2010         Initial version.

--*/

#include "InternalKtlLogger.h"

// BUG, richhas, xxxxx, add detection for STATUS_DELETE_PENDING 
// BUG, richhas, xxxxx, add cache support - part of recovery work



//** RvdLogStream::AsyncReadContext base class ctor and dtor.
//
RvdLogStream::AsyncReadContext::AsyncReadContext()
{
}

RvdLogStream::AsyncReadContext::~AsyncReadContext()
{
}

//** RvdLogStreamImp::AsyncReadStream imp
class RvdLogStreamImp::AsyncReadStream : public RvdLogStream::AsyncReadContext
{
    K_FORCE_SHARED(AsyncReadStream);

public:
    VOID
    StartRead(
        __in RvdLogAsn RecordAsn, 
        __out_opt ULONGLONG* const Version,
        __out KBuffer::SPtr& MetaDataBuffer, 
        __out KIoBuffer::SPtr& IoBuffer, 
        __in_opt KAsyncContextBase* const ParentAsyncContext, 
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

    VOID
    StartRead(
        __in RvdLogAsn RecordAsn,
        __in ReadType Type,
        __out_opt RvdLogAsn* const ActualRecordAsn,
        __out_opt ULONGLONG* const Version,
        __out KBuffer::SPtr& MetaDataBuffer, 
        __out KIoBuffer::SPtr& IoBuffer, 
        __in_opt KAsyncContextBase* const ParentAsyncContext, 
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);
    
    AsyncReadStream(RvdLogStreamImp* const Owner);

private:
    VOID
    OnStart();

    VOID
    DoComplete(NTSTATUS Status);

    VOID
    HeaderReadCompletion(
        KAsyncContextBase* const Parent, 
        KAsyncContextBase& CompletingSubOp);

private:
    RvdLogStreamImp::SPtr                   _Stream;
    RvdLogManagerImp::RvdOnDiskLog::SPtr    _Log;           // The following reference validity is maintained
        LogStreamDescriptor&                _StreamDesc;    // because these are sub-structs of RvdOnDiskLog
    KAsyncContextBase::SPtr                 _ReadOp0;
    KAsyncContextBase::SPtr                 _ReadOp1;
    CompletionCallback                      _HeaderReadCompletion;


    // Per operation parameters:
    RvdLogAsn                           _RecordAsn;
    RvdLogAsn*                          _OptResultRecordAsn;
    ULONGLONG*                          _OptResultVersion;
    KBuffer::SPtr*                      _ResultMetaDataBuffer; 
    KIoBuffer::SPtr*                    _ResultIoBuffer;
    ReadType                            _Type;

    // Per operation working state:
    ReadType _ReadType; 
    KIoBuffer::SPtr                     _WorkingIoBuffer0;
    KIoBufferElement::SPtr              _HeaderBuffer;
        RvdLogUserRecordHeader*         _UserRecordHeader;
    KIoBuffer::SPtr                     _WorkingIoBuffer1;
    RvdLogLsn                           _RecordLsn;
    BOOLEAN                             _DoingSplitRead;
    ULONG                               _NumberOfReads;
    ULONG                               _NumberOfReadFailures;
    NTSTATUS                            _FirstReadFailure;
};


RvdLogStreamImp::AsyncReadStream::AsyncReadStream(RvdLogStreamImp* const Owner)
    :   _Stream(Owner),
        _Log(Owner->_Log),
        _StreamDesc(Owner->_LogStreamDescriptor)
{
    _HeaderReadCompletion = KAsyncContextBase::CompletionCallback(this, &AsyncReadStream::HeaderReadCompletion);

    NTSTATUS status = _Log->_BlockDevice->AllocateReadContext(_ReadOp0, KTL_TAG_LOGGER);
    if (NT_SUCCESS(status))
    {
        status = _Log->_BlockDevice->AllocateReadContext(_ReadOp1, KTL_TAG_LOGGER);
    }

    if (!NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
    }
}

RvdLogStreamImp::AsyncReadStream::~AsyncReadStream()
{
}

NTSTATUS 
RvdLogStreamImp::CreateAsyncReadContext(__out RvdLogStream::AsyncReadContext::SPtr& Context)
{
    Context = _new(KTL_TAG_LOGGER, GetThisAllocator()) AsyncReadStream(this);
    if (!Context)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS status = Context->Status();
    if (!NT_SUCCESS(status))
    {
        Context.Reset();
    }
    return status;    
}

VOID
RvdLogStreamImp::AsyncReadStream::StartRead(
    __in RvdLogAsn RecordAsn, 
    __out_opt ULONGLONG* const Version,
    __out KBuffer::SPtr& MetaDataBuffer, 
    __out KIoBuffer::SPtr& IoBuffer, 
    __in_opt KAsyncContextBase* const ParentAsyncContext, 
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _RecordAsn = RecordAsn;
    _OptResultRecordAsn = NULL;
    _OptResultVersion = Version;
    _ResultMetaDataBuffer = &MetaDataBuffer;
    _ResultIoBuffer = &IoBuffer;
    _Type = ReadExactRecord;

    // Clear results
    MetaDataBuffer.Reset();
    IoBuffer.Reset();
    if (Version != nullptr)
    {
        *Version = 0;
    }

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
RvdLogStreamImp::AsyncReadStream::StartRead(
    __in RvdLogAsn RecordAsn, 
    __in ReadType Type,
    __out_opt RvdLogAsn* ActualRecordAsn,
    __out_opt ULONGLONG* const Version,
    __out KBuffer::SPtr& MetaDataBuffer, 
    __out KIoBuffer::SPtr& IoBuffer, 
    __in_opt KAsyncContextBase* const ParentAsyncContext, 
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _RecordAsn = RecordAsn;
    _OptResultVersion = Version;
    _OptResultRecordAsn = ActualRecordAsn;
    _ResultMetaDataBuffer = &MetaDataBuffer;
    _ResultIoBuffer = &IoBuffer;
    _Type = Type;

    // Clear results
    MetaDataBuffer.Reset();
    IoBuffer.Reset();
    if (Version != nullptr)
    {
        *Version = 0;
    }

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
RvdLogStreamImp::AsyncReadStream::DoComplete(NTSTATUS Status)
{
    _WorkingIoBuffer0.Reset();
    _WorkingIoBuffer1.Reset();
    _HeaderBuffer.Reset();
    _UserRecordHeader = nullptr;

    Complete(Status);
}

VOID
RvdLogStreamImp::AsyncReadStream::OnStart()
//
// Continued from StartRead
{
    ULONGLONG version;
    RvdLogStream::RecordDisposition rd;
    ULONG ioBufferSizeHint;

    switch(_Type)
    {
        case ReadExactRecord:
        {
            _StreamDesc.AsnIndex.GetAsnInformation(
                _RecordAsn,
                version,
                rd,
                _RecordLsn,
                ioBufferSizeHint);
            break;
        }

        case ReadPreviousRecord:
        {
            _StreamDesc.AsnIndex.GetPreviousAsnInformation(
                _RecordAsn,
                version,
                rd,
                _RecordLsn,
                ioBufferSizeHint);
            break;
        }

        case ReadNextRecord:
        {
            _StreamDesc.AsnIndex.GetNextAsnInformation(
                _RecordAsn,
                version,
                rd,
                _RecordLsn,
                ioBufferSizeHint);
            break;
        }

        case ReadContainingRecord:
        {
            _StreamDesc.AsnIndex.GetContainingAsnInformation(
                _RecordAsn,
                version,
                rd,
                _RecordLsn,
                ioBufferSizeHint);
            break;
        }

        case ReadNextFromSpecificAsn:
        {
            _StreamDesc.AsnIndex.GetNextFromSpecificAsnInformation(
                                                            _RecordAsn,
                                                            version,
                                                            rd,
                                                            _RecordLsn,
                                                            ioBufferSizeHint);
            break;

        }

        case ReadPreviousFromSpecificAsn:
        {
            _StreamDesc.AsnIndex.GetPreviousFromSpecificAsnInformation(
                _RecordAsn,
                version,
                rd,
                _RecordLsn,
                ioBufferSizeHint);
            break;
        }

        default:
        {
            DoComplete(STATUS_INVALID_PARAMETER);
            return;         
        }
    }

    if ((rd != RvdLogStream::RecordDisposition::eDispositionPersisted) ||
        (_RecordAsn <= _StreamDesc.TruncationPoint))
    {
        KTraceFailedAsyncRequest(STATUS_NOT_FOUND, this, _RecordAsn.Get(), _StreamDesc.TruncationPoint.Get());
        KTraceFailedAsyncRequest(STATUS_NOT_FOUND, this, _RecordAsn.Get(), rd);
        DoComplete(STATUS_NOT_FOUND);
        return;
    }

    if ((_RecordLsn > _Log->_HighestCompletedLsn) 
        || (_RecordLsn < _StreamDesc.Info.LowestLsn) 
        || (_RecordLsn > _StreamDesc.Info.HighestLsn))
    {
        // record is out of current phy range of the log stream
        KTraceFailedAsyncRequest(STATUS_NOT_FOUND, this, _RecordLsn.Get(), _Log->_HighestCompletedLsn.Get());
        KTraceFailedAsyncRequest(STATUS_NOT_FOUND, this, _RecordLsn.Get(), _StreamDesc.Info.LowestLsn.Get());
        KTraceFailedAsyncRequest(STATUS_NOT_FOUND, this, _RecordLsn.Get(), _StreamDesc.Info.HighestLsn.Get());
        DoComplete(STATUS_NOT_FOUND);
        return;
    }

    //
    // Return ASN actually read
    //
    if (_OptResultRecordAsn != NULL)
    {
        *_OptResultRecordAsn = _RecordAsn;
    }

    //** Record Id appears valid at this point, start the read of the header. Plan the overall read
    // which could require two reads if the record is split across EOF boundary.
    ULONG firstSegmentLength;
    ULONGLONG firstSegmentOffset = _Log->ToFileAddress(_RecordLsn, ioBufferSizeHint, firstSegmentLength);
    ULONGLONG secondSegmentOffset = 0;

    _DoingSplitRead = (_NumberOfReads = ((firstSegmentLength == ioBufferSizeHint) ? 1 : 2)) == 2;
    if (_DoingSplitRead)
    {
        secondSegmentOffset = _Log->ToFileAddress(_RecordLsn + firstSegmentLength);
    }
    _NumberOfReadFailures = 0;
    _FirstReadFailure = STATUS_SUCCESS;

    NTSTATUS status = _Log->_BlockDevice->Read(
        firstSegmentOffset,
        firstSegmentLength,
        TRUE,       // ContiguousIoBuffer
        _WorkingIoBuffer0, 
        _HeaderReadCompletion, 
        this, 
        _ReadOp0);

    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(STATUS_NOT_FOUND, this, _RecordAsn.Get(), firstSegmentOffset);
        DoComplete(status);
        return;
    }

    if (_DoingSplitRead)
    {
        status = _Log->_BlockDevice->Read(
            secondSegmentOffset,
            ioBufferSizeHint - firstSegmentLength,
            TRUE,       // ContiguousIoBuffer
            _WorkingIoBuffer1, 
            _HeaderReadCompletion, 
            this, 
            _ReadOp1);

        if (!NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(STATUS_NOT_FOUND, this, _RecordAsn.Get(), secondSegmentOffset);
            DoComplete(status);
            return;
        }
    }

    // Continued @ HeaderReadCompletion
}

//* Helper for walking thru a KIoBuffer set (2 max)
struct KIoBuffersCursor
{
    K_DENY_COPY(KIoBuffersCursor);

public:
    KIoBuffersCursor(KIoBuffer::SPtr* const First, KIoBuffer::SPtr* const Second = nullptr)
        :   _KIoBuffer0(First),
            _KIoBuffer1(Second)
    {
        Reset();
    }

    ULONG
    QuerySize()
    {
        return (KIoBuffer1() != nullptr) ? (KIoBuffer1()->QuerySize() + KIoBuffer0()->QuerySize()) : 
                                            KIoBuffer0()->QuerySize();
    }

    VOID
    Reset()
    {
        _CurrentKIoBuffer = KIoBuffer0();
        _CurrentElement = _CurrentKIoBuffer->First();
        KInvariant(_CurrentElement != nullptr);
        _CurrentElementSize = _CurrentElement->QuerySize();
        _CurrentOffset = 0;
        _CurrentElementRemainingSize = _CurrentElementSize;
    }

    BOOLEAN
    SeekForward(ULONG By)
    {
        while ((By > 0) && (_CurrentElement != nullptr))
        {
            ULONG todo = __min(By, _CurrentElementRemainingSize);
            By -= todo;
            _CurrentOffset += todo;
            _CurrentElementRemainingSize -= todo;
            if (_CurrentElementRemainingSize == 0)
            {
                _CurrentElement = _CurrentKIoBuffer->Next(*_CurrentElement);
                if (_CurrentElement == nullptr)
                {
                    if (_CurrentKIoBuffer != KIoBuffer1())
                    {
                        _CurrentKIoBuffer = KIoBuffer1();
                        _CurrentElement = (_CurrentKIoBuffer != nullptr) ? _CurrentKIoBuffer->First() : nullptr;
                    }
                }

                _CurrentElementSize = (_CurrentElement != nullptr) ? _CurrentElement->QuerySize() : 0;
                _CurrentElementRemainingSize = _CurrentElementSize;
            }
        }

        return (By == 0);
    }

    VOID
    SeekTo(ULONG Offset)
    {
        Reset();
        SeekForward(Offset);
    }

    VOID
    SeekToNextSegment()
    {
        SeekForward(_CurrentElementRemainingSize);
    }

    void*
    GetCurrentSegmentAddress(ULONG& RemainingSize)
    {
        return ((UCHAR*)_CurrentElement->GetBuffer()) + 
                (_CurrentElementSize - (RemainingSize = _CurrentElementRemainingSize));
    }

    KIoBufferElement&
    GetCurrentSegmentElement(ULONG& RemainingSize)
    {
        RemainingSize = _CurrentElementRemainingSize;
        return *_CurrentElement;
    }

    ULONG
    CurrentOffset()
    {
        return _CurrentOffset;
    }

private:
    KIoBuffersCursor() 
    :   _KIoBuffer0(nullptr),
        _KIoBuffer1(nullptr)
    {}

    KIoBuffer* 
    KIoBuffer0()
    {
        return ((_KIoBuffer0 == nullptr) || (!_KIoBuffer0)) ? nullptr : (*_KIoBuffer0).RawPtr();
    }

    KIoBuffer* 
    KIoBuffer1()
    {
        return ((_KIoBuffer1 == nullptr) || (!_KIoBuffer1)) ? nullptr : (*_KIoBuffer1).RawPtr();
    }

private:
    KIoBuffer::SPtr* const  _KIoBuffer0;
    KIoBuffer::SPtr* const  _KIoBuffer1;
    KIoBuffer*              _CurrentKIoBuffer;
    KIoBufferElement*       _CurrentElement;
    ULONG                   _CurrentElementSize;
    ULONG                   _CurrentElementRemainingSize;
    ULONG                   _CurrentOffset;
};

VOID
RvdLogStreamImp::AsyncReadStream::HeaderReadCompletion(
    KAsyncContextBase* const Parent, 
    KAsyncContextBase& CompletingSubOp)
//
// Continued from OnStart()
{
    UNREFERENCED_PARAMETER(Parent);

    KAssert(_NumberOfReads > 0);
    _NumberOfReads--;

    NTSTATUS status = CompletingSubOp.Status();
    if (!NT_SUCCESS(status))
    {
        _NumberOfReadFailures++;
        if (_FirstReadFailure == STATUS_SUCCESS)
        {
            _FirstReadFailure = status;
        }

        KTraceFailedAsyncRequest(status, &CompletingSubOp, 0, 0);
    }

    if (_NumberOfReads != 0)
    {
        return;
    }

    if (_NumberOfReadFailures > 0)
    {
        DoComplete(_FirstReadFailure);
        return;
    }

    // Do basic header validation
    KAssert(_WorkingIoBuffer0->QuerySize() >= RvdLogUserRecordHeader::ComputeSizeOnDisk(0));
    _HeaderBuffer = _WorkingIoBuffer0->First();
    _UserRecordHeader = (RvdLogUserRecordHeader*)_HeaderBuffer->GetBuffer();

    if ((RtlCompareMemory(
            &_UserRecordHeader->LogSignature[0], 
            &_Log->_LogSignature[0], 
            RvdLogUserRecordHeader::LogSignatureSize) != RvdLogUserRecordHeader::LogSignatureSize)  ||
        (_UserRecordHeader->Reserved1 != 0)                                                         ||
        (_UserRecordHeader->Lsn != _RecordLsn)                                                      ||
        (_UserRecordHeader->LogId != _Log->_LogId)                                                  ||
        (_UserRecordHeader->LogStreamId != _StreamDesc.Info.LogStreamId)                            ||
        (_UserRecordHeader->LogStreamType != _StreamDesc.Info.LogStreamType)                        ||
        (_UserRecordHeader->Asn != _RecordAsn)                                                      ||
        (_UserRecordHeader->RecordType != RvdLogUserRecordHeader::Type::UserRecord)                 ||
        ((_UserRecordHeader->ThisHeaderSize % RvdDiskLogConstants::BlockSize) != 0)                 ||
        (_UserRecordHeader->MetaDataSize > _Log->_Config.GetMaxMetadataSize())                      ||
        (_UserRecordHeader->IoBufferSize > _Log->_Config.GetMaxIoBufferSize()))
    {
        KTraceFailedAsyncRequest(K_STATUS_LOG_STRUCTURE_FAULT, this, _RecordAsn.Get(),  _UserRecordHeader->Asn.Get());
        DoComplete(K_STATUS_LOG_STRUCTURE_FAULT);
        return;
    }

    ULONG totalRecordSizeOnDisk = RvdLogUserRecordHeader::ComputeSizeOnDisk(_UserRecordHeader->GetSizeOfUserMetaData()) 
                                  + _UserRecordHeader->IoBufferSize;
    if (totalRecordSizeOnDisk > _Log->_Config.GetMaxRecordSize())
    {
        KTraceFailedAsyncRequest(STATUS_DATA_ERROR, this, _RecordAsn.Get(), totalRecordSizeOnDisk);      
        KTraceFailedAsyncRequest(STATUS_DATA_ERROR, this, _RecordAsn.Get(), _Log->_Config.GetMaxRecordSize());      
        DoComplete(STATUS_DATA_ERROR);
        return;
    }

    ULONGLONG               savedHeaderChecksum = _UserRecordHeader->ThisBlockChecksum;
    ULONGLONG               partialMetadataAndHeaderChecksum = 0;
    KIoBuffersCursor        bufferCursor(&_WorkingIoBuffer0, &_WorkingIoBuffer1);
    ULONG                   sizeToChksum = _UserRecordHeader->MetaDataSize 
                                            + (sizeof(RvdLogRecordHeader) 
                                               - (sizeof _UserRecordHeader->LsnChksumBlock));

    if (sizeToChksum > (bufferCursor.QuerySize() - sizeof(_UserRecordHeader->LsnChksumBlock)))
    {
        KTraceFailedAsyncRequest(K_STATUS_LOG_STRUCTURE_FAULT, this, _RecordAsn.Get(), sizeToChksum);          
        DoComplete(K_STATUS_LOG_STRUCTURE_FAULT);
        return;     
    }

    
    // Checksum header and metadata
    bufferCursor.SeekForward(sizeof _UserRecordHeader->LsnChksumBlock);
    _UserRecordHeader->ThisBlockChecksum = 0;

#pragma warning(disable:4127)   // C4127: conditional expression is constant
    while (TRUE)
    {
        ULONG todo;
        void* nextSrcPtr = bufferCursor.GetCurrentSegmentAddress(todo);
        todo = __min(todo, sizeToChksum);

        partialMetadataAndHeaderChecksum = KChecksum::Crc64(nextSrcPtr, todo, partialMetadataAndHeaderChecksum);
        sizeToChksum -= todo;
        if (sizeToChksum > 0)
        {
            if (!bufferCursor.SeekForward(todo))
            {
                KInvariant(FALSE);
            }
        }
        else
        {
            break;
        }
    }
    _UserRecordHeader->ThisBlockChecksum = savedHeaderChecksum;

    partialMetadataAndHeaderChecksum = KChecksum::Crc64(
        _UserRecordHeader, 
        (sizeof(_UserRecordHeader->LsnChksumBlock)), 
        partialMetadataAndHeaderChecksum);

    if (partialMetadataAndHeaderChecksum != savedHeaderChecksum)
    {
        DoComplete(STATUS_CRC_ERROR);
        return;
    }

    if (totalRecordSizeOnDisk > bufferCursor.QuerySize())
    {
        DoComplete(STATUS_BUFFER_TOO_SMALL);
        return;
    };

    //** Record (physical) requested has been read... 
    if (_RecordAsn <= _StreamDesc.TruncationPoint)
    {
        KTraceFailedAsyncRequest(STATUS_NOT_FOUND, this, _RecordAsn.Get(), _StreamDesc.TruncationPoint.Get());     
        DoComplete(STATUS_NOT_FOUND);
        return;
    }

    if ((_RecordLsn > _Log->_HighestCompletedLsn) 
        || (_RecordLsn < _StreamDesc.Info.LowestLsn) 
        || (_RecordLsn > _StreamDesc.Info.HighestLsn))
    {
        // record is out of current phy range of the log stream
        KTraceFailedAsyncRequest(STATUS_NOT_FOUND, this, _RecordAsn.Get(), _RecordLsn.Get());        
        KTraceFailedAsyncRequest(STATUS_NOT_FOUND, this, _StreamDesc.Info.LowestLsn.Get(), _StreamDesc.Info.HighestLsn.Get());        
        DoComplete(STATUS_NOT_FOUND);
        return;
    }


    //** Return user metadata
    ULONG sizeOfUserMetaData = _UserRecordHeader->GetSizeOfUserMetaData();
    status = KBuffer::Create(
        sizeOfUserMetaData, 
        *_ResultMetaDataBuffer, 
        GetThisAllocator(),
        KTL_TAG_LOGGER);

    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, _RecordAsn.Get(), sizeOfUserMetaData);        
        DoComplete(status);
        return;
    }

    bufferCursor.SeekTo(FIELD_OFFSET(RvdLogUserRecordHeader, UserMetaData));
    UCHAR* destUserMetaDataPtr = (UCHAR*)(*_ResultMetaDataBuffer)->GetBuffer();
    
    while (sizeOfUserMetaData > 0)
    {
        ULONG todo;
        void* srcUserMetaDataPtr = bufferCursor.GetCurrentSegmentAddress(todo);
        todo = __min(todo, sizeOfUserMetaData);

        if (todo > 0)
        {
            KMemCpySafe(destUserMetaDataPtr, sizeOfUserMetaData, srcUserMetaDataPtr, todo);
        }

        destUserMetaDataPtr += todo;
        sizeOfUserMetaData -= todo;
        if (!bufferCursor.SeekForward(todo))
        {
            KInvariant(FALSE);
        }
    }

    // return iobuffer payload to user
    status = KIoBuffer::CreateEmpty(*_ResultIoBuffer, GetThisAllocator(), KTL_TAG_LOGGER);
    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, _RecordAsn.Get(), 0);     
        (*_ResultMetaDataBuffer).Reset();
        DoComplete(status);
        return;
    }

    ULONG ioBufferSize = _UserRecordHeader->IoBufferSize;
    bufferCursor.SeekForward(_UserRecordHeader->ThisHeaderSize - bufferCursor.CurrentOffset());
    while (ioBufferSize > 0)
    {
        ULONG               currentSegmentSize;
        KIoBufferElement*   currentElement = &(bufferCursor.GetCurrentSegmentElement(currentSegmentSize));
        currentSegmentSize = __min(currentSegmentSize, ioBufferSize);

        KIoBufferElement::SPtr nextElementToAdd;
        status = KIoBufferElement::CreateReference(
            *currentElement,
            currentElement->QuerySize() - currentSegmentSize, 
            currentSegmentSize, 
            nextElementToAdd,
            GetThisAllocator(),
            KTL_TAG_LOGGER);

        if (!NT_SUCCESS(status))
        {
            (*_ResultMetaDataBuffer).Reset();
            (*_ResultIoBuffer).Reset();
            KTraceFailedAsyncRequest(status, this, _RecordAsn.Get(), 0);     
            DoComplete(status);
            return;
        }

        (*_ResultIoBuffer)->AddIoBufferElement(*nextElementToAdd);
        if (!bufferCursor.SeekForward(currentSegmentSize))
        {
            KInvariant(FALSE);
        }
        ioBufferSize -= currentSegmentSize;
    }

    if (_OptResultVersion != nullptr)
    {
        (*_OptResultVersion) = _UserRecordHeader->AsnVersion;
    }

    DoComplete(STATUS_SUCCESS);
}
