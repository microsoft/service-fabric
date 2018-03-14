// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class CreateSystemServiceMessageBody : public ServiceModel::ClientServerMessageBody
    {
    public:
        CreateSystemServiceMessageBody() : serviceDescription_()
        {
        }

        CreateSystemServiceMessageBody(
            Reliability::ServiceDescription const & serviceDescription)
            : serviceDescription_(serviceDescription)
        {
        }

        __declspec(property(get=get_ServiceDescription)) Reliability::ServiceDescription const & ServiceDescription;
        Reliability::ServiceDescription const & get_ServiceDescription() const { return serviceDescription_; }

        FABRIC_FIELDS_01(serviceDescription_);

    private:
        Reliability::ServiceDescription serviceDescription_;
    };
}
