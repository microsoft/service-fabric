// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Ktl::Com;

namespace ReliableMessaging
{
	/*** ComFabricReliableSessionManager methods ***/

	//
	// Constructor/Destructor.
	//
	ComFabricReliableSessionManager::ComFabricReliableSessionManager()
	{
	}

	ComFabricReliableSessionManager::~ComFabricReliableSessionManager()
	{
	}

	HRESULT ComFabricReliableSessionManager::Create( 
				__in HOST_SERVICE_PARTITION_INFO *servicePartitionInfo,
				__in FABRIC_URI callingServiceInstanceName,
				__in FABRIC_PARTITION_KEY_TYPE partitionKeyType,
				__in const void *partitionKey,
				__out IFabricReliableSessionManager **sessionManager)
	{
		PARAMETER_NULL_CHECK(ComFabricReliableSessionManager::Create,sessionManager,"must be non-NULL")
		API_PARTITION_PARAMETER_CHECK(ComFabricReliableSessionManager::Create,callingServiceInstanceName,partitionKeyType,partitionKey)

		*sessionManager = nullptr;
																															
		ReliableMessagingEnvironmentSPtr singletonEnvironment = ReliableMessagingEnvironment::TryGetsingletonEnvironment();

		if (singletonEnvironment == nullptr)
		{
			return Common::ComUtility::OnPublicApiReturn(E_OUTOFMEMORY);
		}

		ReliableMessagingServicePartitionSPtr myPartition = nullptr;
		Common::ErrorCode result = ReliableMessagingServicePartition::Create(
																		callingServiceInstanceName,
																		partitionKeyType,
																		partitionKey,
																		myPartition);

		if (!result.IsSuccess())
		{
			HRESULT hr = result.ToHResult();
			if (result.ReadValue() != Common::ErrorCodeValue::OutOfMemory)
			{
				RMTRACE->InvalidPartitionDescription(callingServiceInstanceName,partitionKeyType,hr);
			}
			return ComUtility::OnPublicApiReturn(hr);
		}

		PARTITION_TRACE(ComReliableSessionManagerCreateStarted,myPartition)

		ComFabricReliableSessionManager* comSessionManager = new (std::nothrow) ComFabricReliableSessionManager(); // ref count is already 1
		if (nullptr == comSessionManager)
		{
			PARTITION_TRACE_WITH_DESCRIPTION(SessionManagerServiceUnableToCreate, myPartition, "memory allocation Failure")
			return ComUtility::OnPublicApiReturn(E_OUTOFMEMORY);
		}

		Common::Guid ownerServicePartitionId(servicePartitionInfo->PartitionId);
		result = SessionManagerService::Create(ownerServicePartitionId, servicePartitionInfo->ReplicaId, myPartition, comSessionManager->sessionManager_);

		if (!result.IsSuccess())
		{
			HRESULT hr = result.ToHResult();
			PARTITION_TRACE_WITH_HRESULT(CreateSessionManagerServiceFailed,myPartition,hr)
			comSessionManager->Release();
			*sessionManager = nullptr;
			return ComUtility::OnPublicApiReturn(hr);
		}

		*sessionManager = comSessionManager;

		PARTITION_TRACE(ComReliableSessionManagerCreateCompleted,myPartition)

		return Common::ComUtility::OnPublicApiReturn(S_OK);
	}

	// ************
	// IFabricReliableSessionManager Methods
	// ************
        
	HRESULT ComFabricReliableSessionManager::BeginOpen( 
		__in IFabricReliableSessionAbortedCallback *sessionCallback,
		__in IFabricAsyncOperationCallback *callback,
		__out IFabricAsyncOperationContext **context)
	{
		PARAMETER_NULL_CHECK(ComFabricReliableSessionManager::BeginOpen,context,"must be non-NULL")
		PARAMETER_NULL_CHECK(ComFabricReliableSessionManager::BeginOpen,sessionCallback,"must be non-NULL")

		if (sessionManager_ == nullptr) 
		{
			RMTRACE->EmptyComObjectApiCallError("BeginOpen","ComFabricReliableSessionManager");
			return ComUtility::OnPublicApiReturn(FABRIC_E_INVALID_OPERATION);
		}

		*context = nullptr;

		auto startOpen = [&](SessionManagerService& service, KAsyncServiceBase::OpenCompletionCallback callback) -> HRESULT
		{
			return (service.StartOpenSessionManager(sessionCallback, nullptr, callback)).ToHResult(); 
		};

		HRESULT hr = Ktl::Com::ServiceOpenCloseAdapter::StartOpen<SessionManagerService>(
			sessionManager_->GetThisAllocator(), 
			sessionManager_, 
			callback, 
			context, 
			startOpen);

		return ComUtility::OnPublicApiReturn(hr);
	}
        
