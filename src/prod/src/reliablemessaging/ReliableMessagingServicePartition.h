// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"

using namespace Transport;

namespace ReliableMessaging
{
	class ReliableMessagingServicePartition : public KObject<ReliableMessagingServicePartition>, public KShared<ReliableMessagingServicePartition>
	{
		K_FORCE_SHARED(ReliableMessagingServicePartition);

	public:

        __declspec (property(get=get_PartitionKey)) ReliableMessaging::PartitionKey const &PartitionId;
        inline ReliableMessaging::PartitionKey const & get_PartitionKey() const { return partitionId_; }
       
        __declspec(property(get=get_ServiceInstanceName)) Common::Uri const &ServiceInstanceName;
        inline Common::Uri const & get_ServiceInstanceName() const { return serviceInstanceName_; }

        __declspec(property(get=get_SourceHeader)) ReliableMessagingSourceHeaderSPtr const &SourceHeader;
        inline ReliableMessagingSourceHeaderSPtr const & get_SourceHeader() const { return sourceHeader_; }

        __declspec(property(get=get_TargetHeader)) ReliableMessagingTargetHeaderSPtr const &TargetHeader;
        inline ReliableMessagingTargetHeaderSPtr const & get_TargetHeader() const { return targetHeader_; }

		static Common::ErrorCode Create(
						__in FABRIC_URI serviceInstanceName,
						__in FABRIC_PARTITION_KEY_TYPE partitionKeyType,
						__in const void *partitionKey,
						__out ReliableMessagingServicePartitionSPtr &servicePartition);

		static Common::ErrorCode Create(
					__in Common::Uri const &parsedServiceInstance,
					__in PartitionKey const &validatedPartitionId,
					__out ReliableMessagingServicePartitionSPtr &servicePartition);

		bool operator ==(ReliableMessagingServicePartition const & other) const
		{
			return (serviceInstanceName_ == other.ServiceInstanceName) && (partitionId_ == other.PartitionId);
		}

		bool IsValidTargetKey()
		{
			return partitionId_.IsValidTargetKey();
		}

		// Needed to use as key in a std::map structure
		// bool lessThan(ReliableMessagingServicePartition const & other) const; 
		static LONG compare(ReliableMessagingServicePartitionSPtr const & first, ReliableMessagingServicePartitionSPtr const & second);
		static LONG compareForInt64Find(ReliableMessagingServicePartitionSPtr const & first, ReliableMessagingServicePartitionSPtr const & second);

		ReliableMessagingServicePartition(
			Common::Uri const &serviceInstanceName,
			ReliableMessaging::PartitionKey const &partitionId);

		Common::Uri serviceInstanceName_;
		ReliableMessaging::PartitionKey partitionId_;

		// mutable qualifier enables lazy initialization in the get methods
		mutable ReliableMessagingSourceHeaderSPtr sourceHeader_;
		mutable ReliableMessagingTargetHeaderSPtr targetHeader_;
	};
}
