// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace SystemServices
{
    class DirectMessagingFailureBody : public Serialization::FabricSerializable
    {
    public:
        DirectMessagingFailureBody() { }

        explicit DirectMessagingFailureBody(Common::ErrorCode const & error) : errorDetails_(error) { }

        __declspec (property(get=get_ErrorDetails)) Common::ErrorCodeDetails & ErrorDetails;
        Common::ErrorCodeDetails & get_ErrorDetails() { return errorDetails_; }

        FABRIC_FIELDS_01(errorDetails_);

    private:
        Common::ErrorCodeDetails errorDetails_;
    };
}
