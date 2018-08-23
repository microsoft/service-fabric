// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class CodePackageEventNotificationRequest : public Serialization::FabricSerializable
    {
    public:
        CodePackageEventNotificationRequest();
        CodePackageEventNotificationRequest(
            CodePackageEventDescription const & eventDesc);

        __declspec(property(get=get_CodePackageEvent)) CodePackageEventDescription const & CodePackageEvent;
        inline CodePackageEventDescription const & get_CodePackageEvent() const { return codePackageEvent_; };

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_01(codePackageEvent_);

    private:
        CodePackageEventDescription codePackageEvent_;
    };
}
