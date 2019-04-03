//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    namespace ModelV2
    {
        namespace HeaderMatchType
        {
            enum Enum
            {
                Invalid = 0,
                Exact = 1
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

            BEGIN_DECLARE_ENUM_JSON_SERIALIZER(Enum)
                ADD_ENUM_VALUE(Invalid)
                ADD_ENUM_VALUE(Exact)
            END_DECLARE_ENUM_SERIALIZER()
        }
    }
}