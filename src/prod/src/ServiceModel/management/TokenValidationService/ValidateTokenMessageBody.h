// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace TokenValidationService
    {
        class ValidateTokenMessageBody : public ServiceModel::ClientServerMessageBody
        {
        public:
            ValidateTokenMessageBody() : token_() { }

            ValidateTokenMessageBody(std::wstring const & token) : token_(token) { }

            __declspec(property(get=get_Token)) std::wstring const & Token;

            std::wstring const & get_Token() const { return token_; }

            FABRIC_FIELDS_01(token_);

        private:
            std::wstring token_;
        };
    }
}
