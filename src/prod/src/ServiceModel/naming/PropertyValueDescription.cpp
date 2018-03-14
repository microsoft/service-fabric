// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace std;
    using namespace Common;

    PropertyValueDescription::PropertyValueDescription()
        : kind_(FABRIC_PROPERTY_TYPE_INVALID)
    {
    }

    PropertyValueDescription::PropertyValueDescription(FABRIC_PROPERTY_TYPE_ID const & kind)
        : kind_(kind)
    {
    }

    PropertyValueDescription::~PropertyValueDescription() {};

    PropertyValueDescriptionSPtr PropertyValueDescription::CreateSPtr(FABRIC_PROPERTY_TYPE_ID kind)
    {
        switch (kind)
        {
            case FABRIC_PROPERTY_TYPE_BINARY: { return PropertyValueTypeActivator<FABRIC_PROPERTY_TYPE_BINARY>::CreateSPtr(); }
            case FABRIC_PROPERTY_TYPE_INT64: { return PropertyValueTypeActivator<FABRIC_PROPERTY_TYPE_INT64>::CreateSPtr(); }
            case FABRIC_PROPERTY_TYPE_DOUBLE: { return PropertyValueTypeActivator<FABRIC_PROPERTY_TYPE_DOUBLE>::CreateSPtr(); }
            case FABRIC_PROPERTY_TYPE_WSTRING: { return PropertyValueTypeActivator<FABRIC_PROPERTY_TYPE_WSTRING>::CreateSPtr(); }
            case FABRIC_PROPERTY_TYPE_GUID: { return PropertyValueTypeActivator<FABRIC_PROPERTY_TYPE_GUID>::CreateSPtr(); }
            default: return std::make_shared<PropertyValueDescription>();
        }
    }

    // The biggest difference between the typed value classes is the way the Value's
    // ByteBuffer is created. Define those functions here for each type.

    ErrorCode BinaryPropertyValueDescription::GetValueBytes(__out ByteBuffer & buffer)
    {
        buffer = ByteBuffer(value_);
        return ErrorCode::Success();
    }

    ErrorCode Int64PropertyValueDescription::GetValueBytes(__out ByteBuffer & buffer)
    {
        BYTE const * ptr = reinterpret_cast<BYTE const *>(&value_);
        buffer = ByteBuffer(ptr, ptr + sizeof(int64));
        return ErrorCode::Success();
    }

    ErrorCode DoublePropertyValueDescription::GetValueBytes(__out ByteBuffer & buffer)
    {
        BYTE const * ptr = reinterpret_cast<BYTE const *>(&value_);
        buffer = ByteBuffer(ptr, ptr + sizeof(double));
        return ErrorCode::Success();
    }

    ErrorCode StringPropertyValueDescription::GetValueBytes(__out ByteBuffer & buffer)
    {
        BYTE const * ptr = reinterpret_cast<BYTE const *>(value_.data());
        buffer = ByteBuffer(ptr, ptr + StringUtility::GetDataLength(value_.data()));
        return ErrorCode::Success();
    }

    ErrorCode GuidPropertyValueDescription::GetValueBytes(__out ByteBuffer & buffer)
    {
        BYTE const * ptr = reinterpret_cast<BYTE const *>(&value_);
        buffer = ByteBuffer(ptr, ptr + sizeof(Guid));
        return ErrorCode::Success();
    }

}
