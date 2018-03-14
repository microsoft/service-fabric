// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class ServiceTypeNotificationRequestMessageBody : public Serialization::FabricSerializable
    {
    public:
        ServiceTypeNotificationRequestMessageBody(std::vector<ServiceTypeInfo> && infos);
        ServiceTypeNotificationRequestMessageBody(std::vector<ServiceTypeInfo> const & infos);
        ServiceTypeNotificationRequestMessageBody();

        ServiceTypeNotificationRequestMessageBody(ServiceTypeNotificationRequestMessageBody const & other);
        ServiceTypeNotificationRequestMessageBody & operator=(ServiceTypeNotificationRequestMessageBody const & other);

        ServiceTypeNotificationRequestMessageBody(ServiceTypeNotificationRequestMessageBody && other);
        ServiceTypeNotificationRequestMessageBody & operator=(ServiceTypeNotificationRequestMessageBody && other);

        __declspec(property(get = get_Infos)) std::vector<ServiceTypeInfo> const & Infos;
        std::vector<ServiceTypeInfo> const & get_Infos() const { return infos_; }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;
        void WriteToEtw(uint16 contextSequenceId) const;

        FABRIC_FIELDS_01(infos_);

    private:
        std::vector<ServiceTypeInfo> infos_;
    };
}
