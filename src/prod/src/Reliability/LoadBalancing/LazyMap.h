// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        template <typename TKey, typename TValue>
        class LazyMap
        {
            friend class ApplicationPlacement;
            friend class ApplicationNodeLoads;
            typedef LazyMap<TKey, TValue> T;
            DENY_COPY_ASSIGNMENT(T);

        public:
            explicit LazyMap(TValue && empty)
                : baseMap_(nullptr), empty_(std::move(empty))
            {
            }

            explicit LazyMap(T const* baseMap)
                : baseMap_(baseMap), empty_(TValue(baseMap->empty_))
            {
            }

            LazyMap(TValue && empty, std::map<TKey, TValue> && data)
                : baseMap_(nullptr), empty_(std::move(empty)), data_(std::move(data))
            {
            }

            LazyMap(T && other)
                : baseMap_(other.baseMap_), data_(std::move(other.data_)), empty_(std::move(other.empty_))
            {
            }

            LazyMap(T const& other)
                : baseMap_(other.baseMap_), data_(other.data_), empty_(TValue(other.empty_))
            {
            }

            T & operator = (T && other)
            {
                if (this != &other)
                {
                    baseMap_ = other.baseMap_;
                    data_ = std::move(other.data_);
                    empty_ = std::move(other.empty_);
                }

                return *this;
            }

            void ForEach(std::function<bool(std::pair<TKey, TValue> const&)> processor) const
            {
                for (auto it = data_.begin(); it != data_.end(); ++it)
                {
                    if (!processor(*it))
                    {
                        break;
                    }
                }
            }

            bool ForEachKey(std::function<bool(TKey)> processor) const
            {
                bool retValue = true;
                for (auto it = data_.begin(); it != data_.end(); ++it)
                {
                    if (!processor(it->first))
                    {
                        retValue = false;
                        break;
                    }
                }
                return retValue;
            }

            bool HasKey(TKey key) const
            {
                return data_.find(key) != data_.end();
            }

            TValue const& operator[](TKey key) const
            {
                typename std::map<TKey, TValue>::const_iterator it = data_.find(key);

                if (it != data_.end())
                {
                    return it->second;
                }
                else if (baseMap_ != nullptr)
                {
                    return baseMap_->operator[](key);
                }
                else
                {
                    return empty_;
                }
            }

            TValue & operator[](TKey key)
            {
                typename std::map<TKey, TValue>::iterator it = data_.find(key);

                if (it != data_.end())
                {
                    return it->second;
                }
                else if (baseMap_ != nullptr)
                {
                    it = data_.insert(std::make_pair(key, TValue(baseMap_->operator[](key)))).first;
                    return it->second;
                }
                else
                {
                    it = data_.insert(std::make_pair(key, TValue(empty_))).first;
                    return it->second;
                }
            }

            bool IsEmpty() const
            {
                if (!data_.empty())
                {
                    return false;
                }
                else if (baseMap_ == nullptr)
                {
                    return true;
                }
                else
                {
                    return baseMap_->IsEmpty();
                }
            }

            void Clear()
            {
                data_.clear();
            }

            size_t DataSize()
            {
                return data_.size();
            }

            T const* Base() const
            {
                return baseMap_;
            }

        protected:
            TValue empty_;
            T const* baseMap_;
            std::map<TKey, TValue> data_;
        };
    }
}
