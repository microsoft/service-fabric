// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ServiceFactoryRegistrationTable
    {
        DENY_COPY(ServiceFactoryRegistrationTable)

        public:
            ServiceFactoryRegistrationTable();

            Common::ErrorCode Add(ServiceFactoryRegistrationSPtr const & registration);
            Common::ErrorCode Remove(ServiceModel::ServiceTypeIdentifier const & serviceTypeId, bool removeInvalid = false);
            Common::ErrorCode Validate(ServiceModel::ServiceTypeIdentifier const & serviceTypeId);

            Common::AsyncOperationSPtr BeginFind(
				ServiceModel::ServiceTypeIdentifier const & serviceTypeId,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            Common::ErrorCode EndFind(
                Common::AsyncOperationSPtr const & operation,
                __out ServiceFactoryRegistrationSPtr & registration,
                __out ServiceModel::ServiceTypeIdentifier & serviceTypeId);
            //TODO: serviceTypeId is required as a part of End as caller is not aware about the type of operation
            //To avoid this we can create an AsyncOperation in the caller, store serviceTypeId and can pass it as a parent to BeginFind

            //For Test Only
            bool Test_IsServiceTypeInvalid(ServiceModel::ServiceTypeIdentifier const & serviceTypeId);

        private:
            class Entry;
            typedef std::shared_ptr<Entry> EntrySPtr;
            class EntryValidationLinkedAsyncOperation;

        private:
            Common::RwLock lock_;
            std::map<ServiceModel::ServiceTypeIdentifier, EntrySPtr> map_;
    };
}
