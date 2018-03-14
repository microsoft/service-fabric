// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Store
{
    class KeyValueStoreQueryResultWrapper 
        : public Api::IStatefulServiceReplicaStatusResult
        , public Common::ComponentRoot
    {
    public:
        KeyValueStoreQueryResultWrapper(std::shared_ptr<ServiceModel::KeyValueStoreQueryResult> &&);

        virtual void GetResult(
            __inout Common::ScopedHeap &, 
            __out FABRIC_REPLICA_STATUS_QUERY_RESULT &);

    private:
        std::shared_ptr<ServiceModel::KeyValueStoreQueryResult> queryResult_;
    };
}