	HRESULT ComFabricReliableSessionManager::EndOpen( 
		__in IFabricAsyncOperationContext *context)
	{
		PARAMETER_NULL_CHECK(ComFabricReliableSessionManager::EndOpen,context,"must be non-NULL")

		SessionManagerServiceSPtr sessionManagerService;
		HRESULT hr = Ktl::Com::ServiceOpenCloseAdapter::EndOpen(static_cast<Ktl::Com::ServiceOpenCloseAdapter*>(context), sessionManagerService);
		KAssert(sessionManagerService.RawPtr() == sessionManager_.RawPtr());
		return ComUtility::OnPublicApiReturn(hr);
	}
        
	HRESULT ComFabricReliableSessionManager::BeginClose( 
		__in IFabricAsyncOperationCallback *callback,
		__out IFabricAsyncOperationContext **context)
	{
		PARAMETER_NULL_CHECK(ComFabricReliableSessionManager::BeginClose,context,"must be non-NULL")

		PARTITION_TRACE(SessionManagerStartCloseStarted,sessionManager_->OwnerServicePartition)

		if (sessionManager_ == nullptr) 
		{
			RMTRACE->EmptyComObjectApiCallError("BeginClose","ComFabricReliableSessionManager");
			return ComUtility::OnPublicApiReturn(FABRIC_E_INVALID_OPERATION);
		}

		*context = nullptr;

		auto startClose = [&](SessionManagerService& service, KAsyncServiceBase::CloseCompletionCallback callback) -> HRESULT
		{
			return (service.StartCloseSessionManager(nullptr, callback)).ToHResult(); 
		};

		HRESULT hr = Ktl::Com::ServiceOpenCloseAdapter::StartClose<SessionManagerService>(
			sessionManager_->GetThisAllocator(), 
			sessionManager_, 
			callback, 
			context, 
			startClose);

		PARTITION_TRACE_WITH_HRESULT(SessionManagerStartCloseCompleted,sessionManager_->OwnerServicePartition,hr)

		return ComUtility::OnPublicApiReturn(hr);
	}
        
	HRESULT ComFabricReliableSessionManager::EndClose( 
		__in IFabricAsyncOperationContext *context)
	{
		PARAMETER_NULL_CHECK(ComFabricReliableSessionManager::EndClose,context,"must be non-NULL")

		PARTITION_TRACE(SessionManagerFinishCloseStarted,sessionManager_->OwnerServicePartition)

		SessionManagerServiceSPtr service;
		HRESULT hr = Ktl::Com::ServiceOpenCloseAdapter::EndClose(static_cast<Ktl::Com::ServiceOpenCloseAdapter*>(context), service);
		KAssert(service.RawPtr() == sessionManager_.RawPtr());

		PARTITION_TRACE_WITH_HRESULT(SessionManagerFinishCloseCompleted,sessionManager_->OwnerServicePartition,hr)

		return ComUtility::OnPublicApiReturn(hr);
	}

