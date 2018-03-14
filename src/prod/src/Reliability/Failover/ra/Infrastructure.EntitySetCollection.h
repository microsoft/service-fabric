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
            /*
                This class is a registry of all the sets and associated data
                When components are added to the ra they register their sets
                The sets are stored as pointers so the registrars must own the set

                Components can register optional retry timers as well
             */
            class EntitySetCollection
            {
            public:
                class Data
                {
                public:
                    Data(EntitySetIdentifier const & id,
                         EntitySet & set,
                         RetryTimer * timer) :
                    id_(id),
                    set_(&set),
                    timer_(timer)
                    {
                    }

                    __declspec(property(get = get_Id)) EntitySetIdentifier const & Id;
                    EntitySetIdentifier const & get_Id() const { return id_; }

                    __declspec(property(get = get_Set)) EntitySet & Set;
                    EntitySet & get_Set() { return *set_; }

                    __declspec(property(get = get_Timer)) RetryTimer * Timer;
                    RetryTimer * get_Timer() { return timer_; }

                private:
                    EntitySetIdentifier id_;
                    EntitySet * set_;
                    RetryTimer * timer_;
                };
                
                void Add(
                    EntitySetIdentifier const & id,
                    EntitySet & set, 
                    RetryTimer * timer)
                {
                    auto it = TryGet(id);
                    ASSERT_IF(it != store_.end(), "Trying to insert duplicate {0}", id);
                    store_.push_back(Data(id, set, timer));
                }

                Data Get(EntitySetIdentifier const & id) const
                {
                    auto it = TryGet(id);
                    ASSERT_IF(it == store_.end(), "{0} was not found in entity set collection", id);
                    return *it;
                }

            private:
                typedef std::vector<Data> Store;

                Store::const_iterator TryGet(EntitySetIdentifier const & id) const
                {
                    return std::find_if(store_.begin(), store_.end(), [&id](Data const & d) { return d.Id == id; });
                }

                Store store_;
            };
        }
    }
}


