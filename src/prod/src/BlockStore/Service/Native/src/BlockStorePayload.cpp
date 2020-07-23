// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// BlockStore Service Request implementation.

// TODO: Review this.
// Silence warnings for stdenv
#define _CRT_SECURE_NO_WARNINGS

#include "stdafx.h"
#include "../inc/BlockStorePayload.h"
#include <stdlib.h>
#include <stdio.h>

#if defined(PLATFORM_UNIX)
#include <sys/syscall.h>
#include <unistd.h>
#endif


using namespace std;
using namespace Common;
using namespace BlockStore;

const char* BlockStorePayload::get_ModeDescription()
{
    switch (mode_)
    {
    case Mode::Enum::Read: return "READ_MODE";
    case Mode::Enum::Write: return "WRITE_MODE";
    case Mode::Enum::RegisterLU: return "REGISTER_LU";
    case Mode::Enum::UnregisterLU: return "UNREGISTER_LU";
    case Mode::Enum::MountLU: return "MOUNT_LU";
    case Mode::Enum::UnmountLU: return "UNMOUNT_LU";
    default:
        return "UNKNOWN";
    }
}

// Write little-endian unsigned 16bit integer
void BlockStorePayload::WriteUInt16LE(uint16_t val, vector<BlockStoreByte> *vec)
{
    vec->push_back((val & 0xff));
    vec->push_back((val >> 8) & 0xff);
}

// Read little-endian unsigned 32bit integer
uint32_t BlockStorePayload::ReadUInt32LE(const BlockStoreByte **data)
{
    const BlockStoreByte *buf = reinterpret_cast<const BlockStoreByte *>(*data);

    uint32_t value = static_cast<uint32_t>(buf[0]) |
        static_cast<uint32_t>(buf[1]) << 8 |
        static_cast<uint32_t>(buf[2]) << 16 |
        static_cast<uint32_t>(buf[3]) << 24;

    *data += sizeof(uint32_t);
    return value;
}

// Write little-endian unsigned 32bit integer
void BlockStorePayload::WriteUInt32LE(uint32_t val, vector<BlockStoreByte> *vec)
{
    vec->push_back((val & 0xff));
    vec->push_back((val >> 8) & 0xff);
    vec->push_back((val >> 16) & 0xff);
    vec->push_back((val >> 24) & 0xff);
}

// Read little-endian unsigned 64bit integer
uint64_t BlockStorePayload::ReadUInt64LE(const BlockStoreByte **data)
{
    const BlockStoreByte *buf = reinterpret_cast<const BlockStoreByte *>(*data);

    uint64_t value =
        static_cast<uint64_t>(buf[0]) |
        static_cast<uint64_t>(buf[1]) << 8 |
        static_cast<uint64_t>(buf[2]) << 16 |
        static_cast<uint64_t>(buf[3]) << 24 |
        static_cast<uint64_t>(buf[4]) << 32 |
        static_cast<uint64_t>(buf[5]) << 40 |
        static_cast<uint64_t>(buf[6]) << 48 |
        static_cast<uint64_t>(buf[7]) << 56;

    *data += sizeof(uint64_t);
    return value;
}

// Write little-endian unsigned 64bit integer
void BlockStorePayload::WriteUInt64LE(uint64_t val, vector<BlockStoreByte> *vec)
{
    vec->push_back((val & 0xff));
    vec->push_back((val >> 8) & 0xff);
    vec->push_back((val >> 16) & 0xff);
    vec->push_back((val >> 24) & 0xff);
    vec->push_back((val >> 32) & 0xff);
    vec->push_back((val >> 40) & 0xff);
    vec->push_back((val >> 48) & 0xff);
    vec->push_back((val >> 56) & 0xff);
}

