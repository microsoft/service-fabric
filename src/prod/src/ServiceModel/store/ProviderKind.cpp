//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

using namespace std;

namespace Store
{
    namespace ProviderKind
    {
        void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
        {
            switch (e)
            {
            case Unknown: w << "Unknown"; return;
            case ESE: w << "ESE"; return;
            case TStore: w << "TStore"; return;
            }
            w << "ProviderKind(" << static_cast<uint>(e) << ')';
        }

        FABRIC_KEY_VALUE_STORE_PROVIDER_KIND ToPublic(Enum const & e)
        {
            switch (e)
            {
            case ESE: return FABRIC_KEY_VALUE_STORE_PROVIDER_KIND_ESE;
            case TStore: return FABRIC_KEY_VALUE_STORE_PROVIDER_KIND_TSTORE;
            }
            return FABRIC_KEY_VALUE_STORE_PROVIDER_KIND_UNKNOWN;
        }

        Enum FromPublic(FABRIC_KEY_VALUE_STORE_PROVIDER_KIND const & e)
        {
            switch (e)
            {
            case FABRIC_KEY_VALUE_STORE_PROVIDER_KIND_ESE: return ESE;
            case FABRIC_KEY_VALUE_STORE_PROVIDER_KIND_TSTORE: return TStore;
            }
            return Unknown;
        }

        ENUM_STRUCTURED_TRACE( ProviderKind, FirstValidEnum, LastValidEnum )
    }
}
