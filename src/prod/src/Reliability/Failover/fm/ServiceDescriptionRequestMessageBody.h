// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class ServiceDescriptionRequestMessageBody : public Serialization::FabricSerializable
    {
    public:

        ServiceDescriptionRequestMessageBody()
            : serviceName_()
            , includeCuids_(false)
        {
        }

        ServiceDescriptionRequestMessageBody(std::wstring const& serviceName)
            : serviceName_(serviceName)
            , includeCuids_(false)
        {
        }

        ServiceDescriptionRequestMessageBody(
            std::wstring const& serviceName,
            bool includeCuids)
            : serviceName_(serviceName)
            , includeCuids_(includeCuids)
        {
        }

        __declspec (property(get = get_ServiceName)) std::wstring const& ServiceName;
        __declspec (property(get = get_IncludeCuids)) bool IncludeCuids;

        std::wstring const& get_ServiceName() const { return serviceName_; }
        bool get_IncludeCuids() const { return includeCuids_; }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
        {
            w.WriteLine("{0} (includeCuids={1})", serviceName_, includeCuids_);
        }

        FABRIC_FIELDS_02(serviceName_, includeCuids_);

    private:

        std::wstring serviceName_;
        bool includeCuids_;
    };
}
