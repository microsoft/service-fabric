// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Management
{
    namespace ImageStore
    {
        namespace CopyFlag
        {
            void WriteToTextWriter(Common::TextWriter & w, Enum const & val)
            {
                switch (val)
                {
                case Invalid: w << "Invalid"; return;
                case Echo: w << "Echo"; return;
                case Overwrite: w << "Overwrite"; return;
                }
                
                w << "CopyFlag(" << static_cast<int>(val) << ')';
            }

            Enum FromPublicApi(FABRIC_IMAGE_STORE_COPY_FLAG flag)
            {
                switch (flag)
                {
                case FABRIC_IMAGE_STORE_COPY_FLAG_ATOMIC_SKIP_IF_EXISTS:
                    return Echo;

                case FABRIC_IMAGE_STORE_COPY_FLAG_ATOMIC:
                    return Overwrite;

                case FABRIC_IMAGE_STORE_COPY_FLAG_IF_DIFFERENT:
                    // Invalid: not supported
                    
                default:
                    return Invalid;
                }
            }

            FABRIC_IMAGE_STORE_COPY_FLAG ToPublicApi(Enum const & flag)
            {
                switch (flag)
                {
                case Echo:
                    return FABRIC_IMAGE_STORE_COPY_FLAG_ATOMIC_SKIP_IF_EXISTS;

                case Overwrite:
                    return FABRIC_IMAGE_STORE_COPY_FLAG_ATOMIC;

                default:
                    return FABRIC_IMAGE_STORE_COPY_FLAG_INVALID;
                }
            }
        }
    }
}
