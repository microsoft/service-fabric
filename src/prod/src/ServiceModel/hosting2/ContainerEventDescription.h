// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Hosting2
{
    namespace ContainerEventType
    {
        enum Enum : ULONG
        {
            None = 0,
            Stop = 1,
            Die = 2,
            Health = 3,
            FirstValidEnum = None,
            LastValidEnum = Health
        };

        void WriteToTextWriter(__in Common::TextWriter & w, Enum const & e);

        Common::ErrorCode FromPublicApi(__in::FABRIC_CONTAINER_EVENT_TYPE, __out Enum &);

        DECLARE_ENUM_STRUCTURED_TRACE(ServicePackageActivationMode)
    }

    class ContainerEventDescription : public Serialization::FabricSerializable
    {
    public:
        ContainerEventDescription();

        __declspec(property(get = get_EventType)) ContainerEventType::Enum EventType;
        ContainerEventType::Enum get_EventType() const { return eventType_; }

        __declspec(property(get = get_ContainerId)) std::wstring const & ContainerId;
        std::wstring const & get_ContainerId() const { return containerId_; }

        __declspec(property(get = get_ContainerName)) std::wstring const & ContainerName;
        std::wstring const & get_ContainerName() const { return containerName_; }

        __declspec(property(get = get_TimeStampInSeconds)) int64 TimeStampInSeconds;
        int64 get_TimeStampInSeconds() const { return timeStampInSeconds_; }

        __declspec(property(get = get_IsHealthy)) bool IsHealthy;
        bool get_IsHealthy() const { return isHealthy_; }

        __declspec(property(get = get_IsHealthEvent)) bool IsHealthEvent;
        bool get_IsHealthEvent() const { return (eventType_ == ContainerEventType::Health); }

        __declspec(property(get = get_ExitCode)) DWORD ExitCode;
        DWORD get_ExitCode() const { return exitCode_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        Common::ErrorCode FromPublicApi(FABRIC_CONTAINER_EVENT_DESCRIPTION const & eventDescription);

        FABRIC_FIELDS_06(eventType_, containerId_, containerName_, timeStampInSeconds_, isHealthy_, exitCode_);

    private:
        ContainerEventType::Enum eventType_;
        std::wstring containerId_;
        std::wstring containerName_;
        int64 timeStampInSeconds_;
        
        bool isHealthy_; // Applies to health event.
        DWORD exitCode_; // Applies for die event.
    };
}

DEFINE_USER_ARRAY_UTILITY(Hosting2::ContainerEventDescription);
