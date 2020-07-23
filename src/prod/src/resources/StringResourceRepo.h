// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#if defined(PLATFORM_UNIX)
using namespace std;
class StringResourceRepo
{
private:
    map<int, wstring> resourceMap_;
public:
    static StringResourceRepo *Get_Singleton();

    void AddResource(int id, wstring const& str);

    wstring GetResource(int id);

    StringResourceRepo();
};
#endif

