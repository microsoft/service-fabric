// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "FabricSerializationTypes.h"

namespace Serialization
{
    namespace FabricSerializationTypes
    {
        bool IsArray(FabricSerializationTypes::Enum value)
        {
            return ((value & FabricSerializationTypes::Array) > 0);
        }

        bool IsEmpty(FabricSerializationTypes::Enum value)
        {
            return ((value & FabricSerializationTypes::EmptyValueBit) > 0);
        }

        bool IsOfBaseType(FabricSerializationTypes::Enum value, FabricSerializationTypes::Enum baseValue)
        {
            return ((value & FabricSerializationTypes::BaseTypeMask) == baseValue);
        }

        FabricSerializationTypes::Enum MakeArray(FabricSerializationTypes::Enum value)
        {
            return static_cast<FabricSerializationTypes::Enum>(value | FabricSerializationTypes::Array);
        }

        FabricSerializationTypes::Enum MakeEmpty(FabricSerializationTypes::Enum value)
        {
            return static_cast<FabricSerializationTypes::Enum>(value | FabricSerializationTypes::EmptyValueBit);
        }
    }
}
