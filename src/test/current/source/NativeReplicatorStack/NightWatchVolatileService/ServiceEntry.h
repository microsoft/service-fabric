// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace V1ReplPerf
{
    class ServiceEntry
    {
    public:
        ServiceEntry(__int64 key);

        __declspec(property(get = get_Key)) __int64 Key;
        __int64 get_Key() const
        {
            return key_;
        }
        
        HRESULT BeginReplicate(
            Common::ComPointer<IFabricStateReplicator> & replicator,
            Common::ComPointer<ComAsyncOperationCallbackHelper> & callback,
            Common::ComPointer<IFabricAsyncOperationContext> & context,
            FABRIC_SEQUENCE_NUMBER & newVersion,
            int newValueSize);
    
    private:
        __int64 key_;

        static FABRIC_SEQUENCE_NUMBER const InvalidVersion = -1;
    };
}
