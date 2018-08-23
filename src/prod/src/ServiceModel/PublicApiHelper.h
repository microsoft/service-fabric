// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class PublicApiHelper
    {
    public:
        template<class TServiceModel, class TPublicApi, class TPublicApiList>
        static Common::ErrorCode ToPublicApiList(
            __in Common::ScopedHeap & heap, 
            std::vector<TServiceModel> const & list,
            __inout TPublicApiList & publicList)
        {
            ULONG count = static_cast<ULONG>(list.size());
            auto publicArray = heap.AddArray<TPublicApi>(count);
            for (size_t i = 0; i < list.size(); ++i)
            {
                auto error = list[i].ToPublicApi(heap, publicArray[i]);
                if (!error.IsSuccess())
                {
                    return error;
                }
            }

            publicList.Count = count;
            publicList.Items = publicArray.GetRawArray();
            return Common::ErrorCode::Success();
        }

        template<class TServiceModel, class TPublicApiList>
        static Common::ErrorCode FromPublicApiList(
            __in TPublicApiList const * publicList,
            __inout std::vector<TServiceModel> & list)
        {
            if (publicList == NULL)
            {
                return Common::ErrorCode::Success();
            }

            auto publicItems = publicList->Items;
            ULONG listCount = publicList->Count;
            list.clear();
            list.reserve(listCount);
            for (ULONG i = 0; i < listCount; ++i)
            {
                TServiceModel listEntry;
                auto error = listEntry.FromPublicApi(publicItems[i]);
                if (!error.IsSuccess())
                {
                    return error;
                }

                list.push_back(move(listEntry));
            }

            return Common::ErrorCode::Success();
        }

        template<class TStringMap>
        static Common::ErrorCode ToPublicApiStringPairList(
            __in Common::ScopedHeap & heap,
            __in TStringMap const& stringMap,
            __out FABRIC_STRING_PAIR_LIST & fabricStringPairList)
        {
            return ToPublicApiStringMap<TStringMap, FABRIC_STRING_PAIR_LIST>(
                heap,
                stringMap,
                fabricStringPairList);
        }


        template<class TStringMap, class TPublicApiList>
        static Common::ErrorCode ToPublicApiStringMap(
            __in Common::ScopedHeap & heap,
            __in TStringMap const& stringMap,
            __out TPublicApiList & fabricStringPairList)
        {
            auto count = static_cast<ULONG>(stringMap.size());

            fabricStringPairList.Count = count;
            fabricStringPairList.Items = NULL;

            if (count > 0)
            {
                auto items = heap.AddArray<FABRIC_STRING_PAIR>(count);

                size_t i = 0;
                for (auto kvPair : stringMap)
                {
                    items[i].Name = heap.AddString(kvPair.first);
                    items[i].Value = heap.AddString(kvPair.second);

                    ++i;
                }

                fabricStringPairList.Items = items.GetRawArray();
            }

            return ErrorCode::Success();
        }

        template<class TStringMap>
        static Common::ErrorCode FromPublicApiStringPairList(
            _In_ FABRIC_STRING_PAIR_LIST const & fabricStringPairList,
            _Out_ TStringMap & stringMap)
        {
            return FromPublicApiStringMap<TStringMap, FABRIC_STRING_PAIR_LIST>(
                fabricStringPairList,
                stringMap);
        }

        template<class TStringMap, class TPublicApiList>
        static Common::ErrorCode FromPublicApiStringMap(
            _In_ TPublicApiList const & fabricStringPairList,
            _Out_ TStringMap & stringMap)
        {
            for (ULONG i = 0; i < fabricStringPairList.Count; ++i)
            {
                auto stringPair = fabricStringPairList.Items[i];

                wstring key;
                auto error = StringUtility::LpcwstrToWstring2(stringPair.Name, true, key);
                if (!error.IsSuccess())
                {
                    return error;
                }

                wstring value;
                error = StringUtility::LpcwstrToWstring2(stringPair.Value, true, value);
                if (!error.IsSuccess())
                {
                    return error;
                }

                stringMap.insert(make_pair(key, value));
            }

            return ErrorCode::Success();
        }
    };
}