	HRESULT ComFabricReliableSessionManager::BeginCreateOutboundSession( 
				__in FABRIC_URI targetServiceInstanceName,
				__in FABRIC_PARTITION_KEY_TYPE partitionKeyType,
				__in const void *partitionKey,
				__in LPCWSTR targetCommunicationEndpoint,
				__in IFabricAsyncOperationCallback *callback,
				__out IFabricAsyncOperationContext **context)
	{
		API_PARTITION_PARAMETER_CHECK(ComFabricReliableSessionManager::BeginCreateOutboundSession,targetServiceInstanceName,partitionKeyType,partitionKey)
		PARAMETER_NULL_CHECK(ComFabricReliableSessionManager::BeginCreateOutboundSession,targetCommunicationEndpoint,"must be non-NULL")
		PARAMETER_NULL_CHECK(ComFabricReliableSessionManager::BeginCreateOutboundSession,context,"must be non-NULL")

		// TODO: let service logic handle this -- don't ever erase sessionManager_
		if (sessionManager_ == nullptr) 
		{
			RMTRACE->EmptyComObjectApiCallError("BeginCreateOutboundSession","ComFabricReliableSessionManager");
			return ComUtility::OnPublicApiReturn(FABRIC_E_INVALID_OPERATION);
		}

 		ReliableMessagingServicePartitionSPtr targetPartition = nullptr;
		Common::ErrorCode result = ReliableMessagingServicePartition::Create(
																		targetServiceInstanceName,
																		partitionKeyType,
																		partitionKey,
																		targetPartition);

		if (!result.IsSuccess())
		{
			HRESULT hr = result.ToHResult();
			RMTRACE->InvalidPartitionDescription(targetServiceInstanceName,partitionKeyType,hr);
			return ComUtility::OnPublicApiReturn(hr);
		}

		std::wstring targetEndpoint = targetCommunicationEndpoint;

		PARTITION_TRACE(BeginCreateOutboundSessionStarted,targetPartition)

		AsyncCreateOutboundSessionOperationSPtr sessionCreateOp = nullptr;
		result = AsyncCreateOutboundSessionOperation::Create(sessionManager_,sessionCreateOp);

		if (!result.IsSuccess())
		{
			HRESULT hr = result.ToHResult();
			return ComUtility::OnPublicApiReturn(hr);
		}

		HRESULT hr = Ktl::Com::AsyncCallInAdapter::CreateAndStart(
			sessionManager_->GetThisAllocator(),
			sessionCreateOp,
			callback,                // IFabricAsyncOperationCallback
			context,                 // IFabricAsyncOperationContext (out)
			[&] (AsyncCallInAdapter&, 
                 AsyncCreateOutboundSessionOperation& o, 
                 KAsyncContextBase::CompletionCallback asyncCallback) -> HRESULT
			{
			   // start the operation, pass any arguments required
			   return o.StartCreateSession(targetPartition, targetEndpoint, nullptr, asyncCallback);
			});

		PARTITION_TRACE_WITH_HRESULT(BeginCreateOutboundSessionCompleted,targetPartition,hr)

		return Common::ComUtility::OnPublicApiReturn(hr);
	}
        
	HRESULT ComFabricReliableSessionManager::EndCreateOutboundSession( 
		__in IFabricAsyncOperationContext *context,
		__out IFabricReliableSession **session)
	{
		PARAMETER_NULL_CHECK(ComFabricReliableSessionManager::EndCreateOutboundSession,context,"must be non-NULL")
		PARAMETER_NULL_CHECK(ComFabricReliableSessionManager::EndCreateOutboundSession,session,"must be non-NULL")

		AsyncCreateOutboundSessionOperationSPtr sessionCreateOp = nullptr;
		HRESULT hr = Ktl::Com::AsyncCallInAdapter::End(context,sessionCreateOp);

		if (SUCCEEDED(hr))
		{
			auto comSession = new (std::nothrow) ComFabricReliableSession(sessionCreateOp->GetResult());
			if (nullptr == comSession)
			{
				*session = nullptr;
				RMTRACE->ComObjectCreationFailed("ComFabricReliableSession");
				return Common::ComUtility::OnPublicApiReturn(E_OUTOFMEMORY);
			}
			else
			{
				*session = comSession;
				return Common::ComUtility::OnPublicApiReturn(S_OK);
			}
		}
		else 
		{
			return Common::ComUtility::OnPublicApiReturn(hr);
		}
	}
        

	HRESULT ComFabricReliableSessionManager::BeginRegisterInboundSessionCallback( 
		__in IFabricReliableInboundSessionCallback *listenerCallback,
		__in IFabricAsyncOperationCallback *callback,
		__out IFabricAsyncOperationContext **context)
	{
		PARAMETER_NULL_CHECK(ComFabricReliableSessionManager::BeginRegisterInboundSessionCallback,context,"must be non-NULL")
		PARAMETER_NULL_CHECK(ComFabricReliableSessionManager::BeginRegisterInboundSessionCallback,listenerCallback,"must be non-NULL")

		if (sessionManager_ == nullptr) 
		{
			RMTRACE->EmptyComObjectApiCallError("BeginRegisterInboundSessionCallback","ComFabricReliableSessionManager");
			return ComUtility::OnPublicApiReturn(FABRIC_E_INVALID_OPERATION);
		}


		AsyncRegisterInboundSessionCallbackOperationSPtr registerCallbackOp = nullptr;

		auto result = AsyncRegisterInboundSessionCallbackOperation::Create(sessionManager_,registerCallbackOp);

		if (!result.IsSuccess())
		{
			HRESULT hr = result.ToHResult();
			return Common::ComUtility::OnPublicApiReturn(hr);
		}

		HRESULT hr =  Ktl::Com::AsyncCallInAdapter::CreateAndStart(
			sessionManager_->GetThisAllocator(),
			registerCallbackOp,
			callback,                // IFabricAsyncOperationCallback
			context,                 // IFabricAsyncOperationContext (out)
			[&] (AsyncCallInAdapter&, 
                 AsyncRegisterInboundSessionCallbackOperation& o, 
                 KAsyncContextBase::CompletionCallback asyncCallback) -> HRESULT
			{
			   // start the operation, pass any arguments required
			   return o.StartRegisterInboundSessionCallback(listenerCallback, nullptr, asyncCallback);
			});

		return Common::ComUtility::OnPublicApiReturn(hr);
	}
        
