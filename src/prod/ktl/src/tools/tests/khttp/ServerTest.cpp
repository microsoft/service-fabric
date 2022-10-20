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
	 -AdvancedServer

--*/
#pragma once
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ktl.h>
#include <ktrace.h>
#include "KHttpTests.h"
#include <CommandLineParser.h>


VOID DumpUsefulInfo(
	__in PWCHAR String,
    __in KHttpServerRequest::SPtr Request
	)
{
    NTSTATUS status;

    // Dump useful info to the console

    KHttpUtils::OperationType Op = Request->GetOpType();
    KStringView OpString;
    KHttpUtils::OpToString(Op, OpString);

    KTestPrintf("%S: *****Server Operation = %S\n", String, PWSTR(OpString));
    KUri::SPtr Url;
    Request->GetUrl(Url);
    KTestPrintf("URL = %S\n", LPCWSTR(*Url));
    KTestPrintf("Inbound data length = %u\n", Request->GetRequestEntityBodyLength());
    KString::SPtr Val;

    status = Request->GetAllHeaders(Val);
    KTestPrintf("-----------All headers---------:\n%S\n", PWSTR(*Val));
    KTestPrintf("\n----------End Headers------------\n");
}

VOID DefaultAdvancedServerHandler(
    __in KHttpServerRequest::SPtr Request
    )
{
//	DumpUsefulInfo(L"DefaultHandler", Request);

    // If here, send back a 404

    KStringView headerId(L"KTL.Test.Server.Message");
    KStringView value(L"URL not found!");
    Request->SetResponseHeader(headerId, value);
    Request->SendResponse(KHttpUtils::NotFound, "You sent a really bad URL to me");
}

class LegacyGet
{
	public:
		static VOID 
		HeaderHandler(
			__in KHttpServerRequest::SPtr Request,
			__out KHttpServer::HeaderHandlerAction& Action
			)
		{
			Action = KHttpServer::HeaderHandlerAction::DeliverContent;
		}

		static VOID 
		Handler(
			__in KHttpServerRequest::SPtr Request
			)
		{
			NTSTATUS status;
			const ULONG bufferSize = 16384;
			KBuffer::SPtr kBuffer;

//			DumpUsefulInfo(L"LegacyGet", Request);

			status = KBuffer::Create(bufferSize, kBuffer, *g_Allocator);
			if (! NT_SUCCESS(status))
			{
				Request->SendResponse(KHttpUtils::InternalServerError);
			} else {
				PUCHAR buffer = static_cast<PUCHAR>(kBuffer->GetBuffer());

				for (ULONG i = 0; i < bufferSize; i++)
				{
					buffer[i] = (i % 96) + 32;
				}

                KStringView value(L"text/plain");
				Request->SetResponseContent(kBuffer);
				Request->SetResponseHeaderByCode(KHttpUtils::HttpHeaderContentType, value);
				Request->SendResponse(KHttpUtils::Ok);
			}
		}
};

class LegacyGetStreamOut : public KAsyncContextBase
{
	K_FORCE_SHARED(LegacyGetStreamOut);

	public:
		static VOID HeaderHandler(
			__in KHttpServerRequest::SPtr Request,
			__out KHttpServer::HeaderHandlerAction& Action
			)
		{
			Action = KHttpServer::HeaderHandlerAction::DeliverContent;
		}

		static VOID Handler(
			__in KHttpServerRequest::SPtr Request
			)
		{
			LegacyGetStreamOut::SPtr context;

//			DumpUsefulInfo(L"LegacyGetStreamOut", Request);

			context = _new(KHTTP_TAG, *g_Allocator) LegacyGetStreamOut();
			if (! context)
			{
				KTestPrintf("LegacyGetStreamOut: _new failed\n");
				Request->SendResponse(KHttpUtils::InternalServerError);
				return;
			}

			context->_Request = Request;
			context->Start(NULL, nullptr);
		}

