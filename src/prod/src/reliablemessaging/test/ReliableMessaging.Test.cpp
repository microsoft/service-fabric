// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "WexTestClass.h"

using namespace Common;
using namespace std;
using namespace Transport;
using namespace ReliableMessaging;

StringLiteral const TraceType("ReliableSessionTest");

#define ENTER Trace.WriteInfo(TraceType, "Enter test: " __FUNCTION__)
#define LEAVE Trace.WriteInfo(TraceType, "Leave test: " __FUNCTION__)

namespace ReliableTransportTest
{
	IFabricOperationData *standardMessage;

	class SessionManagerContext : public KObject<SessionManagerContext>, public KShared<SessionManagerContext>
	{
	public:
		IFabricReliableSessionManager *mySessionManager;
		IFabricReliableSessionManager *targetSessionManager;

		std::wstring targetService;
		FABRIC_URI targetServiceInstanceName;
		std::wstring singletonListenAddress;

		// protocol completion fields for CreateOutboundSession
		AutoResetEvent registerListenerCompleted;
		HRESULT registerListenerResult;

		// protocol completion fields for CreateOutboundSession
		AutoResetEvent unregisterListenerCompleted;
		HRESULT unregisterListenerResult;

		// protocol completion fields for CreateOutboundSession
		AutoResetEvent openSessionManagerCompleted;
		HRESULT openSessionManagerResult;

		SessionManagerContext() : 
			mySessionManager(nullptr),
			targetSessionManager(nullptr),
			targetServiceInstanceName(nullptr),
			registerListenerResult(E_NOT_SET),
			unregisterListenerResult(E_NOT_SET),
			openSessionManagerResult(E_NOT_SET)
		{}
	};

	typedef KSharedPtr<SessionManagerContext> SessionManagerContextSPtr;
			
	class SessionContext : public KObject<SessionContext>, public KShared<SessionContext>
	{
	public:
		// session manager context
		SessionManagerContextSPtr smContext;

		// sessions
		IFabricReliableSession * inboundSession;
		IFabricReliableSession * outboundSession;

	
		// Each test should set these numbers in initialization phase
		const LONG numberOfMessages;
		LONG numberOfMessagesReceived;
		LONG numberOfSendSleepCycles;
		LONG numberOfReceiveSleepCycles;

		// Accounting for send operations
		LONG numberOfSendsInitiated;
		LONG numberOfSuccessfulSends;
		LONG numberOfCancelledSends;


		// Accounting for receive operations
		// We don't need numberOfSuccessfulReceives since that is the same as numberOfMessagesReceived
		// The special case of a closure message being received via a receive operation is accounted for as a cancelled receive
		LONG numberOfReceivesInitiated;
		LONG numberOfCancelledReceives;

		// field to ensure receiveProtocolCompleted is set only once
		LONG receiveComplete;
		// field to ensure sendProtocolCompleted is set only once
		LONG sendComplete;
		// Field to ensure Inbound session complete is only set once
		LONG inboundSessionComplete;

		// protocol completion fields for CreateOutboundSession
		AutoResetEvent createSessionCompleted;
		HRESULT createSessionResult;

		// protocol completion fields for OpenOutboundSession
		AutoResetEvent openOutboundSessionProtocolCompleted;
		HRESULT openSessionResult;

		// protocol completion fields for CloseOutboundSession
		AutoResetEvent closeOutboundSessionProtocolCompleted;
		HRESULT closeOutboundSessionResult;


		// Closure for inbound session: based on error signals
		AutoResetEvent inboundSessionClosed;
		HRESULT closeInboundSessionResult;


		// Events to wait on for protocol completion: based on expected number of messages
		AutoResetEvent sendProtocolCompleted;
		AutoResetEvent receiveProtocolCompleted;

		SessionContext(SessionManagerContextSPtr smCtx, LONG messageCount) 
		  : smContext(smCtx),
		    numberOfMessages(messageCount),
			createSessionResult(E_NOT_SET),
			openSessionResult(E_NOT_SET),
			closeOutboundSessionResult(E_NOT_SET),
			closeInboundSessionResult(E_NOT_SET)
		{}


		void Initialize()
		{
			inboundSession = nullptr;
			outboundSession = nullptr;
			numberOfSuccessfulSends = 0;	// completed with acknowledgement by receiver session
			numberOfCancelledSends = 0;	    // completed without acknowledgement
			numberOfSendsInitiated = 0;		// successful BeginSends
			numberOfMessagesReceived = 0;	// dequeued and received by service
			numberOfCancelledReceives = 0;	// receives completed without delivering a message
			numberOfSendSleepCycles = 0;
			numberOfReceiveSleepCycles = 0;
			receiveComplete = 0;
			sendComplete = 0;
			inboundSessionComplete = 0;
			numberOfReceivesInitiated = 0;
		}
	};

	typedef KSharedPtr<SessionContext> SessionContextSPtr;

	class CreateSessionAsyncCallback : public IFabricAsyncOperationCallback, private Common::ComUnknownBase
	{
        BEGIN_COM_INTERFACE_LIST(CreateSessionAsyncCallback)
        COM_INTERFACE_ITEM(IID_IUnknown, IFabricAsyncOperationCallback)
        COM_INTERFACE_ITEM(IID_IFabricAsyncOperationCallback, IFabricAsyncOperationCallback)
        END_COM_INTERFACE_LIST()
	public:
		void STDMETHODCALLTYPE  Invoke(IFabricAsyncOperationContext *context)
		{
			sessionContext_->createSessionResult = manager_->EndCreateOutboundSession(context, &sessionContext_->outboundSession);
			sessionContext_->createSessionCompleted.Set();
		}

		CreateSessionAsyncCallback(IFabricReliableSessionManager *manager, SessionContextSPtr context) : manager_(manager), sessionContext_(context)
		{
			manager_->AddRef();
		}

		~CreateSessionAsyncCallback()
		{
			manager_->Release();
		}

	private:
		IFabricReliableSessionManager *manager_;
		SessionContextSPtr sessionContext_;
	};

