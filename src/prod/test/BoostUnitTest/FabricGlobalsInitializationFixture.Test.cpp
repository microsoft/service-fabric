//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include <boost/test/unit_test.hpp>
#include "Common/FabricGlobals.h"

using namespace std;
using namespace Common;

/*
    Global fixture used to initialize the fabric globals 
    This runs before any test case runs and is used to 
    mark the singleton instance of fabric globals in this
    module as primary.

    This is intended for use in any unit test that is testing
    a component that is below fabric common in the hierarchy

    It assumes that the config is in a .cfg file or store -> fails otherwise
*/
namespace
{
    ConfigStoreDescriptionUPtr CreateTestConfigStore()
    {
        ConfigStoreType::Enum storeType;
        wstring storeLocation;

        auto error = FabricEnvironment::GetStoreTypeAndLocation(nullptr, storeType, storeLocation);
        if (!error.IsSuccess())
        {
            throw std::runtime_error("Failed FabricEnv::GetStoreTypeAndLocation");
        }

        if (storeType == ConfigStoreType::Cfg)
        {
            return make_unique<ConfigStoreDescription>(
                make_shared<FileConfigStore>(storeLocation),
                *FabricEnvironment::FileConfigStoreEnvironmentVariable,
                storeLocation);
        }

        if (storeType == ConfigStoreType::None)
        {
            return make_unique<ConfigStoreDescription>(
                make_shared<ConfigSettingsConfigStore>(move(ConfigSettings())),
                L"",
                L"");
        }

        throw std::invalid_argument("Unsupported config store type for test");
    }
}

struct FabricGlobalsInit
{
    FabricGlobalsInit()
    {
        FabricGlobals::InitializeAsMaster(&CreateTestConfigStore);
    }
};

BOOST_GLOBAL_FIXTURE(FabricGlobalsInit);