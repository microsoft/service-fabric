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
                Executes any function under a read lock over the entire contents of a map
            */
            template<typename T>
            void ExecuteUnderReadLockOverMap(
                EntityMap<T> const & entityMap, 
                std::function<void (ReadOnlyLockedEntityPtr<T> &)> action)
            {
                auto entries = entityMap.GetAllEntries();

                ExecuteUnderReadLock<T>(entries.begin(), entries.end(), action);
            }

            /*
                Executes a function over a range taking a read lock over each entity
                Deleted entities are not returned
            */
            template<typename T, typename iterator>
            void ExecuteUnderReadLock(
                iterator begin,
                iterator end,
                std::function<void(ReadOnlyLockedEntityPtr<T> &)> action)
            {
                for (auto it = begin; it != end; ++it)
                {
                    auto & casted = static_cast<EntityEntry<T>&>(**it);
                    auto lock = casted.CreateReadLock();
                    if (lock.IsEntityDeleted)
                    {
                        continue;
                    }

                    action(lock);
                }
            }
            
            /*
                Executes a function over a read lock
            */
            template<typename T>
            void ExecuteUnderReadLock(
                Infrastructure::EntityEntryBaseSPtr const & entity,
                std::function<void(ReadOnlyLockedEntityPtr<T> &)> action)
            {
                auto const & casted = entity->As<EntityEntry<T>>();

                auto lock = casted.CreateReadLock();

                if (lock.IsEntityDeleted)
                {
                    return;
                }

                action(lock);
            }

            /*
                Filters the contents of an entire entity map
                If the entity passes the checks then the filter is invoked
                Any entity that passes through the filter is returned in the list
            */
            template<typename T>
            EntityEntryBaseList FilterUnderReadLockOverMap(
                EntityMap<T> const & entityMap,
                JobItemCheck::Enum checks,
                std::function<bool (ReadOnlyLockedEntityPtr<T> &)> filter,
                __inout Diagnostics::FilterEntityMapPerformanceData & performanceData)
            {
                performanceData.OnStarted();

                auto entries = entityMap.GetAllEntries();

                performanceData.OnSnapshotComplete(entries.size());

                auto rv = FilterUnderReadLock<T>(entries.begin(), entries.end(), checks, filter);

                performanceData.OnFilterComplete(rv.size());

                return rv;
            }

            /*
                Filters the contents of a range
                If the entity passes the checks then the filter is invoked
                Any entity that passes through the filter is returned in the list
            */
            template<typename T, typename iterator>
            EntityEntryBaseList FilterUnderReadLock(
                iterator begin,
                iterator end,
                JobItemCheck::Enum checks,
                std::function<bool (ReadOnlyLockedEntityPtr<T> &)> filter)
            {
                EntityEntryBaseList rv;
                                       
                for (auto it = begin; it != end; ++it)
                {
                    auto & casted = static_cast<EntityEntry<T>&>(**it);
                    auto lock = casted.CreateReadLock();
                    
                    if (lock.IsEntityDeleted || !EntityTraits<T>::PerformChecks(checks, lock.get_Current()))
                    {
                        continue;
                    }

                    if (filter(lock))
                    {
                        rv.push_back(std::move(*it));
                    }
                }

                return rv;
            }
        }
    }
}
