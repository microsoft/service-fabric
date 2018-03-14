// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace Management
{
    namespace InfrastructureService
    {
        class RunCommandReplyMessageBody : public Serialization::FabricSerializable
        {
        public:
            RunCommandReplyMessageBody() : text_() { }

            RunCommandReplyMessageBody(std::wstring && text) : text_(move(text)) { }

            __declspec(property(get=get_Text)) std::wstring & Text;

            std::wstring & get_Text() { return text_; }

            FABRIC_FIELDS_01(text_);

        private:
            std::wstring text_;
        };
    }
}
