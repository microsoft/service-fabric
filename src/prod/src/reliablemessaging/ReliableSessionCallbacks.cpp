// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"

namespace ReliableMessaging
{
	InboundSessionRequestedCallback::~InboundSessionRequestedCallback()
	{
		realCallback_->Release();
	}

	Common::ErrorCode InboundSessionRequestedCallback::SessionRequested(
		__in ReliableMessagingServicePartitionSPtr const &source,	// source partition
		__in ReliableSessionServiceSPtr const &newSession,			// session offered
		__out LONG &response
		)
	{
		ComFabricReliableSession *newFabricSession 
				= new (std::nothrow) ComFabricReliableSession(newSession);

		if (newFabricSession == nullptr) 
		{
			RMTRACE->ComObjectCreationFailed("ComFabricReliableSession");
			return Common::ErrorCodeValue::OutOfMemory;
		}

		ComFabricReliableSessionPartitionIdentity *newFabricPartitionId 
				= new (std::nothrow) ComFabricReliableSessionPartitionIdentity(source);

		if (newFabricPartitionId == nullptr) 
		{
			RMTRACE->ComObjectCreationFailed("ComFabricReliableSessionPartitionIdentity");
			return Common::ErrorCodeValue::OutOfMemory;
		}

		HRESULT hr = S_OK;
		try 
		{
			hr = realCallback_->AcceptInboundSession(newFabricPartitionId,newFabricSession,&response);
		}
		catch(...)
		{
			hr = FABRIC_E_RELIABLE_SESSION_SERVICE_FAULTED;
		}

		if (FAILED(hr))
		{
			// there was a fault that either we caught or the managed interop layer did, translates to failure to accept the session
			response = FALSE;
		}

		// TODO: who is responsible for keeping this alive?  Always release?
		if (response == FALSE) 
		{
			newFabricSession->Release();
		}

		newFabricPartitionId->Release();

		return Common::ErrorCode::FromHResult(hr);
	}


	ReliableSessionAbortedCallback::~ReliableSessionAbortedCallback()
	{
		realCallback_->Release();
	}

    Common::ErrorCode ReliableSessionAbortedCallback::SessionAbortedByPartner( 
		__in SessionKind kind,
        __in ReliableMessagingServicePartitionSPtr const &partnerServicePartition,
		__in ReliableSessionServiceSPtr const &sessionToClose
		)
	{
		ComFabricReliableSession *closingSession 
				= new (std::nothrow) ComFabricReliableSession(sessionToClose);

		if (closingSession == nullptr) 
		{
			RMTRACE->ComObjectCreationFailed("ComFabricReliableSession");
			return Common::ErrorCodeValue::OutOfMemory;
		}

		ComFabricReliableSessionPartitionIdentity *newFabricPartitionId 
				= new (std::nothrow) ComFabricReliableSessionPartitionIdentity(partnerServicePartition);

		if (newFabricPartitionId == nullptr) 
		{
			RMTRACE->ComObjectCreationFailed("ComFabricReliableSessionPartitionIdentity");
			return Common::ErrorCodeValue::OutOfMemory;
		}

		BOOLEAN isInbound = kind==SessionKind::Inbound ? TRUE : FALSE;

		HRESULT hr = S_OK;
		try 
		{
			hr = realCallback_->SessionAbortedByPartner(isInbound,newFabricPartitionId,closingSession);
		}
		catch(...)
		{
			hr = FABRIC_E_RELIABLE_SESSION_SERVICE_FAULTED;
		}

		closingSession->Release();
		newFabricPartitionId->Release();

		return Common::ErrorCode::FromHResult(hr);
	}
}
