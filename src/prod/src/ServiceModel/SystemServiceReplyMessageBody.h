// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace ServiceModel
{
    class SystemServiceReplyMessageBody : public Serialization::FabricSerializable
    {
    public:
        SystemServiceReplyMessageBody() : content_() { }

        SystemServiceReplyMessageBody(std::wstring && content) : content_(std::move(content)) { }

        __declspec(property(get = get_Content)) std::wstring & Content;

        std::wstring & get_Content() { return content_; }

        FABRIC_FIELDS_01(content_);

    private:
        std::wstring content_;
    };
}
