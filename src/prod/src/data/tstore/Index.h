// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace TStore
    {
        class Index
        {
        public:
            Index(__in int partitionindex, __in int itemIndex);
            ~Index();

            __declspec(property(get = get_ItemIndex)) int ItemIndex;
            int get_ItemIndex() const
            {
                return itemIndex_;
            }

            __declspec(property(get = get_PartitionIndex)) int PartitionIndex;
            int get_PartitionIndex() const
            {
                return partitionIndex_;
            }

        private:
            int partitionIndex_;
            int itemIndex_;
        };
    }
}
