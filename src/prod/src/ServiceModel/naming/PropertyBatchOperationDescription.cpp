// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace std;
    using namespace Common;

    PropertyBatchOperationDescription::PropertyBatchOperationDescription()
        : kind_(FABRIC_PROPERTY_BATCH_OPERATION_KIND_INVALID)
        , propertyName_()
    {
    }

    PropertyBatchOperationDescription::PropertyBatchOperationDescription(FABRIC_PROPERTY_BATCH_OPERATION_KIND kind)
        : kind_(kind)
        , propertyName_()
    {
    }

    PropertyBatchOperationDescription::PropertyBatchOperationDescription(
        FABRIC_PROPERTY_BATCH_OPERATION_KIND kind,
        std::wstring const & propertyName)
        : kind_(kind)
        , propertyName_(propertyName)
    {
    }

    PropertyBatchOperationDescription::~PropertyBatchOperationDescription()
    {
    }

    ErrorCode PropertyBatchOperationDescription::Verify()
    {
        // delete is the only operation that should be base type
        if (Kind == FABRIC_PROPERTY_BATCH_OPERATION_KIND_DELETE)
        {
            return ErrorCode::Success();
        }
        return ErrorCodeValue::InvalidArgument;
    }

    PropertyBatchOperationDescriptionSPtr PropertyBatchOperationDescription::CreateSPtr(FABRIC_PROPERTY_BATCH_OPERATION_KIND kind)
    {
        switch (kind)
        {
            case FABRIC_PROPERTY_BATCH_OPERATION_KIND_PUT: { return PropertyBatchOperationTypeActivator<FABRIC_PROPERTY_BATCH_OPERATION_KIND_PUT>::CreateSPtr(); }
            case FABRIC_PROPERTY_BATCH_OPERATION_KIND_GET: { return PropertyBatchOperationTypeActivator<FABRIC_PROPERTY_BATCH_OPERATION_KIND_GET>::CreateSPtr(); }
            case FABRIC_PROPERTY_BATCH_OPERATION_KIND_CHECK_EXISTS: { return PropertyBatchOperationTypeActivator<FABRIC_PROPERTY_BATCH_OPERATION_KIND_CHECK_EXISTS>::CreateSPtr(); }
            case FABRIC_PROPERTY_BATCH_OPERATION_KIND_CHECK_SEQUENCE: { return PropertyBatchOperationTypeActivator<FABRIC_PROPERTY_BATCH_OPERATION_KIND_CHECK_SEQUENCE>::CreateSPtr(); }
            // delete has no additional properties, so it can be base class with correct kind value
            case FABRIC_PROPERTY_BATCH_OPERATION_KIND_DELETE: { return std::make_shared<PropertyBatchOperationDescription>(FABRIC_PROPERTY_BATCH_OPERATION_KIND_DELETE); }
            case FABRIC_PROPERTY_BATCH_OPERATION_KIND_CHECK_VALUE: { return PropertyBatchOperationTypeActivator<FABRIC_PROPERTY_BATCH_OPERATION_KIND_CHECK_VALUE>::CreateSPtr(); }
            default: return std::make_shared<PropertyBatchOperationDescription>();
        }
    }
}
