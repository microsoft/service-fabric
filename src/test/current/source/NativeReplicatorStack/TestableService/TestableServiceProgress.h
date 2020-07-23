// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TestableService
{
    class TestableServiceProgress : public Common::IFabricJsonSerializable
    {
    public:
        PROPERTY(LONG64, dataLossNumber_, DataLossNumber)
        PROPERTY(LONG64, configurationNumber_, ConfigurationNumber)
        PROPERTY(LONG64, lastCommittedSequenceNumber_, LastCommittedSequenceNumber)
        PROPERTY(vector<LONG64>, customProgress_, CustomProgress)

    public:
        TestableServiceProgress()
            : dataLossNumber_(0)
            , configurationNumber_(0)
            , lastCommittedSequenceNumber_(0)
            , customProgress_()
        {
        }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(L"DataLossNumber", dataLossNumber_)
            SERIALIZABLE_PROPERTY(L"ConfigurationNumber", configurationNumber_)
            SERIALIZABLE_PROPERTY(L"LastCommittedSequenceNumber", lastCommittedSequenceNumber_)
            SERIALIZABLE_PROPERTY(L"CustomProgress", customProgress_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        LONG64 dataLossNumber_;
        LONG64 configurationNumber_;
        LONG64 lastCommittedSequenceNumber_;
        vector<LONG64> customProgress_;
    };
}
