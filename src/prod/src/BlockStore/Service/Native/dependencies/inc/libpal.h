// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include <stdint.h>
#include <string>

using namespace std;
#ifdef _WIN32

#include <windows.h>
static void * load_library(const char *fileName)
{
    return ::LoadLibraryA(fileName);
}

static void *load_symbol(void * module, const char *symbolName)
{
    return ::GetProcAddress((HMODULE)module, symbolName);
}

static string get_load_error()
{
    DWORD err = ::GetLastError();

    // @TODO: Call FormatMessage
    return to_string(err);
}

// VC++ compiler was giving warning here for unused
// function. This was causing linking error as this
// function is actually being called. So ignore the
// warning here.
#pragma warning(push)
#pragma warning(disable: 4505)
static void unload_library(void *module)
{
    FreeLibrary((HMODULE)module);
    return;
}
#pragma warning(pop)
#else
typedef void *HMODULE;

#include <dlfcn.h>

static HMODULE load_library(const char *fileName)
{
    HMODULE mod;
    // Use RTLD_NOW to force early symbol resolution to flush out issues earlier
    // This enable diagnosing .so symbol binding issues without using a cluster
    mod = dlopen(fileName, RTLD_NOW);
    return mod;
}

static void *load_symbol(HMODULE module, const char *symbolName)
{
    void *ret;
    ret = dlsym(module, symbolName);
    if(!ret)
        printf("[%s@%d] symbol %s not found.\n", __FUNCTION__, __LINE__, symbolName);
    return ret;
}

static string get_load_error()
{
    return string(dlerror());
}

static void unload_library(void *module)
{
    dlclose(module);
    return;
}

#endif
