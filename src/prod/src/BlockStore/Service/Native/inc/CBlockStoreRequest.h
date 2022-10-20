// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------
//
// Represents the request received by the BlockStore Service.
#pragma once

#include "common.h"
#include "CNetworkStream.h"
#include "BlockStorePayload.h"
#include "CDriverClient.h"
#include "trace.h"

//
// A TCP session for block store service
// Responsible for keep receiving requests from volume driver and reply to the requests, until the driver
// close the connection
//
class CBlockStoreRequest
    : public std::enable_shared_from_this<CBlockStoreRequest>
{
public:
    CBlockStoreRequest(service_fabric::reliable_service &reliableService, boost::asio::io_service &ioService, CDriverClient* pDriverClient, unsigned short servicePort)
        : reliableService_(reliableService), socket_(ioService), driverClient_(pDriverClient), servicePort_(servicePort)
    {
        LogSessionStart();
    }

    ~CBlockStoreRequest()
    {
        LogSessionEnd();
    }

    __declspec(property(get = get_Socket)) boost::asio::ip::tcp::socket& Socket;
    boost::asio::ip::tcp::socket& get_Socket() { return socket_; }

    void ProcessRequest();
    static void Split(const string& str, vector<string> &tokens, const string& delimiters = " ", bool trimEmpty = false);

private:

    __declspec(property(get = get_DriverClient)) CDriverClient* DriverClient;
    CDriverClient* get_DriverClient() { return driverClient_;  }

    void LogSessionStart();
    void LogSessionEnd();
    bool IsConnectionClosed(boost::system::system_error &err);
    task<void> ProcessRequestAsync(shared_ptr<CBlockStoreRequest> self);
    task<void> ReadBlock(u16string deviceId, uint64_t offsetToReadFrom, uint32_t bytesToRead, BlockStoreByte *readBuffer, uint32_t readIntoIndex);
    task<void> WriteBlock(u16string deviceId, uint64_t offsetToWriteFrom, uint32_t bytesToWrite, const BlockStoreByte *writeBuffer, uint32_t writeFromIndex);
    u16string UInt64ToUTF16String(uint64_t value);
    
    task<void> FetchRegisteredLUList(BlockStorePayload *payload);
    task<void> ManageLURegistration(BlockStorePayload *payload);
    task<void> ProcessRequestPayload(BlockStorePayload *payload);
    task<void> ReadBlocks(BlockStorePayload *payload);
    task<void> WriteBlocks(BlockStorePayload *payload);
    
    task<void> FetchLURegistrations(BlockStorePayload *payload, vector<pair<u16string, string>> &listLU);
    task<void> InitLURegistration(BlockStorePayload *payload);
    task<bool> RegisterLU(BlockStorePayload *payload);
    task<bool> UnregisterLU(BlockStorePayload *payload);
    task<bool> MountLU(BlockStorePayload *payload);
    task<bool> UnmountLU(BlockStorePayload *payload);

    service_fabric::reliable_service &reliableService_;
    boost::asio::ip::tcp::socket socket_;
    CDriverClient* driverClient_;
    unsigned short servicePort_;
    uint64_t tracking;
};
