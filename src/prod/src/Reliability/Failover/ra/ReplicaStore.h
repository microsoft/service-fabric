// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class ReplicaStore
        {
            class ConfigurationReplicaPredicate;
            class IdleReplicaPredicate;

            DENY_COPY(ReplicaStore);

        public:
            typedef Infrastructure::FilteredVector<Replica, ConfigurationReplicaPredicate> ConfigurationReplicaStore;
            typedef Infrastructure::FilteredVector<Replica, ConfigurationReplicaPredicate> ConfigurationRemoteReplicaStore;
            typedef Infrastructure::FilteredVector<Replica, IdleReplicaPredicate> IdleReplicaStore;
            typedef std::vector<Replica>::iterator IteratorType;
            typedef std::vector<Replica>::const_iterator ConstIteratorType;

            ReplicaStore(std::vector<Replica> * replicas) :
            replicas_(replicas),
            configurationReplicas_(*replicas, ConfigurationReplicaPredicate(*this, false)),
            configurationRemoteReplicas_(*replicas, ConfigurationReplicaPredicate(*this, true)),
            idleReplicaStore_(*replicas, IdleReplicaPredicate(*this))
            {
            }

            __declspec(property(get = get_ConfigurationReplicas)) ConfigurationReplicaStore & ConfigurationReplicas;
            ConfigurationReplicaStore & get_ConfigurationReplicas() { return configurationReplicas_; }
            ConfigurationReplicaStore const & get_ConfigurationReplicas() const { return configurationReplicas_; }

            __declspec(property(get = get_ConfigurationRemoteReplicas)) ConfigurationRemoteReplicaStore & ConfigurationRemoteReplicas;
            ConfigurationRemoteReplicaStore & get_ConfigurationRemoteReplicas() { return configurationRemoteReplicas_; }
            ConfigurationRemoteReplicaStore const & get_ConfigurationRemoteReplicas() const { return configurationRemoteReplicas_; }

            __declspec(property(get = get_IdleReplicas)) IdleReplicaStore & IdleReplicas;
            IdleReplicaStore & get_IdleReplicas() { return idleReplicaStore_; }
            IdleReplicaStore const & get_IdleReplicas() const { return idleReplicaStore_; }

            __declspec (property(get = get_LocalReplica)) Replica & LocalReplica;
            Replica const & get_LocalReplica() const
            {
                AssertIfEmpty();
                return replicas_->front();
            }
            Replica & get_LocalReplica()
            {
                AssertIfEmpty();
                return replicas_->front();
            }

            bool IsLocalReplica(Replica const & replica) const
            {
                AssertIfEmpty();
                auto const & replicas = GetVector();
                return &replica == &(*replicas.cbegin());
            }

            Replica & Test_AddReplica(Replica const & replica)
            {
                ASSERT_IFNOT(GetReplica(replica.FederationNodeId).IsInvalid, "Trying to add an existing replica:\n{0}", replica);

                GetVector().push_back(replica);

                return GetVector().back();
            }

            Replica & AddReplica(ReplicaDescription const & replicaDesc, ReplicaRole::Enum pcRole, ReplicaRole::Enum ccRole)
            {
                ASSERT_IFNOT(GetReplica(replicaDesc.FederationNodeId).IsInvalid, "Trying to add an existing replica:\n{0}", replicaDesc);

                GetVector().push_back(Replica(replicaDesc.FederationNodeInstance, replicaDesc.ReplicaId, replicaDesc.InstanceId));

                Replica & newReplica = GetVector().back();

                newReplica.PreviousConfigurationRole = pcRole;
                newReplica.CurrentConfigurationRole = ccRole;

                newReplica.PackageVersionInstance = replicaDesc.PackageVersionInstance;

                return newReplica;
            }

            Replica const & GetReplica(Federation::NodeId id) const
            {
                auto & replicas = GetVector();
                auto it = std::find_if(replicas.cbegin(), replicas.cend(), [id](Replica const & r) { return r.FederationNodeId == id; });
                return it == replicas.cend() ? Replica::InvalidReplica : *it;
            }

            Replica & GetReplica(Federation::NodeId id)
            {
                auto & replicas = GetVector();
                auto it = std::find_if(replicas.begin(), replicas.end(), [id](Replica const & r) { return r.FederationNodeId == id; });
                return it == replicas.end() ? Replica::InvalidReplica : *it;
            }

            void RemoveReplica(int64 replicaId)
            {
                auto & replicas = GetVector();
                replicas.erase
                (
                    remove_if
                    (
                        replicas.begin(), replicas.end(),
                        [replicaId](Replica const & replica) -> bool
                        {
                            return replica.ReplicaId == replicaId;
                        }
                    ),
                    replicas.end()
                );
            }

            void Remove(std::vector<int64> const & replicaIdsToRemove)
            {
                for (auto it : replicaIdsToRemove)
                {
                    RemoveReplica(it);
                }
            }

            void Clear()
            {
                replicas_->clear();
            }

            IteratorType begin()
            {
                return replicas_->begin();
            }

            IteratorType end()
            {
                return replicas_->end();
            }

            ConstIteratorType begin() const
            {
                return replicas_->begin();
            }

            ConstIteratorType end() const
            {
                return replicas_->end();
            }

        private:
            class ConfigurationReplicaPredicate
            {
            public:
                ConfigurationReplicaPredicate(ReplicaStore & store, bool excludeLocalReplica) :
                store_(&store),
                excludeLocalReplica_(excludeLocalReplica)
                {
                }

                bool operator()(Replica const & replica) const
                {
                    if (!replica.IsInConfiguration)
                    {
                        return false;
                    }
                    
                    if (excludeLocalReplica_)
                    {
                        return !store_->IsLocalReplica(replica);
                    }

                    return true;
                }

            private:
                ReplicaStore * store_;
                bool excludeLocalReplica_;
            };

            class IdleReplicaPredicate
            {
            public:
                IdleReplicaPredicate(ReplicaStore & store) :
                store_(&store)
                {
                }

                bool operator()(Replica const & replica) const
                {
                    return !store_->IsLocalReplica(replica) && replica.PreviousConfigurationRole == ReplicaRole::None && replica.CurrentConfigurationRole == ReplicaRole::Idle;
                }

            private:
                ReplicaStore * store_;
            };

            void AssertIfEmpty() const
            {
                ASSERT_IF(replicas_->empty(), "Trying to empty vector");
            }

            std::vector<Replica> & GetVector()
            {
                return *replicas_;
            }

            std::vector<Replica> const & GetVector() const
            {
                return *replicas_;
            }

            std::vector<Replica> * replicas_;
            ConfigurationReplicaStore configurationReplicas_;
            ConfigurationRemoteReplicaStore configurationRemoteReplicas_;
            IdleReplicaStore idleReplicaStore_;
        };
    }
}



