// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class StringList
    {
    public:
        static void ToPublicAPI(
            __in Common::ScopedHeap & heap, 
            __in Common::StringCollection const& stringCollection, 
            __out Common::ReferencePointer<FABRIC_STRING_LIST> & publicDescription);

        static Common::ErrorCode FromPublicApi(__in FABRIC_STRING_LIST const &publicStringList, __out Common::StringCollection & stringCollection);

        static void WriteTo(
            __in Common::TextWriter & w, 
            std::wstring const & stringCollectionDescription,
            Common::StringCollection const & v);

        static std::wstring ToString(
            std::wstring const & stringCollectionDescription,
            Common::StringCollection const & v);
    };
}