	class RegisterListenerAsyncCallback : public IFabricAsyncOperationCallback, private Common::ComUnknownBase
	{
        BEGIN_COM_INTERFACE_LIST(RegisterListenerAsyncCallback)
        COM_INTERFACE_ITEM(IID_IUnknown, IFabricAsyncOperationCallback)
        COM_INTERFACE_ITEM(IID_IFabricAsyncOperationCallback, IFabricAsyncOperationCallback)
        END_COM_INTERFACE_LIST()
	public:
		void STDMETHODCALLTYPE  Invoke(IFabricAsyncOperationContext *context)
		{
			IFabricStringResult *endpoint;

			sessionManagerContext_->registerListenerResult = manager_->EndRegisterInboundSessionCallback(context, &endpoint);

			if (SUCCEEDED(sessionManagerContext_->registerListenerResult))
			{
				// Just checking that we got something
				LPCWSTR actualEndpoint = endpoint->get_String();
				VERIFY_IS_NOT_NULL(actualEndpoint);
			}

			sessionManagerContext_->registerListenerCompleted.Set();
		}

		RegisterListenerAsyncCallback(IFabricReliableSessionManager *manager, SessionManagerContextSPtr context) : manager_(manager), sessionManagerContext_(context)
		{
			manager_->AddRef();
		}

		~RegisterListenerAsyncCallback()
		{
			manager_->Release();
		}

	private:
		IFabricReliableSessionManager *manager_;
		SessionManagerContextSPtr sessionManagerContext_;
	};

	class UnregisterListenerAsyncCallback : public IFabricAsyncOperationCallback, private Common::ComUnknownBase
	{
        BEGIN_COM_INTERFACE_LIST(UnregisterListenerAsyncCallback)
        COM_INTERFACE_ITEM(IID_IUnknown, IFabricAsyncOperationCallback)
        COM_INTERFACE_ITEM(IID_IFabricAsyncOperationCallback, IFabricAsyncOperationCallback)
        END_COM_INTERFACE_LIST()
	public:
		void STDMETHODCALLTYPE  Invoke(IFabricAsyncOperationContext *context)
		{
			sessionManagerContext_->unregisterListenerResult = manager_->EndUnregisterInboundSessionCallback(context);
			sessionManagerContext_->unregisterListenerCompleted.Set();
		}

		UnregisterListenerAsyncCallback(IFabricReliableSessionManager *manager, SessionManagerContextSPtr context) : manager_(manager), sessionManagerContext_(context)
		{
			manager_->AddRef();
		}

		~UnregisterListenerAsyncCallback()
		{
			manager_->Release();
		}


	private:
		IFabricReliableSessionManager *manager_;
		SessionManagerContextSPtr sessionManagerContext_;
	};


	class OpenSessionManagerAsyncCallback : public IFabricAsyncOperationCallback, private Common::ComUnknownBase
	{
        BEGIN_COM_INTERFACE_LIST(OpenSessionManagerAsyncCallback)
        COM_INTERFACE_ITEM(IID_IUnknown, IFabricAsyncOperationCallback)
        COM_INTERFACE_ITEM(IID_IFabricAsyncOperationCallback, IFabricAsyncOperationCallback)
        END_COM_INTERFACE_LIST()
	public:
		void STDMETHODCALLTYPE  Invoke(IFabricAsyncOperationContext *context)
		{
			sessionManagerContext_->openSessionManagerResult = manager_->EndOpen(context);
			sessionManagerContext_->openSessionManagerCompleted.Set();
		}

		OpenSessionManagerAsyncCallback(IFabricReliableSessionManager *sessionManager, SessionManagerContextSPtr context) 
			: manager_(sessionManager), sessionManagerContext_(context)
		{
			manager_->AddRef();
		}

		~OpenSessionManagerAsyncCallback()
		{
			manager_->Release();
		}

	private:
		IFabricReliableSessionManager *manager_;
		SessionManagerContextSPtr sessionManagerContext_;
	};

	class OpenOutboundSessionAsyncCallback : public IFabricAsyncOperationCallback, private Common::ComUnknownBase
	{
        BEGIN_COM_INTERFACE_LIST(OpenOutboundSessionAsyncCallback)
        COM_INTERFACE_ITEM(IID_IUnknown, IFabricAsyncOperationCallback)
        COM_INTERFACE_ITEM(IID_IFabricAsyncOperationCallback, IFabricAsyncOperationCallback)
        END_COM_INTERFACE_LIST()
	public:
		void STDMETHODCALLTYPE  Invoke(IFabricAsyncOperationContext *context)
		{
			sessionContext_->openSessionResult = session_->EndOpen(context);
			sessionContext_->openOutboundSessionProtocolCompleted.Set();
		}

		OpenOutboundSessionAsyncCallback(IFabricReliableSession *session, SessionContextSPtr context) : session_(session), sessionContext_(context)
		{
			session_->AddRef();
		}

		~OpenOutboundSessionAsyncCallback()
		{
			session_->Release();
		}


	private:
		IFabricReliableSession *session_;
		SessionContextSPtr sessionContext_;
	};


	class CloseOutboundSessionAsyncCallback : public IFabricAsyncOperationCallback, private Common::ComUnknownBase
	{
        BEGIN_COM_INTERFACE_LIST(CloseOutboundSessionAsyncCallback)
        COM_INTERFACE_ITEM(IID_IUnknown, IFabricAsyncOperationCallback)
        COM_INTERFACE_ITEM(IID_IFabricAsyncOperationCallback, IFabricAsyncOperationCallback)
        END_COM_INTERFACE_LIST()
	public:
		void STDMETHODCALLTYPE  Invoke(IFabricAsyncOperationContext *context)
		{
			sessionContext_->closeOutboundSessionResult = session_->EndClose(context);
			sessionContext_->closeOutboundSessionProtocolCompleted.Set();
		}

		CloseOutboundSessionAsyncCallback(IFabricReliableSession *session, SessionContextSPtr context) : session_(session), sessionContext_(context)
		{
			session_->AddRef();
		}

		~CloseOutboundSessionAsyncCallback()
		{
			session_->Release();
		}

	private:
		IFabricReliableSession *session_;
		SessionContextSPtr sessionContext_;
	};




