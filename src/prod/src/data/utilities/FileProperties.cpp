// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data::Utilities;

FileProperties::FileProperties()
    : KObject()
    , KShared()
{
}

FileProperties::~FileProperties()
{
}

void FileProperties::Write(__in BinaryWriter & binaryWriter)
{
    UNREFERENCED_PARAMETER(binaryWriter);

    // Base class has no properties to write
}

void FileProperties::ReadProperty(
    __in BinaryReader & reader, 
    __in KStringView const & property, 
    __in ULONG32 valueSize)
{
    UNREFERENCED_PARAMETER(property);

    reader.Position = reader.get_Position() + static_cast<ULONG>(valueSize);
}
