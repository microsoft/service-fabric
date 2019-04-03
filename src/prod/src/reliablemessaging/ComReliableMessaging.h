// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#pragma once

namespace ReliableMessaging
{
	// COM wrapper for ReliableMessagingServicePartition Descriptor
	class ComFabricReliableSessionPartitionIdentity :
        public IFabricReliableSessionPartitionIdentity,
        private Common::ComUnknownBase 
    {
        BEGIN_COM_INTERFACE_LIST(ComFabricReliableSessionPartitionIdentity)
        COM_INTERFACE_ITEM(IID_IUnknown, ComFabricReliableSessionPartitionIdentity)
        COM_INTERFACE_ITEM(IID_IFabricReliableSessionPartitionIdentity, IFabricReliableSessionPartitionIdentity)
        END_COM_INTERFACE_LIST()

		ComFabricReliableSessionPartitionIdentity(ReliableMessagingServicePartitionSPtr const &partitionInfo) : partitionInfo_(partitionInfo)
		{
			serviceInstanceName_ = partitionInfo->ServiceInstanceName.ToString();
		}

		ReliableMessagingServicePartitionSPtr partitionInfo_;
		std::wstring serviceInstanceName_;
		INTEGER_PARTITION_KEY_RANGE integerRangeKey_;
		std::wstring  stringKey_;
		LPCWSTR stringKeyPtr_;

	public:

        FABRIC_URI STDMETHODCALLTYPE getServiceName()
		{
			FABRIC_URI result = serviceInstanceName_.c_str();
			return result;
		}

        FABRIC_PARTITION_KEY_TYPE STDMETHODCALLTYPE getPartitionKeyType()
		{
			return partitionInfo_->PartitionId.KeyType;
		}

        void *STDMETHODCALLTYPE getPartitionKey()
		{
			FABRIC_PARTITION_KEY_TYPE keyType = partitionInfo_->PartitionId.KeyType;
			void * partitionKey = nullptr;

			switch (keyType)
			{
			case FABRIC_PARTITION_KEY_TYPE_NONE:
				partitionKey = nullptr;
				break;
			case FABRIC_PARTITION_KEY_TYPE_INT64:
				integerRangeKey_ = partitionInfo_->PartitionId.Int64RangeKey;
				partitionKey = (void*)(&integerRangeKey_);
				break;
			case FABRIC_PARTITION_KEY_TYPE_STRING:
				stringKey_ = partitionInfo_->PartitionId.StringKey;
				partitionKey = (void*)stringKey_.c_str();
				break;
			}

			return partitionKey;
		}
	};

	// COM wrapper for the session manager service
    class ComFabricReliableSessionManager :
        public IFabricReliableSessionManager,
        private Common::ComUnknownBase 
    {
        BEGIN_COM_INTERFACE_LIST(ComFabricReliableSessionManager)
        COM_INTERFACE_ITEM(IID_IUnknown, ComFabricReliableSessionManager)
        COM_INTERFACE_ITEM(IID_IFabricReliableSessionManager, IFabricReliableSessionManager)
        END_COM_INTERFACE_LIST()

        ComFabricReliableSessionManager();
        ~ComFabricReliableSessionManager();

    public:

		static HRESULT Create( 
			__in HOST_SERVICE_PARTITION_INFO *servicePartitionInfo,
			__in FABRIC_URI callingServiceInstanceName,
			__in FABRIC_PARTITION_KEY_TYPE partitionKeyType,
			__in const void *partitionKey,
			__out IFabricReliableSessionManager **sessionManager);

        // ************
        // IFabricReliableSessionManager
        // ************
        
        HRESULT STDMETHODCALLTYPE BeginOpen( 
            __in IFabricReliableSessionAbortedCallback *sessionCallback,
            __in IFabricAsyncOperationCallback *operationCallback,
            __out IFabricAsyncOperationContext **context);
        
        HRESULT STDMETHODCALLTYPE EndOpen( 
            __in IFabricAsyncOperationContext *context);
		
        HRESULT STDMETHODCALLTYPE BeginClose( 
            __in IFabricAsyncOperationCallback *callback,
            __out IFabricAsyncOperationContext **context);
        
        HRESULT STDMETHODCALLTYPE EndClose( 
            __in IFabricAsyncOperationContext *context);

		HRESULT STDMETHODCALLTYPE BeginCreateOutboundSession( 
            __in FABRIC_URI targetServiceInstanceName,
            __in FABRIC_PARTITION_KEY_TYPE partitionKeyType,
            __in const void *partitionKey,
			__in LPCWSTR targetCommunicationEndpoint,
            __in IFabricAsyncOperationCallback *callback,
            __out IFabricAsyncOperationContext **context);
        
        HRESULT STDMETHODCALLTYPE EndCreateOutboundSession( 
            __in IFabricAsyncOperationContext *context,
            __out IFabricReliableSession **session);
        
        HRESULT STDMETHODCALLTYPE BeginRegisterInboundSessionCallback( 
            __in IFabricReliableInboundSessionCallback *listenerCallback,
            __in IFabricAsyncOperationCallback *callback,
            __out IFabricAsyncOperationContext **context);
        
        HRESULT STDMETHODCALLTYPE EndRegisterInboundSessionCallback( 
            __in IFabricAsyncOperationContext *context,
			__out IFabricStringResult **sessionListenerEndpoint);
        
        HRESULT STDMETHODCALLTYPE BeginUnregisterInboundSessionCallback( 
            __in IFabricAsyncOperationCallback *callback,
            __out IFabricAsyncOperationContext **context);
        
        HRESULT STDMETHODCALLTYPE EndUnregisterInboundSessionCallback( 
            __in IFabricAsyncOperationContext *context);
        

	private:

		SessionManagerServiceSPtr sessionManager_;
    };


