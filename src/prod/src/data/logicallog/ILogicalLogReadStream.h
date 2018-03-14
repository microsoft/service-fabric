// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Log
    {
        interface ILogicalLogReadStream 
            : public ktl::io::KStream
            , public Utilities::IDisposable
        {
            K_SHARED_INTERFACE(ILogicalLogReadStream);

        public:

            virtual LONGLONG GetLength() const = 0;
        };
    }
}
