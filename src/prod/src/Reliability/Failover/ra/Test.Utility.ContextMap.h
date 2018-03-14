// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "RATestPointers.h"

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace ReliabilityUnitTest
        {
            class TestContextMap
            {
            public:
                typedef int KeyType;

                static const KeyType TestEntityMapKey = 0;

                template<typename T>
                void Add(KeyType key, std::shared_ptr<T> const & value)
                {
                    auto casted = std::static_pointer_cast<void>(value);
                    map_[key] = casted;
                }

                template<typename T>
                T & Get(KeyType key)
                {
                    auto it = map_.find(key);
                    ASSERT_IF(it == map_.end(), "Context does not contain {0}", key);

                    auto casted = std::static_pointer_cast<T>(it->second);

                    return *casted;
                }

                template<typename T>
                T const & Get(KeyType key) const
                {
                    auto it = map_.find(key);
                    ASSERT_IF(it == map_.cend(), "Context does not contain {0}", key);

                    auto casted = std::dynamic_pointer_cast<const T>(it->second);
                    ASSERT_IF(casted == nullptr, "Could not typecast {0}", key);

                    return *casted;
                }

            private:
                std::map<KeyType, std::shared_ptr<void>> map_;
            };
        }
    }
}
