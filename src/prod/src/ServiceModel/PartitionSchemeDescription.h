// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    namespace PartitionSchemeDescription
    {
        enum Enum
        {
            Invalid = 0,
            Singleton = 1,
            UniformInt64Range = 2,
            Named = 3,
        };

        void WriteToTextWriter(__in Common::TextWriter & w, Enum const & val);

        BEGIN_DECLARE_ENUM_JSON_SERIALIZER(Enum)
            ADD_ENUM_VALUE(Invalid)
            ADD_ENUM_VALUE(Singleton)
            ADD_ENUM_VALUE(UniformInt64Range)
            ADD_ENUM_VALUE(Named)
        END_DECLARE_ENUM_SERIALIZER()
    };
}
