// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Common;
using namespace Data::Utilities;
using namespace Data::TStore;


FilePropertySection::FilePropertySection()
{
}

FilePropertySection::~FilePropertySection()
{
}

void FilePropertySection::Write(__in BinaryWriter&)
{
    // Base class has no properties to write.
}

void FilePropertySection::ReadProperty(
    __in BinaryReader& reader,
    __in ULONG32 property,
    __in ULONG32 valueSize)
{
    UNREFERENCED_PARAMETER(property);

    // Unknown property - skip over the value.
    reader.Position += valueSize;
}
