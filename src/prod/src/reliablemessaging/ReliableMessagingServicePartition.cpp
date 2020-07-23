// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"

namespace ReliableMessaging
{
	K_FORCE_SHARED_NO_OP_DESTRUCTOR(ReliableMessagingServicePartition)

	ReliableMessagingServicePartition::ReliableMessagingServicePartition(
												Common::Uri const &serviceInstanceName,
												ReliableMessaging::PartitionKey const &partitionId) :
																				serviceInstanceName_(serviceInstanceName),
																				partitionId_(partitionId)
	{
		try
		{
			sourceHeader_ = std::make_shared<ReliableMessagingSourceHeader>(serviceInstanceName,partitionId);
			targetHeader_ = std::make_shared<ReliableMessagingTargetHeader>(serviceInstanceName,partitionId);
		}
		catch(std::exception const&)
		{
			SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
		}
	}

	LONG ReliableMessagingServicePartition::compare(ReliableMessagingServicePartitionSPtr const & first, ReliableMessagingServicePartitionSPtr const & second)
	{
		int svcNameComparison = first->ServiceInstanceName.Compare(second->ServiceInstanceName);
		if (svcNameComparison != 0) return svcNameComparison;
		else return PartitionKey::compare(first->PartitionId,second->PartitionId);
	}

	// This method is used to construct the sessionManagerInboundSessionRequestFinderIndex_ which is used to match a requested Int64
	// partition number with the Int64 range owned by a session manager (and its owner service) processing an incoming OpenSession request
	// In Find the firstKey is the search key (single Int64 used in CreateOutboundSession) and the second key is the target key (partition range for session manager)
	// This compare function can only be used like a secondary index where the standard one above is used first for insert/delete in a primary index
	LONG ReliableMessagingServicePartition::compareForInt64Find(ReliableMessagingServicePartitionSPtr const & first, ReliableMessagingServicePartitionSPtr const & second)
	{
		int svcNameComparison = first->ServiceInstanceName.Compare(second->ServiceInstanceName);
		if (svcNameComparison != 0) return svcNameComparison;
		else return PartitionKey::compareForInt64Find(first->PartitionId,second->PartitionId);
	}

	/* 
		{
		LONG servicePartitionComparison = compare(*this,other);
		return (servicePartitionComparison < 0);
	}

	bool ReliableMessagingServicePartition::lessThan(ReliableMessagingServicePartition const & other) const
	{
		int svcNameComparison = ServiceInstanceName.Compare(other.ServiceInstanceName);
		if (svcNameComparison < 0) return TRUE;
		else if (svcNameComparison > 0) return FALSE;
		else 
		{
			PartitionKey otherPartitionId = other.PartitionId;

			FABRIC_PARTITION_KEY_TYPE selfKeyType = PartitionId.KeyType;
			FABRIC_PARTITION_KEY_TYPE otherKeyType = otherPartitionId.KeyType;

            ASSERT_IF(selfKeyType == FABRIC_PARTITION_KEY_TYPE_INVALID, "invalid key type for ReliableMessagingServicePartition");
            ASSERT_IF(otherKeyType == FABRIC_PARTITION_KEY_TYPE_INVALID, "invalid key type for ReliableMessagingServicePartition");

			switch (otherKeyType)
			{
			case FABRIC_PARTITION_KEY_TYPE_NONE: return FALSE;
			case FABRIC_PARTITION_KEY_TYPE_INT64: 
				{
					switch (selfKeyType)
					{
					case FABRIC_PARTITION_KEY_TYPE_NONE: return TRUE;
					case FABRIC_PARTITION_KEY_TYPE_INT64: return PartitionId.Int64Key < otherPartitionId.Int64Key;
					case FABRIC_PARTITION_KEY_TYPE_STRING: return TRUE;
					}			
				}
			case FABRIC_PARTITION_KEY_TYPE_STRING:
				{
					switch (selfKeyType)
					{
					case FABRIC_PARTITION_KEY_TYPE_NONE: return TRUE;
					case FABRIC_PARTITION_KEY_TYPE_INT64: return FALSE;
					case FABRIC_PARTITION_KEY_TYPE_STRING: return PartitionId.StringKey < otherPartitionId.StringKey;
					}						
				}
			}

            Common::Assert::CodingError("Unexpected outcome for ReliableMessagingServicePartition comparison");
		}
	}

	*/

	// This signature is meant to match the reliable session IDL signatures
	Common::ErrorCode ReliableMessagingServicePartition::Create(
				__in FABRIC_URI serviceInstanceName,
				__in FABRIC_PARTITION_KEY_TYPE partitionKeyType,
				__in const void *partitionKey,
				__out ReliableMessagingServicePartitionSPtr &servicePartition)
	{
		Common::Uri parsedServiceInstance;

		bool success = Common::Uri::TryParse(serviceInstanceName,parsedServiceInstance);
		if (!success) return Common::ErrorCodeValue::InvalidNameUri;

		PartitionKey validatedPartitionId;

		Common::ErrorCode partitionConversionResult = validatedPartitionId.FromPublicApi(partitionKeyType, partitionKey);

		if (partitionConversionResult.IsSuccess())
		{
			return Create(parsedServiceInstance,validatedPartitionId,servicePartition);
		}
		else
		{
			servicePartition = nullptr;
			return partitionConversionResult;
		}
	}


	Common::ErrorCode ReliableMessagingServicePartition::Create(
				__in Common::Uri const &parsedServiceInstance,
				__in PartitionKey const &validatedPartitionId,
				__out ReliableMessagingServicePartitionSPtr &servicePartition)
	{
		servicePartition = _new (RMSG_ALLOCATION_TAG, Common::GetSFDefaultPagedKAllocator())
			ReliableMessagingServicePartition(parsedServiceInstance, validatedPartitionId);

		KTL_CREATE_CHECK_AND_RETURN(servicePartition,KtlObjectCreationFailed,ReliableMessagingServicePartition)
	}
}