void BlockStorePayload::Reset()
{
    payloadHeaderParsed_ = false;

    mode_ = BlockStore::Mode::Enum::OperationFailed;
    offsetToRW_ = 0;
    dataBufferLength_ = 0;
    deviceIdLength_ = 0;
    addressSRB_ = 0;
    trackingId_ = 0;
    deviceId_.clear();
    endMarker_ = 0;

    vector<BlockStoreByte>().swap(dataBuffer_);
    actualDataLength_ = 0;
}

// Parses the specified buffer as payload header
void BlockStorePayload::ParsePayloadHeader(const BlockStoreByte *pBuffer)
{
    _ASSERT(pBuffer != nullptr);
    
    TRACE_NOISE("Parsing payload header");

    // Reset the header fields
    Reset();

    // Read the Mode
    mode_ = (BlockStore::Mode::Enum)ReadUInt32LE(&pBuffer);

    // Skip over the ActualPayloadSize field
    ReadUInt32LE(&pBuffer);

    // Read the offset to R/W from
    offsetToRW_ = ReadUInt64LE(&pBuffer);
    
    // Read the size of the data buffer accompanying the payload
    dataBufferLength_ = ReadUInt32LE(&pBuffer);

    // Read the length of the LUN (aka Disk) ID.
    deviceIdLength_ = ReadUInt32LE(&pBuffer);

    // Read the SRB Address
    addressSRB_ = ReadUInt64LE(&pBuffer);
    useDMA_ = (addressSRB_ > 0) ? true : false;

#if defined(PLATFORM_UNIX)
    // Read Tracking Id
    trackingId_ = ReadUInt64LE(&pBuffer);
#endif

    // Read the LUN (aka Disk) ID string.
    deviceId_.assign(
        reinterpret_cast<const char16_t *>(pBuffer),
        deviceIdLength_ / sizeof(char16_t) - 1    // skip the /0 at the end
    );

    // Move past the LUN ID.
    pBuffer += deviceIdLength_;

    // Read the EndOfPayload marker
    endMarker_ = ReadUInt32LE(&pBuffer);

    TRACE_NOISE("ID=" << trackingId_ << ",mode=" << mode_ << ",offsetToRW_=" << offsetToRW_ << ",dataBufferLength_=" << dataBufferLength_ << ",deviceIdLength_=" << deviceIdLength_ << ",addressSRB_=" << addressSRB_ << ",deviceId_=" << string(deviceId_.begin(), deviceId_.end()) <<",endMarker_=" << endMarker_);

    // Mark the header as parsed
    payloadHeaderParsed_ = true;

    // Resize the data buffer per the incoming payload length
    dataBuffer_.resize(dataBufferLength_);
}

// Reads the payload header from the specified stream and returns an instance of BlockStorePayload
// representing it.
task<shared_ptr<BlockStorePayload>> BlockStorePayload::CreateInstanceFromStream(CNetworkStream &stream)
{
    auto payload = std::make_shared<BlockStorePayload>();

    // Read the payload header from the stream and parse it.
    TRACE_NOISE("Reading payload header from the stream");

    // Don't have the tracking id yet, passing 0 here.
    const BlockStoreByte *data = co_await stream.Read(payload->payloadHeaderBuffer_, sizeof(payload->payloadHeaderBuffer_), 0);

    payload->ParsePayloadHeader(data);

    payload->clientStream_ = &stream;

    co_return payload;
}

