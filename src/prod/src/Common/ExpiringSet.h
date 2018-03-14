// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    template <typename T1, typename T2 = detail::NilType>
    class ExpiringSet : public std::map<T1, T2>
    {
        struct ExpireEntry
        {
            T1 Key;
            StopwatchTime ExpireTime;
            std::unique_ptr<ExpireEntry> Next;

            ExpireEntry(T1 key, StopwatchTime expireTime)
                : Key(key), ExpireTime(expireTime), Next(nullptr)
            {
            }
        };

        DENY_COPY(ExpiringSet);

    public:
        ExpiringSet(TimeSpan ttl, size_t maxCapacity = 0)
            : queue_(T1(), StopwatchTime::MaxValue), tail_(&queue_), ttl_(ttl), maxCapacity_(maxCapacity)
        {
        }

        bool TryAdd(T1 const & key, T2 && value = T2())
        {
            if (this->find(key) != this->end())
            {
                return false;
            }

            if (maxCapacity_ > 0 && this->size() >= maxCapacity_)
            {
                RemoveEntries(true);
                RemoveEntries(false);

                if (this->size() >= maxCapacity_)
                {
                    return false;
                }
            }

            this->insert(make_pair(key, std::move(value)));
            tail_->Next = make_unique<ExpireEntry>(key, Stopwatch::Now() + ttl_);
            tail_ = tail_->Next.get();

            return true;
        }

        void RemoveExpiredEntries()
        {
            RemoveEntries(true);
        }

    private:
        void RemoveEntries(bool removeExpiredOnly)
        {
            StopwatchTime now = Stopwatch::Now();

            ExpireEntry* prev = &queue_;
            while (prev->Next)
            {
                ExpireEntry* entry = prev->Next.get();
                bool remove = (removeExpiredOnly? now > entry->ExpireTime : this->size() >= maxCapacity_);
                if (!remove)
                {
                    return;
                }

                auto it = this->find(entry->Key);
                if (it != this->end())
                {
                    __if_exists (T2::IsCompleted)
                    {
                        remove = it->second.IsCompleted();
                    }

                    if (remove)
                    {
                        this->erase(it);
                    }
                }

                if (remove)
                {
                    if (tail_ == entry)
                    {
                        tail_ = prev;
                    }

                    prev->Next = std::move(entry->Next);
                }
                else
                {
                    prev = prev->Next.get();
                }
            }
        }

        ExpireEntry queue_;
        ExpireEntry* tail_;
        TimeSpan ttl_;
        size_t maxCapacity_;
    };
}