		VOID OnStart()
		{
			NTSTATUS status;

			status = KBuffer::Create(BufferSize, _KBuffer, *g_Allocator);
			if (! NT_SUCCESS(status))
			{
				KTestPrintf("LegacyGetStreamOut: KBuffer::Create failed %0x\n", status);
				_Request->SendResponse(KHttpUtils::InternalServerError);
				Complete(STATUS_SUCCESS);
				return;
			}

			status = KHttpServerRequest::AsyncResponseDataContext::Create(_Asdr, *g_Allocator, KTL_TAG_TEST);
			if (! NT_SUCCESS(status))
			{
				KTestPrintf("LegacyGetStreamOut: KHttpServerRequest::AsyncResponseDataContext::Create failed %0x\n", status);
				_Request->SendResponse(KHttpUtils::InternalServerError);
				Complete(STATUS_SUCCESS);
				return;
			}
	
			PUCHAR buffer = static_cast<PUCHAR>(_KBuffer->GetBuffer());
			for (ULONG i = 0; i < BufferSize; i++)
			{
				buffer[i] = (i % 96) + 32;
			}

            KStringView value(L"text/plain");
			_Request->SetResponseHeaderByCode(KHttpUtils::HttpHeaderContentType, value);

			_Iterator = 0;
			KAsyncContextBase::CompletionCallback sendHeadersAndDataCompletion(this, &LegacyGetStreamOut::SendHeadersAndDataCompletion);
			_Asdr->StartSendResponseHeader(*_Request, 
				                           KHttpUtils::Ok, 
				                           NULL, 
										   HTTP_SEND_RESPONSE_FLAG_MORE_DATA, 
										   this, 
										   sendHeadersAndDataCompletion);
		}

		VOID
		SendHeadersAndDataCompletion(
		   __in KAsyncContextBase* const Parent,
		   __in KAsyncContextBase& Op
		   )
		{
			UNREFERENCED_PARAMETER(Parent);

			NTSTATUS status;
			ULONG flags;
			KAsyncContextBase::CompletionCallback sendHeadersAndDataCompletion;

			status = Op.Status();
			if (! NT_SUCCESS(status))
			{
				KTestPrintf("LegacyGetStreamOut: SendHeadersAndDataCompletion %d failed %0x\n", _Iterator, status);
				return;
			}

			_Iterator++;
			if (_Iterator == NumberBuffers)
			{
				flags = HTTP_SEND_RESPONSE_FLAG_DISCONNECT;
				sendHeadersAndDataCompletion = NULL;
				Complete(STATUS_SUCCESS);
			} else {
				flags = HTTP_SEND_RESPONSE_FLAG_MORE_DATA;
				sendHeadersAndDataCompletion = KAsyncContextBase::CompletionCallback(this, &LegacyGetStreamOut::SendHeadersAndDataCompletion);
			}

			
			_Asdr->Reuse();
			_Asdr->StartSendResponseDataContent(*_Request, _KBuffer, flags, this, sendHeadersAndDataCompletion);
		}

	private:
		static const ULONG NumberBuffers = 16;
		static const ULONG BufferSize = 1024;

		KHttpServerRequest::SPtr _Request;
		KHttpServerRequest::AsyncResponseDataContext::SPtr _Asdr;
		KBuffer::SPtr _KBuffer;
		ULONG _Iterator;
};

LegacyGetStreamOut::LegacyGetStreamOut()
{
}

LegacyGetStreamOut::~LegacyGetStreamOut()
{
}

class StreamInStreamOutEcho : public KAsyncContextBase
{
	K_FORCE_SHARED(StreamInStreamOutEcho);

	public:
		static VOID HeaderHandler(
			__in KHttpServerRequest::SPtr Request,
			__out KHttpServer::HeaderHandlerAction& Action
			)
		{
//			DumpUsefulInfo(L"StreamInStreamOutEcho", Request);

			//
			// Note: Be sure to set this before starting any asyncs as there could be a case
			//       where an async started in this routine would race ahead of the thread
			//       running this routine.
			//
			Action = KHttpServer::HeaderHandlerAction::ApplicationMultiPartRead;

			StreamInStreamOutEcho::SPtr context;

			context = _new(KHTTP_TAG, *g_Allocator) StreamInStreamOutEcho();
			if (! context)
			{
				KTestPrintf("StreamInStreamOutEcho: _new failed\n");
				Request->SendResponse(KHttpUtils::InternalServerError);
				return;
			}

			context->_Request = Request;
			context->Start(NULL, nullptr);
		}

		static VOID Handler(
			__in KHttpServerRequest::SPtr Request
			)
		{
			KInvariant(FALSE);
		}