	HRESULT ComFabricReliableSessionManager::EndRegisterInboundSessionCallback( 
		__in IFabricAsyncOperationContext *context,
		__out IFabricStringResult **sessionListenerEndpoint)
	{
		PARAMETER_NULL_CHECK(ComFabricReliableSessionManager::EndRegisterInboundSessionCallback,context,"must be non-NULL")
		PARAMETER_NULL_CHECK(ComFabricReliableSessionManager::EndRegisterInboundSessionCallback,sessionListenerEndpoint,"must be non-NULL")

		AsyncRegisterInboundSessionCallbackOperationSPtr registerCallbackOp = nullptr;
		HRESULT hr = Ktl::Com::AsyncCallInAdapter::End(context,registerCallbackOp);

		if (SUCCEEDED(hr))
		{
			ComStringResult *newStringResult = new (std::nothrow) ComStringResult(registerCallbackOp->GetResult());

			if (newStringResult == nullptr)
			{
				RMTRACE->ComObjectCreationFailed("ComStringResult");
				return Common::ComUtility::OnPublicApiReturn(E_OUTOFMEMORY);
			}

			*sessionListenerEndpoint = newStringResult;
		}
	
		return Common::ComUtility::OnPublicApiReturn(hr);
	}
        
	HRESULT ComFabricReliableSessionManager::BeginUnregisterInboundSessionCallback( 
		__in IFabricAsyncOperationCallback *callback,
		__out IFabricAsyncOperationContext **context)
	{
		PARAMETER_NULL_CHECK(ComFabricReliableSessionManager::BeginUnregisterInboundSessionCallback,context,"must be non-NULL")

		if (sessionManager_ == nullptr) 
		{
			RMTRACE->EmptyComObjectApiCallError("BeginUnregisterInboundSessionCallback","ComFabricReliableSessionManager");
			return ComUtility::OnPublicApiReturn(FABRIC_E_INVALID_OPERATION);
		}

		AsyncUnregisterInboundSessionCallbackOperationSPtr unregisterCallbackOp = nullptr;

		auto result = AsyncUnregisterInboundSessionCallbackOperation::Create(sessionManager_,unregisterCallbackOp);

		if (!result.IsSuccess())
		{
			HRESULT hr = result.ToHResult();
			return Common::ComUtility::OnPublicApiReturn(hr);
		}

		HRESULT hr = Ktl::Com::AsyncCallInAdapter::CreateAndStart(
			sessionManager_->GetThisAllocator(),
			unregisterCallbackOp,
			callback,                // IFabricAsyncOperationCallback
			context,                 // IFabricAsyncOperationContext (out)
			[&] (AsyncCallInAdapter&, 
                 AsyncUnregisterInboundSessionCallbackOperation& o, 
                 KAsyncContextBase::CompletionCallback asyncCallback) -> HRESULT
			{
			   // start the operation, pass any arguments required
			   return o.StartUnregisterInboundSessionCallback(nullptr, asyncCallback);
			});

			return Common::ComUtility::OnPublicApiReturn(hr);
	}
        
	HRESULT ComFabricReliableSessionManager::EndUnregisterInboundSessionCallback( 
		__in IFabricAsyncOperationContext *context)
	{
		PARAMETER_NULL_CHECK(ComFabricReliableSessionManager::EndUnregisterInboundSessionCallback,context,"must be non-NULL")
		AsyncUnregisterInboundSessionCallbackOperationSPtr unregisterCallbackOp = nullptr;
		HRESULT hr = Ktl::Com::AsyncCallInAdapter::End(context,unregisterCallbackOp);

		return Common::ComUtility::OnPublicApiReturn(hr);
	}


	/*** ComFabricReliableSessionMessage methods ***/

