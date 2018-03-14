// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class Utility
        {
        public:
            template<class KeyT, class ValueT> 
            static void AppendIfFoundInMap(std::map<KeyT, ValueT> const & m, KeyT const & key, std::string const & valueToWrite, std::string & outputString)
            {
                if (m.find(key) != m.end())
                {
                    outputString += valueToWrite;
                }
            }
        };
    }
}
