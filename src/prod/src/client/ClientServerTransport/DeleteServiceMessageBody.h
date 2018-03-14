// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class DeleteServiceMessageBody : public ServiceModel::ClientServerMessageBody
    {
    public:
        DeleteServiceMessageBody() 
            : name_(), 
            isForce_(false)
        { 
        }

        DeleteServiceMessageBody(Common::NamingUri const & name)
            : name_(name),
            isForce_(false)
        {
        }

        DeleteServiceMessageBody(
            Common::NamingUri const & name,
            bool isForce)
            : name_(name),
            isForce_(isForce)
        {
        }

        DeleteServiceMessageBody(
            ServiceModel::DeleteServiceDescription const & description)
            : name_(description.ServiceName),
            isForce_(description.IsForce)
        {
        }

        __declspec(property(get=get_NamingUri)) Common::NamingUri const & NamingUri;
        Common::NamingUri const & get_NamingUri() const { return name_; }
 
        __declspec(property(get=get_IsForce, put=put_IsForce)) bool IsForce;
        bool get_IsForce() const { return isForce_; }
        void put_IsForce(bool const value) { isForce_ = value; }

        FABRIC_FIELDS_02(name_, isForce_);

    private:
        Common::NamingUri name_;
        bool isForce_;
    };
}