	HRESULT ComFabricReliableSessionMessage::CreateFromTransportMessage(
		__in Transport::MessageUPtr &&inMessage, 
		__out IFabricOperationData** result)
	{
		PARAMETER_NULL_CHECK(ComFabricReliableSessionMessage::CreateFromTransportMessage,inMessage,"must be non-NULL")
		PARAMETER_NULL_CHECK(ComFabricReliableSessionMessage::CreateFromTransportMessage,result,"must be non-NULL")

		*result = nullptr;

		ComFabricReliableSessionMessage *newMessage = new (std::nothrow) ComFabricReliableSessionMessage(std::move(inMessage));

		if (!newMessage)
		{
			RMTRACE->ComObjectCreationFailed("ComFabricReliableSessionMessage");
			return Common::ComUtility::OnPublicApiReturn(E_OUTOFMEMORY);
		}

		std::vector<Common::const_buffer> inBuffers;

		try
		{
            newMessage->inboundMessage_->GetBody(inBuffers);
			newMessage->bufferCount_ = (ULONG)inBuffers.size();
		}
		catch (std::exception const &)
		{
			RMTRACE->OutOfMemory("ComFabricReliableSessionMessage::CreateFromTransportMessage");
			return Common::ComUtility::OnPublicApiReturn(E_OUTOFMEMORY);
		}

		newMessage->dataBuffers_ = new (std::nothrow) FABRIC_OPERATION_DATA_BUFFER[newMessage->bufferCount_];

		if (newMessage->dataBuffers_ == nullptr)
		{
			RMTRACE->OutOfMemory("ComFabricReliableSessionMessage::CreateFromTransportMessage");
			return Common::ComUtility::OnPublicApiReturn(E_OUTOFMEMORY);
		}

		for (ULONG i=0; i<newMessage->bufferCount_; i++)
		{
			newMessage->dataBuffers_[i].Buffer = (BYTE*)inBuffers[i].buf;
			newMessage->dataBuffers_[i].BufferSize = inBuffers[i].len;
		}

		*result = newMessage;
		return Common::ComUtility::OnPublicApiReturn(S_OK);
	}

	HRESULT ComFabricReliableSessionMessage::GetData(__out ULONG* count, __out const FABRIC_OPERATION_DATA_BUFFER** buffers) 
	{
		PARAMETER_NULL_CHECK(ComFabricReliableSessionMessage::GetData,count,"must be non-NULL")
		PARAMETER_NULL_CHECK(ComFabricReliableSessionMessage::GetData,buffers,"must be non-NULL")

		*count = bufferCount_;
		*buffers = dataBuffers_;
		return Common::ComUtility::OnPublicApiReturn(S_OK);
	}


	// ************
	// IFabricReliableSession Methods
	// ************

	HRESULT STDMETHODCALLTYPE ComFabricReliableSession::BeginOpen(
		__in IFabricAsyncOperationCallback * callback,
		__out IFabricAsyncOperationContext ** context)
	{
		PARAMETER_NULL_CHECK(ComFabricReliableSession::BeginOpen,context,"must be non-NULL")

		if (session_ == nullptr) 
		{
			RMTRACE->EmptyComObjectApiCallError("ComFabricReliableSession::BeginOpen","ComFabricReliableSession");
			return ComUtility::OnPublicApiReturn(FABRIC_E_INVALID_OPERATION);
		}

		AsyncOutboundOpenOperationSPtr openOp = nullptr;

		auto result = AsyncOutboundOpenOperation::Create(session_->GetThisAllocator(),openOp);

		if (!result.IsSuccess())
		{
			HRESULT hr = result.ToHResult();
			return Common::ComUtility::OnPublicApiReturn(hr);
		}

		auto startOpen = [&] (AsyncCallInAdapter&, 
                 AsyncOutboundOpenOperation& o, 
                 KAsyncContextBase::CompletionCallback asyncCallback) -> HRESULT
			{
			    // start the open operation
				return o.StartOpenSession(nullptr, asyncCallback).ToHResult();
			};

		openOp->SetSessionContext(session_);

		HRESULT hr =  Ktl::Com::AsyncCallInAdapter::CreateAndStart(
			session_->GetThisAllocator(),
			openOp,
			callback,                // IFabricAsyncOperationCallback
			context,                 // IFabricAsyncOperationContext (out)
			startOpen);

		return Common::ComUtility::OnPublicApiReturn(hr);
	}

	HRESULT STDMETHODCALLTYPE ComFabricReliableSession::EndOpen(
		__in IFabricAsyncOperationContext * context)
	{
		PARAMETER_NULL_CHECK(ComFabricReliableSession::EndOpen,context,"must be non-NULL")
		AsyncOutboundOpenOperationSPtr openOp = nullptr;
		HRESULT hr = Ktl::Com::AsyncCallInAdapter::End(context,openOp);
		// TODO: this is an opportunity to release the openOp if we want to recycle it
		return Common::ComUtility::OnPublicApiReturn(hr);
	}

