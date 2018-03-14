// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class NotificationClientSynchronizationReplyBody : public ServiceModel::ClientServerMessageBody
    {
    public:
        NotificationClientSynchronizationReplyBody() 
            : generation_()
            , deletedVersions_()
        { 
        }

        NotificationClientSynchronizationReplyBody(
            Reliability::GenerationNumber const & generation,
            std::vector<int64> && versions)
            : generation_(generation)
            , deletedVersions_(std::move(versions))
        {
        }

        Reliability::GenerationNumber const & GetGeneration() { return generation_; }
        std::vector<int64> && TakeVersions() { return std::move(deletedVersions_); }

        FABRIC_FIELDS_02(generation_, deletedVersions_);
        
    private:
        Reliability::GenerationNumber generation_;
        std::vector<int64> deletedVersions_;
    };
}

