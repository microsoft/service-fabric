// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class UpdateSystemServiceMessageBody : public Serialization::FabricSerializable
    {
        DEFAULT_COPY_ASSIGNMENT(UpdateSystemServiceMessageBody)
    
    public:
        UpdateSystemServiceMessageBody()
        {
        }

        UpdateSystemServiceMessageBody(std::wstring const& serviceName, Naming::ServiceUpdateDescription const& serviceUpdateDescription)
            : serviceName_(serviceName),
              serviceUpdateDescription_(serviceUpdateDescription)
        {
        }

        __declspec (property(get = get_ServiceName)) std::wstring const& ServiceName;
        std::wstring const& get_ServiceName() const { return serviceName_; }

        __declspec (property(get=get_ServiceUpdateDescription)) Naming::ServiceUpdateDescription const& ServiceUpdateDescription;
        Naming::ServiceUpdateDescription const& get_ServiceUpdateDescription() const { return serviceUpdateDescription_; }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
        {
            w.WriteLine("{0}: {1}", serviceName_, serviceUpdateDescription_);
        }

        FABRIC_FIELDS_02(serviceName_, serviceUpdateDescription_);

    private:

        std::wstring serviceName_;
        Naming::ServiceUpdateDescription serviceUpdateDescription_;
    };
}
