//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

#pragma once

namespace Store
{
    namespace MigrationState
    {
        enum Enum
        {
            Inactive = 0,
            Processing = 1,
            Completed = 2,
            Canceled = 3,
            Failed = 4,

            FirstValidEnum = Inactive,
            LastValidEnum = Failed
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

        FABRIC_KEY_VALUE_STORE_MIGRATION_STATE ToPublic(Enum const &);
        Enum FromPublic(FABRIC_KEY_VALUE_STORE_MIGRATION_STATE const &);

        DECLARE_ENUM_STRUCTURED_TRACE( MigrationState )

        BEGIN_DECLARE_ENUM_JSON_SERIALIZER(Enum)
            ADD_ENUM_VALUE(Inactive)
            ADD_ENUM_VALUE(Processing)
            ADD_ENUM_VALUE(Completed)
            ADD_ENUM_VALUE(Canceled)
            ADD_ENUM_VALUE(Failed)
        END_DECLARE_ENUM_SERIALIZER()
    };
}
