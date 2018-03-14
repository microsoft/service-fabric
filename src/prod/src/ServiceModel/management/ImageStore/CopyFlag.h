// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ImageStore
    {
        namespace CopyFlag
        {
            enum Enum
            {
                Invalid = 0,
                Echo = 1, // (folders) - do not delete extra 
                Overwrite = 2, // (files and folders) - delete folder or file and do fresh copy
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

            Enum FromPublicApi(FABRIC_IMAGE_STORE_COPY_FLAG);
            FABRIC_IMAGE_STORE_COPY_FLAG ToPublicApi(Enum const &);

            BEGIN_DECLARE_ENUM_JSON_SERIALIZER(Enum)
                ADD_ENUM_VALUE(Invalid)
                ADD_ENUM_VALUE(Echo)
                ADD_ENUM_VALUE(Overwrite)
            END_DECLARE_ENUM_SERIALIZER()
        }
    }
}
