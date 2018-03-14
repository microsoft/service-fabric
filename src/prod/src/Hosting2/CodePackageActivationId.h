// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class CodePackageActivationId : public Serialization::FabricSerializable
    {
        public:
        CodePackageActivationId();
        CodePackageActivationId(CodePackageActivationId const & other);
        CodePackageActivationId(CodePackageActivationId && other);
        CodePackageActivationId(std::wstring const & hostId);

        __declspec(property(get=get_HostId)) std::wstring const & HostId;
        inline std::wstring const & get_HostId() const { return hostId_; };

        __declspec(property(get=get_InstanceId)) int64 const & InstanceId;
        inline int64 const & get_InstanceId() const { return instanceId_; };
        
        CodePackageActivationId const & operator = (CodePackageActivationId const & other);
        CodePackageActivationId const & operator = (CodePackageActivationId && other);

        bool operator == (CodePackageActivationId const & other) const;
        bool operator != (CodePackageActivationId const & other) const;
        bool operator < (CodePackageActivationId const & other) const;

        bool IsEmpty() const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
        FABRIC_FIELDS_02(hostId_, instanceId_);

    private:
        std::wstring hostId_;
        int64 instanceId_;
    };
}