	// using a single instance of this class for callback for multiple send contexts -- InterlockedIncrement serializes the work
	class SendAsyncCallback : public IFabricAsyncOperationCallback, private Common::ComUnknownBase
	{
        BEGIN_COM_INTERFACE_LIST(SendAsyncCallback)
        COM_INTERFACE_ITEM(IID_IUnknown, IFabricAsyncOperationCallback)
        COM_INTERFACE_ITEM(IID_IFabricAsyncOperationCallback, IFabricAsyncOperationCallback)
        END_COM_INTERFACE_LIST()
	public:
		void STDMETHODCALLTYPE  Invoke(IFabricAsyncOperationContext *context)
		{
			HRESULT hr = session_->EndSend(context);

			// E_ABORT occurs when the outbound session is aborted and an outbound message is cancelled in the outbound buffer
			// FABRIC_E_INVALID_OPERATION will occur when the outbound session is aborted and the start send operation fails 
			// FABRIC_E_INVALID_OPERATION will NOT occur in the completion callback when the inbound session on the receiving side is aborted 
			//	  and the message is rejected because the outbound session will be aborted if this response is received from the receiver
			// FABRIC_E_RELIABLE_SESSION_NOT_FOUND must not occur given the current design: Aborted inbound sessions leave tombstones
			VERIFY_IS_TRUE(SUCCEEDED(hr) || hr==E_ABORT || hr==FABRIC_E_INVALID_OPERATION);

			if (SUCCEEDED(hr))  // we are counting towards the expected number of sends
			{
				LONG observedSentMessages = InterlockedIncrement(&sessionContext_->numberOfSuccessfulSends);

					std::cout << sessionContext_->numberOfSuccessfulSends << "th send completed\n";

				if (observedSentMessages==sessionContext_->numberOfMessages)  // sessionContext_->numberOfMessages is a const
				{
					LONG sendCompleteOldValue = InterlockedCompareExchange(&sessionContext_->sendComplete, 1, 0);

					if (sendCompleteOldValue==0)
					{
						// first time through
						sessionContext_->sendProtocolCompleted.Set();
					}
				}
			}
			else  // the session was aborted, set send protocol complete event if all expected cancels have happened
			{
				LONG observedCancelledMessages = InterlockedIncrement(&sessionContext_->numberOfCancelledSends);

				// every successfully initiated send operation must either complete because it gets an Ack or Nack or
				// because it is cancelled from the outbound buffer during outbound session abort
				if (sessionContext_->numberOfSuccessfulSends + observedCancelledMessages == sessionContext_->numberOfSendsInitiated)
				{
					LONG sendCompleteOldValue = InterlockedCompareExchange(&sessionContext_->sendComplete, 1, 0);

					if (sendCompleteOldValue==0)
					{
						// first time through
						sessionContext_->sendProtocolCompleted.Set();
					}
				}
			}
		}

		SendAsyncCallback(IFabricReliableSession *session, SessionContextSPtr context) : session_(session), sessionContext_(context)
		{
			session_->AddRef();
		}

		~SendAsyncCallback()
		{
			session_->Release();
		}

	private:
		IFabricReliableSession *session_;
		SessionContextSPtr sessionContext_;
	};


	// using a single instance of this class for callback for multiple receive contexts -- InterlockedIncrement serializes the work
	class ReceiveAsyncCallback : public IFabricAsyncOperationCallback, private Common::ComUnknownBase
	{
        BEGIN_COM_INTERFACE_LIST(ReceiveAsyncCallback)
        COM_INTERFACE_ITEM(IID_IUnknown, IFabricAsyncOperationCallback)
        COM_INTERFACE_ITEM(IID_IFabricAsyncOperationCallback, IFabricAsyncOperationCallback)
        END_COM_INTERFACE_LIST()
	public:
		void STDMETHODCALLTYPE  Invoke(IFabricAsyncOperationContext *context)
		{
			HRESULT receiveAsyncCallbackHresult = E_FAIL;
			IFabricOperationData *messageReceived;
			receiveAsyncCallbackHresult = session_->EndReceive(context, &messageReceived);

			// either receive succeeded or the session closed before it could be fulfilled
			VERIFY_IS_TRUE(SUCCEEDED(receiveAsyncCallbackHresult) || receiveAsyncCallbackHresult == E_ABORT || receiveAsyncCallbackHresult == FABRIC_E_OBJECT_CLOSED);

			if (SUCCEEDED(receiveAsyncCallbackHresult))
			{
				InterlockedIncrement(&sessionContext_->numberOfMessagesReceived);
					
				std::cout << sessionContext_->numberOfMessagesReceived << "th receive completed\n";
			}
			else if (receiveAsyncCallbackHresult == FABRIC_E_OBJECT_CLOSED) // session closed normally
			{
				// We account for this as a cancelled receive because it does not actually correspond to a received message
				InterlockedIncrement(&sessionContext_->numberOfCancelledReceives);

				// is this the first time we are trying to set inboundSessionClosed?
				LONG inboundSessionCompleteOldValue = InterlockedCompareExchange(&sessionContext_->inboundSessionComplete, 1, 0);

				// this can only happen once because if the session closes normally it cannot also close abnormally
				VERIFY_IS_TRUE(inboundSessionCompleteOldValue==0); 

				if (inboundSessionCompleteOldValue==0)
				{
					// first time through
					sessionContext_->inboundSessionClosed.Set();
					sessionContext_->closeInboundSessionResult = FABRIC_E_OBJECT_CLOSED;
				}

				LONG receiveCompleteOldValue = InterlockedCompareExchange(&sessionContext_->receiveComplete, 1, 0);

				// this can only happen once because if the session closes normally it cannot also close abnormally
				VERIFY_IS_TRUE(receiveCompleteOldValue==0); 

				if (receiveCompleteOldValue==0)
				{
					// first time through
					sessionContext_->receiveProtocolCompleted.Set();
				}			
			}
			else
			{
				// This is the cancellation of an extra outstanding receive after any sort of closure
				InterlockedIncrement(&sessionContext_->numberOfCancelledReceives);
			}

		}

		ReceiveAsyncCallback(IFabricReliableSession *session, SessionContextSPtr context) : session_(session), sessionContext_(context)
		{
			session_->AddRef();
		}

		~ReceiveAsyncCallback()
		{
			session_->Release();
		}

	private:
		IFabricReliableSession *session_;
		SessionContextSPtr sessionContext_;
	};

	// TODO: This is not being tested currently: need a way to initialize sessionContext_ correctly
	class SessionAbortedCallback : public IFabricReliableSessionAbortedCallback, private Common::ComUnknownBase
	{
        BEGIN_COM_INTERFACE_LIST(SessionAbortedCallback)
        COM_INTERFACE_ITEM(IID_IUnknown, SessionAbortedCallback)
        COM_INTERFACE_ITEM(IID_IFabricReliableSessionAbortedCallback, IFabricReliableSessionAbortedCallback)
        END_COM_INTERFACE_LIST()

