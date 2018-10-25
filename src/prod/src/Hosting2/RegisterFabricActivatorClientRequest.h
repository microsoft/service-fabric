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
            std::wstring const & nodeId,
            uint64 nodeInstanceId = 0);
            
        __declspec(property(get=get_ProcessId)) DWORD ParentProcessId;
        DWORD get_ProcessId() const { return parentProcessId_; }

        __declspec(property(get=get_NodeId)) std::wstring const & NodeId;
        std::wstring const & get_NodeId() const { return nodeId_; }

        __declspec(property(get=get_NodeInstanceId)) uint64 NodeInstanceId;
        uint64 get_NodeInstanceId() const { return nodeInstanceId_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_03(parentProcessId_, nodeId_, nodeInstanceId_);

    private:
        DWORD parentProcessId_;
        std::wstring nodeId_;
        uint64 nodeInstanceId_;
    };
}
