// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Hosting
        {
            class ServiceTypeMap
            {
                DENY_COPY(ServiceTypeMap);

            public:
                ServiceTypeMap(ReconfigurationAgent & ra);

                __declspec(property(get = get_IsEmpty)) bool IsEmpty;
                bool get_IsEmpty() const
                {
                    Common::AcquireReadLock grab(lock_);
                    return map_.size() == 0;
                }

                int GetServiceTypeCount(ServiceModel::ServiceTypeIdentifier const & identifier) const
                {
                    Common::AcquireReadLock grab(lock_);
                    auto it = map_.find(identifier);
                    ASSERT_IF(it == map_.end(), "Could not find id {0}", identifier);
                    return it->second->Count;
                }

                void Test_Add(
                    Hosting2::ServiceTypeRegistrationSPtr const & registration)
                {
                    bool ignored = false;
                    Add(L"", registration, ignored);
                }

                void OnServiceTypeRegistrationAdded(
                    std::wstring const & activityId,
                    Hosting2::ServiceTypeRegistrationSPtr const & registration,
                    Infrastructure::BackgroundWorkManagerWithRetry & bgmr);

                void OnServiceTypeRegistrationRemoved(
                    std::wstring const & activityId,
                    Hosting2::ServiceTypeRegistrationSPtr const & registration);

                void OnWorkManagerRetry(
                    std::wstring const & activityId,
                    Infrastructure::BackgroundWorkManagerWithRetry & bgmr,
                    ReconfigurationAgent & ra) const;
                
                void OnReplyFromFM(std::vector<ServiceTypeInfo> const & infos);

            private:
                typedef std::shared_ptr<ServiceTypeMapEntity> ServiceTypeMapEntitySPtr;
                typedef std::map<ServiceModel::ServiceTypeIdentifier, ServiceTypeMapEntitySPtr> ServiceTypeEntityMap;

                void Add(
                    std::wstring const & activityId,
                    Hosting2::ServiceTypeRegistrationSPtr const & registration,
                    __out bool & isSendRequired);

                ServiceTypeEntityMap map_;
                mutable Common::RwLock lock_;
                IntConfigEntry const & numberOfEntriesInMessage_;
                ReconfigurationAgent & ra_;
            };
        }
    }
}


