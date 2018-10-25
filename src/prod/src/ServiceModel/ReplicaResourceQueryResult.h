// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ReplicaResourceQueryResult
        : public ModelV2::ServiceReplicaDescription
    {
    public:
        ReplicaResourceQueryResult() = default;
        ReplicaResourceQueryResult(ReplicaResourceQueryResult const &) = default;
        ReplicaResourceQueryResult(ReplicaResourceQueryResult && other) = default;
        ReplicaResourceQueryResult(std::wstring const & replicaName)
            : ModelV2::ServiceReplicaDescription(replicaName) {}

        std::wstring CreateContinuationToken() const;
    };

    QUERY_JSON_LIST(ReplicaResourceList, ReplicaResourceQueryResult)
}
