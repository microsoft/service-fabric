// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class NotificationClientSynchronizationRequestBody : public ServiceModel::ClientServerMessageBody
    {
        DECLARE_SERIALIZATION_OVERHEAD_ESTIMATE()

    public:
        NotificationClientSynchronizationRequestBody() 
            : clientId_()
            , generation_()
            , undeletedPartitionsToCheck_()
        { 
        }

        NotificationClientSynchronizationRequestBody(
            std::wstring const & clientId,
            Reliability::GenerationNumber const & generation,
            std::vector<Reliability::VersionedConsistencyUnitId> && partitions)
            : clientId_(clientId)
            , generation_(generation)
            , undeletedPartitionsToCheck_(std::move(partitions))
        {
        }

        NotificationClientSynchronizationRequestBody(
            NotificationClientSynchronizationRequestBody const & other)
            : clientId_(other.clientId_)
            , generation_(other.generation_)
            , undeletedPartitionsToCheck_(other.undeletedPartitionsToCheck_)
        {
        }

        std::wstring const & GetClientId() { return clientId_; }
        Reliability::GenerationNumber const & GetGeneration() { return generation_; }
        std::vector<Reliability::VersionedConsistencyUnitId> && TakePartitions() { return std::move(undeletedPartitionsToCheck_); }

        FABRIC_FIELDS_03(clientId_, generation_, undeletedPartitionsToCheck_);
        
    private:
        std::wstring clientId_;
        Reliability::GenerationNumber generation_;
        std::vector<Reliability::VersionedConsistencyUnitId> undeletedPartitionsToCheck_;
    };
}
