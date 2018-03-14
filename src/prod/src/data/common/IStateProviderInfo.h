// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    interface IStateProviderInfo
    {
        K_SHARED_INTERFACE(IStateProviderInfo);

    public:
        //
        // Get Type Information corresponding to given language from stateProvider
        //
        virtual KString* GetLangTypeInfo(KStringView const & lang) const = 0;

        //
        // Set Type Information corresponding to given language
        //
        virtual NTSTATUS SetLangTypeInfo(KStringView const & lang, KStringView const & typeInfo) = 0;

        //
        // returns state provider kind eg. TStore, concurrent queue etc.
        //
        virtual StateProviderKind GetKind() const = 0;
    };
}
