// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class RuntimeRegistrationTable :
        public Common::RootedObject,
        Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(RuntimeRegistrationTable)
    public:
        RuntimeRegistrationTable(Common::ComponentRoot const & root);
        virtual ~RuntimeRegistrationTable();

        Common::ErrorCode Add(
            FabricRuntimeContext const & runtimeContext);

        Common::ErrorCode Validate(
            std::wstring const & runtimeId, 
            __out RuntimeRegistrationSPtr & registration);

        Common::ErrorCode Find(
            std::wstring const & runtimeId, 
            __out RuntimeRegistrationSPtr & registration);

        Common::ErrorCode Find(
            ServicePackageInstanceIdentifier const & servicePackageInstanceId,
            __out std::vector<RuntimeRegistrationSPtr> & registrations);

        Common::ErrorCode Find(
            CodePackageInstanceIdentifier const & codePackageInstanceId,
            __out std::vector<RuntimeRegistrationSPtr> & registrations);

        Common::ErrorCode Remove(
            std::wstring const & runtimeId, 
            __out RuntimeRegistrationSPtr & registration);

        Common::ErrorCode RemoveRegistrationsFromHost(
            std::wstring const & hostId, 
        __out std::vector<RuntimeRegistrationSPtr> & registrations);

        Common::ErrorCode RemoveRegistrationsFromServicePackage(
            ServicePackageInstanceIdentifier const & servicePackageInstanceId,
            int64 servicePackageInstanceSeqNum,
            __out std::vector<RuntimeRegistrationSPtr> & registrations);

        Common::ErrorCode RemoveRegistrationsFromCodePackage(
            CodePackageInstanceIdentifier const & codePackageInstanceId,
            int64 codePackageInstanceSeqNum,
            __out std::vector<RuntimeRegistrationSPtr> & registrations);

        Common::ErrorCode UpdateRegistrationsFromCodePackage(
            CodePackageContext const & existingContext,
            CodePackageContext const & updatedContext);

        std::vector<RuntimeRegistrationSPtr> Close();

    private:
        class Entry;
        typedef std::shared_ptr<Entry> EntrySPtr;

        void AddEntryTx_CallerHoldsLock(EntrySPtr const & entry);
        void RemoveFromMainMap_CallersHoldsLock(EntrySPtr const & entry);
        void RemoveFromHostIndex_CallerHoldsLock(EntrySPtr const & entry);
        void RemoveFromServicePackageIndex_CallerHoldsLock(EntrySPtr const & entry);
        void RemoveFromCodePackageIndex_CallerHoldsLock(EntrySPtr const & entry);

        typedef std::map<std::wstring, EntrySPtr, Common::IsLessCaseInsensitiveComparer<std::wstring>> EntryMap;
        typedef std::map<std::wstring, std::vector<EntrySPtr>, Common::IsLessCaseInsensitiveComparer<std::wstring>> HostIdIndexMap;
        typedef std::map<ServicePackageInstanceIdentifier, std::vector<EntrySPtr>> ServicePackageIdIndexMap;
        typedef std::map<CodePackageInstanceIdentifier, std::vector<EntrySPtr>> CodePackageIdIndexMap;

    private:
        bool isClosed_;
        Common::RwLock lock_;
        EntryMap map_;
        HostIdIndexMap hostIndex_;
        ServicePackageIdIndexMap servicePackageIndex_;
        CodePackageIdIndexMap codePackageIndex_;
    };
}
