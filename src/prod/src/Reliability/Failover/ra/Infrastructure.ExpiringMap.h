// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Infrastructure
        {
            template <class Key, class T>
            class ExpiringMap
            {
            public:
                typedef std::pair<const Key, T> value_type;
                typedef map<Key, pair<T, Common::StopwatchTime>> inner_map_type;

                ExpiringMap()
                {
                }

                ExpiringMap(ExpiringMap && other)
                  : map_(move(other.map_))
                {
                }

                ~ExpiringMap()
                {
                }

                typename inner_map_type::iterator begin()
                {
                    return map_.begin();
                }

                typename inner_map_type::const_iterator cbegin()
                {
                    return map_.cbegin();
                }

                typename inner_map_type::iterator end()
                {
                    return map_.end();
                }

                typename inner_map_type::const_iterator cend()
                {
                    return map_.cend();
                }

                typename inner_map_type::iterator find(const Key& key)
                {
                    return map_.find(key);
                }

                std::pair<typename inner_map_type::iterator, bool> insert(
                    value_type const& value,
                    Common::StopwatchTime time)
                {
                    return map_.insert(make_pair(value.first, make_pair(value.second, time)));
                }

                std::pair<typename inner_map_type::iterator, bool> insert_or_assign(
                    Key const& key,
                    T & value,
                    Common::StopwatchTime time)
                {
                    auto it = map_.find(key);
                    return insert_or_assign(it, key, value, time);
                }

                std::pair<typename inner_map_type::iterator, bool> insert_or_assign(
                    typename inner_map_type::iterator it,
                    Key const& key,
                    T & value,
                    Common::StopwatchTime time)
                {
                    if (it != map_.end())
                    {
                        it->second = make_pair(value, time);
                        return make_pair(it, false);
                    }

                    return insert(make_pair(key, value), time);
                }

                void PerformCleanup(Common::StopwatchTime expirationTime)
                {
                    auto it = map_.begin();
                    while (it != map_.end())
                    {
                        if (it->second.second < expirationTime)
                        {
                            it = map_.erase(it);
                        }
                        else
                        {
                            ++it;
                        }
                    }
                }

            private:
                inner_map_type map_;
            };
        }
    }
}
