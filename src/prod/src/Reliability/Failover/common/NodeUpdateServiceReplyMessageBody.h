// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class NodeUpdateServiceReplyMessageBody : public Serialization::FabricSerializable
    {
    public:

        NodeUpdateServiceReplyMessageBody()
        {
        }

        NodeUpdateServiceReplyMessageBody(std::wstring const& serviceName, uint64 serviceInstance, uint64 serviceUpdateVersion)
            : serviceName_(serviceName),
              serviceInstance_(serviceInstance),
              serviceUpdateVersion_(serviceUpdateVersion)
        {
        }

        __declspec (property(get=get_ServiceName)) std::wstring const& ServiceName;
        std::wstring const& get_ServiceName() const { return serviceName_; }

        __declspec (property(get=get_ServiceInstance)) uint64 ServiceInstance;
        uint64 get_ServiceInstance() const { return serviceInstance_; }

        __declspec (property(get=get_ServiceUpdateVersion)) uint64 ServiceUpdateVersion;
        uint64 get_ServiceUpdateVersion() const { return serviceUpdateVersion_; }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
        {
            w.WriteLine("{0} {1}:{2}", serviceName_, serviceInstance_, serviceUpdateVersion_);
        }

        void WriteToEtw(uint16 contextSequenceId) const;

        FABRIC_FIELDS_03(serviceName_, serviceInstance_, serviceUpdateVersion_);

    private:

        std::wstring serviceName_;
        uint64 serviceInstance_;
        uint64 serviceUpdateVersion_;
    };
}
