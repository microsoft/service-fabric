// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class UpdateServiceReplyMessageBody : public Serialization::FabricSerializable
    {
    public:

        UpdateServiceReplyMessageBody()
        {
        }

        UpdateServiceReplyMessageBody(Common::ErrorCodeValue::Enum error)
            : error_(error)
        {
        }

        __declspec (property(get=get_error)) Common::ErrorCodeValue::Enum Error;
        Common::ErrorCodeValue::Enum get_error() const { return error_; }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
        {
            w.WriteLine("Error: {0}", error_);
        }

        FABRIC_FIELDS_01(error_);

    private:

        Common::ErrorCodeValue::Enum error_;
    };
}
