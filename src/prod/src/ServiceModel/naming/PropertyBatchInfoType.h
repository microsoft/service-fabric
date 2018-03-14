// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    namespace PropertyBatchInfoType
    {
        enum Enum
        {
            Invalid = 0,
            Successful = 1,
            Failed = 2,
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

        BEGIN_DECLARE_ENUM_JSON_SERIALIZER(Enum)
            ADD_ENUM_VALUE(Invalid)
            ADD_ENUM_VALUE(Successful)
            ADD_ENUM_VALUE(Failed)
        END_DECLARE_ENUM_SERIALIZER()
    }
}
