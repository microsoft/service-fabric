// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ServiceModel;

ServiceGroupMemberDescriptionAdaptor::ServiceGroupMemberDescriptionAdaptor(const ServiceGroupMemberDescriptionAdaptor &obj) :
    serviceName_(obj.serviceName_),
    serviceTypeName_(obj.serviceTypeName_),
    initializationData_(obj.initializationData_),
    metrics_(obj.metrics_.begin(),obj.metrics_.end())
{
};

ServiceGroupMemberDescriptionAdaptor& ServiceGroupMemberDescriptionAdaptor::operator=(const ServiceGroupMemberDescriptionAdaptor& obj)
{
    if (this != &obj)
    {
        serviceName_ = obj.serviceName_;
        serviceTypeName_ = obj.serviceTypeName_;
        initializationData_ = obj.initializationData_;
        metrics_ = obj.metrics_;
    }
    return *this;
};
