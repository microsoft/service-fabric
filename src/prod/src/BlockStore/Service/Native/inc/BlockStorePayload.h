// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include <cstdint>
#include <memory>
#include "common.h"
#include "CNetworkStream.h"
#include "boost_async_awaitable.h"

struct BlockStorePayload
{
private:

    // These are the fields corresponding to the data that comes in 
    // from the client.
    //
    // Operation mode for the request
    BlockStore::Mode::Enum mode_;

    // Absolute disk offset from which the read/write operation needs to be performed.
    uint64_t offsetToRW_;

    // Number of bytes to be read/written from/to the absolute offset.
    uint32_t dataBufferLength_;

    // Length of LUN ID in bytes.
    uint32_t deviceIdLength_;

    // When using shared memory, the SRB (SCSI Request Block) address comes in here.
    // If this is non-zero, it implies we are in Shared memory mode for buffer R/W.
    uint64_t addressSRB_;

    // Request tracking ID
    uint64_t trackingId_;

    // LUN ID.
    u16string deviceId_;

    // Marker that identifies end of payload request header.
    uint32_t endMarker_;
    
    // Fields present in LU registration payload
    uint32_t lengthLUID_;
    uint32_t diskSize_;
    uint32_t diskSizeUnit_;
    uint32_t mounted_;
    u16string LUID_;

    // Reference to the networkstream to be used
    CNetworkStream *clientStream_;

    // These are the fields used internally by the type.
    bool payloadHeaderParsed_;
    bool useDMA_;
    vector<BlockStoreByte> dataBuffer_;

    // Actual size of the data in the DataBuffer since DataBuffer is DataBufferLength in size but can carry data *upto*
    // DataBufferLength in size.
    uint32_t actualDataLength_;

    // Data buffer for holding the payload header
    BlockStoreByte payloadHeaderBuffer_[BlockStore::Constants::SizeBlockStoreRequestPayloadHeader];

 public :
   
    __declspec(property(get = get_Mode, put = set_Mode)) BlockStore::Mode::Enum Mode;
    BlockStore::Mode::Enum get_Mode() const { return mode_; }
    void set_Mode(BlockStore::Mode::Enum newMode) { mode_ = newMode; }

    __declspec(property(get = get_ModeDescription)) const char* ModeDescription;
    const char *get_ModeDescription();

    __declspec(property(get = get_DataBufferLength, put = set_DataBufferLength)) uint32_t DataBufferLength;
    uint32_t get_DataBufferLength() const
    {
        return dataBufferLength_;
    }

    void set_DataBufferLength(uint32_t newDataBufferLength)
    {
        dataBufferLength_ = newDataBufferLength;
    }

    __declspec(property(get = get_OffsetToRW)) uint64_t OffsetToRW;
    uint64_t get_OffsetToRW() const
    {
        return offsetToRW_;
    }

    __declspec(property(get = get_DeviceIdLength)) uint32_t DeviceIdLength;
    uint32_t get_DeviceIdLength() const
    {
        return deviceIdLength_;
    }
    
    __declspec(property(get = get_DeviceId)) u16string DeviceId;
    u16string& get_DeviceId()
    {
        return deviceId_;
    }

    __declspec(property(get = get_EndMarker)) uint32_t EndMarker;
    uint32_t get_EndMarker() const
    {
        return endMarker_;
    }


    // TODO: Merge these fields into one
    __declspec(property(get = get_LengthLUID, put = put_LengthLUID)) uint32_t LengthLUID;
    uint32_t get_LengthLUID() const { return lengthLUID_; }
    void put_LengthLUID(uint32_t val) { lengthLUID_ = val; }

    __declspec(property(get = get_DiskSize, put = put_DiskSize)) uint32_t DiskSize;
    uint32_t get_DiskSize() const { return diskSize_; }
    void put_DiskSize(uint32_t val) { diskSize_ = val; }

    __declspec(property(get = get_DiskSizeUnit, put = put_DiskSizeUnit)) uint32_t DiskSizeUnit;
    uint32_t get_DiskSizeUnit() const { return diskSizeUnit_; }
    void put_DiskSizeUnit(uint32_t val) { diskSizeUnit_ = val; }

    __declspec(property(get = get_Mounted, put = put_Mounted)) uint32_t Mounted;
    uint32_t get_Mounted() const { return mounted_; }
    void put_Mounted(uint32_t val) { mounted_ = val; }

    __declspec(property(get = get_LUID)) u16string LUID;
    u16string& get_LUID() { return LUID_; }

    __declspec(property(get = get_ClientStream)) CNetworkStream *ClientStream;
    CNetworkStream *get_ClientStream() const
    {
        return clientStream_;
    }

    __declspec(property(get = get_DataBuffer)) vector<BlockStoreByte>& DataBuffer;
    vector<BlockStoreByte>& get_DataBuffer()
    {
        return dataBuffer_;
    }

    __declspec(property(get = get_UseDMA)) bool UseDMA;
    bool get_UseDMA()
    {
        return useDMA_;
    }

    __declspec(property(get = get_SRBAddress)) void* SRBAddress;
    void* get_SRBAddress()
    {
        return reinterpret_cast<void*>(addressSRB_);
    }

    __declspec(property(get = get_TrackingId, put = put_TrackingId)) uint64_t TrackingId;
    uint64_t get_TrackingId(){ return trackingId_; }
    void put_TrackingId(uint64_t actualTrackingId){ trackingId_ = actualTrackingId; }

    __declspec(property(get = get_ActualDataLength, put = put_ActualDataLength)) uint32_t ActualDataLength;
    uint32_t get_ActualDataLength() { return actualDataLength_; }
    void put_ActualDataLength(uint32_t actualDataLength) { actualDataLength_ = actualDataLength; }

    bool IsEndOfPayloadMarkerValid() const
    {
        return endMarker_ == BlockStore::Constants::MarkerEndOfPayload;
    }

    void Reset();
    shared_ptr<vector<BlockStoreByte>> ToBytes();
    static task<shared_ptr<BlockStorePayload>> CreateInstanceFromStream(CNetworkStream &stream);
    void HandlePayloadProcessException(const std::exception& refException);

    static uint32_t ReadUInt32LE(const BlockStoreByte **data);
    static uint64_t ReadUInt64LE(const BlockStoreByte **data);
    static void WriteUInt16LE(uint16_t val, vector<BlockStoreByte> *vec);
    static void WriteUInt32LE(uint32_t val, vector<BlockStoreByte> *vec);
    static void WriteUInt64LE(uint64_t val, vector<BlockStoreByte> *vec);

private:
    void ParsePayloadHeader(const BlockStoreByte *pBuffer);
    
};
