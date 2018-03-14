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
                An entity set holds a set of pointers to entities
                For example: the set of all fts for which replica open is pending
                
                There are two aspects to being part of a set of entities:
                - The entity being present in the std::set itself
                - The SetMembershipFlag in the entity state that is set to true

                The presence in a std::set allows for easy enumeration of such entities 
                without having to travel the entire LFUM. NOTE: It is possible that the
                snapshot of the set generated is out of date by the time the entity is locked
                so any processor needs to first ensure that the entity is still in the set
                before processing it. 

                The SetMembershipFlag is a boolean state on an entity that indicates whether
                an entity is part of a set or not

                It is required that when the entity write lock is held the set membership flag and
                the contents of the std::set are equivalent

                It is not possible to add items directly into an entity set. Instead a ChangeSetMembershipAction 
                must be created by changing the value of a SetMembershipFlag while holding on to a locked entity.
                When the locked entity is committed the action queue is executed which will insert the entity
                into the set (the lock on the entity is still held which avoids any threading issues).
            */
            class EntitySet
            {
                DENY_COPY(EntitySet);

            public:
                struct Parameters
                {
                    Parameters() : PerfData(nullptr) {}

                    Parameters(std::wstring const & displayName, EntitySetIdentifier const & id, Common::PerformanceCounterData * perfData) :
                    DisplayName(displayName),
                    Id(id),
                    PerfData(perfData)
                    {
                    }

                    std::wstring DisplayName;
                    EntitySetIdentifier Id;
                    Common::PerformanceCounterData * PerfData;
                };

                EntitySet(Parameters const & parameters);

                ~EntitySet();

                __declspec(property(get = get_Id)) EntitySetIdentifier const & Id;
                EntitySetIdentifier const & get_Id() const { return id_; }

                __declspec(property(get = get_IsEmpty)) bool IsEmpty;
                bool get_IsEmpty() const
                {
                    Common::AcquireReadLock grab(lock_);
                    return entities_.empty();
                }

                __declspec(property(get = get_Size)) uint64 Size;
                uint64 get_Size() const
                {
                    Common::AcquireReadLock grab(lock_);
                    return static_cast<uint64>(entities_.size());
                }

                // Returns a random entity id (or empty string)
                std::string GetRandomEntityTraceId() const;

                Infrastructure::EntityEntryBaseList GetEntities() const;

                bool IsSubsetOf(Infrastructure::EntityEntryBaseSet const & s) const;

                __declspec(property(get = get_Test_FTSet)) Infrastructure::EntityEntryBaseSet & Test_FTSet;
                Infrastructure::EntityEntryBaseSet & get_Test_FTSet() { return entities_; }

            private:
                uint64 AddEntity(Infrastructure::EntityEntryBaseSPtr const & id);

                uint64 RemoveEntity(Infrastructure::EntityEntryBaseSPtr const & id);

                std::wstring const displayName_;
                EntitySetIdentifier id_;
                Common::PerformanceCounterData * perfData_;

                mutable Common::RwLock lock_;
                Infrastructure::EntityEntryBaseSet entities_;

            public:

                // This action is used to add or remove an FT from a set
                class ChangeSetMembershipAction : public StateMachineAction
                {
                    DENY_COPY(ChangeSetMembershipAction);

                public:
                    ChangeSetMembershipAction(EntitySetIdentifier const & id, bool addToSet);

                    void OnPerformAction(
                        std::wstring const &, 
                        Infrastructure::EntityEntryBaseSPtr const &,
                        ReconfigurationAgent & reconfigurationAgent);

                    void OnPerformAction(
                        std::wstring const &,
                        Infrastructure::EntityEntryBaseSPtr const &,
                        Infrastructure::EntitySetCollection const & setCollection);

                    void OnCancelAction(ReconfigurationAgent&);

                private:
                    bool const addToSet_;
                    Infrastructure::EntitySetIdentifier id_;
                };
            };
        }
    }
}

