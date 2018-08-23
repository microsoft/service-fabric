//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ConfigSettingsDescription
    {
    public:
        Common::ConfigSettings Settings;

    private:
        friend struct Parser;

        void ReadFromXml(Common::XmlReaderUPtr const &);

        static Common::ConfigSettings ReadSettings(Common::XmlReader &);
        static Common::ConfigSection ReadSection(Common::XmlReader &);
        static Common::ConfigParameter ReadParameter(Common::XmlReader &);
    };
}