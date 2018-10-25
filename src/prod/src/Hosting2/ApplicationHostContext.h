// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Hosting2
{
    //
    // Created at the HostingSubsystem and saved to Environment for Activated hosts. Read back by the ApplicationHost 
    // from the Environment for Activated hosts. For Non-Activated hosts it is created by the ApplicationHostContainer
    // The processId field is updated by the ApplicationHost in its ctor
    //
    class ApplicationHostContext : public Serialization::FabricSerializable
    {
    public:
        ApplicationHostContext();
        ApplicationHostContext(
            std::wstring const & hostId,
            ApplicationHostType::Enum const hostType,
            bool isContainerHost,
            bool isCodePackageActivatorHost);

        ApplicationHostContext(ApplicationHostContext const & other);
        ApplicationHostContext(ApplicationHostContext && other);

        __declspec(property(get = get_HostId)) std::wstring const & HostId;
        inline std::wstring const & get_HostId() const { return hostId_; };

        __declspec(property(get = get_HostType)) ApplicationHostType::Enum const & HostType;
        inline ApplicationHostType::Enum const & get_HostType() const { return hostType_; };

        __declspec(property(get = get_IsContainerHost)) bool IsContainerHost;
        inline bool get_IsContainerHost() const { return isContainerHost_; };

        __declspec(property(get = get_IsCodePackageActivatorHost)) bool IsCodePackageActivatorHost;
        inline bool get_IsCodePackageActivatorHost() const { return isCodePackageActivatorHost_; };

        __declspec(property(get = get_ProcessId, put = put_ProcessId)) DWORD ProcessId;
        inline DWORD get_ProcessId() const { return processId_; };
        inline void put_ProcessId(DWORD value) { processId_ = value; }

        ApplicationHostContext const & operator = (ApplicationHostContext const & other);
        ApplicationHostContext const & operator = (ApplicationHostContext && other);

        bool operator == (ApplicationHostContext const & other) const;
        bool operator != (ApplicationHostContext const & other) const;

        static Common::ErrorCode FromEnvironmentMap(
            Common::EnvironmentMap const & envMap, 
            __out ApplicationHostContext & hostContext);
        
        void ToEnvironmentMap(Common::EnvironmentMap & envMap) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_05(hostId_, hostType_, processId_, isContainerHost_, isCodePackageActivatorHost_);

    private:
        std::wstring hostId_;
        ApplicationHostType::Enum hostType_;
        DWORD processId_;
        bool isContainerHost_;
        bool isCodePackageActivatorHost_;

        static Common::GlobalWString EnvVarName_HostId;
        static Common::GlobalWString EnvVarName_HostType;
        static Common::GlobalWString EnvVarName_IsCodePackageActivatorHost;
    };
}
