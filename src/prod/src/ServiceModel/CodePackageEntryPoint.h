// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ServiceModel
{
    class CodePackageEntryPoint 
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        CodePackageEntryPoint();

        CodePackageEntryPoint(
            std::wstring const & entryPointLocation, 
            int64 processId, 
            std::wstring const & runAsUserName, 
            Common::DateTime const & nextActivationTime,        
            EntryPointStatus::Enum entryPointStatus,
            CodePackageEntryPointStatistics const & statistics,
            int64 instanceId);

        void ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_CODE_PACKAGE_ENTRY_POINT & publicCodePackageEntryPoint) const; 

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        Common::ErrorCode FromPublicApi(__in FABRIC_CODE_PACKAGE_ENTRY_POINT const & publicCodePackageEntryPoint);

        __declspec(property(get=get_EntryPointLocation)) std::wstring const & EntryPointLocation;
        std::wstring const& get_EntryPointLocation() const { return entryPointLocation_; }

        __declspec(property(get=get_ProcessId)) int64 ProcessId;
        int64 get_ProcessId() const { return processId_; }

         __declspec(property(get=get_RunAsUserName)) std::wstring const & RunAsUserName;
        std::wstring const& get_RunAsUserName() const { return runAsUserName_; }

        __declspec(property(get=get_NextActivationTime)) Common::DateTime const & NextActivationTime;
        Common::DateTime const & get_NextActivationTime() const { return nextActivationTime_; }

        __declspec(property(get=get_EntryPointStatus)) EntryPointStatus::Enum EntryPointStatus;
        EntryPointStatus::Enum get_EntryPointStatus() const { return entryPointStatus_; }

        __declspec(property(get=get_Statistics)) CodePackageEntryPointStatistics const & Statistics;
        CodePackageEntryPointStatistics const & get_Statistics() const { return statistics_; }

        __declspec(property(get=get_InstanceId)) uint64 InstanceId;
        uint64 get_InstanceId() const { return instanceId_; }    

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::EntryPointLocation, entryPointLocation_)
            SERIALIZABLE_PROPERTY(Constants::ProcessId, processId_)
            SERIALIZABLE_PROPERTY(Constants::RunAsUserName, runAsUserName_)
            SERIALIZABLE_PROPERTY(Constants::NextActivationTime, nextActivationTime_)
            SERIALIZABLE_PROPERTY_ENUM(Constants::Status, entryPointStatus_)
            SERIALIZABLE_PROPERTY(Constants::CodePackageEntryPointStatistics, statistics_)
            SERIALIZABLE_PROPERTY(Constants::InstanceId, instanceId_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        FABRIC_FIELDS_07(entryPointLocation_, processId_, runAsUserName_, nextActivationTime_, entryPointStatus_, statistics_, instanceId_)

    private:
        std::wstring entryPointLocation_;
        int64 processId_;
        std::wstring runAsUserName_;        
        Common::DateTime nextActivationTime_;        
        EntryPointStatus::Enum entryPointStatus_;
        CodePackageEntryPointStatistics statistics_;
        int64 instanceId_;
    };
}
