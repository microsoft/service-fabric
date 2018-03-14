// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

namespace Common
{
    namespace LinuxPackageManagerType
    {
        void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
        {
            switch (e)
            {
            case Unknown:
                w << "UNKNOWN";
                break;

            case Deb:
                w << "DEB";
                break;

            case Rpm:
                w << "RPM";
                break;

            default:
                w << "Invalid" << static_cast<int>(e);
                break;
            };
        }

        ENUM_STRUCTURED_TRACE(LinuxPackageManagerType, Unknown, LastValidEnum);
    }
}
