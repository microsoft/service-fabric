// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace ReliabilityUnitTest
        {
            class ReplicaStoreHolder
            {
            public:
                ReplicaStoreHolder() : replicaStore_(&replicas_)
                {
                }

                ReplicaStoreHolder(std::vector<Replica> && replicas) : 
                    replicas_(replicas),
                    replicaStore_(&replicas_)
                {
                }

                ReplicaStoreHolder(ReplicaStoreHolder const & other) :
                    replicas_(other.replicas_),
                    replicaStore_(&replicas_)
                {
                }

                ReplicaStoreHolder& operator=(ReplicaStoreHolder const & other) 
                {
                    if (this != &other)
                    {
                        replicas_ = other.replicas_;
                    }

                    return *this;
                }

                __declspec(property(get = get_ReplicaStoreObj)) ReplicaStore & ReplicaStoreObj;
                ReplicaStore & get_ReplicaStoreObj() { return replicaStore_; }
                ReplicaStore const & get_ReplicaStoreObj() const { return replicaStore_; }

                __declspec(property(get = get_Replicas)) std::vector<Replica> & Replicas;
                std::vector<Replica> & get_Replicas() { return replicas_; }
                std::vector<Replica> const & get_Replicas() const { return replicas_; }

            private:
                std::vector<Replica> replicas_;
                ReplicaStore replicaStore_;
            };
        }
    }
}



