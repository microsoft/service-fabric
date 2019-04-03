// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"

// Need to convert BOOLEAN to bool for Common asserts
#define KSHARED_IS_NULL(shared_ptr) ((shared_ptr==nullptr)==TRUE)

// standard destructor declared by K_FORCE_SHARED -- use macro when all members either struct value types or ref counted so nothing to do
#define K_FORCE_SHARED_NO_OP_DESTRUCTOR(class_name) \
	class_name::~class_name() {}

#define ERROR_RETURN_ACTIVITY_RELEASE(serviceContext,codeContext,errorCodeValue)				\
	{																							\
		Common::ErrorCode errorCode = errorCodeValue;											\
		RMTRACE->FailureCompletionFromInternalFunction(traceId_,#codeContext,errorCode.ErrorCodeValueToString()); \
		RMTRACE->ServiceActivityReleased(traceId_,#codeContext);											\
		BOOLEAN lastServiceActivityReleased = serviceContext->ReleaseServiceActivity();			\
		if (lastServiceActivityReleased)														\
			RMTRACE->LastServiceActivityReleased(#codeContext);									\
	}																							\
	return errorCodeValue;

// TODO:  Need to be more selective about use of LastServiceActivityReleased trace
#define ERROR_NO_RETURN_ACTIVITY_RELEASE(serviceContext,codeContext,errorCodeValue)				\
	{																							\
		Common::ErrorCode errorCode = errorCodeValue;											\
		RMTRACE->FailureCompletionFromInternalFunction(traceId_,#codeContext,errorCode.ErrorCodeValueToString()); \
		RMTRACE->ServiceActivityReleased(traceId_,#codeContext);											\
		BOOLEAN lastServiceActivityReleased = serviceContext->ReleaseServiceActivity();			\
		if (lastServiceActivityReleased)														\
			RMTRACE->LastServiceActivityReleased(#codeContext);									\
	}


#define NORMAL_ACTIVITY_RELEASE(serviceContext,codeContext)										\
	RMTRACE->ServiceActivityReleased(traceId_,#codeContext);												\
	{																							\
		BOOLEAN lastServiceActivityReleased = serviceContext->ReleaseServiceActivity();			\
		if (lastServiceActivityReleased)														\
			RMTRACE->LastServiceActivityReleased(#codeContext);									\
	}


#define ACQUIRE_ACTIVITY_VERIFY(codeContext)									\
	{																			\
		BOOLEAN acqireWorked = TryAcquireServiceActivity();						\
		if (!acqireWorked)														\
		{                                                                       \
			Common::ErrorCode errorCode = Common::ErrorCodeValue::ObjectClosed;	\
			RMTRACE->FailureCompletionFromInternalFunction(traceId_,#codeContext,errorCode.ErrorCodeValueToString()); \
			return errorCode;													\
		}																		\
		else																	\
		{																		\
			RMTRACE->ServiceActivityAcquired(traceId_,#codeContext);						\
		}																		\
	}

#define PARAMETER_NULL_CHECK(api_name,parameter_name,comment)				\
	if (parameter_name==nullptr)											\
	{																		\
		RMTRACE->InvalidParameterForAPI(#api_name,#parameter_name,comment);	\
		return ComUtility::OnPublicApiReturn(E_INVALIDARG);					\
	}

// TODO: are we doing proper/sufficient cleanup on failed Create in general?
#define KTL_CREATE_CHECK_AND_RETURN(result,trace_name,object_type)						\
			if (KSHARED_IS_NULL(result))														\
			{																			\
				RMTRACE->trace_name(#object_type);										\
				return Common::ErrorCode::FromNtStatus(STATUS_INSUFFICIENT_RESOURCES);	\
			}																			\
			else if (!NT_SUCCESS(result->Status()))										\
			{																			\
				RMTRACE->trace_name(#object_type);										\
				return Common::ErrorCode::FromNtStatus(result->Status());				\
			}																			\
			else																		\
			{																			\
				return Common::ErrorCode::FromNtStatus(STATUS_SUCCESS);					\
			}



#define API_PARTITION_PARAMETER_CHECK(api_name,serviceInstanceParameter,partitionKeyTypeParameter,partitionKeyParameter)								\
	PARAMETER_NULL_CHECK(api_name,serviceInstanceParameter,"must be non-null")									\
																												\
	if (partitionKeyTypeParameter != FABRIC_PARTITION_KEY_TYPE_NONE &&											\
		partitionKeyTypeParameter != FABRIC_PARTITION_KEY_TYPE_INT64 &&											\
		partitionKeyTypeParameter != FABRIC_PARTITION_KEY_TYPE_STRING)											\
	{																											\
		RMTRACE->InvalidParameterForAPI(#api_name,"partitionKeyType","invalid code");							\
		return ComUtility::OnPublicApiReturn(E_INVALIDARG);														\
	}																											\
																												\
	if (partitionKeyTypeParameter == FABRIC_PARTITION_KEY_TYPE_INT64 ||											\
		partitionKeyTypeParameter == FABRIC_PARTITION_KEY_TYPE_STRING)											\
	{																											\
		PARAMETER_NULL_CHECK(#api_name,partitionKeyParameter,"must be non-null for non-singleton partition");	\
	}