// Transforms the BlockStorePayload instance to BlockStoreByte array
// that can be sent back to the driver as a response.
shared_ptr<vector<BlockStoreByte>> BlockStorePayload::ToBytes()
{
    // TODO: Payload buffer allocation here is being done again even though the 
    //       R/W buffer was already sized once. Avoid this.
    //
    // TODO: We should have a statically allocated buffer that can be sent for failure cases.
    auto payloadBuffer = std::make_shared<vector<BlockStoreByte>>();

    // Write the mode
    WriteUInt32LE((uint32_t)mode_, payloadBuffer.get());

    // Write the padding
    WriteUInt32LE(BlockStore::Constants::Padding, payloadBuffer.get());

    // Write the offset we did R/W from.
    WriteUInt64LE(offsetToRW_, payloadBuffer.get());

    // Write the length of data in data buffer. 
    // By default, it is the DataBufferLength unless ActualDataLength has been explicitly specified.
    uint32_t responseDataLength = DataBufferLength;
    if (ActualDataLength > 0)
    {
        responseDataLength = ActualDataLength;
    }

    WriteUInt32LE(responseDataLength, payloadBuffer.get());

    // Write the length of the LUN (aka Device) ID.
    WriteUInt32LE(deviceIdLength_, payloadBuffer.get());

    // Write the SRB address
    WriteUInt64LE(addressSRB_, payloadBuffer.get());

#if defined(PLATFORM_UNIX)
    // Write the tracking ID
    WriteUInt64LE(trackingId_, payloadBuffer.get());
#endif

    // Write the LUN (aka Device) ID
    for (int i = 0; i < deviceId_.length(); ++i)
    {
        char16_t ch = deviceId_[i];
        payloadBuffer->push_back(ch & 0xff);
        payloadBuffer->push_back((ch >> 8) & 0xff);
    }

    // Write the terminating NULL for the LUN ID
    payloadBuffer->push_back(0);
    payloadBuffer->push_back(0);

    // Write the EndOfPayload marker
    WriteUInt32LE(BlockStore::Constants::MarkerEndOfPayload, payloadBuffer.get());

    // The size of the buffer for outgoing payload is the sum of the payload header and any data we are going to send.
    // So, resize it accordingly.
    payloadBuffer->resize(BlockStore::Constants::SizeBlockStoreRequestPayloadHeader + dataBufferLength_);

    // Finally, write the data buffer if applicable.
    if (dataBufferLength_ > 0)
    {
        if (dataBuffer_.size() > 0)
        {
            // Copy the data for the response.
            memcpy(&((*payloadBuffer)[BlockStore::Constants::SizeBlockStoreRequestPayloadHeader]), &dataBuffer_[0], dataBufferLength_);
        }
    }

    return payloadBuffer;
}

// Generates the response payload for exceptional case.
void BlockStorePayload::HandlePayloadProcessException(const std::exception& refException)
{
    TRACE_ERROR("\"" << BlockStore::StringUtil::ToString(DeviceId, true) << "\":" << " ID=" << trackingId_ << " Mode=" << Mode << ", OffsetToRW=" << OffsetToRW << ", Length=" << DataBufferLength << ", Marker=" << endMarker_ << ", Exception=" << refException.what() << endl );

    // When building the payload, default the size of response data buffer to be that
    // expected by the disk management commands.
    //
    // TODO: We should look into fixing this.
    uint32_t defaultResponseDataBufferSize = BlockStore::Constants::SizeDiskManagementRequestBuffer;
    if (Mode == BlockStore::Mode::Enum::Read)
    {
        if (UseDMA)
        {
            // When using DMA, we do not have any databuffer to respond with.
            defaultResponseDataBufferSize = 0;
        }
        else
        {
            defaultResponseDataBufferSize = DataBufferLength;
        }
    }
    else if (Mode == BlockStore::Mode::Enum::Write)
    {
        // Receiver only expects the payload header
        defaultResponseDataBufferSize = 0;
    }
    else
    {
        // All other requests are disk management requests, for which we have set the
        // size already.
    }

    // Set the data buffer length that is expected for the response payload.
    DataBufferLength = defaultResponseDataBufferSize;

    // Update the mode to reflect the operation status
    if (!IsEndOfPayloadMarkerValid())
    {
        // Server got an invalid payload
        Mode = BlockStore::Mode::Enum::ServerInvalidPayload;
    }
    else
    {
        Mode = BlockStore::Mode::Enum::OperationFailed;
    }

    // Reset DataBuffer to be null so that we send an dummy
    // buffer back to the client in response to an exception.
    DataBuffer.resize(0);
}
