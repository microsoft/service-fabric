// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"

namespace ReliableMessaging
{
	// Needed for unknown reasons to get K_FORCE_SHARED to compile properly below
	using ::_delete;


	// This class is a simple async open helper for ReliableSessionService 
	// It is really just a part of the implementation of the ReliableSessionService
	class AsyncOutboundOpenOperation : public SessionAsyncOperationContext
	{
		friend class ReliableSessionService;

		K_FORCE_SHARED(AsyncOutboundOpenOperation);

	public:

		static const ULONG LinkOffset;

		static Common::ErrorCode Create(
			__in KAllocator &allocator, 
			__out AsyncOutboundOpenOperationSPtr &result)
		{
			result = _new(RMSG_ALLOCATION_TAG, allocator)
													AsyncOutboundOpenOperation();
				 						     			 
			KTL_CREATE_CHECK_AND_RETURN(result,AsyncOperationCreateFailed,AsyncOutboundOpenOperation)
		}

		Common::ErrorCode StartOpenSession(    
			__in_opt KAsyncContextBase* const ParentAsyncContext,
			__in_opt KAsyncContextBase::CompletionCallback CallbackPtr
			)
		{
			return sessionContext_->StartOpenSessionOutbound(KSharedPtr<AsyncOutboundOpenOperation>(this),ParentAsyncContext,CallbackPtr);
		}

	private:
		
		void OnStart() override
		{
			sessionContext_->OnServiceOpenOutbound();
		}
	};

	// This class is a simple async close helper for ReliableSessionService 
	// It is really just a part of the implementation of the ReliableSessionService
	class AsyncOutboundCloseOperation : public SessionAsyncOperationContext
	{
		friend class ReliableSessionService;

		K_FORCE_SHARED(AsyncOutboundCloseOperation);

	public:

		static const ULONG LinkOffset;

		static Common::ErrorCode Create(
			__in KAllocator &allocator, 
			__out AsyncOutboundCloseOperationSPtr &result)
		{
			result = _new(RMSG_ALLOCATION_TAG, allocator)
													AsyncOutboundCloseOperation();
				 						     			 
			KTL_CREATE_CHECK_AND_RETURN(result,AsyncOperationCreateFailed,AsyncOutboundCloseOperation)
		}

		Common::ErrorCode StartCloseSession(    
			__in_opt KAsyncContextBase* const ParentAsyncContext,
			__in_opt KAsyncContextBase::CompletionCallback CallbackPtr
			)
		{
			return sessionContext_->StartCloseSessionOutbound(KSharedPtr<AsyncOutboundCloseOperation>(this),ParentAsyncContext,CallbackPtr);
		}

	private:
			
		void OnStart() override
		{}
	};
}
