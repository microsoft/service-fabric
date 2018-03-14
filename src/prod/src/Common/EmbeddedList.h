// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    // This class is used by the LruCache to maintain an LRU list
    // across items also stored in another structure (hash table).
    //
    // It supports moving an item within the same list by just
    // calling UpdateHead(item). If the item is coming from another
    // list, it must be removed from the other list first. 
    //
    template <typename TListEntry>
    class EmbeddedListEntry
    {
        DENY_COPY(EmbeddedListEntry)

    public:
        EmbeddedListEntry(std::shared_ptr<TListEntry> && entry) 
            : prev_()
            , next_() 
            , entry_(std::move(entry))
        { }

        EmbeddedListEntry(std::shared_ptr<TListEntry> const & entry) 
            : prev_()
            , next_() 
            , entry_(entry)
        { }

        virtual ~EmbeddedListEntry() { }

        std::shared_ptr<TListEntry> const & GetListEntry() const { return entry_; }
        void SetListEntry(std::shared_ptr<EmbeddedListEntry<TListEntry>> const & other) { entry_ = other->entry_; }

        void SetPrev(std::shared_ptr<EmbeddedListEntry<TListEntry>> const & prev) const { prev_ = prev; }
        void SetNext(std::shared_ptr<EmbeddedListEntry<TListEntry>> const & next) const { next_ = next; }

        void SwapPrev(__inout std::shared_ptr<EmbeddedListEntry<TListEntry>> & prev) const { prev_.swap(prev); }
        void SwapNext(__inout std::shared_ptr<EmbeddedListEntry<TListEntry>> & next) const { next_.swap(next); }

        std::shared_ptr<EmbeddedListEntry<TListEntry>> const & GetPrev() const { return prev_; }
        std::shared_ptr<EmbeddedListEntry<TListEntry>> const & GetNext() const { return next_; }

        bool TryUnlink() const
        {
            bool unlinked = false;

            auto next = next_;

            if (prev_)
            {
                prev_->SwapNext(next_);
                unlinked = true;
            }

            if (next)
            {
                next->SwapPrev(prev_);
                unlinked = true;
            }

            prev_.reset();
            next_.reset();

            return unlinked;
        }

        void Reset() const
        {
            prev_.reset();
            next_.reset();
        }

    private:
        mutable std::shared_ptr<EmbeddedListEntry<TListEntry>> prev_;
        mutable std::shared_ptr<EmbeddedListEntry<TListEntry>> next_;
        std::shared_ptr<TListEntry> entry_;
    };

    template <typename TListEntry>
    class EmbeddedList
    {
        DENY_COPY(EmbeddedList)

    private:
        typedef std::shared_ptr<EmbeddedListEntry<TListEntry>> ListEntrySPtr;

    public:
        EmbeddedList() 
            : head_()
            , tail_()
            , lock_()
            , count_(0) 
            , isCleared_(false)
        { }

        virtual ~EmbeddedList() { }

        size_t GetSize() const 
        { 
            AcquireReadLock lock(lock_);

            return count_; 
        }

        ListEntrySPtr const & GetHead() const { return head_; }
        ListEntrySPtr const & GetTail() const { return tail_; }

        ListEntrySPtr UpdateHead(__in ListEntrySPtr const & entry)
        {
            if (entry)
            {
                AcquireWriteLock lock(lock_);

                return this->InternalUpdateHead(entry);
            }

            return ListEntrySPtr();
        }

        ListEntrySPtr UpdateHeadAndTrim(
            ListEntrySPtr const & entry, 
            size_t limit)
        {
            ListEntrySPtr removed;

            if (entry)
            {

                AcquireWriteLock lock(lock_);

                this->InternalUpdateHead(entry);

                if (limit > 0 && count_ > limit)
                {
                    removed = tail_;

                    if (removed)
                    {
                        this->InternalRemoveFromList(removed);
                    }
                }
            }

            return removed;
        }

        void RemoveFromList(ListEntrySPtr const & entry)
        {
            if (entry)
            {
                AcquireWriteLock lock(lock_);

                if (isCleared_) { return; }

                this->InternalRemoveFromList(entry);
            }
        }

        void Clear()
        {
            this->TakeEntriesAndClear();
        }

        std::vector<ListEntrySPtr> TakeEntriesAndClear()
        {
            std::vector<ListEntrySPtr> entries;
            {
                AcquireWriteLock lock(lock_);

                for (auto current = head_; current; )
                {
                    entries.push_back(current);

                    auto prev = current;

                    current = current->GetNext(); 

                    prev->Reset();
                }

                head_.reset();
                tail_.reset();
                count_ = 0;
                isCleared_ = true;
            }

            return std::move(entries);
        }

    private:

        ListEntrySPtr InternalUpdateHead(ListEntrySPtr const & entry)
        {
            // Already first entry, so no-op
            if (entry == head_) { return head_; }

            // Update tail if moving tail
            if (entry == tail_)
            {
                tail_ = entry->GetPrev();
            }

            // Count does not change if moving the entry around
            // within the same list. Must remove from old list 
            // first if moving the entry between lists.
            //
            if (!entry->TryUnlink())
            {
                ++count_;
            }
            
            if (!head_)
            {
                tail_ = entry;
            }
            else
            {
                entry->SetNext(head_);
                head_->SetPrev(entry);
            }

            auto previousHead = head_;

            head_ = entry;

            return previousHead;
        }

        void InternalRemoveFromList(ListEntrySPtr const & entry)
        {
            if (entry == head_)
            {
                head_ = entry->GetNext();
            }

            if (entry == tail_)
            {
                tail_ = entry->GetPrev();
            }

            // TryUnlink will return false in the special case
            // where this is the last entry
            //
            if (entry->TryUnlink() || count_ == 1)
            {
                --count_;
            }
        }

        ListEntrySPtr head_;
        ListEntrySPtr tail_;
        mutable RwLock lock_;

        size_t count_;
        bool isCleared_;
    };
}
