// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "DnsHelper.h"

const ULONG LocalhostIP = 16777343;

void DnsHelper::CreateDnsServiceHelper(
    __in KAllocator& allocator,
    __in DnsServiceParams& params,
    __out IDnsService::SPtr& spService,
    __out ComServiceManager::SPtr& spServiceManager,
    __out ComPropertyManager::SPtr& spPropertyManager,
    __out FabricData::SPtr& spData,
    __out IDnsParser::SPtr& spParser,
    __out INetIoManager::SPtr& spNetIoManager
)
{
    NullTracer::SPtr spTracer;
    NullTracer::Create(/*out*/spTracer, allocator);

    DNS::CreateDnsParser(/*out*/spParser, allocator);
    DNS::CreateNetIoManager(/*out*/spNetIoManager, allocator, spTracer.RawPtr(), params.NumberOfConcurrentQueries);

    KString::SPtr spName;
    KString::Create(/*out*/spName, allocator, L"fabric:/System/DNS", TRUE/*appendnull*/);

    ComServiceManager::Create(/*out*/spServiceManager, allocator);
    ComPropertyManager::Create(/*out*/spPropertyManager, allocator);

    FabricData::Create(/*out*/spData, allocator);

    IDnsCache::SPtr spCache;
    CreateDnsCache(/*out*/spCache, allocator, params.MaxCacheSize);

    IFabricResolve::SPtr spResolve;
    DNS::CreateFabricResolve(/*out*/spResolve, allocator,
        spName, spTracer.RawPtr(), spData.RawPtr(), spCache.RawPtr(),
        spServiceManager.RawPtr(), spPropertyManager.RawPtr());

    ComFabricStatelessServicePartition2::SPtr spHealth;
    ComFabricStatelessServicePartition2::Create(/*out*/spHealth, allocator);

    DNS::CreateDnsService(
        /*out*/spService,
        allocator,
        spTracer.RawPtr(),
        spParser.RawPtr(),
        spNetIoManager.RawPtr(),
        spResolve.RawPtr(),
        spCache,
        *spHealth.RawPtr(),
        params
    );
}

DNS_STATUS DnsHelper::QueryEx(
    __in KBuffer& buffer,
    __in LPCWSTR wszQuery,
    __in USHORT type,
    __in KArray<DNS_DATA>& results
)
{
    KAllocator& allocator = buffer.GetThisAllocator();

    DNS_STATUS status = 0;

    IDnsService::SPtr spServiceInner;
    DnsServiceSynchronizer dnsServiceSyncInner;
    ComServiceManager::SPtr spServiceManagerInner;
    ComPropertyManager::SPtr spPropertyManagerInner;
    FabricData::SPtr spDataInner;
    IDnsParser::SPtr spParser;
    INetIoManager::SPtr spNetIoManager;

    DnsServiceParams params;
    params.IsRecursiveQueryEnabled = true;
    CreateDnsServiceHelper(allocator, params,
        /*out*/spServiceInner, /*out*/spServiceManagerInner, /*out*/spPropertyManagerInner,
        /*out*/spDataInner, /*out*/spParser, /*out*/spNetIoManager);

    USHORT port = 0;
    if (!spServiceInner->Open(/*inout*/port, static_cast<DnsServiceCallback>(dnsServiceSyncInner)))
    {
        VERIFY_FAIL(L"");
    }

    status = DnsHelper::Query(*spNetIoManager, buffer, *spParser, port, wszQuery, type, results);

    spServiceInner->CloseAsync();
    dnsServiceSyncInner.Wait();

    return status;
}