	public:

		SessionAbortedCallback(SessionContextSPtr context) : sessionContext_(context)
		{}

		HRESULT STDMETHODCALLTYPE SessionAbortedByPartner(
			__in BOOLEAN isInbound,
			__in IFabricReliableSessionPartitionIdentity *partitionId,
            __in IFabricReliableSession *session) 
		{
			FABRIC_URI rawServiceInstanceName = partitionId->getServiceName();
			FABRIC_PARTITION_KEY_TYPE keyType = partitionId->getPartitionKeyType();
			void * partitionKey = partitionId->getPartitionKey();
			std::wstring stringKey = L"";
			int64 intKey = 0;

			std::wstring serviceInstanceName = rawServiceInstanceName;

			if (keyType == FABRIC_PARTITION_KEY_TYPE_INT64) intKey = *((int64*)partitionKey);
			if (keyType == FABRIC_PARTITION_KEY_TYPE_STRING) stringKey = (LPCWSTR)partitionKey;

			UNREFERENCED_PARAMETER(serviceInstanceName);
			UNREFERENCED_PARAMETER(keyType);
			UNREFERENCED_PARAMETER(intKey);
			UNREFERENCED_PARAMETER(stringKey);
			UNREFERENCED_PARAMETER(session);
			UNREFERENCED_PARAMETER(isInbound);

			return Common::ComUtility::OnPublicApiReturn(S_OK);
		}

	private:

		SessionContextSPtr sessionContext_;
	};

	class DummyInboundSessionCallback : public IFabricReliableInboundSessionCallback, private Common::ComUnknownBase
	{
        BEGIN_COM_INTERFACE_LIST(DummyInboundSessionCallback)
        COM_INTERFACE_ITEM(IID_IUnknown, DummyInboundSessionCallback)
        COM_INTERFACE_ITEM(IID_IFabricReliableInboundSessionCallback, IFabricReliableInboundSessionCallback)
        END_COM_INTERFACE_LIST()

	public:

		DummyInboundSessionCallback(SessionContextSPtr context, BOOLEAN willAccept) : sessionContext_(context), willAccept_(willAccept)
		{}

		HRESULT STDMETHODCALLTYPE AcceptInboundSession(
			__in IFabricReliableSessionPartitionIdentity *partitionId,
            __in IFabricReliableSession *session,
            __out LONG *response) 
		{
			/*
			FABRIC_URI rawServiceInstanceName = partitionId->getServiceName();
			FABRIC_PARTITION_KEY_TYPE keyType = partitionId->getPartitionKeyType();
			void * partitionKey = partitionId->getPartitionKey();
			std::wstring stringKey = L"";
			int64 intKey = 0;

			std::wstring serviceInstanceName = rawServiceInstanceName;

			if (keyType == FABRIC_PARTITION_KEY_TYPE_INT64) intKey = *((int64*)partitionKey);
			if (keyType == FABRIC_PARTITION_KEY_TYPE_STRING) stringKey = (LPCWSTR)partitionKey;
			*/

			UNREFERENCED_PARAMETER(partitionId);

			*response = willAccept_;

			if (willAccept_==TRUE)
			{
				sessionContext_->inboundSession = session;
			}
			else
			{
				sessionContext_->inboundSession = nullptr;
			}

			return Common::ComUtility::OnPublicApiReturn(S_OK);
		}

	private:

		SessionContextSPtr sessionContext_;
		BOOLEAN willAccept_;
	};

	class DummySessionAbortedCallback : public IFabricReliableSessionAbortedCallback, private Common::ComUnknownBase
	{
        BEGIN_COM_INTERFACE_LIST(DummySessionAbortedCallback)
        COM_INTERFACE_ITEM(IID_IUnknown, DummySessionAbortedCallback)
        COM_INTERFACE_ITEM(IID_IFabricReliableSessionAbortedCallback, IFabricReliableSessionAbortedCallback)
        END_COM_INTERFACE_LIST()

	public:
		HRESULT STDMETHODCALLTYPE SessionAbortedByPartner(
			__in BOOLEAN isInbound,
			__in IFabricReliableSessionPartitionIdentity *partitionId,
            __in IFabricReliableSession *session) 
		{
			UNREFERENCED_PARAMETER(partitionId);
			UNREFERENCED_PARAMETER(session);
			UNREFERENCED_PARAMETER(isInbound);
			return Common::ComUtility::OnPublicApiReturn(S_OK);
		}
	};


	class ReliableMessagingBasicTests : public WEX::TestClass<ReliableMessagingBasicTests>
    {
    public:
        TEST_CLASS(ReliableMessagingBasicTests);

        TEST_METHOD(LoopbackSessionNormalAndAbort);

    private:

        void SendMessages(LONG count, SessionContextSPtr);
        void ReceiveMessagesWait(SessionContextSPtr);

		void StartSessionManagers(SessionManagerContextSPtr);
		HRESULT RegisterListener(SessionManagerContextSPtr, SessionContextSPtr sessionContext, BOOLEAN willAccept); 
		HRESULT CreateSession(SessionManagerContextSPtr, SessionContextSPtr);
		HRESULT OpenSession(SessionContextSPtr);
		HRESULT UnregisterListener(SessionManagerContextSPtr);
		HRESULT CloseSession(SessionContextSPtr);
		void RunSessionTest(LONG messageCount, SessionManagerContextSPtr);
		void RunInboundSessionAbortTest(LONG messageCount, SessionManagerContextSPtr);
    };

	 FABRIC_OPERATION_DATA_BUFFER makeFODB(const char *bufferdata)
	 {
		 FABRIC_OPERATION_DATA_BUFFER result;
		 result.BufferSize = (ULONG) strlen(bufferdata) + 1;
		 result.Buffer = (BYTE*)bufferdata;
		 return result;
	 }

	typedef struct FABRIC_RELIABLE_MESSAGE_CONTENT
    {
		ULONG bufferCount;
        FABRIC_OPERATION_DATA_BUFFER* buffers;
    } 	FABRIC_RELIABLE_MESSAGE_CONTENT;


	class TestReliableSessionMessage : public IFabricOperationData, Common::ComUnknownBase
    {
        BEGIN_COM_INTERFACE_LIST(TestReliableSessionMessage)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricOperationData)
            COM_INTERFACE_ITEM(IID_IFabricOperationData, IFabricOperationData)
        END_COM_INTERFACE_LIST()

