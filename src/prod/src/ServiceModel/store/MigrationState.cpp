//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

using namespace std;

namespace Store
{
    namespace MigrationState
    {
        void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
        {
            switch (e)
            {
            case Inactive: w << "Inactive"; return;
            case Processing: w << "Processing"; return;
            case Completed: w << "Completed"; return;
            case Canceled: w << "Canceled"; return;
            case Failed: w << "Failed"; return;
            }
            w << "MigrationState(" << static_cast<uint>(e) << ')';
        }

        FABRIC_KEY_VALUE_STORE_MIGRATION_STATE ToPublic(Enum const & e)
        {
            switch (e)
            {
            case Processing: return FABRIC_KEY_VALUE_STORE_MIGRATION_STATE_PROCESSING;
            case Completed: return FABRIC_KEY_VALUE_STORE_MIGRATION_STATE_COMPLETED;
            case Canceled: return FABRIC_KEY_VALUE_STORE_MIGRATION_STATE_CANCELED;
            case Failed: return FABRIC_KEY_VALUE_STORE_MIGRATION_STATE_FAILED;
            }
            return FABRIC_KEY_VALUE_STORE_MIGRATION_STATE_INACTIVE;
        }

        Enum FromPublic(FABRIC_KEY_VALUE_STORE_MIGRATION_STATE const & e)
        {
            switch (e)
            {
            case FABRIC_KEY_VALUE_STORE_MIGRATION_STATE_PROCESSING: return Processing;
            case FABRIC_KEY_VALUE_STORE_MIGRATION_STATE_COMPLETED: return Completed;
            case FABRIC_KEY_VALUE_STORE_MIGRATION_STATE_CANCELED: return Canceled;
            case FABRIC_KEY_VALUE_STORE_MIGRATION_STATE_FAILED: return Failed;
            }
            return Inactive;
        }

        ENUM_STRUCTURED_TRACE( MigrationState, FirstValidEnum, LastValidEnum )
    }
}
