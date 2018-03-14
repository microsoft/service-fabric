// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    namespace LinuxPackageManagerType
    {
        enum Enum : int
        {
            Unknown = 0,
            Deb = 1,
            Rpm = 2,

            LastValidEnum = Rpm
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

        DECLARE_ENUM_STRUCTURED_TRACE(LinuxPackageManagerType);
    }
}
