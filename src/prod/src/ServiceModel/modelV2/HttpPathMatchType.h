//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    namespace ModelV2
    {
        namespace PathMatchType
        {
            enum Enum
            {
                Invalid = 0,
                Prefix = 1
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

            BEGIN_DECLARE_ENUM_JSON_SERIALIZER(Enum)
                ADD_ENUM_VALUE(Invalid)
                ADD_ENUM_VALUE(Prefix)
            END_DECLARE_ENUM_SERIALIZER()
        }
    }
}