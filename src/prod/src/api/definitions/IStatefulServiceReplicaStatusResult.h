// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IStatefulServiceReplicaStatusResult
    {
    public:
        virtual ~IStatefulServiceReplicaStatusResult() {};

        virtual void GetResult(
            __inout Common::ScopedHeap &, 
            __out FABRIC_REPLICA_STATUS_QUERY_RESULT &) = 0;
    };
}
