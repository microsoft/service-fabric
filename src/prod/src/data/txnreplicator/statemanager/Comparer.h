// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace StateManager
    {
        class Comparer
        {
        public:
            // #9019461 KUri should support Compare that actually orders.
            // Compare function is used in the StateManager to sort the metadata array.
            static LONG32 Compare(__in Metadata::SPtr const & itemOne, __in Metadata::SPtr const & itemTwo);

            // #9019461 KUri should support Compare that actually orders.
            // Compare function is used in CheckpointManager Binary insertion sort to find the right position to insert.
            static LONG32 Compare(__in Metadata::CSPtr const & itemOne, __in Metadata::CSPtr const & itemTwo);

            // Equals function is used in the MetadataManager KHashSet creation, in this way, different KUri::CSPtrs point to 
            // the same KUriView will consider as the same key.
            static BOOLEAN Equals(__in KUri::CSPtr const & keyOne, __in KUri::CSPtr const & keyTwo);

            static BOOLEAN Comparer::Equals(__in KString::CSPtr const & keyOne, __in KString::CSPtr const & keyTwo);
        };
    }
}