	// COM wrapper for an inbound message
    class ComFabricReliableSessionMessage : public IFabricOperationData, Common::ComUnknownBase
    {
        BEGIN_COM_INTERFACE_LIST(ComFabricReliableSessionMessage)
            COM_INTERFACE_ITEM(IID_IUnknown, ComFabricReliableSessionMessage)
            COM_INTERFACE_ITEM(IID_IFabricOperationData, IFabricOperationData)
        END_COM_INTERFACE_LIST()

    public:
        // Create an IFabricOperationData COM object from a Transport::MessageUPtr instance
		// Used for receiving messages from transport layer
		//
		// Every ComFabricReliableSessionMessage instance holds transport buffers during its lifectime 
		// by holding the Transport::MessageUPtr as a member until it is destroyed when released by the client
        //
        //  Parameters:
        //      inMessage - the Transport::MessageUPtr instance that is to be aliased 
        //      Result - Resulting interface pointer to the created ComFabricReliableSessionMessage instance
        //      
        HRESULT static
        CreateFromTransportMessage(__in Transport::MessageUPtr &&inMessage, __out IFabricOperationData** Result);

    private:
        HRESULT STDMETHODCALLTYPE
        GetData(__out ULONG* Count, __out const FABRIC_OPERATION_DATA_BUFFER** Buffer) override;

		ComFabricReliableSessionMessage(__in Transport::MessageUPtr &&inMessage) 
			: inboundMessage_(std::move(inMessage)) {}

		~ComFabricReliableSessionMessage()
		{
			if (dataBuffers_)
			{
				delete dataBuffers_;
			}
		}

		Transport::MessageUPtr inboundMessage_;

		ULONG bufferCount_;
        FABRIC_OPERATION_DATA_BUFFER* dataBuffers_;       
    };

	// COM wrapper for reliable session service
    class ComFabricReliableSession :
        public IFabricReliableSession,
		public IFabricMessageDataFactory,
        private Common::ComUnknownBase 
    {
        DENY_COPY(ComFabricReliableSession);

        BEGIN_COM_INTERFACE_LIST(ComFabricReliableSession)
        COM_INTERFACE_ITEM(IID_IUnknown, IFabricReliableSession)
        COM_INTERFACE_ITEM(IID_IFabricReliableSession, IFabricReliableSession)
        COM_INTERFACE_ITEM(IID_IFabricMessageDataFactory, IFabricMessageDataFactory)
        END_COM_INTERFACE_LIST()

    public:

		ComFabricReliableSession(ReliableSessionServiceSPtr session) : session_(session)
		{}

        // ************
        // IFabricReliableSession
        // ************
        
        HRESULT STDMETHODCALLTYPE BeginOpen( 
            __in IFabricAsyncOperationCallback *operationCallback,
            __out IFabricAsyncOperationContext **context);
        
        HRESULT STDMETHODCALLTYPE EndOpen( 
            __in IFabricAsyncOperationContext *context);
              
        HRESULT STDMETHODCALLTYPE BeginClose( 
            __in IFabricAsyncOperationCallback *callback,
            __out IFabricAsyncOperationContext **context);
        
        HRESULT STDMETHODCALLTYPE EndClose( 
            __in IFabricAsyncOperationContext *context);
        
        HRESULT STDMETHODCALLTYPE Abort( void);
        
        HRESULT STDMETHODCALLTYPE BeginReceive( 
			__in BOOLEAN waitForMessages, 
            __in IFabricAsyncOperationCallback *callback,
            __out IFabricAsyncOperationContext **context);
        
        HRESULT STDMETHODCALLTYPE EndReceive( 
            __in IFabricAsyncOperationContext *context,
            __out IFabricOperationData **message);
        
        HRESULT STDMETHODCALLTYPE BeginSend( 
            __in IFabricOperationData *message,
            __in IFabricAsyncOperationCallback *callback,
            __out IFabricAsyncOperationContext **context);
        
        HRESULT STDMETHODCALLTYPE EndSend( 
            __in IFabricAsyncOperationContext *context);

        HRESULT STDMETHODCALLTYPE GetSessionDataSnapshot( 
			__out const SESSION_DATA_SNAPSHOT **sessionDataSnapshot);


        // ************
        // IFabricMessageDataFactory
        // ************
        
		// Used by managed code to create the unmanaged buffer array in which pinned managed buffer pointers will be placed
        HRESULT STDMETHODCALLTYPE CreateMessageData( 
            __in ULONG buffercount,
            __in ULONG *bufferSizes,
            __in const void *bufferPointers,
            __out IFabricOperationData **messageData);

    private:
		ReliableSessionServiceSPtr session_;
		SESSION_DATA_SNAPSHOT lastSnapshot_;
    };
}
