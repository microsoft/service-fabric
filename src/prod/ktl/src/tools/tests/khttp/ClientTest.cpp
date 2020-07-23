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

class PostEcho : public KAsyncContextBase
{
	K_FORCE_SHARED(PostEcho);

	public:
		static NTSTATUS Create(
			__out PostEcho::SPtr& Context,
			__in KAllocator& Allocator,
			__in ULONG AllocationTag
			);

		typedef enum 
		      { Undefined = 0, MPSendMPReceive = 1, SPSendSPReceive = 2, MPGet = 3 } RunModeType;

		VOID StartPostEcho(
			__in RunModeType RunMode,
			__in KHttpClient& Client,
			__in KUri& Uri,
			__in KMemRef& Data,
			__in_opt KAsyncContextBase* const ParentAsyncContext,
			__in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

	private:
		enum { Idle, InitiatingRequest, SendingData, ReceivingData, ComparingData, AlreadyCompleted } _State;

		VOID Initialize(
			);

		VOID FSM(
			NTSTATUS Status
			);

	    VOID
		RequestCompletion(
			__in KAsyncContextBase* const Parent,
			__in KAsyncContextBase& Op
	        );

	    VOID
		OperationCompletion(
			__in KAsyncContextBase* const Parent,
			__in KAsyncContextBase& Op
	        );

	protected:
        VOID
        OnStart(
            ) override ;

        VOID
        OnReuse(
            ) override ;

        VOID
        OnCompleted(
            ) override ;

        VOID
        OnCancel(
            ) override ;

    private:
        //
        // General members
        //
		static const ULONG MaxPartSizeOnWire = 1024;
                
        //
        // Parameters to api
        //
		RunModeType _RunMode;
		KHttpClient::SPtr _Client;
		KUri::SPtr _Uri;
		KMemRef _Data;

        //
        // Members needed for functionality
        //
		KBuffer::SPtr _EchoedBuffer;
		KNetworkEndpoint::SPtr _Ep;
		KHttpCliRequest::SPtr _Request;
		KHttpCliRequest::Response::SPtr _Response;
		KHttpCliRequest::AsyncRequestDataContext::SPtr _Asdr;

		ULONG _PartNumber;
		ULONG _NumberOfParts;
		PUCHAR _LastDataPtr;
		ULONG _LastDataSize;
};

VOID
PostEcho::OnCompleted(
	)
{
	_EchoedBuffer = nullptr;
	_Ep = nullptr;
	_Request = nullptr;
	_Response = nullptr;
	_Asdr = nullptr;
}

VOID
PostEcho::RequestCompletion(
	__in KAsyncContextBase* const Parent,
	__in KAsyncContextBase& Op
	)
{
	UNREFERENCED_PARAMETER(Parent);

	FSM(Op.Status());
}

VOID
PostEcho::OperationCompletion(
	__in KAsyncContextBase* const Parent,
	__in KAsyncContextBase& Op
	)
{
	UNREFERENCED_PARAMETER(Parent);

	FSM(Op.Status());
}


VOID PostEcho::FSM(
	NTSTATUS Status
	)
{
	KAsyncContextBase::CompletionCallback requestCompletion(this, &PostEcho::RequestCompletion);
	KAsyncContextBase::CompletionCallback completion(this, &PostEcho::OperationCompletion);

	if (! NT_SUCCESS(Status))
	{
		KTraceFailedAsyncRequest(Status, this, _State, _RunMode);
		_State = AlreadyCompleted;
		Complete(Status);
		return;
	}

	switch(_State)
	{
		case InitiatingRequest:
		{
			Status = KHttpNetworkEndpoint::Create(*_Uri, *g_Allocator, _Ep);
			if (! NT_SUCCESS(Status))
			{
				KAssert(FALSE);    // CONSIDER:  A better way to break ?
				KTraceFailedAsyncRequest(Status, this, _State, _RunMode);
				_State = AlreadyCompleted;
				Complete(Status);
				return;
			}

			if ((_RunMode == PostEcho::RunModeType::MPSendMPReceive) ||
				(_RunMode == PostEcho::RunModeType::MPGet))
			{
				Status = _Client->CreateMultiPartRequest(_Request);
				if (NT_SUCCESS(Status))
				{
					Status = KHttpCliRequest::AsyncRequestDataContext::Create(_Asdr, *g_Allocator, KTL_TAG_TEST);
				}
			} 
			else if (_RunMode == PostEcho::RunModeType::SPSendSPReceive) 
			{
				Status = _Client->CreateRequest(_Request);
			} 
			else 
			{
				KInvariant(FALSE);
				Status = STATUS_UNSUCCESSFUL;
			}

			if (! NT_SUCCESS(Status))
			{
				KAssert(FALSE);    // CONSIDER:  A better way to break ?
				KTraceFailedAsyncRequest(Status, this, _State, _RunMode);
				_State = AlreadyCompleted;
				Complete(Status);
				return;
			}

			Status = _Request->SetTarget(_Ep, (_RunMode == PostEcho::RunModeType::MPGet) ? KHttpUtils::eHttpGet : KHttpUtils::eHttpPost);
			if (! NT_SUCCESS(Status))
			{
				KAssert(FALSE);    // CONSIDER:  A better way to break ?
				KTraceFailedAsyncRequest(Status, this, _State, _RunMode);
				_State = AlreadyCompleted;
				Complete(Status);
				return;
			}

			if (_RunMode != PostEcho::RunModeType::MPGet)
			{
                KStringView value(L"text/plain");
				Status = _Request->AddHeaderByCode(KHttpUtils::HttpHeaderContentType, value);
				if (! NT_SUCCESS(Status))
				{
					KAssert(FALSE);    // CONSIDER:  A better way to break ?
					KTraceFailedAsyncRequest(Status, this, _State, _RunMode);
					_State = AlreadyCompleted;
					Complete(Status);
					return;
				}
			}

			if (_RunMode == PostEcho::RunModeType::MPSendMPReceive)
			{
				_State = SendingData;

				_NumberOfParts = _Data._Size / MaxPartSizeOnWire;
				_PartNumber = 0;

				_Asdr->StartSendRequestHeader(*_Request,
					                          _Data._Size,
											  _Response,
											  this,
											  requestCompletion,
											  this,
											  completion);
				return;
			} else if (_RunMode == PostEcho::RunModeType::SPSendSPReceive) {
				Status = _Request->SetContent(_Data);
				if (! NT_SUCCESS(Status))
				{
					KAssert(FALSE);    // CONSIDER:  A better way to break ?
					KTraceFailedAsyncRequest(Status, this, _State, _RunMode);
					_State = AlreadyCompleted;
					Complete(Status);
					return;
				}

				_State = ComparingData;
				Status = _Request->Execute(_Response, this, requestCompletion);
				if (! NT_SUCCESS(Status))
				{
					KAssert(FALSE);    // CONSIDER:  A better way to break ?
					KTraceFailedAsyncRequest(Status, this, _State, _RunMode);
					_State = AlreadyCompleted;
					Complete(Status);
					return;
				}

				return;
			} else {
				_State = SendingData;

				_PartNumber = 0;
				_NumberOfParts = 0;
				_Data._Address = NULL;

				_Asdr->StartSendRequestHeader(*_Request,
					                          0,
											  _Response,
											  this,
											  requestCompletion,
											  this,
											  completion);
				return;
			}
		}
	
		case SendingData:
		{
			KMemRef memRef;
			ULONG skip;
			ULONG length;

			if (_PartNumber == _NumberOfParts)
			{
				_State = ReceivingData;

				_PartNumber = 0;
				_LastDataPtr = (PUCHAR)_Data._Address;
				_LastDataSize = _Data._Size;

				_Asdr->Reuse();
				_Asdr->StartEndRequest(*_Request, this, completion);
				return;
			}

			skip = (_PartNumber * MaxPartSizeOnWire);
			_PartNumber++;
			length = (_PartNumber == _NumberOfParts) ? _Data._Size - skip : MaxPartSizeOnWire;

			memRef._Address = (PUCHAR)_Data._Address + skip;
			memRef._Param = length;
			memRef._Size = length;              // Need to set both _Param and _Size due to implementation of KHttp

			_Asdr->Reuse();
			_Asdr->StartSendRequestDataContent(*_Request, memRef, this, completion);
			return;
		}

		case ReceivingData:
		{
			ULONG length;

			if (_PartNumber != 0)
			{
				if (_Response->GetHttpResponseCode() != KHttpUtils::HttpResponseCode::Ok)
				{
					KTestPrintf("Request %x received HTTP response %d\n", (DWORD_PTR)this, _Response->GetHttpResponseCode());
					_State = AlreadyCompleted;
					Complete(STATUS_SUCCESS);
					return;
				}

				if (_EchoedBuffer == nullptr)
				{
					_State = AlreadyCompleted;
					Complete(STATUS_SUCCESS);
					return;
				}

				if (_LastDataPtr != NULL)
				{
					length = _EchoedBuffer->QuerySize();
					if ((length > _LastDataSize) ||
						(! (memcmp(_LastDataPtr, _EchoedBuffer->GetBuffer(), length) == 0)))
					{
						KAssert(FALSE);

						Status = STATUS_BAD_DATA;
						KTraceFailedAsyncRequest(Status, this, _State, _RunMode);
						Complete(Status);
						return;
					}

					_LastDataPtr += length;
					_LastDataSize -= length;
				}
			}

			_PartNumber++;
			_Asdr->Reuse();
			_Asdr->StartReceiveResponseData(*_Request, MaxPartSizeOnWire, _EchoedBuffer, this, completion);
			return;
		}

		case ComparingData:
		{
			KInvariant(_RunMode == PostEcho::RunModeType::SPSendSPReceive);

			if (_Response->GetHttpResponseCode() != KHttpUtils::HttpResponseCode::Ok)
			{
				KTestPrintf("Request %x received HTTP response %d\n", (DWORD_PTR)this, _Response->GetHttpResponseCode());
				_State = AlreadyCompleted;
				Complete(STATUS_SUCCESS);
				return;
			}

			_Response->GetContent(_EchoedBuffer);
			ULONG result = memcmp(_Data._Address, _EchoedBuffer->GetBuffer(), _Data._Size); 
			if ((_Data._Size != _EchoedBuffer->QuerySize()) ||
				(! (result == 0)))
			{
				KAssert(FALSE);

				Status = STATUS_BAD_DATA;
				KTraceFailedAsyncRequest(Status, this, _State, _RunMode);
				_State = AlreadyCompleted;
				Complete(Status);
				return;
			}

			_State = AlreadyCompleted;
			Complete(Status);
		}

		case AlreadyCompleted:
		{
			return;
		}

		default:
		{
			KInvariant(FALSE);

			Status = STATUS_UNSUCCESSFUL;
			KTraceFailedAsyncRequest(Status, this, _State, _RunMode);
			_State = AlreadyCompleted;
			Complete(Status);
			return;
		}
	}
}

VOID
PostEcho::OnStart(
	)
{
	FSM(STATUS_SUCCESS);
}

VOID PostEcho::StartPostEcho(
	__in PostEcho::RunModeType RunMode,
	__in KHttpClient& Client,
	__in KUri& Uri,
	__in KMemRef& Data,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
	)
{
	_State = InitiatingRequest;

	_RunMode = RunMode;
	_Client = &Client;
	_Uri = &Uri;
	_Data = Data;
	Start(ParentAsyncContext, CallbackPtr);
}

VOID
PostEcho::OnCancel(
	)
{
}

VOID PostEcho::Initialize(
	)
{
	_State = Idle;
	_RunMode = Undefined;
	_Client = nullptr;
	_Uri = nullptr;

	_EchoedBuffer = nullptr;
	_Ep = nullptr;
	_Request = nullptr;
	_Response = nullptr;
	_Asdr = nullptr;

	_PartNumber = 0;
	_NumberOfParts = 0;
	_LastDataPtr = NULL;
	_LastDataSize = 0;
}

VOID
PostEcho::OnReuse(
	)
{
	Initialize();
}

PostEcho::PostEcho()
{
}

PostEcho::~PostEcho()
{
}

NTSTATUS PostEcho::Create(
	__out PostEcho::SPtr& Context,
	__in KAllocator& Allocator,
	__in ULONG AllocationTag
	)
{
    UNREFERENCED_PARAMETER(Allocator);
    UNREFERENCED_PARAMETER(AllocationTag);

	NTSTATUS status;
	PostEcho::SPtr context;

	context = _new(KTL_TAG_TEST, *g_Allocator) PostEcho();
	if (context == nullptr)
	{
		return(STATUS_INSUFFICIENT_RESOURCES);
	}

	status = context->Status();
	if (! NT_SUCCESS(status))
	{
		return(status);
	}

	Context.Attach(context.Detach());
	return(STATUS_SUCCESS);
}



NTSTATUS
RunAdvancedClient(
    __in LPCWSTR Url,
	__in ULONG ActiveRequests
    )
{
	NTSTATUS status;
	NTSTATUS finalStatus = STATUS_SUCCESS;
	static const ULONG MaxActiveRequests = 512;
	PostEcho::SPtr postEcho[MaxActiveRequests];
	KSynchronizer sync[MaxActiveRequests];
	KHttpClient::SPtr client;
	KUri::SPtr uriStreamInStreamOutEcho;
	KUri::SPtr uriLegacyGet;
	KUri::SPtr uriLegacyGetStreamOut;
	KBuffer::SPtr data;
	KMemRef dataReference[MaxActiveRequests];
	PUCHAR dataPtr;
	ULONG randomSize[MaxActiveRequests];
	ULONG largestBuffer = 0;
	KLocalString<MAX_PATH> s;

	if (ActiveRequests > MaxActiveRequests)
	{
		KTestPrintf("Only up to %d concurrent requests allowed\n", MaxActiveRequests);
		return(STATUS_UNSUCCESSFUL);
	}

	srand((unsigned int)GetTickCount());

	for (ULONG i = 0; i < ActiveRequests; i++)
	{
		status = PostEcho::Create(postEcho[i], *g_Allocator, KTL_TAG_TEST);
		if (! NT_SUCCESS(status))
		{
			KTestPrintf("Error creating postecho 0x%x\n", status);
			return(status);
		}

		randomSize[i] = ((rand() % 1024) * 1024) + (rand() % 1024);
		if (randomSize[i] > largestBuffer)
		{
			largestBuffer = randomSize[i];
		}
	}

    KStringView userAgent(L"KHttpClientTest Agent");
	status = KHttpClient::Create(userAgent, 
				                *g_Allocator, 
								client);
	if (! NT_SUCCESS(status))
	{
		KTestPrintf("KHttpClient::Create failed 0x%x\n", status);
		return(status);
	}

	s.CopyFrom(Url);
	s.Concat(KStringView(L"/StreamInStreamOutEcho"));
	status = KUri::Create(KStringView(s), *g_Allocator, uriStreamInStreamOutEcho);
	if (! NT_SUCCESS(status))
	{
		KTestPrintf("KUri::Create(%ws) failed 0x%x\n", (LPCWCHAR)s, status);
		return(status);
	}

	s.CopyFrom(Url);
	s.Concat(KStringView(L"/LegacyGet"));
	status = KUri::Create(KStringView(s), *g_Allocator, uriLegacyGet);
	if (! NT_SUCCESS(status))
	{
		KTestPrintf("KUri::Create(%ws) failed 0x%x\n", (LPCWCHAR)s, status);
		return(status);
	}

	s.CopyFrom(Url);
	s.Concat(KStringView(L"/LegacyGetStreamOut"));
	status = KUri::Create(KStringView(s), *g_Allocator, uriLegacyGetStreamOut);
	if (! NT_SUCCESS(status))
	{
		KTestPrintf("KUri::Create(%ws) failed 0x%x\n", (LPCWCHAR)s, status);
		return(status);
	}

	status = KBuffer::Create(largestBuffer, data, *g_Allocator, KTL_TAG_TEST);
	if (! NT_SUCCESS(status))
	{
		KTestPrintf("KBuffer::Create failed 0x%x\n", status);
		return(status);
	}

	dataPtr = (PUCHAR)data->GetBuffer();
	for (ULONG i = 0; i < largestBuffer; i++)
	{
		dataPtr[i] = (rand() % 96) + 32;
	}

	for (ULONG i = 0; i < ActiveRequests; i++)
	{
		KUri::SPtr uri;
		KMemRef memRef;
		PostEcho::RunModeType runModeRandomArr[3] = { PostEcho::RunModeType::MPGet, PostEcho::RunModeType::MPSendMPReceive, PostEcho::RunModeType::SPSendSPReceive };
		PostEcho::RunModeType runModeRandom = runModeRandomArr[(rand() % 3)];
//		runModeRandom = PostEcho::RunModeType::MPGet;

		if (runModeRandom == PostEcho::RunModeType::MPGet)
		{
			uri = ((rand() % 1) == 1) ? uriLegacyGet : uriLegacyGetStreamOut;
			memRef._Address = NULL;
			memRef._Param = 0;
			memRef._Size = 0;
		} else {
			uri = uriStreamInStreamOutEcho;
			memRef._Address = data->GetBuffer();
			memRef._Param = randomSize[i];
			memRef._Size = randomSize[i];
		}

		postEcho[i]->StartPostEcho(runModeRandom, *client, *uri, memRef, NULL, sync[i]);
	}

	for (ULONG i = 0; i < ActiveRequests; i++)
	{
		status = sync[i].WaitForCompletion();
		if (! NT_SUCCESS(status))
		{
			KTestPrintf("Async %d failed 0x%x\n", i, status);
			finalStatus = status;
		}
	}

	if (! NT_SUCCESS(finalStatus))
	{
		return(finalStatus);
	}

    return STATUS_SUCCESS;
}




