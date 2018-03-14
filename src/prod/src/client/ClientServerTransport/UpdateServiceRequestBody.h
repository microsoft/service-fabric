// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class UpdateServiceRequestBody : public ServiceModel::ClientServerMessageBody
    {
    public:
        UpdateServiceRequestBody()
            : name_() 
            , updateDescription_()
        {
        }

        UpdateServiceRequestBody(
            Common::NamingUri const & name,
            ServiceUpdateDescription const & updateDescription)
            : name_(name) 
            , updateDescription_(updateDescription)
        {
        }

        __declspec(property(get=get_NamingUri)) Common::NamingUri const & ServiceName;
        __declspec(property(get=get_ServiceUpdateDescription)) ServiceUpdateDescription const & UpdateDescription;

        Common::NamingUri const & get_NamingUri() const { return name_; }
        ServiceUpdateDescription const & get_ServiceUpdateDescription() const { return updateDescription_; }

        FABRIC_FIELDS_02(name_, updateDescription_);

    private:
        Common::NamingUri name_;
        ServiceUpdateDescription updateDescription_;
    };
}

