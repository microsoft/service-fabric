// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace AccessControl
{
    namespace ResourceType 
    {
        bool IsValid(Enum value)
        {
            return
                (value == PathInImageStore) ||
                (value == ApplicationType) ||
                (value == Application) ||
                (value == Service);
        }

        void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
        {
            switch (e)
            {
            case Invalid: w << L"Invalid"; return;
            case PathInImageStore: w << L"PathInImageStore"; return;
            case ApplicationType: w << L"ApplicationType"; return;
            case Application: w << L"Application"; return;
            case Service: w << L"Serivce"; return;
            }

            w << "UnknownResourceType(" << static_cast<int>(e) << ')';
        }

        ENUM_STRUCTURED_TRACE(ResourceType, Invalid, Service);
    }
}
