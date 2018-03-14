// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once


namespace Data
{
    namespace StateManager
    {
        namespace MetadataMode
        {
            /// <summary>
            /// State of metadata that indicates if the state provider is active or deleted.
            /// </summary>
            enum Enum
            {
                /// <summary>
                /// Metadata that corresponds to a state provider that is active.
                /// </summary>
                Active = 0,

                /// <summary>
                /// Metadata that corresponds to a state provider that has been soft-deleted.
                /// </summary>
                DelayDelete = 1,

                /// <summary>
                /// Metadata that corresponds to a state provider that needs to be cleaned up as a part of copy.
                /// </summary>
                FalseProgress = 2,

                LastValidEnum = FalseProgress
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

            char ToChar(Enum const & val);

            DECLARE_ENUM_STRUCTURED_TRACE(MetadataMode);
        }
    }
}