	HRESULT STDMETHODCALLTYPE ComFabricReliableSession::BeginClose(
		__in IFabricAsyncOperationCallback * callback,
		__out IFabricAsyncOperationContext ** context)
	{
		PARAMETER_NULL_CHECK(ComFabricReliableSession::BeginClose,context,"must be non-NULL")

		// TODO: this looks more like something to Assert
		if (session_ == nullptr) 
		{
			RMTRACE->EmptyComObjectApiCallError("ComFabricReliableSession::BeginClose","ComFabricReliableSession");
			return ComUtility::OnPublicApiReturn(FABRIC_E_INVALID_OPERATION);
		}

		AsyncOutboundCloseOperationSPtr closeOp = nullptr;

		auto result = AsyncOutboundCloseOperation::Create(session_->GetThisAllocator(),closeOp);

		if (!result.IsSuccess())
		{
			HRESULT hr = result.ToHResult();
			return Common::ComUtility::OnPublicApiReturn(hr);
		}

		auto startClose = [&] (AsyncCallInAdapter&, 
                 AsyncOutboundCloseOperation& o, 
                 KAsyncContextBase::CompletionCallback asyncCallback) -> HRESULT
			{
			    // start the open operation
				return o.StartCloseSession(nullptr, asyncCallback).ToHResult();
			};

		closeOp->SetSessionContext(session_);

		HRESULT hr =  Ktl::Com::AsyncCallInAdapter::CreateAndStart(
			session_->GetThisAllocator(),
			closeOp,
			callback,                // IFabricAsyncOperationCallback
			context,                 // IFabricAsyncOperationContext (out)
			startClose);

		return Common::ComUtility::OnPublicApiReturn(hr);
	}

	HRESULT STDMETHODCALLTYPE ComFabricReliableSession::EndClose(
		__in IFabricAsyncOperationContext * context)
	{
		PARAMETER_NULL_CHECK(ComFabricReliableSession::EndClose,context,"must be non-NULL")
		AsyncOutboundCloseOperationSPtr closeOp = nullptr;
		HRESULT hr = Ktl::Com::AsyncCallInAdapter::End(context,closeOp);
		// TODO: this is an opportunity to release the closeOp if we want to recycle it
		return Common::ComUtility::OnPublicApiReturn(hr);
	}

	HRESULT STDMETHODCALLTYPE ComFabricReliableSession::Abort()
	{
		if (session_ == nullptr) 
		{
			RMTRACE->EmptyComObjectApiCallError("Abort","ComFabricReliableSession");
			return ComUtility::OnPublicApiReturn(FABRIC_E_INVALID_OPERATION);
		}

		Common::ErrorCode abortResult = session_->AbortSession(ReliableSessionService::AbortApiCalled);

		return ComUtility::OnPublicApiReturn(abortResult.ToHResult());
	}
    
	HRESULT STDMETHODCALLTYPE ComFabricReliableSession::BeginReceive(
		__in BOOLEAN waitForMessages, 
		__in IFabricAsyncOperationCallback * callback,
		__out IFabricAsyncOperationContext ** context)
	{
		PARAMETER_NULL_CHECK(ComFabricReliableSession::BeginReceive,context,"must be non-NULL")

		if (waitForMessages != TRUE && waitForMessages != FALSE) 
		{
			RMTRACE->InvalidParameterForAPI("ComFabricReliableSession::BeginReceive","waitForMessages","Non boolean waitForMessages");
			return ComUtility::OnPublicApiReturn(E_INVALIDARG);
		}

		if (session_ == nullptr) 
		{
			RMTRACE->EmptyComObjectApiCallError("ComFabricReliableSession::BeginReceive","ComFabricReliableSession");
			return ComUtility::OnPublicApiReturn(FABRIC_E_INVALID_OPERATION);
		}

		// Check this first for efficiency: we guarantee a FABRIC_E_OBJECT_CLOSED result for normally closed sessions
		if (!session_->IsOutbound && session_->WasClosedNormally)
		{
			return ComUtility::OnPublicApiReturn(FABRIC_E_OBJECT_CLOSED);
		}
		
		if (session_->IsOutbound)
		{
			//RMTRACE
			return ComUtility::OnPublicApiReturn(FABRIC_E_INVALID_OPERATION);
		}

		bool applyQuota = waitForMessages==TRUE;
		AsyncReceiveOperationSPtr receiveOp = session_->GetAsyncReceiveOperation(applyQuota);

		if (receiveOp==nullptr)
		{
			RMTRACE->OutOfMemory("ComFabricReliableSession::BeginReceive");
			return ComUtility::OnPublicApiReturn(FABRIC_E_RELIABLE_SESSION_QUOTA_EXCEEDED);
		}

		HRESULT hr = Ktl::Com::AsyncCallInAdapter::CreateAndStart(
			session_->GetThisAllocator(),
			receiveOp,
			callback,                // IFabricAsyncOperationCallback
			context,                 // IFabricAsyncOperationContext (out)
			[&] (AsyncCallInAdapter&, 
                 AsyncReceiveOperation& o, 
                 KAsyncContextBase::CompletionCallback asyncCallback) -> HRESULT
			{
				// start the operation, pass any arguments required
				return o.StartReceiveOperation(waitForMessages,nullptr,asyncCallback).ToHResult();
			});
			
		if (FAILED(hr))
		{
			session_->ReturnAsyncReceiveOperation(receiveOp);
		}
			
		return ComUtility::OnPublicApiReturn(hr);
	}

