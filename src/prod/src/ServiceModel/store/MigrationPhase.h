//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

#pragma once

namespace Store
{
    namespace MigrationPhase
    {
        enum Enum
        {
            Inactive = 0,
            Migration = 1,
            TargetDatabaseSwap = 2,
            TargetDatabaseCleanup = 3,
            SourceDatabaseCleanup = 4,
            TargetDatabaseActive = 5,
            RestoreSourceBackup = 6,

            FirstValidEnum = Inactive,
            LastValidEnum = RestoreSourceBackup
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

        FABRIC_KEY_VALUE_STORE_MIGRATION_PHASE ToPublic(Enum const &);
        Enum FromPublic(FABRIC_KEY_VALUE_STORE_MIGRATION_PHASE const &);

        DECLARE_ENUM_STRUCTURED_TRACE( MigrationPhase )

        BEGIN_DECLARE_ENUM_JSON_SERIALIZER(Enum)
            ADD_ENUM_VALUE(Inactive)
            ADD_ENUM_VALUE(Migration)
            ADD_ENUM_VALUE(TargetDatabaseSwap)
            ADD_ENUM_VALUE(TargetDatabaseCleanup)
            ADD_ENUM_VALUE(SourceDatabaseCleanup)
            ADD_ENUM_VALUE(TargetDatabaseActive)
            ADD_ENUM_VALUE(RestoreSourceBackup)
        END_DECLARE_ENUM_SERIALIZER()
    };
}
