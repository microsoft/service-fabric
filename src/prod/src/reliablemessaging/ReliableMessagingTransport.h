// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"

using namespace Transport;

namespace ReliableMessaging
{
	enum ResourceState
	{
		NotInitialized,
		InitializedNotStarted,
		Started,
		Stopped
	} ;



	class ReliableMessagingTransport : public KObject<ReliableMessagingTransport>, public KShared<ReliableMessagingTransport>
	{
		friend class ReliableMessagingEnvironment;

		K_FORCE_SHARED(ReliableMessagingTransport);

	public:

        __declspec (property(get=get_Hostname)) std::wstring const &Hostname;
        inline std::wstring const & get_Hostname() const { return hostname_; }

        __declspec (property(get=get_ListenAddress)) std::wstring const &ListenAddress;
        inline std::wstring const & get_ListenAddress() const { return listeningAt_; }

        __declspec (property(get=get_SendTargetCount)) size_t const SendTargetCount;
        inline size_t const get_SendTargetCount() const { return transport_->SendTargetCount(); }

		ReliableMessagingTransport(ReliableMessagingEnvironmentSPtr myEnvironment) : state_(NotInitialized), transport_(nullptr), hostname_(L""), listeningAt_(L""),
			inboundDequeuecallback_(KAsyncContextBase::CompletionCallback(this, &ReliableMessagingTransport::AsyncDequeueCallback)), myEnvironment_(myEnvironment)
		{}

		// Start and Stop are idempotent operations
		Common::ErrorCode Start();
		void Stop();

		/*** Begin Methods used to access the singletonTransport_ used for most purposes ***/

		/*

		static void StopAndDeleteSingletonTransport()
		{
			if (singletonTransport_ != nullptr)
			{
				singletonTransport_->Stop();
				delete singletonTransport_;
				singletonTransport_ = nullptr;
			}
		}
		*/

		static Common::ErrorCode GetSingletonListenAddress(std::wstring & address);

		Common::ErrorCode ResolveTarget(
			__in std::wstring const & address,
			__out ISendTarget::SPtr &target);

        Common::ErrorCode SendOneWay(ISendTarget::SPtr const & target, MessageUPtr && message);

		/*** End Methods used to access the singletonTransport_ used for most purposes ***/


	private:	

		void CommonMessageHandler(MessageUPtr &message, ISendTarget::SPtr const & responseTarget);

		void AsyncDequeueCallback(
			        KAsyncContextBase* const,
					KAsyncContextBase& CompletingSubOp      // CompletingSubOp                                                           
					);                                   
		                                   

		// Common queue for holding and dispatching inbound messages across all services
		InboundMessageQueueSPtr inboundAsyncTransportQueue_;

		// Only three states are currently supported for this resource: NotInitialized, Started and Stopped
		ResourceState state_;
		IDatagramTransportSPtr transport_;
		std::wstring hostname_;
		std::wstring listeningAt_;
		KAsyncContextBase::CompletionCallback inboundDequeuecallback_;
		ReliableMessagingEnvironmentSPtr myEnvironment_;
	};
			
}

