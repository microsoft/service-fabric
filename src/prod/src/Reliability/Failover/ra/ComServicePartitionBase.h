// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
		//TODO move other common code from ComStatelessServicePartition and ComStatefulServicePartition into this base class
        class ComServicePartitionBase
        {
			DENY_COPY(ComServicePartitionBase);
         
        public:
			ComServicePartitionBase(std::weak_ptr<FailoverUnitProxy> && failoverUnitProxyWPtr);
			HRESULT __stdcall ReportLoad(ULONG metricCount, FABRIC_LOAD_METRIC const *metrics);

        protected:
			Common::ErrorCode CheckIfOpen(Common::AcquireReadLock & lock);
			Common::ErrorCode LockFailoverUnitProxy(FailoverUnitProxySPtr & failoverUnitProxy);
			Common::ErrorCode CheckIfOpen();
			Common::ErrorCode CheckIfOpenAndLockFailoverUnitProxy(FailoverUnitProxySPtr & failoverUnitProxy);

			mutable Common::RwLock lock_;
			bool isValidPartition_;
			FailoverUnitProxyWPtr failoverUnitProxyWPtr_;
        };
    }
}
