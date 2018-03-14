// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{

    class RegisterContainerActivatorServiceRequest : public Serialization::FabricSerializable
    {
    public:
        RegisterContainerActivatorServiceRequest();
        RegisterContainerActivatorServiceRequest(DWORD processId, std::wstring const & listenAddress);
            
        __declspec(property(get=get_ProcessId)) DWORD ProcessId;
        DWORD get_ProcessId() const { return processId_; }

        __declspec(property(get=get_ListenAddress)) std::wstring const & ListenAddress;
        std::wstring const & get_ListenAddress() const { return listenAddress_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_02(processId_, listenAddress_);

    private:
        DWORD processId_;
        std::wstring listenAddress_;
    };
}
