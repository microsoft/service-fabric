// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
	namespace ResourceManager
	{
		using QueryMetadataFunction =
			std::function<Common::ErrorCode(
				__in std::wstring const & resourceName,
				__in Common::ActivityId const & activityId,
				__out ResourceMetadata & metadata)>;

		class Context
		{
		public:
			Context(
				Common::ComponentRoot const & componentRoot,
				Store::PartitionedReplicaId const & partitionedReplicaId,
				Store::IReplicatedStore * replicatedStore,
				std::wstring const & resourceTypeName,
				std::wstring const & traceSubComponent,
				QueryMetadataFunction const & queryResourceMetadataFunction,
				Common::TimeSpan const & maxRetryDelay);

			// MaxRetryDelay Property
		public:
			__declspec(property(get = get_MaxRetryDelay)) Common::TimeSpan const & MaxRetryDelay;
			Common::TimeSpan const & get_MaxRetryDelay() const { return maxRetryDelay_; }
		private:
			Common::TimeSpan const maxRetryDelay_;

			// PartitionedReplicaId Property
		public:
			__declspec(property(get = get_PartitionedReplicaId)) Store::PartitionedReplicaId const & PartitionedReplicaId;
			Store::PartitionedReplicaId const & get_PartitionedReplicaId() const { return partitionedReplicaId_; }
		private:
			Store::PartitionedReplicaId const partitionedReplicaId_;

			// ResourceManager Property
		public:
			__declspec(property(get = get_ResourceManager)) ResourceManagerCore & ResourceManager;
			ResourceManagerCore & get_ResourceManager() { return resourceManager_; }
		private:
			ResourceManagerCore resourceManager_;

			// QueryResourceMetadataFunction Property 
		public:
			__declspec(property(get = get_QueryResourceMetadataFunction)) QueryMetadataFunction const & QueryResourceMetadataFunction;
			QueryMetadataFunction const & get_QueryResourceMetadataFunction() const { return queryResourceMetadataFunction_; }
		private:
			QueryMetadataFunction const queryResourceMetadataFunction_;

			// RequestTracker Property
		public:
			__declspec(property(get = get_RequestTracker)) Naming::StringRequestInstanceTracker & RequestTracker;
			Naming::StringRequestInstanceTracker & get_RequestTracker() { return requestTracker_; }
		private:
			Naming::StringRequestInstanceTracker requestTracker_;
		};
	}
}
