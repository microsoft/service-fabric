// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ReliabilityTestApi
{
    template<typename TInput, typename TOutput>
    std::vector<TOutput> ConstructSnapshotObjectList(std::vector<TInput> & input)
    {
        std::vector<TOutput> rv;

        for(auto it = input.begin(); it != input.end(); ++it)
        {
            rv.push_back(TOutput(*it));
        }

        return rv;
    }
}
