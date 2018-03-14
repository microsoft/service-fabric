// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class ServiceTypeNotificationReplyMessageBody : public Serialization::FabricSerializable
    {
    public:
        ServiceTypeNotificationReplyMessageBody(std::vector<ServiceTypeInfo> && infos);
        ServiceTypeNotificationReplyMessageBody(std::vector<ServiceTypeInfo> const & infos);
        ServiceTypeNotificationReplyMessageBody();

        ServiceTypeNotificationReplyMessageBody(ServiceTypeNotificationReplyMessageBody const & other);
        ServiceTypeNotificationReplyMessageBody & operator=(ServiceTypeNotificationReplyMessageBody const & other);

        ServiceTypeNotificationReplyMessageBody(ServiceTypeNotificationReplyMessageBody && other);
        ServiceTypeNotificationReplyMessageBody & operator=(ServiceTypeNotificationReplyMessageBody && other);

        __declspec(property(get = get_Infos)) std::vector<ServiceTypeInfo> const & Infos;
        std::vector<ServiceTypeInfo> const & get_Infos() const { return infos_; }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;
        void WriteToEtw(uint16 contextSequenceId) const;

        FABRIC_FIELDS_01(infos_);

    private:
        std::vector<ServiceTypeInfo> infos_;
    };
}