    public:
 
		static HRESULT CreateFromMessageContent(
			__in FABRIC_RELIABLE_MESSAGE_CONTENT content, 
			__out IFabricOperationData** result)
		{
			TestReliableSessionMessage *newMessage = new (std::nothrow) TestReliableSessionMessage;

			if (!newMessage)
			{
				return Common::ComUtility::OnPublicApiReturn(E_OUTOFMEMORY);
			}

			newMessage->bufferCount_ = content.bufferCount;
			newMessage->dataBuffers_ = content.buffers;

			*result = newMessage;
			return Common::ComUtility::OnPublicApiReturn(S_OK);
		}

    private:

        HRESULT STDMETHODCALLTYPE
        GetData(__out ULONG* count, __out const FABRIC_OPERATION_DATA_BUFFER** buffers) override
		{
			*count = bufferCount_;
			*buffers = dataBuffers_;
			return Common::ComUtility::OnPublicApiReturn(S_OK);
		}


		~TestReliableSessionMessage()
		{
			if (dataBuffers_)
			{
				delete dataBuffers_;
			}
		}

		ULONG bufferCount_;
        FABRIC_OPERATION_DATA_BUFFER* dataBuffers_;       
    };

	 IFabricOperationData *CreateMessage()
	{
		FABRIC_RELIABLE_MESSAGE_CONTENT messageContent;

		messageContent.bufferCount = 2;
		messageContent.buffers = new FABRIC_OPERATION_DATA_BUFFER[2];

		messageContent.buffers[0] = makeFODB("buffer#1");
		messageContent.buffers[1] = makeFODB("buffer#2");

		IFabricOperationData *result = nullptr;

		HRESULT hr = TestReliableSessionMessage::CreateFromMessageContent(messageContent,&result);
		VERIFY_IS_TRUE(SUCCEEDED(hr));

		return result;
	}

	void ReliableMessagingBasicTests::StartSessionManagers(SessionManagerContextSPtr smContext)
	{		
		Common::ErrorCode result = ReliableMessagingTransport::GetSingletonListenAddress(smContext->singletonListenAddress);
		VERIFY_IS_TRUE(result.IsSuccess());

		std::wstring myService = L"fabric:/service/my_service";
		FABRIC_URI myServiceInstanceName = myService.c_str();

		INTEGER_PARTITION_KEY_RANGE myPartitionKey;
		myPartitionKey.IntegerKeyLow = 514326;
		myPartitionKey.IntegerKeyHigh = 514326;
		// LPCWSTR targetPartitionKey = L"TargetPartitionKey";

		HOST_SERVICE_PARTITION_INFO dummyInfo;

		// use dummy partition ID
		dummyInfo.PartitionId = Guid::NewGuid().AsGUID();
		dummyInfo.ReplicaId = 0;

		HRESULT hr = ComFabricReliableSessionManager::Create(
														&dummyInfo,
														myServiceInstanceName,
														FABRIC_PARTITION_KEY_TYPE_INT64,
														&myPartitionKey,
														&smContext->mySessionManager);

		VERIFY_IS_TRUE(SUCCEEDED(hr));

        OpenSessionManagerAsyncCallback *openSessionManagerCallback = new OpenSessionManagerAsyncCallback(smContext->mySessionManager, smContext);

		IFabricAsyncOperationContext *context;
		DummySessionAbortedCallback *abortedCallback = new DummySessionAbortedCallback();

		smContext->openSessionManagerResult = E_NOT_SET;
		hr = smContext->mySessionManager->BeginOpen(abortedCallback,openSessionManagerCallback,&context);
		VERIFY_IS_TRUE(SUCCEEDED(hr));
		context->Release();

		// Wait for protocol to work through
		bool protocolCompleted = smContext->openSessionManagerCompleted.WaitOne(TimeSpan::FromSeconds(50));
		VERIFY_IS_TRUE(protocolCompleted);
		VERIFY_IS_TRUE(SUCCEEDED(smContext->openSessionManagerResult));
		openSessionManagerCallback->Release();
		openSessionManagerCallback = nullptr;
		context = nullptr;

		smContext->targetService = L"fabric:/service/target_service";
		smContext->targetServiceInstanceName = smContext->targetService.c_str();

		// use different dummy partition ID
		dummyInfo.PartitionId = Guid::NewGuid().AsGUID();

		hr = ComFabricReliableSessionManager::Create(
												&dummyInfo,
												smContext->targetServiceInstanceName,
												FABRIC_PARTITION_KEY_TYPE_NONE,
												nullptr,
												&smContext->targetSessionManager);
		VERIFY_IS_TRUE(SUCCEEDED(hr));

        openSessionManagerCallback = new OpenSessionManagerAsyncCallback(smContext->targetSessionManager, smContext);

		smContext->openSessionManagerResult = E_NOT_SET;
		hr = smContext->targetSessionManager->BeginOpen(abortedCallback,openSessionManagerCallback,&context);
		VERIFY_IS_TRUE(SUCCEEDED(hr));
		context->Release();

		// Wait for protocol to work through
		protocolCompleted = smContext->openSessionManagerCompleted.WaitOne(TimeSpan::FromSeconds(50));
		VERIFY_IS_TRUE(protocolCompleted);
		VERIFY_IS_TRUE(SUCCEEDED(smContext->openSessionManagerResult));
		openSessionManagerCallback->Release();
		openSessionManagerCallback = nullptr;
		context = nullptr;
	}
 
	HRESULT ReliableMessagingBasicTests::RegisterListener(SessionManagerContextSPtr smContext, SessionContextSPtr sessionContext, BOOLEAN willAccept)
	{
		IFabricAsyncOperationContext *context = nullptr;

		IFabricAsyncOperationCallback *registerCallback = new RegisterListenerAsyncCallback(smContext->targetSessionManager, smContext);

		DummyInboundSessionCallback *acceptSessionCallback = new DummyInboundSessionCallback(sessionContext,willAccept);

		HRESULT hr = smContext->targetSessionManager->BeginRegisterInboundSessionCallback(
																			acceptSessionCallback,
																			registerCallback,
																			&context);

		if (FAILED(hr))
		{
			return hr;
		}

		context->Release();
		context = nullptr;

		bool registerCompleted = smContext->registerListenerCompleted.WaitOne(TimeSpan::FromSeconds(50));
        VERIFY_IS_TRUE(registerCompleted);	
		registerCallback->Release();

		return smContext->registerListenerResult;
	}

	HRESULT ReliableMessagingBasicTests::CreateSession(SessionManagerContextSPtr smContext, SessionContextSPtr sessionContext)
    {				
		IFabricAsyncOperationContext *context = nullptr;

        IFabricAsyncOperationCallback *createCallback = new CreateSessionAsyncCallback(smContext->mySessionManager, sessionContext);

		HRESULT hr = smContext->mySessionManager->BeginCreateOutboundSession(
													smContext->targetServiceInstanceName,
													FABRIC_PARTITION_KEY_TYPE_NONE,
													nullptr,
													smContext->singletonListenAddress.c_str(),
													createCallback,
													&context);

		if (FAILED(hr))
		{
			return hr;
		}

		context->Release();
		context = nullptr;

		bool createCompleted = sessionContext->createSessionCompleted.WaitOne(TimeSpan::FromSeconds(50));
		VERIFY_IS_TRUE(createCompleted);
		createCallback->Release();

		return sessionContext->createSessionResult;
	}

	HRESULT ReliableMessagingBasicTests::OpenSession(SessionContextSPtr sessionContext)
    {				
		IFabricAsyncOperationContext *context = nullptr;
        OpenOutboundSessionAsyncCallback *openOutboundSessionCallback = new OpenOutboundSessionAsyncCallback(sessionContext->outboundSession, sessionContext);

		HRESULT hr = sessionContext->outboundSession->BeginOpen(openOutboundSessionCallback,&context);

		if (FAILED(hr))
		{
			return hr;
		}

		context->Release();
		context = nullptr;

		// Wait for protocol to work through
		bool protocolCompleted = sessionContext->openOutboundSessionProtocolCompleted.WaitOne(TimeSpan::FromSeconds(50));
        VERIFY_IS_TRUE(protocolCompleted);
		openOutboundSessionCallback->Release();

		return sessionContext->openSessionResult;
	}

	HRESULT ReliableMessagingBasicTests::UnregisterListener(SessionManagerContextSPtr smContext)
	{
		IFabricAsyncOperationContext *context = nullptr;
		IFabricAsyncOperationCallback *unregisterCallback = new UnregisterListenerAsyncCallback(smContext->targetSessionManager, smContext);
		                                             
		HRESULT hr = smContext->targetSessionManager->BeginUnregisterInboundSessionCallback(
																			unregisterCallback,
																			&context);

		if (FAILED(hr))
		{
			return hr;
		}

		context->Release();
		context = nullptr;

		bool unregisterCompleted = smContext->unregisterListenerCompleted.WaitOne(TimeSpan::FromSeconds(50));
        VERIFY_IS_TRUE(unregisterCompleted);
		unregisterCallback->Release();

		return smContext->unregisterListenerResult;
	}

	HRESULT ReliableMessagingBasicTests::CloseSession(SessionContextSPtr sessionContext)
	{
		IFabricAsyncOperationContext *context = nullptr;
		CloseOutboundSessionAsyncCallback* closeCallback = new CloseOutboundSessionAsyncCallback(sessionContext->outboundSession, sessionContext);

		HRESULT hr = sessionContext->outboundSession->BeginClose(closeCallback,&context);

		if (FAILED(hr))
		{
			return hr;
		}

		context->Release();
		context = nullptr;

		bool outboundCloseCompleted = sessionContext->closeOutboundSessionProtocolCompleted.WaitOne(TimeSpan::FromSeconds(50));
		VERIFY_IS_TRUE(outboundCloseCompleted);

		if (SUCCEEDED(sessionContext->closeOutboundSessionResult))
		{
			bool inboundCloseCompleted = sessionContext->inboundSessionClosed.WaitOne(TimeSpan::FromSeconds(50));
			VERIFY_IS_TRUE(inboundCloseCompleted);
		}

		sessionContext->inboundSession->Release();
		sessionContext->outboundSession->Release();

		return sessionContext->closeOutboundSessionResult;
	}

	void ReliableMessagingBasicTests::SendMessages(LONG count, SessionContextSPtr sessionContext)
	{
		Sleep(100); // this will run on a separate thread: give time for other threads to get going
		//IFabricReliableSession *outboundSession = sessionContext->outboundSession;
		IFabricAsyncOperationCallback *sendCallback = new SendAsyncCallback(sessionContext->outboundSession, sessionContext);

		for (int i = 0; i<count; i++)
		{
			IFabricAsyncOperationContext *context = nullptr;
			HRESULT sendResult = sessionContext->outboundSession->BeginSend(standardMessage,sendCallback,&context);
			for (;sendResult==FABRIC_E_RELIABLE_SESSION_QUOTA_EXCEEDED;)
			{
				Sleep(100);
				sendResult = sessionContext->outboundSession->BeginSend(standardMessage,sendCallback,&context);
				InterlockedIncrement(&sessionContext->numberOfSendSleepCycles);

				std::cout << sessionContext->numberOfSendSleepCycles << "th send sleep cycle\n";
			}

			if (SUCCEEDED(sendResult))
			{
				context->Release();
				InterlockedIncrement(&sessionContext->numberOfSendsInitiated);
				std::cout << sessionContext->numberOfSendsInitiated << " sends initiated\n";
			}
			else
			{
				VERIFY_IS_TRUE(sendResult==FABRIC_E_INVALID_OPERATION);	// the session closed abnormally

				LONG sendCompleteOldValue = InterlockedCompareExchange(&sessionContext->sendComplete, 1, 0);

				if (sendCompleteOldValue==0)
				{
					// first time through
					sessionContext->sendProtocolCompleted.Set();
				}

				break;
			}
		}

		sendCallback->Release();
	}

	void ReliableMessagingBasicTests::ReceiveMessagesWait(SessionContextSPtr sessionContext)
	{
		// This local variable seems to change under us dynamically when SendMessages is launched as a parallel thread: mystery
		// IFabricReliableSession *inboundSession = sessionContext->inboundSession;
        IFabricAsyncOperationCallback *receiveCallback = new ReceiveAsyncCallback(sessionContext->inboundSession, sessionContext);

		HRESULT receiveResult = S_OK;
		while (SUCCEEDED(receiveResult))
		{
			IFabricAsyncOperationContext *context = nullptr;
			receiveResult = sessionContext->inboundSession->BeginReceive(TRUE,receiveCallback,&context);
			for (;receiveResult==FABRIC_E_RELIABLE_SESSION_QUOTA_EXCEEDED;)
			{
				Sleep(100);
				//context = nullptr;
				receiveResult = sessionContext->inboundSession->BeginReceive(TRUE,receiveCallback,&context);
				InterlockedIncrement(&sessionContext->numberOfReceiveSleepCycles);
				std::cout << sessionContext->numberOfReceiveSleepCycles << "th receive sleep cycle\n";
			}

			if (SUCCEEDED(receiveResult))
			{
				//if (context->CompletedSynchronously()) DbgBreakPoint();
				context->Release();
				InterlockedIncrement(&sessionContext->numberOfReceivesInitiated);
				std::cout << sessionContext->numberOfReceivesInitiated << "th receive initiated\n";
			}
			else
			{
				VERIFY_IS_TRUE(receiveResult == FABRIC_E_INVALID_OPERATION || receiveResult == FABRIC_E_OBJECT_CLOSED);

				// Of the three possible failure return types from a receive process, only two are  possible here
				// 1. FABRIC_E_OBJECT_CLOSED if the session closed normally after we dequeued the actual session closure message from the send-side
				// 2. FABRIC_E_INVALID_OPERATION when the session is not open for receive for whatever other reason

			 
				if (receiveResult == FABRIC_E_OBJECT_CLOSED) // session closed normally
				{
					/*
					// is this the first time we are trying to set inboundSessionClosed?
					LONG inboundSessionCompleteOldValue = InterlockedCompareExchange(&sessionContext->inboundSessionComplete, 1, 0);

					if (inboundSessionCompleteOldValue==0)
					{
						// first time through
						sessionContext->inboundSessionClosed.Set();
						sessionContext->closeInboundSessionResult = FABRIC_E_OBJECT_CLOSED;
					}

					// This does not mean the receiveProtocolCompleted should be set: there could be a race with receive operation completes
					*/
				}
				else if (receiveResult == FABRIC_E_INVALID_OPERATION) // session closed abnormally
				{
					// is this the first time we are trying to set inboundSessionClosed?
					LONG inboundSessionCompleteOldValue = InterlockedCompareExchange(&sessionContext->inboundSessionComplete, 1, 0);

					if (inboundSessionCompleteOldValue==0)
					{
						// first time through
						sessionContext->inboundSessionClosed.Set();
						sessionContext->closeInboundSessionResult = FABRIC_E_INVALID_OPERATION;
					}

					LONG receiveCompleteOldValue = InterlockedCompareExchange(&sessionContext->receiveComplete, 1, 0);

					if (receiveCompleteOldValue==0)
					{
						// first time through
						sessionContext->receiveProtocolCompleted.Set();
					}
				}
			}
		}

		receiveCallback->Release();
	}
    
	void ReliableMessagingBasicTests::RunSessionTest(LONG messageCount, SessionManagerContextSPtr smContext)
	{
		SessionContextSPtr sessionContext = _new(RMSG_ALLOCATION_TAG, Common::GetSFDefaultPagedKAllocator())
												SessionContext(smContext, messageCount);
						 						     			 
		VERIFY_IS_FALSE(KSHARED_IS_NULL(sessionContext));
		VERIFY_IS_TRUE(NT_SUCCESS(sessionContext->Status()));

		sessionContext->Initialize();

		HRESULT registerResult = RegisterListener(smContext,sessionContext,TRUE);
		VERIFY_IS_TRUE(SUCCEEDED(registerResult));

		HRESULT createResult = CreateSession(smContext,sessionContext);
		VERIFY_IS_TRUE(SUCCEEDED(createResult));

		HRESULT openResult = OpenSession(sessionContext);
		VERIFY_IS_TRUE(SUCCEEDED(openResult));
		VERIFY_IS_TRUE(sessionContext->outboundSession != nullptr && sessionContext->inboundSession != nullptr);

		Common::StopwatchTime startTime = Common::Stopwatch::Now();
		standardMessage = CreateMessage();

		//SendMessages(messageCount,sessionContext);

		LONG halfCount1 = messageCount/2;
		LONG halfCount2 = messageCount-halfCount1;

		Common::Threadpool::Post([this,halfCount1,sessionContext](){this->SendMessages(halfCount1,sessionContext);});
		Common::Threadpool::Post([this,halfCount2,sessionContext](){this->SendMessages(halfCount2,sessionContext);});
		Common::Threadpool::Post([this,sessionContext](){this->ReceiveMessagesWait(sessionContext);});
		Common::Threadpool::Post([this,sessionContext](){this->ReceiveMessagesWait(sessionContext);});

		bool sendCompleted = sessionContext->sendProtocolCompleted.WaitOne(TimeSpan::FromSeconds(5000));
		VERIFY_IS_TRUE(sendCompleted);

		// Now close the session so receive protocol can complete with a closure message sent and received
		HRESULT closeResult = CloseSession(sessionContext);
		VERIFY_IS_TRUE(SUCCEEDED(closeResult));

		bool receiveCompleted = sessionContext->receiveProtocolCompleted.WaitOne(TimeSpan::FromSeconds(5000));
		VERIFY_IS_TRUE(receiveCompleted);

		const SESSION_DATA_SNAPSHOT* dataSnapshot;
		HRESULT dataSnapshotResult = sessionContext->outboundSession->GetSessionDataSnapshot(&dataSnapshot);
		VERIFY_IS_TRUE(SUCCEEDED(dataSnapshotResult));

		VERIFY_IS_TRUE(dataSnapshot->LastOutboundSequenceNumber==messageCount-1);

		std::cout << "NormalTest Completing, Verifying outcomes\n";

		std::cout << "numberOfMessages = " << sessionContext->numberOfMessages << "\n";
		std::cout << "numberOfReceivesInitiated = " << sessionContext->numberOfReceivesInitiated << "\n";
		std::cout << "numberOfMessagesReceived = " << sessionContext->numberOfMessagesReceived << "\n";
		std::cout << "numberOfCancelledReceives = " << sessionContext->numberOfCancelledReceives << "\n";
		std::cout << "numberOfSendsInitiated = " << sessionContext->numberOfSendsInitiated << "\n";
		std::cout << "numberOfSuccessfulSends = " << sessionContext->numberOfSuccessfulSends << "\n";
		std::cout << "numberOfCancelledSends = " << sessionContext->numberOfCancelledSends << "\n";
		std::cout << "\n";

		// this is the normal close case there should be no cancellations of sends or receives
		VERIFY_IS_TRUE(sessionContext->numberOfMessages==sessionContext->numberOfMessagesReceived);
		VERIFY_IS_TRUE(sessionContext->numberOfMessages==sessionContext->numberOfSuccessfulSends);

		Common::StopwatchTime finishTime = Common::Stopwatch::Now();
		int64 elapsedTime = (finishTime-startTime).TotalPositiveMilliseconds()/1000;
		UNREFERENCED_PARAMETER(elapsedTime);
    
		HRESULT unregisterResult = UnregisterListener(smContext);
		VERIFY_IS_TRUE(SUCCEEDED(unregisterResult));
	}

	void ReliableMessagingBasicTests::RunInboundSessionAbortTest(LONG messageCount, SessionManagerContextSPtr smContext)
	{
		SessionContextSPtr sessionContext = _new(RMSG_ALLOCATION_TAG, Common::GetSFDefaultPagedKAllocator())
												SessionContext(smContext, messageCount);
						 						     			 
		VERIFY_IS_FALSE(KSHARED_IS_NULL(sessionContext));
		VERIFY_IS_TRUE(NT_SUCCESS(sessionContext->Status()));

		sessionContext->Initialize();

		HRESULT registerResult = RegisterListener(smContext,sessionContext,TRUE);
		VERIFY_IS_TRUE(SUCCEEDED(registerResult));

		HRESULT createResult = CreateSession(smContext,sessionContext);
		VERIFY_IS_TRUE(SUCCEEDED(createResult));

		HRESULT openResult = OpenSession(sessionContext);
		VERIFY_IS_TRUE(SUCCEEDED(openResult));
		VERIFY_IS_TRUE(sessionContext->outboundSession != nullptr && sessionContext->inboundSession != nullptr);

		Common::StopwatchTime startTime = Common::Stopwatch::Now();
		standardMessage = CreateMessage();

		LONG halfCount1 = messageCount/2;
		LONG halfCount2 = messageCount-halfCount1;

		Common::Threadpool::Post([this,sessionContext](){this->ReceiveMessagesWait(sessionContext);});
		Common::Threadpool::Post([this,halfCount1,sessionContext](){this->SendMessages(halfCount1,sessionContext);});
		Common::Threadpool::Post([this,halfCount2,sessionContext](){this->SendMessages(halfCount2,sessionContext);});
		Common::Threadpool::Post([this,sessionContext](){this->ReceiveMessagesWait(sessionContext);});

		Sleep(100);

		HRESULT abortInboundResult = sessionContext->inboundSession->Abort();
		VERIFY_IS_TRUE(SUCCEEDED(abortInboundResult));

		bool sendCompleted = sessionContext->sendProtocolCompleted.WaitOne(TimeSpan::FromSeconds(50));
		VERIFY_IS_TRUE(sendCompleted);

		// we still try to close but this should fail -- inbound Abort causes outbound Abort 
		HRESULT closeResult = CloseSession(sessionContext);
		VERIFY_IS_TRUE(FAILED(closeResult));

		bool  inboundSessionCloseCompleted = sessionContext->inboundSessionClosed.WaitOne(TimeSpan::FromSeconds(50));
		VERIFY_IS_TRUE(inboundSessionCloseCompleted);

		bool receiveCompleted = sessionContext->receiveProtocolCompleted.WaitOne(TimeSpan::FromSeconds(50));
		VERIFY_IS_TRUE(receiveCompleted);

		Sleep(1000);

		std::cout << "AbortTest Completing, Verifying outcomes\n";

		std::cout << "numberOfMessages = " << sessionContext->numberOfMessages << "\n";
		std::cout << "numberOfReceivesInitiated = " << sessionContext->numberOfReceivesInitiated << "\n";
		std::cout << "numberOfMessagesReceived = " << sessionContext->numberOfMessagesReceived << "\n";
		std::cout << "numberOfCancelledReceives = " << sessionContext->numberOfCancelledReceives << "\n";
		std::cout << "numberOfSendsInitiated = " << sessionContext->numberOfSendsInitiated << "\n";
		std::cout << "numberOfSuccessfulSends = " << sessionContext->numberOfSuccessfulSends << "\n";
		std::cout << "numberOfCancelledSends = " << sessionContext->numberOfCancelledSends << "\n";
		std::cout << "\n";

		VERIFY_IS_TRUE(sessionContext->numberOfMessages>=sessionContext->numberOfMessagesReceived);
		VERIFY_IS_TRUE(sessionContext->numberOfMessages>=sessionContext->numberOfSendsInitiated);
		VERIFY_IS_TRUE(sessionContext->numberOfSuccessfulSends+sessionContext->numberOfCancelledSends==sessionContext->numberOfSendsInitiated);
		VERIFY_IS_TRUE(sessionContext->numberOfMessagesReceived+sessionContext->numberOfCancelledReceives==sessionContext->numberOfReceivesInitiated);

		Common::StopwatchTime finishTime = Common::Stopwatch::Now();
		int64 elapsedTime = (finishTime-startTime).TotalPositiveMilliseconds()/1000;
		UNREFERENCED_PARAMETER(elapsedTime);
    
		HRESULT unregisterResult = UnregisterListener(smContext);
		VERIFY_IS_TRUE(SUCCEEDED(unregisterResult));
	}

	void ReliableMessagingBasicTests::LoopbackSessionNormalAndAbort()
    {
        //ENTER;

		SessionManagerContextSPtr smContext = _new(RMSG_ALLOCATION_TAG, Common::GetSFDefaultPagedKAllocator())
												   SessionManagerContext();
						 						     			 
		VERIFY_IS_FALSE(KSHARED_IS_NULL(smContext));
		VERIFY_IS_TRUE(NT_SUCCESS(smContext->Status()));

		StartSessionManagers(smContext);

		RunSessionTest(2000,smContext);

		std::cout << "Ran test part 1 successfully\n";

		// old session closed, should be able to start a new session between the same session managers
		RunSessionTest(2500,smContext);

		std::cout << "Ran test part 2 successfully\n";

		RunInboundSessionAbortTest(2000,smContext);

		std::cout << "Ran session abort test successfully\n";

		Sleep(2000);

		//LEAVE;
    }	
}
