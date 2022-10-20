// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------
// BlockStore Service Request implementation.

// TODO: Review this.
// Silence warnings for stdenv
#define _CRT_SECURE_NO_WARNINGS

#include "stdafx.h"
#include <codecvt>
#include <locale>
#include "../inc/CBlockStoreRequest.h"
#include "../inc/CNetworkStream.h"
#if defined(PLATFORM_UNIX)
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#endif

using namespace std;
using namespace Common;
using boost::asio::ip::tcp;
using namespace service_fabric;
using namespace BlockStore;

// Logs the session details
void CBlockStoreRequest::LogSessionStart()
{
    TRACE_NOISE("Session = " << this);
}

void CBlockStoreRequest::LogSessionEnd()
{
    TRACE_NOISE("Session = " << this);
}


// Processes the incoming request by dispatching it on one of the IOService threads so that
// its caller can resume its processing without get blocked. 
void CBlockStoreRequest::ProcessRequest()
{
    // fire a work item to the thread pool to do real work
    // we can't block the async accept
    auto self(shared_from_this());
    Socket.get_io_service().dispatch([this, self]() {
        // this is a coroutine task<T>, but we are not blocking/awaiting on it
        // await on any async task will fire the task and we'll suspend/resume as needed
        ProcessRequestAsync(self);
    });
}

bool CBlockStoreRequest::IsConnectionClosed(boost::system::system_error &err)
{
    auto code = err.code();
    if (code.category() == boost::asio::error::misc_category &&
        code.value() == boost::asio::error::eof)
        return true;
    
    if (code.category() == boost::asio::error::system_category)
    {
        if (code.value() == boost::asio::error::connection_aborted ||
            code.value() == boost::asio::error::connection_reset)
        {
            return true;
        }
    }

    return false;
}

// Actual processing of an incoming request happens here.
task<void> CBlockStoreRequest::ProcessRequestAsync(shared_ptr<CBlockStoreRequest> self)
{
    TRACE_NOISE("Session = " << this << "; ID:" << tracking << "; Socket = " << &this->socket_ << "; RemotePort = " << this->socket_.remote_endpoint().port());

    // Initialize the stream instance pointing to the payload header buffer.
    CNetworkStream stream(&self->Socket, self.get());
    
    try
    {
        //
        // Read the BlockStore payload header from the stream.
        //
        shared_ptr<BlockStorePayload> payload = co_await BlockStorePayload::CreateInstanceFromStream(stream);
   
	    //Update tracking.
	    tracking = payload->TrackingId; 

        try
        {
            co_await ProcessRequestPayload(payload.get());
        }
        catch (std::exception &err)
        {
            payload->HandlePayloadProcessException(err);
        }

        // Fetch the byte array corresponding to the response payload
        // and send it on it's way.
        auto bytes = payload->ToBytes();
        
        size_t bytesToSend = bytes->size();
        BlockStoreByte* pDataToSend = bytes->data();

        // Stream may not send all the bytes to the peer, so loop until all the data
        // has been sent.
        //
        while (bytesToSend > 0)
        {
            size_t bytesSent = co_await stream.Write(pDataToSend, bytesToSend, tracking);
            bytesToSend -= bytesSent;
            pDataToSend += bytesSent;
        }
        payload->Reset();

        // Client will close the socket.
        TRACE_NOISE("ID:" << tracking << "; Session = " << this << "; Socket = " << &this->socket_);
    }
    catch (boost::system::system_error &err)
    {
        if (!IsConnectionClosed(err))
        {
            TRACE_ERROR(err.what());
        }

        stream.CloseSocket();
        TRACE_ERROR("ID:" << tracking << "; system_error Session = " << this << "; Socket = " << &this->socket_);
    }
    catch (std::exception &ex)
    {
        TRACE_ERROR("ID:" << tracking << "; Exception " << ex.what());

        stream.CloseSocket();
        TRACE_ERROR("ID:" << tracking << "; Exception Session = " << this << "; Socket = " << &this->socket_);
    }

    co_return;
}

// Helper to convert unsigned 64bit integer to u16string
u16string CBlockStoreRequest::UInt64ToUTF16String(uint64_t value)
{
    if (value == 0)
        return u"0";

    uint64_t origValue = value;
    uint32_t size = 0;
    while (value > 0)
    {
        size++;
        value /= 10;
    }

    u16string str;
    str.resize(size);

    value = origValue;
    for (uint32_t i = 0; i < size; ++i)
    {
        str[size - i - 1] = (short)u'0' + (value % 10);
        value /= 10;
    }

    return str;

}

