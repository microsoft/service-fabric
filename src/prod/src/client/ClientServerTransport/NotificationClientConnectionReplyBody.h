// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class NotificationClientConnectionReplyBody : public ServiceModel::ClientServerMessageBody
    {
    public:
        NotificationClientConnectionReplyBody() 
            : generation_()
            , lastDeletedEmptyPartitionVersion_(0)
            , gateway_()
        { 
        }

        NotificationClientConnectionReplyBody(
            Reliability::GenerationNumber const & generation,
            int64 lastDeletedEmptyPartitionVersion,
            GatewayDescription const & gateway)
            : generation_(generation)
            , lastDeletedEmptyPartitionVersion_(lastDeletedEmptyPartitionVersion)
            , gateway_(gateway)
        {
        }

        Reliability::GenerationNumber const & GetGeneration() { return generation_; }
        int64 GetLastDeletedEmptyPartitionVersion() { return lastDeletedEmptyPartitionVersion_; }
        GatewayDescription const & GetGatewayDescription() { return gateway_; }

        FABRIC_FIELDS_03(generation_, lastDeletedEmptyPartitionVersion_, gateway_);
        
    private:
        Reliability::GenerationNumber generation_;
        int64 lastDeletedEmptyPartitionVersion_;
        GatewayDescription gateway_;
    };
}
