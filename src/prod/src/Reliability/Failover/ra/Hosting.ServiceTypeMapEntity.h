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
            class ServiceTypeMapEntity : public Infrastructure::EntityRetryComponent<Reliability::ServiceTypeInfo>
            {
            public:
                ServiceTypeMapEntity(
                    Federation::NodeInstance const & nodeInstance,
                    Hosting2::ServiceTypeRegistrationSPtr const & registration,
                    TimeSpanConfigEntry const & retryInterval);

                __declspec(property(get = get_Count)) int Count;
                int get_Count() const { return count_; }

                void OnRegistrationAdded(
                    std::wstring const & activityId,
                    Hosting2::ServiceTypeRegistrationSPtr const & registration,
                    __out bool & isSendRequired);

                void OnRegistrationRemoved(
                    std::wstring const & activityId,
                    __out bool & isDeleteRequired);

                void OnReply(ServiceTypeInfo const & info);

            private:
                Reliability::ServiceTypeInfo GenerateInternal() const override;
                void Trace() const;

                int count_;
                ServiceModel::ServiceTypeIdentifier id_;
                ServiceModel::ServicePackageVersionInstance version_;
                std::wstring codePackageName_;

                std::wstring idStr_;
                Federation::NodeInstance nodeInstance_;
            };
        }
    }
}


