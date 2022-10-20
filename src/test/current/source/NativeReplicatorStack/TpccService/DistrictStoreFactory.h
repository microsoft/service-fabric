// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
	class DistrictStoreFactory
	{
	public:
		static void Create(
			__in const Data::StateManager::FactoryArguments& factoryArg,
			__in KAllocator& allocator,
			__out TxnReplicator::IStateProvider2::SPtr& stateProvider);
	};
}
