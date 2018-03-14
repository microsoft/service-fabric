// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace TokenValidationService
    {
        class TokenValidationMessage : public Serialization::FabricSerializable
        {
        public:

            TokenValidationMessage();
            TokenValidationMessage(
                ServiceModel::ClaimsCollection const & claims,
                Common::TimeSpan const & tokenExpiryTime,
                Common::ErrorCode const & error);

            __declspec(property(get=get_TokenExpiryTime)) Common::TimeSpan const & TokenExpiryTime;
            Common::TimeSpan const & get_TokenExpiryTime() const { return tokenExpiryTime_; }

            __declspec(property(get=get_Error)) Common::ErrorCode const & Error;
            Common::ErrorCode const & get_Error() const { return error_; }

            __declspec(property(get=get_ClaimsResult)) ServiceModel::ClaimsCollection const & ClaimsResult;
            ServiceModel::ClaimsCollection const & get_ClaimsResult() const { return claims_; }

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            FABRIC_FIELDS_03(error_, claims_, tokenExpiryTime_);

        private:
            Common::ErrorCode error_;
            ServiceModel::ClaimsCollection claims_;
            Common::TimeSpan tokenExpiryTime_;
        };
    }
}
