// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class CodePackageEventNotificationReply : public Serialization::FabricSerializable
    {
    public:
        CodePackageEventNotificationReply();
        CodePackageEventNotificationReply(Common::ErrorCode const & errorCode);

        __declspec(property(get=get_Error)) Common::ErrorCode const & Error;
        Common::ErrorCode const & get_Error() const { return errorCode_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_01(errorCode_);

    private:
        Common::ErrorCode errorCode_;
    };
}
