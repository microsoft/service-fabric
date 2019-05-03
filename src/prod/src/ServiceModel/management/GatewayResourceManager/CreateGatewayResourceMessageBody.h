//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace Management
{
    namespace GatewayResourceManager
    {
        class CreateGatewayResourceMessageBody : public ServiceModel::ClientServerMessageBody
        {
        public:

            CreateGatewayResourceMessageBody() = default;

            CreateGatewayResourceMessageBody(std::wstring && descriptionStr)
                : description_(move(descriptionStr))
            {
            }

            FABRIC_FIELDS_01(description_);

            //std::wstring const & get_GatewayName() const { return description_.Name; }
            //__declspec(property(get=get_GatewayName)) std::wstring const & GatewayName;

            std::wstring const & get_GatewayDescription() const { return description_; }
            __declspec(property(get = get_GatewayDescription)) std::wstring const & GatewayDescription;

            std::wstring  && TakeDescription() { return std::move(description_); }

        private:
            std::wstring description_;
        };
    }
}
