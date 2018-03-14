// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

// ********************************************************************************************************************
// FileXmlSettingsStore Implementation
//

FileXmlSettingsStore::FileXmlSettingsStore(
    wstring const & fileName, 
    ConfigSettingsConfigStore::Preprocessor const & preprocessor)
    : MonitoredConfigSettingsConfigStore(fileName, preprocessor),
    fileName_(fileName)
{
    MonitoredConfigSettingsConfigStore::Initialize();
}

FileXmlSettingsStore::~FileXmlSettingsStore()
{
}

FileXmlSettingsStoreSPtr FileXmlSettingsStore::Create(
    wstring const & fileName, 
    ConfigSettingsConfigStore::Preprocessor const & preprocessor,
    bool doNotEnableChangeMonitoring)
 {
    auto store = make_shared<FileXmlSettingsStore>(fileName, preprocessor);

    if (!doNotEnableChangeMonitoring)
    {
        auto error = store->EnableChangeMonitoring();
        if (!error.IsSuccess())
        {
            Assert::CodingError(
                "Failed to enable change monitoring on FileXmlSettingsStore. ErrorCode={0}, FileName={1}",
                error,
                fileName);
        }
    }

     return store;
 }

ErrorCode FileXmlSettingsStore::LoadConfigSettings(__out Common::ConfigSettings & settings)
{
    auto error = ServiceModel::Parser::ParseConfigSettings(fileName_, settings);

    if (!error.IsSuccess()) 
    { 
        return error; 
    }

    ConfigEventSource::Events.XmlSettingsStoreSettingsLoaded(fileName_);
    return ErrorCode(ErrorCodeValue::Success);
}
