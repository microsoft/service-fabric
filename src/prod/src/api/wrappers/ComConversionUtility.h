// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class ComConversionUtility
    {
    public:
        template <typename ServiceModelType, typename PublicType, typename PublicListType>
        static void ToPublicList(
            __in Common::ScopedHeap &heap,
            __in std::vector<ServiceModelType> const &list,
            __out PublicListType &publicList)
        {
            auto itemCount = list.size();
            publicList.Count = static_cast<ULONG>(itemCount);

            if (itemCount > 0)
            {
                auto items = heap.AddArray<PublicType>(itemCount);
                for(size_t i =0; i < items.GetCount(); ++i)
                {
                    list[i].ToPublicApi(heap, items[i]);
                }

                publicList.Items = items.GetRawArray();
            }
            else
            {
                publicList.Items = NULL;
            }
        }
    };
}
