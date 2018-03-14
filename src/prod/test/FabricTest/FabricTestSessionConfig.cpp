// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace FabricTest
{
    using namespace std;
    using namespace Common;

DEFINE_SINGLETON_COMPONENT_CONFIG(FabricTestSessionConfig)

    FabricTestQueryWeights FabricTestQueryWeights::Parse(StringMap const & entries)
    {
        FabricTestQueryWeights queryWeightsMap;

        for (auto entry = entries.begin(); entry != entries.end(); ++entry)
        {
            wstring const & key = entry->first;
            wstring const & value = entry->second;
            int weight = 0;
            TestSession::FailTestIfNot(StringUtility::TryFromWString(value, weight), "Could nto parse {0} as int", value);
            queryWeightsMap.insert(make_pair(key, weight));
        }

        return queryWeightsMap;
    }
    
    void FabricTestQueryWeights::WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
    {
        for (auto it = begin(); it != end(); ++it)
        {
            w.WriteLine("{0}:{1}", it->first, it->second);
        }
    }
}
