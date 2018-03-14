// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace FederationTest
{
    using namespace Common;
    using namespace Federation;
    
    void RandomFederationTestConfig::Initialize()
    {
        VoteConfig::iterator it;
        VoteConfig votes = FederationConfig::GetConfig().Votes;
        nonSeedNodeVotes_ = 0;
        for ( it=votes.begin() ; it != votes.end(); it++ )
        {
            if (it->Type == Constants::SeedNodeVoteType)
            {
                seedNodes_.push_back(it->Id.IdValue);
            }
            else
            {
                nonSeedNodeVotes_++;
            }
        }
    }

    DEFINE_SINGLETON_COMPONENT_CONFIG(RandomFederationTestConfig)
};
