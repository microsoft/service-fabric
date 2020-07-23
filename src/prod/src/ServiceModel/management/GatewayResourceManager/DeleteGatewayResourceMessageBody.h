//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace Management
{
    namespace GatewayResourceManager
    {
        class DeleteGatewayResourceMessageBody : public ServiceModel::ClientServerMessageBody
        {
        public:

            DeleteGatewayResourceMessageBody() = default;

            DeleteGatewayResourceMessageBody(std::wstring const& name)
                : name_(name)
            {
            }

            FABRIC_FIELDS_01(name_);

            __declspec(property(get = get_Name)) std::wstring const & Name;
            std::wstring const & get_Name() const { return name_; }

        private:
            std::wstring name_;
        };
    }
}
