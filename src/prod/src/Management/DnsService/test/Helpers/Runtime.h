// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace DNS { namespace Test
{
    class Runtime :
        public KtlSystemBase
    {
    private:
        using KtlSystemBase::Activate;
        using KtlSystemBase::Deactivate;

    public:
        static Runtime* Create();

    public:
        KAllocator& GetAllocator() { return NonPagedAllocator(); }

        void Delete();

    private:
        Runtime(
            __in const GUID& id
            );

        virtual ~Runtime();
    };
}}
