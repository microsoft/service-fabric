/*++

Copyright (c) Microsoft Corporation

Module Name:

    KHttpTests.cpp

Abstract:

    This file contains test case implementations.

    To add a new unit test case:
    1. Declare your test case routine in KHttpTests.h.
    2. Add an entry to the gs_KuTestCases table in KHttpTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
    this file or any other file.

	Test command prompt:
	 -mppost http://alanwar-t6:8081/testurl/suffix1 text/plain c:\temp\uxresctl.txt c:\temp\fooback.txt

--*/
#pragma once
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ktl.h>
#include <ktrace.h>
#include "KHttpTests.h"
#include <CommandLineParser.h>

NTSTATUS MultiPartPost(
    __in LPCWSTR Url,
    __in LPCWSTR FilePath,
    __in LPCWSTR ContentType,
    __in LPCWSTR Dest
    )
{
	NTSTATUS Result;
    KHttpClient::SPtr Client;
    KNetworkEndpoint::SPtr Ep;
    KHttpCliRequest::SPtr Req;
    KHttpCliRequest::Response::SPtr Resp;
	KHttpCliRequest::AsyncRequestDataContext::SPtr asdr;
    KBuffer::SPtr FileContent;
	KSynchronizer syncResp, sync;
	ULONG contentSize;
	ULONG partSize;

    if (!FilePathToKBuffer(FilePath, FileContent))
    {
        KTestPrintf("Unable to read file\n");
        return STATUS_UNSUCCESSFUL;
    }

	contentSize = FileContent->QuerySize();
    KTestPrintf("Posting %u bytes of type %S\n", contentSize, ContentType);

    KStringView userAgent(L"KTL HTTP Test Agent");
    Result = KHttpClient::Create(userAgent, *g_Allocator, Client);
    if (!NT_SUCCESS(Result))
    {
        return Result;
    }

    // Convert Url to a network endpoint.
    //
    KUriView httpUri(Url);
    NTSTATUS Res = KHttpNetworkEndpoint::Create(httpUri, *g_Allocator, Ep);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }


    // Create a new request and address
    //
    Res = Client->CreateMultiPartRequest(Req);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

	//
	// Create an AsyncDataContext to drive the request
	//
	Res = KHttpCliRequest::AsyncRequestDataContext::Create(asdr, *g_Allocator, KHTTP_TAG);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    Res = Req->SetTarget(Ep, KHttpUtils::eHttpPost);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    KStringView value(ContentType);
    Res = Req->AddHeaderByCode(KHttpUtils::HttpHeaderContentType, value);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }
	
	//
	// Send the headers 
	asdr->StartSendRequestHeader(*Req, contentSize, Resp, NULL, syncResp, NULL, sync); 
	Res = sync.WaitForCompletion();
    if (!NT_SUCCESS(Res))
    {
		if (Resp)
		{
			Res = syncResp.WaitForCompletion();
		}
        return Res;
    }

	KInvariant(Resp != nullptr);

	//
	// Send the contents in multiple parts
	//
	const ULONG maxPartSize = 0x1000;
	ULONG sizeToSend;
	PUCHAR p;

	contentSize = FileContent->QuerySize();
	partSize = (contentSize > maxPartSize) ? maxPartSize : contentSize;
	p = (PUCHAR)FileContent->GetBuffer();
	while (contentSize > 0)
	{
		if (contentSize < maxPartSize)
		{
			sizeToSend = contentSize;
		} else {
			sizeToSend = maxPartSize;
		}

		// CONSIDER: KHttp uses the Param of the KMemRef as the size of the buffer to send instead of the Size.
		//           Should KHttp break backcompat and use Size instead ? 
		KMemRef mem(sizeToSend, p, sizeToSend);

		asdr->Reuse();
		asdr->StartSendRequestDataContent(*Req, mem, NULL, sync);
		Res = sync.WaitForCompletion();
		if (!NT_SUCCESS(Res))
		{
			Res = syncResp.WaitForCompletion();
			return Res;
		}

		p = p + sizeToSend;
		contentSize = contentSize - sizeToSend;
	}

	//
	// Now end the request
	//
	asdr->Reuse();
	asdr->StartEndRequest(*Req, NULL, sync);
	Res = sync.WaitForCompletion();
	if (!NT_SUCCESS(Res))
	{
		Res = syncResp.WaitForCompletion();
		return Res;
	}


    // When here, we have a response or timeout
    //
    ULONG Code;
    NTSTATUS Status;

    Status = Resp->Status();
    Code = Resp->GetHttpResponseCode();

    KTestPrintf("HTTP Response code is %u\n", Code);

    KString::SPtr Header;
    Resp->GetHeaderByCode(KHttpUtils::HttpHeaderContentType, Header);

    ULONG ContLength = Resp->GetContentLengthHeader();

    KTestPrintf("Content type is        %S\n",   Header ? PWSTR(*Header) : L"(null)");
    KTestPrintf("Content Length is      %u\n", ContLength);
    KTestPrintf("Entity Body Length is  %u\n", Resp->GetEntityBodyLength());


    KString::SPtr AllHeaders;
    Resp->GetAllHeaders(AllHeaders);
    if (AllHeaders)
    {
        KTestPrintf("\n-------------Begin headers:\n%S\n", PWSTR(*AllHeaders));
    }
    KTestPrintf("\n--------End headers------------\n");


	//
	// Now perform multipart read of the results
	//
    if (FilePath)
    {
        FILE* f;
        f = _wfopen(Dest, L"wb");
        if (!f)
        {
            return STATUS_UNSUCCESSFUL;
        }

		KBuffer::SPtr readKBuffer;

		do
		{
			readKBuffer = nullptr;
			asdr->Reuse();
		    // TODO: Increase this when there is a better server
			asdr->StartReceiveResponseData(*Req, 2, readKBuffer, NULL, sync);

			Res = sync.WaitForCompletion();
			if (!NT_SUCCESS(Res))
			{
				Res = syncResp.WaitForCompletion();
				return Res;
			}

			if (readKBuffer != nullptr)
			{
				fwrite(readKBuffer->GetBuffer(), 1, readKBuffer->QuerySize(), f);
			}

		} while (readKBuffer != nullptr);

        fclose(f);
	}

	//
	// Once all data is read the request should completed
	//
	Res = syncResp.WaitForCompletion();
	if (!NT_SUCCESS(Res))
	{
		return Res;
	}

    return STATUS_SUCCESS;
}