		VOID OnStart()
		{
			NTSTATUS status;

			status = _Request->GetRequestHeaderByCode(KHttpUtils::HttpHeaderType::HttpHeaderContentType, _ContentType);
			if (! NT_SUCCESS(status))
			{
				_ContentType = nullptr;
			}

			status = KHttpServerRequest::AsyncResponseDataContext::Create(_Asdr, *g_Allocator, KTL_TAG_TEST);
			if (! NT_SUCCESS(status))
			{
				KTestPrintf("StreamInStreamOutEcho: KHttpServerRequest::AsyncResponseDataContext::Create failed %0x\n", status);
				_Request->SendResponse(KHttpUtils::InternalServerError);
				Complete(STATUS_SUCCESS);
				return;
			}

			_Iterator = 0;
			_State = Reading;

			FSM(STATUS_SUCCESS);
		}

		VOID FSM(
			__in NTSTATUS Status
			)
		{
			KAsyncContextBase::CompletionCallback completion(this, &StreamInStreamOutEcho::OperationCompletion);

			if (_State == Reading)
			{
				if (! NT_SUCCESS(Status))
				{
					if (Status == STATUS_END_OF_FILE)
					{
						_State = SendingHeader;
						_Iterator = 0;
						FSM(STATUS_SUCCESS);
						return;
					}

					KTestPrintf("StreamInStreamOutEcho::FSM:(%d) failed %0x %d\n", _State, Status, _Request->GetHttpError());
					_Request->SendResponse(KHttpUtils::InternalServerError);
					Complete(STATUS_SUCCESS);
					return;
				}
				
				if (_Iterator > 0)
				{
					_KBufferArray.Append(_KBuffer);
					_KBuffer = nullptr;
				}

				_Iterator++;
				_Asdr->Reuse();
				_Asdr->StartReceiveRequestData(*_Request, BufferSize, _KBuffer, this, completion);

				return;
			} else if (_State == SendingHeader) {
				ULONG totalBufferSize = 0;
				KLocalString<128> s;
				ULONG flags;

				for (ULONG i = 0; i < _KBufferArray.Count(); i++)
				{
					totalBufferSize += _KBufferArray[i]->QuerySize();
				}

				if (_ContentType)
				{
					_Request->SetResponseHeaderByCode(KHttpUtils::HttpHeaderContentType, *_ContentType);
				}

				s.FromULONG(totalBufferSize);
				_Request->SetResponseHeaderByCode(KHttpUtils::HttpHeaderContentLength, s);

				if (totalBufferSize == 0)
				{
					flags = HTTP_SEND_RESPONSE_FLAG_DISCONNECT;
				} else {
					flags = HTTP_SEND_RESPONSE_FLAG_MORE_DATA;
				}

				_State = SendingData;

				_Asdr->Reuse();
				_Asdr->StartSendResponseHeader(*_Request, KHttpUtils::Ok, NULL, flags, this, completion);

			} else if (_State == SendingData) {
				ULONG flags;

				if (! NT_SUCCESS(Status))
				{
					KTestPrintf("StreamInStreamOutEcho::FSM:(%d) failed %0x %d\n", _State, Status, _Request->GetHttpError());
					_Request->SendResponse(KHttpUtils::InternalServerError);
					Complete(STATUS_SUCCESS);
					return;
				}

				if (_Iterator == _KBufferArray.Count())
				{
					Complete(STATUS_SUCCESS);
					return;
				}

				if (_Iterator == (_KBufferArray.Count() - 1))
				{
					flags = HTTP_SEND_RESPONSE_FLAG_DISCONNECT;
				} else {
					flags = HTTP_SEND_RESPONSE_FLAG_MORE_DATA;
				}

				_Asdr->Reuse();
				_Asdr->StartSendResponseDataContent(*_Request, _KBufferArray[_Iterator], flags, this, completion);
				_Iterator++;

			} else {
				KTestPrintf("StreamInStreamOutEcho::FSM: Invalid State %d\n", _State);
				KInvariant(FALSE);
			}

			return;
		}

		VOID
		OperationCompletion(
		   __in KAsyncContextBase* const Parent,
		   __in KAsyncContextBase& Op
		   )
		{
            UNREFERENCED_PARAMETER(Parent);

			NTSTATUS status = Op.Status();
			FSM(status);
		}


	private:
		enum { Reading, SendingHeader, SendingData } _State;

