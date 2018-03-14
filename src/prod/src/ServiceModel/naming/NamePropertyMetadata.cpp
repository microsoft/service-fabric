// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

namespace Naming
{
    INITIALIZE_BASE_DYNAMIC_SIZE_ESTIMATION(NamePropertyMetadata)

    NamePropertyMetadata::NamePropertyMetadata()
        : propertyName_()
        , typeId_(::FABRIC_PROPERTY_TYPE_INVALID)
        , sizeInBytes_(0)
        , customTypeId_()
    {
    }

    NamePropertyMetadata::NamePropertyMetadata(NamePropertyMetadata const & other)
        : propertyName_(other.propertyName_)
        , typeId_(other.typeId_)
        , sizeInBytes_(other.sizeInBytes_)
        , customTypeId_(other.customTypeId_)
    {
    }

    NamePropertyMetadata & NamePropertyMetadata::operator=(NamePropertyMetadata const & other)
    {
        if (this != &other)
        {
            propertyName_ = other.propertyName_;
            typeId_ = other.typeId_;
            sizeInBytes_ = other.sizeInBytes_;
            customTypeId_ = other.customTypeId_;
        }

        return *this;
    }

    NamePropertyMetadata::NamePropertyMetadata(NamePropertyMetadata && other)
        : propertyName_(std::move(other.propertyName_))
        , typeId_(std::move(other.typeId_))
        , sizeInBytes_(std::move(other.sizeInBytes_))
        , customTypeId_(std::move(other.customTypeId_))
    {
    }

    NamePropertyMetadata & NamePropertyMetadata::operator=(NamePropertyMetadata && other)
    {
        if (this != &other)
        {
            propertyName_ = std::move(other.propertyName_);
            typeId_ = std::move(other.typeId_);
            sizeInBytes_ = std::move(other.sizeInBytes_);
            customTypeId_ = std::move(other.customTypeId_);
        }

        return *this;
    }

    NamePropertyMetadata::NamePropertyMetadata(
        std::wstring const & propertyName,
        ::FABRIC_PROPERTY_TYPE_ID typeId,
        ULONG sizeInBytes,
        std::wstring const & customTypeId)
        : propertyName_(propertyName)
        , typeId_(typeId)
        , sizeInBytes_(sizeInBytes)
        , customTypeId_(customTypeId)
    {
    }
}
