//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

#pragma once

namespace Store
{
    namespace ProviderKind
    {
        enum Enum
        {
            Unknown = 0,
            ESE = 1,
            TStore = 2,

            FirstValidEnum = Unknown,
            LastValidEnum = TStore
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

        FABRIC_KEY_VALUE_STORE_PROVIDER_KIND ToPublic(Enum const &);
        Enum FromPublic(FABRIC_KEY_VALUE_STORE_PROVIDER_KIND const &);

        DECLARE_ENUM_STRUCTURED_TRACE( ProviderKind )

        BEGIN_DECLARE_ENUM_JSON_SERIALIZER(Enum)
            ADD_ENUM_VALUE(Unknown)
            ADD_ENUM_VALUE(ESE)
            ADD_ENUM_VALUE(TStore)
        END_DECLARE_ENUM_SERIALIZER()
    };
}