task<void> CBlockStoreRequest::InitLURegistration(BlockStorePayload *payload)
{
    payload->DataBuffer.resize(payload->DataBufferLength);
    co_await payload->ClientStream->Read(&payload->DataBuffer[0], payload->DataBufferLength, payload->TrackingId);

    const BlockStoreByte *pBuffer = payload->DataBuffer.data();

    // Read the LU registration information
    payload->LengthLUID = BlockStorePayload::ReadUInt32LE(&pBuffer);

    // Read the Disk size
    payload->DiskSize = BlockStorePayload::ReadUInt32LE(&pBuffer);

    // Read the Disk size unit
    payload->DiskSizeUnit = BlockStorePayload::ReadUInt32LE(&pBuffer);

    // Read the Mount status
    payload->Mounted = BlockStorePayload::ReadUInt32LE(&pBuffer);

    // Read the LUID
    payload->LUID.assign(
        reinterpret_cast<const char16_t *>(pBuffer),
        payload->LengthLUID / sizeof(char16_t) - 1    // skip the /0 at the end
    );

    // Move past the LUID.
    pBuffer += payload->LengthLUID;

    co_return;
}

task<bool> CBlockStoreRequest::RegisterLU(BlockStorePayload *payload)
{
    auto context = reliableService_.get_context(0);
#if defined(PLATFORM_UNIX)
    shared_ptr<bool> fRetVal = make_shared<bool>(false);
    co_await context.execute([=](auto &txn) mutable -> task<void>
#else
    bool fRetVal = false;
    co_await context.execute([&](auto &txn) -> task<void> 
#endif
    {
        auto map = co_await context.get_reliable_map<u16string, vector<BlockStoreByte>>(payload->DeviceId.c_str(), BlockStore::Constants::DefaultTransactionTimeoutSeconds);
        auto map_holder = map.bind(txn, BlockStore::Constants::DefaultTransactionTimeoutSeconds);

        u16string blockKey = payload->LUID;
        vector<BlockStoreByte> registrationInfo;

        bool exists = co_await map_holder.get(blockKey, &registrationInfo);
        TRACE_INFO("ID:" << payload->TrackingId << " DiskExists:" << exists);
        if (!exists)
        {
            string registrationInfoString = to_string(payload->DiskSize) + "_" + to_string(payload->DiskSizeUnit) + "_" + to_string(payload->Mounted);
            registrationInfo.assign(registrationInfoString.begin(), registrationInfoString.end());
            co_await map_holder.add(blockKey, registrationInfo);

#if defined(PLATFORM_UNIX)
            *fRetVal = true;
#else
            fRetVal = true;
#endif
        }
    });

#if defined(PLATFORM_UNIX)
    co_return *fRetVal;
#else
    co_return fRetVal;
#endif
}

task<bool> CBlockStoreRequest::UnregisterLU(BlockStorePayload *payload)
{
#if defined(PLATFORM_UNIX)
    shared_ptr<bool> fRetVal = make_shared<bool>(false);
#else
    bool fRetVal = false;
#endif
    auto context = reliableService_.get_context(0);

    // First, delete the reliable map containing the disk blocks
    try
    {
        co_await context.remove_reliable_map(payload->LUID.data(), BlockStore::Constants::DefaultTransactionTimeoutSeconds);
    }
    catch (...)
    {

    }

    // Next, remove the entry from the LURegistrar
#if defined(PLATFORM_UNIX)
    co_await context.execute([=](auto &txn) mutable -> task<void> 
#else
    co_await context.execute([&](auto &txn) -> task<void> 
#endif
    {
        auto map = co_await context.get_reliable_map<u16string, vector<BlockStoreByte>>(payload->DeviceId.c_str(), BlockStore::Constants::DefaultTransactionTimeoutSeconds);
        auto map_holder = map.bind(txn, BlockStore::Constants::DefaultTransactionTimeoutSeconds);

        u16string blockKey = payload->LUID;
        vector<BlockStoreByte> registrationInfo;

        bool exists = co_await map_holder.get(blockKey, &registrationInfo);
        if (exists)
        {
            co_await map_holder.remove(blockKey);
#if defined(PLATFORM_UNIX)
            *fRetVal = true;
#else
            fRetVal = true;
#endif
        }
    });
#if defined(PLATFORM_UNIX)
    co_return *fRetVal;
#else
    co_return fRetVal;
#endif
}

task<bool> CBlockStoreRequest::MountLU(BlockStorePayload *payload)
{
    bool fRetVal = false;
    auto context = reliableService_.get_context(0);

#if defined(PLATFORM_UNIX)
    co_await context.execute([=](auto &txn) mutable -> task<void> 
#else
    co_await context.execute([&](auto &txn) -> task<void> 
#endif
    {

        auto map = co_await context.get_reliable_map<u16string, vector<BlockStoreByte>>(payload->DeviceId.c_str(), BlockStore::Constants::DefaultTransactionTimeoutSeconds);
        auto map_holder = map.bind(txn, BlockStore::Constants::DefaultTransactionTimeoutSeconds);

        u16string blockKey = payload->LUID;
        vector<BlockStoreByte> registrationInfo;

        // Get the current registration info
        bool exists = co_await map_holder.get(blockKey, &registrationInfo);
        if (exists)
        {
            string registrationInfoString(registrationInfo.begin(), registrationInfo.end());

            // Get the actual registration info
            vector<string> arrData;
            Split(registrationInfoString, arrData, "_");

            // Update Mounted field
            registrationInfoString = arrData[0] + "_" + arrData[1] + "_" + "1";
            registrationInfo.assign(registrationInfoString.begin(), registrationInfoString.end());

            // Update reliable map
            co_await map_holder.update(blockKey, registrationInfo);

            fRetVal = true;
        }
    });
    co_return fRetVal;
}

task<bool> CBlockStoreRequest::UnmountLU(BlockStorePayload *payload)
{
    bool fRetVal = false;

    auto context = reliableService_.get_context(0);
#if defined(PLATFORM_UNIX)
    co_await context.execute([=](auto &txn) mutable -> task<void> 
#else
    co_await context.execute([&](auto &txn) -> task<void> 
#endif
    {

        auto map = co_await context.get_reliable_map<u16string, vector<BlockStoreByte>>(payload->DeviceId.c_str(), BlockStore::Constants::DefaultTransactionTimeoutSeconds);
        auto map_holder = map.bind(txn, BlockStore::Constants::DefaultTransactionTimeoutSeconds);

        u16string blockKey = payload->LUID;
        vector<BlockStoreByte> registrationInfo;

        // Get the current registration info
        bool exists = co_await map_holder.get(blockKey, &registrationInfo);
        if (exists)
        {
            string registrationInfoString(registrationInfo.begin(), registrationInfo.end());

            // Get the actual registration info
            vector<string> arrData;
            Split(registrationInfoString, arrData, "_");

            // Update Mounted field
            registrationInfoString = arrData[0] + "_" + arrData[1] + "_" + "0";
            registrationInfo.assign(registrationInfoString.begin(), registrationInfoString.end());

            // Update reliable map
            co_await map_holder.update(blockKey, registrationInfo);

            fRetVal = true;
        }
    });

    co_return fRetVal;
}

task<void> CBlockStoreRequest::ManageLURegistration(BlockStorePayload *payload)
{
    bool fRetVal = false;

    // Fetch the LU registration data from the payload sent to us.
    co_await InitLURegistration(payload);

    string sLUID = BlockStore::StringUtil::ToString(payload->LUID, true);

    // Process the command by mode
    if (payload->Mode == BlockStore::Mode::Enum::RegisterLU)
    {
        TRACE_INFO("Registering LU, " << sLUID << ", of size " << payload->DiskSize << " " << payload->DiskSizeUnit);
        fRetVal = co_await RegisterLU(payload);
    }
    else if (payload->Mode == BlockStore::Mode::Enum::UnregisterLU)
    {
        TRACE_INFO("Unregistering LU, " << sLUID);
        fRetVal = co_await UnregisterLU(payload);
    }
    else if ((payload->Mode == BlockStore::Mode::Enum::MountLU) || (payload->Mode == BlockStore::Mode::Enum::UnmountLU))
    {
        TRACE_INFO("Setting mount status of LU, " << sLUID << ", to " << payload->Mounted);

        if (payload->Mode == BlockStore::Mode::Enum::MountLU)
            fRetVal = co_await MountLU(payload);
        else
            fRetVal = co_await UnmountLU(payload);
    }

    if (fRetVal)
    {
        TRACE_INFO("Registration management of LU, " << sLUID << ", succeeded.");
        payload->Mode = BlockStore::Mode::OperationCompleted;
    }
    else
    {
        TRACE_ERROR("Registration management of LU, " << sLUID << ", failed.");
        payload->Mode = BlockStore::Mode::OperationFailed;
    }

    co_return;
}

// Processes the BlockStore request based upon the payload header
task<void> CBlockStoreRequest::ProcessRequestPayload(BlockStorePayload *payload)
{
    // If the EndOfPayload marker is invalid, fail the processing of the payload
    if (!payload->IsEndOfPayloadMarkerValid())
    {
        throw std::runtime_error("Invalid EndOfPayload marker received.");
    }
    
    if (payload->Mode == BlockStore::Mode::Enum::FetchRegisteredLUList)
    {
        co_await FetchRegisteredLUList(payload);
    }
    else if ((payload->Mode == BlockStore::Mode::Enum::RegisterLU) || (payload->Mode == BlockStore::Mode::Enum::UnregisterLU) || (payload->Mode == BlockStore::Mode::Enum::MountLU) || (payload->Mode == BlockStore::Mode::Enum::UnmountLU))
    {
        co_await ManageLURegistration(payload);
    }
    else if (payload->Mode == BlockStore::Mode::Enum::Read)
    {
        // Read block data from reliable_map
        TRACE_NOISE("ID:" << payload->TrackingId << " Calling ReadBlocks");
        co_await ReadBlocks(payload);
    }
    else if (payload->Mode == BlockStore::Mode::Enum::Write)
    {
        // Write block data
        TRACE_NOISE("ID:" << payload->TrackingId << " Calling WriteBlocks");
        co_await WriteBlocks(payload);
    }
    else
    {
        throw std::runtime_error("Invalid BlockMode in the payload header.");
    }

    if (payload->Mode != BlockStore::Mode::Enum::OperationCompleted)
    {
        TRACE_ERROR("ID:" << payload->TrackingId << " ,mode=" << payload->Mode << ",offsetToRW_=" << payload->OffsetToRW << ",dataBufferLength_=" << payload->DataBufferLength << ",deviceIdLength_=" << payload->DeviceIdLength << ",addressSRB_=" << payload->SRBAddress << ",deviceId_=" << string(payload->DeviceId.begin(), payload->DeviceId.end()) << ",endMarker_=" << payload->EndMarker);
    }

    co_return;
}

// Helper function to read block from the reliable_map under a transaction
task<void> CBlockStoreRequest::ReadBlock(u16string deviceId, uint64_t offsetToReadFrom, uint32_t bytesToRead, BlockStoreByte *readBuffer, uint32_t readIntoIndex)
{
    // If we are reading aligned block size, then our read offset should be aligned.
    if (bytesToRead == BlockStore::Constants::SizeDiskBlock)
    {
        if ((offsetToReadFrom % BlockStore::Constants::SizeDiskBlock) != 0)
        {
            throw std::runtime_error("Invalid read offset for block sized read!");
        }
    }
    TRACE_NOISE("ID:" << tracking << " readBuffer:" << (void *)&readBuffer[0]<< " readBuffer+Index:" << (void *)(((BlockStoreByte *)(&readBuffer[0])) + readIntoIndex) << " bytesToRead:" << bytesToRead);

    // @TODO - Read the key
    auto context = reliableService_.get_context(0);

#if defined(PLATFORM_UNIX)
    co_await context.execute([=](auto &txn) mutable -> task<void>
#else
    co_await context.execute([&](auto &txn) -> task<void>
#endif
    {
        auto map = co_await context.get_reliable_map<u16string, vector<BlockStoreByte>>(deviceId.c_str(), BlockStore::Constants::DefaultTransactionTimeoutSeconds);
        auto map_holder = map.bind(txn, BlockStore::Constants::DefaultTransactionTimeoutSeconds);

        u16string blockKey = u"Block_";
        uint64_t iBlockId = offsetToReadFrom / BlockStore::Constants::SizeDiskBlock;
        u16string blockId = UInt64ToUTF16String(iBlockId);
        blockKey.append(blockId);

        uint64_t readOffsetWithinBlock = offsetToReadFrom % BlockStore::Constants::SizeDiskBlock;

        // TODO: Read into readBuffer directly instead of using readData.
        vector<BlockStoreByte> readData;
        bool exists = co_await map_holder.get(blockKey, &readData);
        if (!exists)
        {
            // The block doesn't exist, so treat the read as reading a zeroed out block.
            TRACE_NOISE("ID:" << tracking << " readBuffer:" << (void *)&readBuffer[0]<< " readBuffer+Index:" << (void *)(((BlockStoreByte *)(&readBuffer[0])) + readIntoIndex) << " bytesToRead:" << bytesToRead);
            memset((BlockStoreByte *)(&readBuffer[0]) + readIntoIndex, 0, bytesToRead);
        }
        else
        {
            TRACE_NOISE("ID:" << tracking << " readBuffer:" << (void *)&readBuffer[0]<< " readBuffer+Index:" << (void *)(((BlockStoreByte *)(&readBuffer[0])) + readIntoIndex) << " bytesToRead:" << bytesToRead);
            if (readData.size() != BlockStore::Constants::SizeDiskBlock)
            {
                TRACE_ERROR("Read a block that is not size-aligned: " << readData.size() << "!");
                throw std::runtime_error("Read a block that is not size-aligned!");
            }
            else
            {
                // Copy the data to the specified location in the buffer from the intended offset on the disk.
                memcpy(((BlockStoreByte *)(&readBuffer[0])) + readIntoIndex, &readData[readOffsetWithinBlock], bytesToRead);
            }
        }
    });
    co_return;
}

// Reads the data from the blocks corresponding to the incoming request.
task<void> CBlockStoreRequest::ReadBlocks(BlockStorePayload *payload)
{
    // Setup initial state to read the blocks
    uint64_t offsetToReadFrom = payload->OffsetToRW;
    uint64_t endOffset = offsetToReadFrom + payload->DataBufferLength;
    uint32_t bytesToReadInCurrentBlock = 0;
    uint32_t readIntoIndex = 0;
    vector <task<void>> readTasks;
    bool successfulRead = true;

    std::string usingDMA(".");
    if (payload->UseDMA)
    {
        usingDMA = " using DMA";
    }

    TRACE_NOISE("ID:" << payload->TrackingId << " \"" << BlockStore::StringUtil::ToString(payload->DeviceId, true) << "\": Reading from offset " << offsetToReadFrom << " for length " << payload->DataBufferLength << usingDMA);

    // Fail if DMA cannot be used
    if (payload->UseDMA)
    {
        if (DriverClient == nullptr)
        {
            throw std::runtime_error("Unable to use DMA without driver client!");
        }
    }

    try
    {
        // Resize the data buffer to read the blocks into.
        payload->DataBuffer.resize(payload->DataBufferLength);

        while (offsetToReadFrom < endOffset)
        {
            // Compute the number of bytes to be read in the current block
            bytesToReadInCurrentBlock = BlockStore::Constants::SizeDiskBlock - (offsetToReadFrom % BlockStore::Constants::SizeDiskBlock);
            if ((offsetToReadFrom + bytesToReadInCurrentBlock) >= endOffset)
            {
                // We cannot go past the intended length, so adjust the read size accordingly
                bytesToReadInCurrentBlock = (uint32_t)(endOffset - offsetToReadFrom);
            }

            task<void> t = ReadBlock(payload->DeviceId, offsetToReadFrom, bytesToReadInCurrentBlock, &(payload->DataBuffer[0]), readIntoIndex);
            readTasks.push_back(t);

            // Move to the next read
            offsetToReadFrom += bytesToReadInCurrentBlock;
            readIntoIndex += bytesToReadInCurrentBlock;
        }

        // Wait for all the read tasks to complete
        for (auto &task : readTasks)
        {
            co_await task;
        }
    }
    catch (std::exception& ex)
    {
        TRACE_ERROR("ID:" << payload->TrackingId << " \"" << BlockStore::StringUtil::ToString(payload->DeviceId, true) << "\": Exception when reading from offset " << offsetToReadFrom << " for length " << payload->DataBufferLength << usingDMA << ": " << ex.what());
        successfulRead = false;
    }

    if (payload->UseDMA)
    {
        if (successfulRead)
        {
            uint32_t errorDMA = 0;
            successfulRead = DriverClient->CopyBlockToSRB(payload->SRBAddress, &(payload->DataBuffer[0]), payload->DataBufferLength, &errorDMA);

            if (!successfulRead)
            {
                TRACE_ERROR("ID:" << payload->TrackingId << " \"" << BlockStore::StringUtil::ToString(payload->DeviceId, true) << "\": Failed to read from offset " << offsetToReadFrom << ", due to error " << errorDMA << ", for length " << payload->DataBufferLength << usingDMA);
            }
        }

        // When using DMA, there is no outgoing buffer
        payload->DataBufferLength = 0;
        payload->DataBuffer.resize(0);
    }

    if (successfulRead)
    {
        TRACE_NOISE("ID:" << payload->TrackingId << " \"" << BlockStore::StringUtil::ToString(payload->DeviceId, true) << "\": Successfully read from offset " << offsetToReadFrom << " for length " << payload->DataBufferLength << usingDMA);
        payload->Mode = BlockStore::Mode::Enum::OperationCompleted;
    }
    else
    {
        TRACE_ERROR("ID:" << payload->TrackingId << " \"" << BlockStore::StringUtil::ToString(payload->DeviceId, true) << "\": Failed to read from offset " << offsetToReadFrom << " for length " << payload->DataBufferLength << usingDMA);
        payload->Mode = BlockStore::Mode::Enum::OperationFailed;
    }
    co_return;
}

// Helper function to write the block to the reliable_map under a transaction
task<void> CBlockStoreRequest::WriteBlock(u16string deviceId, uint64_t offsetToWriteFrom, uint32_t bytesToWrite, const BlockStoreByte *writeBuffer, uint32_t writeFromIndex)
{
    // If we are writing aligned block size, then our write offset should be aligned.
    if (bytesToWrite == BlockStore::Constants::SizeDiskBlock)
    {
        if ((offsetToWriteFrom % BlockStore::Constants::SizeDiskBlock) != 0)
        {
            throw std::runtime_error("Invalid write offset for block sized write!");
        }
    }

    auto context = reliableService_.get_context(0);
#if defined(PLATFORM_UNIX)
    co_await context.execute([=](auto &txn) mutable -> task<void>
#else
    co_await context.execute([&](auto &txn) -> task<void>
#endif
    {
        auto map = co_await context.get_reliable_map<u16string, vector<BlockStoreByte>>(deviceId.c_str(), BlockStore::Constants::DefaultTransactionTimeoutSeconds);
        auto map_holder = map.bind(txn, BlockStore::Constants::DefaultTransactionTimeoutSeconds);

        u16string blockKey = u"Block_";
        uint64_t iBlockId = offsetToWriteFrom / BlockStore::Constants::SizeDiskBlock;
        u16string blockId = UInt64ToUTF16String(iBlockId);
        blockKey.append(blockId);

        uint64_t writeOffsetWithinBlock = offsetToWriteFrom % BlockStore::Constants::SizeDiskBlock;

        pair<int64_t, vector<BlockStoreByte>> pairData;

        bool updated = false;
        uint8_t retry = 0;
        while (!updated && retry++ < BlockStore::Constants::MaxUpdateRetry)
        {
            bool exists = co_await map_holder.get(blockKey, &pairData, Store_LockMode::Store_LockMode_Update);
            if (exists)
            {
                // We should have read block-sized data
                if (pairData.second.size() != BlockStore::Constants::SizeDiskBlock)
                {
                    TRACE_ERROR("Read a block that is not size-aligned: " << pairData.second.size() << "!");
                    throw std::runtime_error("Read a block that is not size-aligned!");
                }

                // Block data already exists, so overwrite the the contents from the intended offset.
                memcpy(&pairData.second[writeOffsetWithinBlock], ((BlockStoreByte *)(&writeBuffer[0])) + writeFromIndex, bytesToWrite);
                updated = co_await map_holder.update(blockKey, pairData);
                if (!updated)
                {
                    TRACE_ERROR("update fail! " 
                        << "retry:" << (uint32_t)retry
                        << ",deviceID:" << boost::locale::conv::utf_to_utf<char>(deviceId)
                        << ",offsetToWriteFrom:" << offsetToWriteFrom
                        << ",bytesToWrite:" << bytesToWrite
                        << ",writeBuffer:" << (uint64_t)writeBuffer
                        << ",writeFromIndex:" << writeFromIndex
                        << ",version:" << pairData.first << endl);
                }
            }
            else
            {
                // The block does not exist, so resize the buffer
                // and copy the data to the target index in the buffer.
                vector<BlockStoreByte> writeData;
                writeData.resize(BlockStore::Constants::SizeDiskBlock);
                memcpy(&writeData[writeOffsetWithinBlock], ((BlockStoreByte *)(&writeBuffer[0])) + writeFromIndex, bytesToWrite);
                co_await map_holder.add(blockKey, writeData);
                updated = true;
            }
        }

        if (!updated)
        {
            std::string errorMesg("Unable to update existing block with ID ");
            errorMesg.append(std::to_string(iBlockId));
            errorMesg.append(",retry:" + to_string(retry));
            throw std::runtime_error(errorMesg.c_str());
        }
    });
    co_return;
}

// Writes the data to the blocks corresponding to the incoming request.
task<void> CBlockStoreRequest::WriteBlocks(BlockStorePayload *payload)
{
    // Setup initial state to write the blocks
    uint64_t offsetToWriteFrom = payload->OffsetToRW;
    uint64_t endOffset = offsetToWriteFrom + payload->DataBufferLength;
    uint32_t bytesToWriteInCurrentBlock = 0;
    uint32_t writeFromIndex = 0;
    vector <task<void>> writeTasks;
    bool successfulWrite = true;

    std::string usingDMA(".");
    if (payload->UseDMA)
    {
        usingDMA = " using DMA";
    }

    TRACE_NOISE("ID:" << payload->TrackingId << " \"" << BlockStore::StringUtil::ToString(payload->DeviceId, true) << "\": Writing to offset " << offsetToWriteFrom << " for length " << payload->DataBufferLength << usingDMA);

    // Fail if DMA cannot be used
    if (payload->UseDMA)
    {
        if (DriverClient == nullptr)
        {
            throw std::runtime_error("Unable to use DMA without driver client!");
        }
    }

    try
    {
        // Resize the data buffer to read the data from the stream.
        payload->DataBuffer.resize(payload->DataBufferLength);

        const BlockStoreByte *writeFromBuffer = nullptr;

        if (payload->UseDMA)
        {
            uint32_t errorDMA = 0;
            successfulWrite = DriverClient->CopyBlockFromSRB(payload->SRBAddress, &(payload->DataBuffer[0]), payload->DataBufferLength, &errorDMA);

            if (!successfulWrite)
            {
                TRACE_ERROR("ID:" << payload->TrackingId << "\"" << BlockStore::StringUtil::ToString(payload->DeviceId, true) << "\": Failed to read from offset " << offsetToWriteFrom << ", due to error " << errorDMA << ", for length " << payload->DataBufferLength << usingDMA);
            }
            else
            {
                writeFromBuffer = &(payload->DataBuffer[0]);
            }
        }
        else
        {
            // Fetch the data from the stream that needs to be saved.
            writeFromBuffer = co_await payload->ClientStream->Read(&payload->DataBuffer[0], payload->DataBufferLength, payload->TrackingId);
        }   

        if (successfulWrite)
        {
            _ASSERT(writeFromBuffer != nullptr);

            while (offsetToWriteFrom < endOffset)
            {
                // Compute the number of bytes to be read in the current block
                bytesToWriteInCurrentBlock = BlockStore::Constants::SizeDiskBlock - (offsetToWriteFrom % BlockStore::Constants::SizeDiskBlock);
                if ((offsetToWriteFrom + bytesToWriteInCurrentBlock) >= endOffset)
                {
                    // We cannot go past the intended length, so adjust the write size accordingly
                    bytesToWriteInCurrentBlock = (uint32_t)(endOffset - offsetToWriteFrom);
                }

                task<void> t = WriteBlock(payload->DeviceId, offsetToWriteFrom, bytesToWriteInCurrentBlock, writeFromBuffer, writeFromIndex);
                writeTasks.push_back(t);

                // Move to the next read
                offsetToWriteFrom += bytesToWriteInCurrentBlock;
                writeFromIndex += bytesToWriteInCurrentBlock;
            }

            // Wait for all the write tasks to complete
            for (auto &task : writeTasks)
            {
                co_await task;
            }

            // This TRACE_INFO is shorterm needed for Linux debug
            // TODO: Remove it or change it to TRACE_NOISE
#ifdef PLATFORM_UNIX
            TRACE_INFO("ID:" << payload->TrackingId << " All writes completed.");
#endif
        }
    }
    catch (std::exception& ex)
    {
        TRACE_ERROR("ID:" << tracking << " \"" << BlockStore::StringUtil::ToString(payload->DeviceId, true) << "\": Exception when writing to offset " << offsetToWriteFrom << " for length " << payload->DataBufferLength << usingDMA << ": " << ex.what());
        successfulWrite = false;
    }

    if (successfulWrite)
    {
        TRACE_NOISE("ID:" << tracking << "\"" << BlockStore::StringUtil::ToString(payload->DeviceId, true) << "\": Successfully written to offset " << offsetToWriteFrom << " for length " << payload->DataBufferLength << usingDMA);
        payload->Mode = BlockStore::Mode::Enum::OperationCompleted;
    }
    else
    {
        TRACE_ERROR("ID:" << tracking << " \"" << BlockStore::StringUtil::ToString(payload->DeviceId, true) << "\": Failed to write to offset " << offsetToWriteFrom << " for length " << payload->DataBufferLength << usingDMA);
        payload->Mode = BlockStore::Mode::Enum::OperationFailed;
    }

    // Response to the client only has payload header and no data buffer.
    payload->DataBufferLength = 0;
    co_return;
}

void CBlockStoreRequest::Split(const string& str, vector<string> &tokens, const string& delimiters, bool trimEmpty)
{
    string::size_type pos, lastPos = 0, length = str.length();

    while (lastPos < length + 1)
    {
        pos = str.find_first_of(delimiters, lastPos);
        if (pos == string::npos)
        {
            pos = length;
        }

        if (pos != lastPos || !trimEmpty)
        {
            tokens.push_back(string(str.data() + lastPos, pos - lastPos));
        }
        lastPos = pos + 1;
    }
}


task<void> CBlockStoreRequest::FetchLURegistrations(BlockStorePayload *payload, vector<pair<u16string, string>> &listLU)
{
    auto context = reliableService_.get_context(0);
#if defined(PLATFORM_UNIX)
    shared_ptr<vector<pair<u16string, string>>> newlistLU = make_shared<vector<pair<u16string, string>>>();
    co_await context.execute([=](auto &txn) mutable -> task<void> 
#else
    co_await context.execute([&](auto &txn) -> task<void> 
#endif
    {

        auto map = co_await context.get_reliable_map<u16string, vector<BlockStoreByte>>(payload->DeviceId.c_str(), BlockStore::Constants::DefaultTransactionTimeoutSeconds);
        auto map_holder = map.bind(txn, BlockStore::Constants::DefaultTransactionTimeoutSeconds);

        // Unit testing code
        // co_await map_holder.add(u16string(u"Disk1"), vector<char>({ '1','_','2','_','3' }));

        auto iter = co_await map_holder.get_keys();

        // Loop through each registration to compose the payload
        while (iter.move_next())
        {
            u16string key = iter.get_key();

            // @PERF - if life time of buffer extends towards end of transaction, we can avoid the copy
            vector<BlockStoreByte> value;
            bool exists = co_await map_holder.get(key, &value);
            if (!exists)
            {
                // Bypass this one and continue
                TRACE_ERROR("key= " << string(key.begin(), key.end()) << " does not exist");
                continue;
            }
#if defined(PLATFORM_UNIX)
            newlistLU->push_back(make_pair(key, string(value.begin(), value.end())));
#else
            listLU.push_back(make_pair(key, string(value.begin(), value.end())));
#endif
        }
    });
#if defined(PLATFORM_UNIX)
    listLU = *newlistLU;
#endif
    co_return;
}



//  Fetch Registered LU List
//  Registrated LU List format is as follows:
//
//  <Number of Entries>
//  LengthLUID            <-- Entry starts here
//  DiskSize
//  DiskSizeUnit
//  Mounted
//  LUID                  <-- Entry ends here
//
// ...repeat


task<void> CBlockStoreRequest::FetchRegisteredLUList(BlockStorePayload *payload)
{
    // Skip over the dummy buffer that accompanies the command.
    payload->DataBuffer.resize(payload->DataBufferLength);
    co_await payload->ClientStream->Read(&payload->DataBuffer[0], payload->DataBufferLength, payload->TrackingId);

    // TODO: re-implement to avoid resize here and end which need to modify function<WriteUInt32LE>
    // Actually the resize here and end doesn't change vector.capacity() which means no memory re-allocation
    payload->DataBuffer.resize(0);

    // Fetch LUList
    vector<pair<u16string, string>> listLU;
    co_await FetchLURegistrations(payload, listLU);

    // Process LUList
    int iLURegistrations = (int)listLU.size();
    if (iLURegistrations > 0)
    {
        // Write the number of registration entries
        if (iLURegistrations > BlockStore::Constants::MaxLUNRegistrations)
        {
            TRACE_INFO("Trimming LU registration list from " << iLURegistrations << " to " << BlockStore::Constants::MaxLUNRegistrations);
            iLURegistrations = BlockStore::Constants::MaxLUNRegistrations;
        }
        BlockStorePayload::WriteUInt32LE(iLURegistrations, &payload->DataBuffer);

        // Loop through each registration to compose the payload
        // Sample: <LUID,RegistrationInfo>, <"Disk1","2_1_1">
        int iIndex = 0;
        for (pair<u16string, string> entry : listLU)
        {
            if (iIndex < iLURegistrations)
            {
                // Add the NULL to the string when sending across the wire.
                u16string szLUID = entry.first + char16_t('\0');

                // Write the length of LUID in bytes (and not characters)
                BlockStorePayload::WriteUInt32LE((uint32_t)szLUID.size() * sizeof(char16_t), &payload->DataBuffer);

                // Get the Disksize and Size unit
                vector<string> arrDiskSizeData;
                CBlockStoreRequest::Split(entry.second, arrDiskSizeData, "_");

                // Format: {0}_{1}_{2}
                _ASSERTE(arrDiskSizeData.size() == BlockStore::Constants::RegistrationInfoFormat);

                // Write the DiskSize
                BlockStorePayload::WriteUInt32LE(stoi(arrDiskSizeData[0]), &payload->DataBuffer);

                // Write the DiskSize Unit
                BlockStorePayload::WriteUInt32LE(stoi(arrDiskSizeData[1]), &payload->DataBuffer);

                // Write the Mounted status
                BlockStorePayload::WriteUInt32LE(stoi(arrDiskSizeData[2]), &payload->DataBuffer);

                TRACE_INFO("LUID=" << BlockStore::StringUtil::ToString(szLUID, true) << ", DiskSize=" << arrDiskSizeData[0] << ", DiskSizeUnit=" << arrDiskSizeData[1] << ", Mounted=" << arrDiskSizeData[2]);

                // Finally, write the LUID
                for (char16_t c : szLUID)
                {
                    BlockStorePayload::WriteUInt16LE(uint16_t(c), &payload->DataBuffer);
                }
                iIndex++;
            }
            else
            {
                break;
            }
        }
        TRACE_INFO("Fetched LU registrations=" << iLURegistrations);
    }
    else
    {
        iLURegistrations = 0;
        BlockStorePayload::WriteUInt32LE(iLURegistrations, &payload->DataBuffer);
        TRACE_INFO("No LU registrations found.");
    }

    // Update the DataBufferLength to reflect the actual size of the content in the outgoing DataBuffer
    payload->ActualDataLength = (uint32_t)payload->DataBuffer.size();

    // Resize the outgoing buffer to be the size expected by the client.
    payload->DataBuffer.resize(BlockStore::Constants::SizeDiskManagementRequestBuffer);

    // Reply with actual data length and mark as completed
    payload->Mode = BlockStore::Mode::Enum::OperationCompleted;
    co_return;
}
