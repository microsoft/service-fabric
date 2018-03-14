// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{

    class RegisterContainerActivatorServiceReply : public Serialization::FabricSerializable
    {
    public:
        RegisterContainerActivatorServiceReply();
        RegisterContainerActivatorServiceReply(
            Common::ErrorCode error,
            bool isContainerServicePresent,
            bool isContainerServiceManaged,
            uint64 eventSinceTime);
            
        __declspec(property(get=get_Error)) Common::ErrorCode Error;
        Common::ErrorCode const & get_Error() const { return error_; }

        __declspec(property(get = get_IsContainerServicePresent)) bool IsContainerServicePresent;
        bool get_IsContainerServicePresent() const { return isContainerServicePresent_; }

        __declspec(property(get = get_IsContainerServiceManaged)) bool IsContainerServiceManaged;
        bool get_IsContainerServiceManaged() const { return isContainerServiceManaged_; }

        __declspec(property(get = get_EventSinceTime)) uint64 EventSinceTime;
        uint64 get_EventSinceTime() const { return eventSinceTime_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_04(error_, isContainerServicePresent_, isContainerServiceManaged_, eventSinceTime_);

    private:
        Common::ErrorCode error_;
        bool isContainerServicePresent_;
        bool isContainerServiceManaged_;
        uint64 eventSinceTime_;
    };
}