		static const ULONG BufferSize = 1024;

		KHttpServerRequest::SPtr _Request;
		KHttpServerRequest::AsyncResponseDataContext::SPtr _Asdr;
		KBuffer::SPtr _KBuffer;
		KArray<KBuffer::SPtr> _KBufferArray;
		ULONG _Iterator;
		KString::SPtr _ContentType;
};

StreamInStreamOutEcho::StreamInStreamOutEcho() :
	_KBufferArray(GetThisAllocator())
{
}

StreamInStreamOutEcho::~StreamInStreamOutEcho()
{
}


NTSTATUS
RunAdvancedServer(
    __in LPCWSTR Url,
    __in ULONG Seconds,
	__in ULONG NumberPrepostedRequests
    )
{
	NTSTATUS status;
    QuickTimer t;
    KHttpServer::SPtr server;
    KEvent OpenEvent;
    KEvent ShutdownEvent;

    p_ServerOpenEvent = &OpenEvent;
    p_ServerShutdownEvent = &ShutdownEvent;

    status = KHttpServer::Create(*g_Allocator, server);
    if (! NT_SUCCESS(status))
    {
        if (status == STATUS_ACCESS_DENIED)
        {
            KTestPrintf("ERROR: Advanced server must run under admin privileges.\n");
        }
		KTestPrintf("ERROR: %0x creating server\n", status);

        return(status);
    }


    KUriView ListenUrl(Url);
    KTestPrintf("Running advanced server on URL = %S\n", Url);

    status = server->StartOpen(ListenUrl, 
		                       DefaultAdvancedServerHandler, 
							   NumberPrepostedRequests,
							   8192,                     // DefaultHeaderBufferSize
							   8192,                     // DefaultEntityBodySize
							   NULL,                     // Parent async
							   ServerOpenCallback, 
							   nullptr,                  // KAsyncGlobalContext
							   KHttpUtils::HttpAuthType::AuthNone, 
							   0);                       // Max incoming entity size

    if (! NT_SUCCESS(status))
    {
        if (status == STATUS_INVALID_ADDRESS)
        {
            KTestPrintf("ERROR: URL is bad or missing the port number, which is required for HTTP.SYS based servers\n");
        }
        return status;
    }

    OpenEvent.WaitUntilSet();
    KInvariant(server->Status() == STATUS_PENDING);

	//
	// Register a different URL suffix for different server behaviors
	//

	// LegacyGet
	// LegacyGetStreamOut
	// StreamInStreamOutEcho
	// PostFailAtHeader
	// PostLegacyEcho
	// 

	PWCHAR urlFragment;
    KUriView urlFragmentView;

	urlFragment = L"LegacyGet";
    urlFragmentView = urlFragment;
	status = server->RegisterHandler(urlFragmentView, 
		                             TRUE, 
									 LegacyGet::Handler,
									 LegacyGet::HeaderHandler);

	if (! NT_SUCCESS(status))
	{
		KTestPrintf("RegisterHandler %S failed %0x\n", urlFragment, status);
		return(status);
	}

	urlFragment = L"LegacyGetStreamOut";
    urlFragmentView = urlFragment;
	status = server->RegisterHandler(urlFragmentView, 
		                             TRUE, 
									 LegacyGetStreamOut::Handler,
									 LegacyGetStreamOut::HeaderHandler);

	if (! NT_SUCCESS(status))
	{
		KTestPrintf("RegisterHandler %S failed %0x\n", urlFragment, status);
		return(status);
	}

	urlFragment = L"StreamInStreamOutEcho";
    urlFragmentView = urlFragment;
	status = server->RegisterHandler(urlFragmentView, 
		                             TRUE, 
									 StreamInStreamOutEcho::Handler,
									 StreamInStreamOutEcho::HeaderHandler);

	if (! NT_SUCCESS(status))
	{
		KTestPrintf("RegisterHandler %S failed %0x\n", urlFragment, status);
		return(status);
	}


    for (;;)
    {
        KNt::Sleep(5000);
        if (t.HasElapsed(Seconds))
        {
            break;
        }
    }

    KTestPrintf("...Deactivating server\n");
    server->StartClose(0, ServerShutdownCallback);

    ShutdownEvent.WaitUntilSet();
    KInvariant(server->Status() == STATUS_SUCCESS);

    return STATUS_SUCCESS;
}