DNS_STATUS DnsHelper::Query(
    __in INetIoManager& netIoManager,
    __in KBuffer& buffer,
    __in IDnsParser& dnsParser,
    __in USHORT port,
    __in LPCWSTR wszQuery,
    __in WORD type,
    __in KArray<DNS_DATA>& results
)
{
#if !defined(PLATFORM_UNIX)
    UNREFERENCED_PARAMETER(dnsParser);
#endif

    DNS_STATUS status = 0;

    KAllocator& allocator = buffer.GetThisAllocator();

    DWORD dwSize = buffer.QuerySize();
    DnsHelper::SerializeQuestion(dnsParser, wszQuery, buffer, type, /*out*/dwSize);

    NullTracer::SPtr spTracer;
    NullTracer::Create(/*out*/spTracer, allocator);

    IUdpListener::SPtr spListener;
    netIoManager.CreateUdpListener(/*out*/spListener, false /*AllowMultipleListeners*/);

    USHORT portClient = 0;
    DnsServiceSynchronizer syncListener;
    if (!spListener->StartListener(nullptr, /*inout*/portClient, syncListener))
    {
        VERIFY_FAIL_FMT(L"Failed to start UDP listener");
    }

    ISocketAddress::SPtr spAddress;
    DNS::CreateSocketAddress(/*out*/spAddress, allocator, LocalhostIP, htons(port));

    DnsServiceSynchronizer syncWrite;
    spListener->WriteAsync(buffer, dwSize, *spAddress, syncWrite);
    syncWrite.Wait(5000);

    ISocketAddress::SPtr spAddressFrom;
    DNS::CreateSocketAddress(/*out*/spAddressFrom, allocator);
    DnsServiceSynchronizer syncRead;
    spListener->ReadAsync(buffer, *spAddressFrom, syncRead, 5000);
    syncRead.Wait(5000);

    spListener->CloseAsync();
    syncListener.Wait(5000);

#if !defined(PLATFORM_UNIX)
    // From MSDN:
    // The DnsExtractRecordsFromMessage function is designed to operate on messages in host byte order.
    // As such, received messages should be converted from network byte order to host byte order before extraction,
    // or before retransmission onto the network. Use the DNS_BYTE_FLIP_HEADER_COUNTS macro to change byte ordering.
    //
    PDNS_MESSAGE_BUFFER dnsBuffer = (PDNS_MESSAGE_BUFFER)buffer.GetBuffer();
    DNS_BYTE_FLIP_HEADER_COUNTS(&dnsBuffer->MessageHead);
    PDNS_RECORD record = nullptr;
    status = DnsExtractRecordsFromMessage_W(dnsBuffer, (WORD)syncRead.Size(), &record);

    DNS_DATA dnsData;
    while (record != NULL)
    {
        dnsData.Type = record->wType;
        switch (record->wType)
        {
        case DNS_TYPE_A:
        {
            DNS_A_DATA& data = record->Data.A;
            IN_ADDR addr;
            addr.S_un.S_addr = data.IpAddress;
            WCHAR temp[256];
            InetNtop(AF_INET, &addr, temp, ARRAYSIZE(temp));

            dnsData.DataStr = KString::Create(temp, allocator);

            results.Append(dnsData);
        }
        break;
        case DNS_TYPE_SRV:
        {
            DNS_SRV_DATA& data = record->Data.SRV;
            WCHAR temp[1024];
            STRPRINT(temp, ARRAYSIZE(temp), L"%s:%d", data.pNameTarget, data.wPort);
            CharLowerBuff(temp, (DWORD)wcslen(temp));

            dnsData.DataStr = KString::Create(temp, allocator);

            results.Append(dnsData);
        }
        break;
        case DNS_TYPE_TEXT:
        {
            DNS_TXT_DATA& data = record->Data.TXT;
            dnsData.DataStr = KString::Create(KStringView(data.pStringArray[0]), allocator);

            results.Append(dnsData);
        }
        break;
        case DNS_TYPE_AAAA:
        {
            // we don't care about the value here
            results.Append(dnsData);
        }
        break;
        } // switch

        record = record->pNext;
    }

    DnsRecordListFree(record, DnsFreeFlat);

#else
    IDnsMessage::SPtr spAnswerMessage;
    dnsParser.Deserialize(/*out*/spAnswerMessage, buffer, syncRead.Size(), true);
    KArray<IDnsRecord::SPtr>& arrDnsRecords = spAnswerMessage->Answers();
    for (ULONG i = 0; i < arrDnsRecords.Count(); i++)
    {
        IDnsRecord::SPtr spAnswer = arrDnsRecords[i];
        DNS_DATA dnsData;
        dnsData.Type = spAnswer->Type();
        dnsData.DataStr = spAnswer->DebugString();
        results.Append(dnsData);
    }

    status = DNS::DnsFlags::GetResponseCode(spAnswerMessage->Flags());
    switch (status)
    {
    case DNS::DnsFlags::RC_FORMERR:
        status = DNS_ERROR_RCODE_FORMAT_ERROR;
        break;
    case DNS::DnsFlags::RC_SERVFAIL:
        status = DNS_ERROR_RCODE_SERVER_FAILURE;
        break;
    case DNS::DnsFlags::RC_NXDOMAIN:
        status = DNS_ERROR_RCODE_NAME_ERROR;
        break;

    case DNS::DnsFlags::RC_NOERROR:
        if (results.Count() == 0)
        {
            status = DNS_INFO_NO_RECORDS;
        }
        break;
    }
#endif

    return status;
}

void DnsHelper::SerializeQuestion(
    __in IDnsParser& dnsParser,
    __in LPCWSTR wszQuery,
    __in KBuffer& buffer,
    __in WORD type,
    __out ULONG& size,
    __in DnsTextEncodingType encodingType
)
{
    buffer.Zero();
    size = buffer.QuerySize();

#if !defined(PLATFORM_UNIX)
    UNREFERENCED_PARAMETER(dnsParser);
	UNREFERENCED_PARAMETER(encodingType);

    if (!DnsWriteQuestionToBuffer_W((PDNS_MESSAGE_BUFFER)buffer.GetBuffer(), &size, wszQuery, type, 1, TRUE))
    {
        VERIFY_FAIL_FMT(L"Failed to serialize question to buffer");
    }
#else
    KAllocator& allocator = buffer.GetThisAllocator();
    KString::SPtr spQuestionStr;
    KString::Create(/*out*/spQuestionStr, allocator, wszQuery);
    spQuestionStr->SetNullTerminator();
    IDnsMessage::SPtr spMessage = dnsParser.CreateQuestionMessage(rand() % MAXUSHORT, *spQuestionStr, (DnsRecordType)type, encodingType);
    if (!dnsParser.Serialize(buffer, /*out*/size, *spMessage, 1, encodingType))
    {
        VERIFY_FAIL_FMT(L"Failed to serialize question to buffer");
    }
#endif
}