	HRESULT STDMETHODCALLTYPE ComFabricReliableSession::EndReceive(
		__in IFabricAsyncOperationContext * context,
		__out IFabricOperationData ** msg)
	{
		PARAMETER_NULL_CHECK(ComFabricReliableSession::EndReceive,context,"must be non-NULL")
		PARAMETER_NULL_CHECK(ComFabricReliableSession::EndReceive,msg,"must be non-NULL")

		if (session_ == nullptr) 
		{
			RMTRACE->EmptyComObjectApiCallError("ComFabricReliableSession::EndReceive","ComFabricReliableSession");
			return ComUtility::OnPublicApiReturn(FABRIC_E_INVALID_OPERATION);
		}

		if (session_->IsOutbound)
		{
			//RMTRACE
			return ComUtility::OnPublicApiReturn(FABRIC_E_INVALID_OPERATION);
		}

		AsyncReceiveOperationSPtr receiveOp = nullptr;
		HRESULT hr = Ktl::Com::AsyncCallInAdapter::End(context,receiveOp);

		if (SUCCEEDED(hr)) 
		{
			*msg = receiveOp->GetReceivedMessage();
		}
		else
		{
			*msg = nullptr;
		}

		ASSERT_IF(KSHARED_IS_NULL(receiveOp), "ComFabricReliableSession::EndReceive gets NULL sendOp");
		session_->ReturnAsyncReceiveOperation(receiveOp);

		if (hr==FABRIC_E_RELIABLE_SESSION_QUEUE_EMPTY)
		{
			// the only way this happens is if BeginReceive was called with waitForMessages==FALSE, in which case there is no error
			hr = S_OK;
		}

		return ComUtility::OnPublicApiReturn(hr);
	}

	HRESULT STDMETHODCALLTYPE ComFabricReliableSession::BeginSend( 
		__in IFabricOperationData * operationDataMessage,
		__in IFabricAsyncOperationCallback *callback,
		__out IFabricAsyncOperationContext **context)
	{
		PARAMETER_NULL_CHECK(ComFabricReliableSession::BeginSend,context,"must be non-NULL")
		PARAMETER_NULL_CHECK(ComFabricReliableSession::BeginSend,operationDataMessage,"must be non-NULL")

		if (session_ == nullptr) 
		{
			RMTRACE->EmptyComObjectApiCallError("BeginSend","ComFabricReliableSession");
			return ComUtility::OnPublicApiReturn(FABRIC_E_INVALID_OPERATION);
		}

		if (!session_->IsOutbound || !session_->IsOpened())
		{
			//RMTRACE
			return ComUtility::OnPublicApiReturn(FABRIC_E_INVALID_OPERATION);
		}

		AsyncSendOperationSPtr sendOp = session_->GetAsyncSendOperation();

		if (sendOp==nullptr)
		{
			// TODO:  Create a specialized trace
			return ComUtility::OnPublicApiReturn(FABRIC_E_RELIABLE_SESSION_QUOTA_EXCEEDED);
		}

		HRESULT hr = Ktl::Com::AsyncCallInAdapter::CreateAndStart(
			session_->GetThisAllocator(),
			sendOp,
			callback,                // IFabricAsyncOperationCallback
			context,                 // IFabricAsyncOperationContext (out)
			[&] (AsyncCallInAdapter&, 
                 AsyncSendOperation& o, 
                 KAsyncContextBase::CompletionCallback asyncCallback) -> HRESULT
			{
				// start the operation, pass any arguments required
				return o.StartSendOperation(operationDataMessage,nullptr,asyncCallback).ToHResult();
			});

		if (FAILED(hr))
		{
			session_->ReturnAsyncSendOperation(sendOp);
		}
			
		return ComUtility::OnPublicApiReturn(hr);
	}

