// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"

namespace ReliableMessaging
{

	class InboundSessionRequestedCallback: public KObject<InboundSessionRequestedCallback>, public KShared<InboundSessionRequestedCallback>
	{
		K_FORCE_SHARED(InboundSessionRequestedCallback);

	public:

		Common::ErrorCode SessionRequested(
			__in ReliableMessagingServicePartitionSPtr const &source,	// source partition
			__in ReliableSessionServiceSPtr const &newSession,			// session offered
			__out LONG &response
			);
			
		InboundSessionRequestedCallback(IFabricReliableInboundSessionCallback *callback)
			 : realCallback_(callback)
		{
			ASSERT_IF(callback==nullptr, "NULL callback registration in internal code");
			realCallback_->AddRef();
		}

	private:

		IFabricReliableInboundSessionCallback *realCallback_;
	};

	class ReliableSessionAbortedCallback: public KObject<ReliableSessionAbortedCallback>, public KShared<ReliableSessionAbortedCallback>
	{
		K_FORCE_SHARED(ReliableSessionAbortedCallback);

	public:
      
        Common::ErrorCode SessionAbortedByPartner( 
			__in SessionKind kind,
            __in ReliableMessagingServicePartitionSPtr const &partnerServicePartition,
			__in ReliableSessionServiceSPtr const &sessionToClose
			);

		ReliableSessionAbortedCallback(IFabricReliableSessionAbortedCallback *callback)
			 : realCallback_(callback)
		{
			ASSERT_IF(callback==nullptr, "NULL callback registration in internal code");
			realCallback_->AddRef();
		}

	private:

		IFabricReliableSessionAbortedCallback *realCallback_;	
	};
}
