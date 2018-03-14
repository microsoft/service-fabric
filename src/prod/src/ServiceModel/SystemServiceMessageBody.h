// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace ServiceModel
{
    class SystemServiceMessageBody : public ClientServerMessageBody
    {
    public:
        SystemServiceMessageBody() : content_() { }

        SystemServiceMessageBody(std::wstring const & content) : content_(content) { }

        __declspec(property(get = get_Content)) std::wstring const & Content;

        std::wstring const & get_Content() const { return content_; }

        FABRIC_FIELDS_01(content_);

    private:
        std::wstring content_;
    };
}