	HRESULT STDMETHODCALLTYPE ComFabricReliableSession::EndSend( 
		__in IFabricAsyncOperationContext *context)
	{
		PARAMETER_NULL_CHECK(ComFabricReliableSession::EndSend,context,"must be non-NULL")


		if (session_ == nullptr) 
		{
			RMTRACE->EmptyComObjectApiCallError("EndSend","ComFabricReliableSession");
			return ComUtility::OnPublicApiReturn(FABRIC_E_INVALID_OPERATION);
		}

		if (!session_->IsOutbound)
		{
			//RMTRACE
			return ComUtility::OnPublicApiReturn(FABRIC_E_INVALID_OPERATION);
		}

		AsyncSendOperationSPtr sendOp = nullptr;
		HRESULT hr = Ktl::Com::AsyncCallInAdapter::End(context,sendOp);
		ASSERT_IF(KSHARED_IS_NULL(sendOp), "ComFabricReliableSession::EndSend gets NULL sendOp");
		session_->ReturnAsyncSendOperation(sendOp);
		return ComUtility::OnPublicApiReturn(hr);
	}

    HRESULT STDMETHODCALLTYPE ComFabricReliableSession::GetSessionDataSnapshot( 
        __out const SESSION_DATA_SNAPSHOT **sessionDataSnapshot)
	{
		PARAMETER_NULL_CHECK(ComFabricReliableSession::GetSessionDataSnapshot,sessionDataSnapshot,"must be non-NULL")

		if (session_ == nullptr) 
		{
			RMTRACE->EmptyComObjectApiCallError("GetSessionDataSnapshot","ComFabricReliableSession");
			return ComUtility::OnPublicApiReturn(FABRIC_E_INVALID_OPERATION);
		}

		session_->GetDataSnapshot(lastSnapshot_);

		*sessionDataSnapshot = &lastSnapshot_;

		return S_OK;
	}


    // *****************************
    // IFabricMessageDataFactory methods
    // *****************************

    class ComReliableMessageData : public IFabricOperationData, private Common::ComUnknownBase
    {
        DENY_COPY(ComReliableMessageData)
        COM_INTERFACE_LIST1(ComReliableMessageData, IID_IFabricOperationData, IFabricOperationData)

    public:
        ComReliableMessageData(
            __in ULONG buffercount,
            __in ULONG *bufferSizes,
            __in const void *bufferPointers)
        {
			BYTE **realBufferPointers = (BYTE**)bufferPointers;

            for (ULONG i = 0; i < buffercount; ++i)
            {
                FABRIC_OPERATION_DATA_BUFFER buffer;
                buffer.Buffer = realBufferPointers[i];
                buffer.BufferSize = bufferSizes[i];
                messageBuffers_.push_back(buffer);
            }
        }

        ~ComReliableMessageData()
        { 
        }

        virtual HRESULT STDMETHODCALLTYPE GetData(
            __out ULONG * count,
            __out FABRIC_OPERATION_DATA_BUFFER const ** buffers)
        {
			PARAMETER_NULL_CHECK(ComReliableMessageData::GetData,count,"must be non-NULL")
			PARAMETER_NULL_CHECK(ComReliableMessageData::GetData,buffers,"must be non-NULL")

            *count = static_cast<ULONG>(messageBuffers_.size());

            if (messageBuffers_.empty())
            {
                *buffers = nullptr;
            }
            else
            {
                *buffers = messageBuffers_.data();
            }

            return ComUtility::OnPublicApiReturn(S_OK);
        }

    private:

        // This list has poitners and sizes into the above bytes_ buffer
        std::vector<FABRIC_OPERATION_DATA_BUFFER> messageBuffers_;
    };

    HRESULT ComFabricReliableSession::CreateMessageData(
            __in ULONG buffercount,
            __in ULONG *bufferSizes,
            __in const void *bufferPointers,
            __out IFabricOperationData **messageData)
    {
		PARAMETER_NULL_CHECK(ComFabricReliableSession::CreateMessageData,bufferSizes,"must be non-NULL")
		PARAMETER_NULL_CHECK(ComFabricReliableSession::CreateMessageData,bufferPointers,"must be non-NULL")

		ComReliableMessageData *msgData = new (std::nothrow) ComReliableMessageData(
            buffercount,
            bufferSizes,
			bufferPointers);

			if (nullptr == msgData)
			{
				*messageData = nullptr;
				RMTRACE->ComObjectCreationFailed("ComReliableMessageData");
				return Common::ComUtility::OnPublicApiReturn(E_OUTOFMEMORY);
			}


        *messageData = msgData;
        return ComUtility::OnPublicApiReturn(S_OK);
    }
} 
