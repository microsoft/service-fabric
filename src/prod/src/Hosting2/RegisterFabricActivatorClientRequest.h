// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{

    class RegisterFabricActivatorClientRequest : public Serialization::FabricSerializable
    {
    public:
        RegisterFabricActivatorClientRequest();
        RegisterFabricActivatorClientRequest(
            DWORD parentProcessId,
            std::wstring const & nodeId);
            
        __declspec(property(get=get_ProcessId)) DWORD ParentProcessId;
        DWORD get_ProcessId() const { return parentProcessId_; }

        __declspec(property(get=get_NodeId)) std::wstring const & NodeId;
        std::wstring const & get_NodeId() const { return nodeId_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_02(parentProcessId_, nodeId_);

    private:
        DWORD parentProcessId_;
        std::wstring nodeId_;
    };
}
