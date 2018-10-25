//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

using namespace std;

namespace Store
{
    namespace MigrationPhase
    {
        void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
        {
            switch (e)
            {
            case Inactive: w << "Inactive"; return;
            case Migration: w << "Migration"; return;
            case TargetDatabaseSwap: w << "TargetDatabaseSwap"; return;
            case TargetDatabaseCleanup: w << "TargetDatabaseCleanup"; return;
            case SourceDatabaseCleanup: w << "SourceDatabaseCleanup"; return;
            case TargetDatabaseActive: w << "TargetDatabaseActive"; return;
            case RestoreSourceBackup: w << "RestoreSourceBackup"; return;
            }
            w << "MigrationPhase(" << static_cast<uint>(e) << ')';
        }

        FABRIC_KEY_VALUE_STORE_MIGRATION_PHASE ToPublic(Enum const & e)
        {
            switch (e)
            {
            case Migration: return FABRIC_KEY_VALUE_STORE_MIGRATION_PHASE_MIGRATION;
            case TargetDatabaseSwap: return FABRIC_KEY_VALUE_STORE_MIGRATION_PHASE_TARGET_DATABASE_SWAP;
            case TargetDatabaseCleanup: return FABRIC_KEY_VALUE_STORE_MIGRATION_PHASE_TARGET_DATABASE_CLEANUP;
            case SourceDatabaseCleanup: return FABRIC_KEY_VALUE_STORE_MIGRATION_PHASE_SOURCE_DATABASE_CLEANUP;
            case TargetDatabaseActive: return FABRIC_KEY_VALUE_STORE_MIGRATION_PHASE_TARGET_DATABASE_ACTIVE;
            case RestoreSourceBackup: return FABRIC_KEY_VALUE_STORE_MIGRATION_PHASE_RESTORE_SOURCE_BACKUP;
            }
            return FABRIC_KEY_VALUE_STORE_MIGRATION_PHASE_INACTIVE;
        }

        Enum FromPublic(FABRIC_KEY_VALUE_STORE_MIGRATION_PHASE const & e)
        {
            switch (e)
            {
            case FABRIC_KEY_VALUE_STORE_MIGRATION_PHASE_MIGRATION: return Migration;
            case FABRIC_KEY_VALUE_STORE_MIGRATION_PHASE_TARGET_DATABASE_SWAP: return TargetDatabaseSwap;
            case FABRIC_KEY_VALUE_STORE_MIGRATION_PHASE_TARGET_DATABASE_CLEANUP: return TargetDatabaseCleanup;
            case FABRIC_KEY_VALUE_STORE_MIGRATION_PHASE_SOURCE_DATABASE_CLEANUP: return SourceDatabaseCleanup;
            case FABRIC_KEY_VALUE_STORE_MIGRATION_PHASE_TARGET_DATABASE_ACTIVE: return TargetDatabaseActive;
            case FABRIC_KEY_VALUE_STORE_MIGRATION_PHASE_RESTORE_SOURCE_BACKUP: return RestoreSourceBackup;
            }
            return Inactive;
        }

        ENUM_STRUCTURED_TRACE( MigrationPhase, FirstValidEnum, LastValidEnum )
    }
}
