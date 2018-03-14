// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    namespace PartitionKind
    {
        enum Enum
        {
            Invalid = 0,
            Singleton = 1,
            UniformInt64 = 2,
            Named = 3
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const& value);

        Enum FromPublicApi(FABRIC_SERVICE_PARTITION_KIND const publicKind);

        BEGIN_DECLARE_ENUM_JSON_SERIALIZER(Enum)
            ADD_ENUM_VALUE(Invalid)
            ADD_ENUM_VALUE(Singleton)
            ADD_ENUM_VALUE(UniformInt64)
            ADD_ENUM_VALUE(Named)
        END_DECLARE_ENUM_SERIALIZER()
    }
}
