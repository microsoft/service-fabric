// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TracePoints
{
    class TracePointModule;
    typedef std::unique_ptr<TracePointModule> TracePointModuleUPtr;

    class TracePointModule : private DenyCopy
    {
    public:
        TracePointModule(std::wstring const & name);
        ~TracePointModule();

        void Invoke(LPCSTR functionName, ProviderMap & providerMap, LPCWSTR userData);

    private:
        HMODULE handle_;

        static HMODULE Load(std::wstring const & name);
    };
}
