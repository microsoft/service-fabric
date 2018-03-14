// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace std;
    using namespace Common;

    PropertyInfo::PropertyInfo()
        : value_()
        , metadata_()
        , name_()
        , includeValues_(false)
    {
    }

    PropertyInfo::PropertyInfo(NamePropertyMetadataResult && metadata)
        : value_()
        , metadata_(std::move(metadata))
        , name_(metadata.PropertyName)
        , includeValues_(false)
    {
    }

    PropertyInfo::PropertyInfo(
        PropertyValueDescriptionSPtr value,
        NamePropertyMetadataResult && metadata,
        bool includeValues)
        : value_(value)
        , metadata_(std::move(metadata))
        , name_(metadata.PropertyName)
        , includeValues_(includeValues)
    {
    }

    PropertyInfo::PropertyInfo(
        PropertyInfo && other)
        : value_(std::move(other.value_))
        , metadata_(std::move(other.metadata_))
        , name_(std::move(other.name_))
        , includeValues_(std::move(other.includeValues_))
    {
    }

    PropertyInfo::~PropertyInfo()
    {
    }

    // includeValues necessary for case where Value is empty ByteBuffer and should be included
    PropertyInfo PropertyInfo::Create(NamePropertyResult && nameProperty, bool includeValues)
    {
        PropertyValueDescriptionSPtr propVal;

        if (!includeValues)
        {
            return PropertyInfo(nameProperty.TakeMetadata());
        }

        ByteBuffer valueBytes = nameProperty.TakeBytes();
        switch (nameProperty.Metadata.TypeId)
        {
            case FABRIC_PROPERTY_TYPE_BINARY:
            {
                propVal = std::make_shared<BinaryPropertyValueDescription>(std::move(valueBytes));
                break;
            }
            case FABRIC_PROPERTY_TYPE_INT64:
            {
                int64* value = reinterpret_cast<int64*>(valueBytes.data());
                propVal = std::make_shared<Int64PropertyValueDescription>(std::move(*value));
                break;
            }
            case FABRIC_PROPERTY_TYPE_DOUBLE:
            {
                double* value = reinterpret_cast<double*>(valueBytes.data());
                propVal = std::make_shared<DoublePropertyValueDescription>(std::move(*value));
                break;
            }
            case FABRIC_PROPERTY_TYPE_WSTRING:
            {
                std::wstring value(reinterpret_cast<WCHAR const *>(valueBytes.data()));
                propVal = std::make_shared<StringPropertyValueDescription>(std::move(value));
                break;
            }
            case FABRIC_PROPERTY_TYPE_GUID:
            {
                Guid value = Guid(valueBytes);
                propVal = std::make_shared<GuidPropertyValueDescription>(std::move(value));
                break;
            }
            default:
            {
                propVal = std::make_shared<PropertyValueDescription>();
                break;
            }
        }
        return PropertyInfo(propVal, nameProperty.TakeMetadata(), includeValues);
    }
}
