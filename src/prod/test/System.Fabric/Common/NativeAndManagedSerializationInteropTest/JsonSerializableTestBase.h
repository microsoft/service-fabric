// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"

namespace NativeAndManagedSerializationInteropTest
{
    class JsonSerializableTestBase : public Common::IFabricJsonSerializable
    {
        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
        END_JSON_SERIALIZABLE_PROPERTIES()
    };
}
