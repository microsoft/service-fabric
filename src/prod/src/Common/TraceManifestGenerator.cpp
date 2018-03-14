// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;

namespace Common
{
    TraceManifestGenerator::TraceManifestGenerator()
        :   events_(),
            maps_(),
            templates_(),
            tasks_(),
            keyword_(),
            eventsWriter_(events_),
            mapsWriter_(maps_),
            templatesWriter_(templates_),
            tasksWriter_(tasks_),
            keywordWriter_(keyword_)
    {
    }

    std::string TraceManifestGenerator::StringResource(StringLiteral original)
    {
        return StringResource(string(original.begin(), original.end()));
    }

    std::string TraceManifestGenerator::StringResource(std::string const & original)
    {
        std::wstring originalWide(original.begin(), original.end());
        size_t hashValue = StringUtility::GetHash(originalWide);
        size_t differentiator;
        auto it = stringTable_.find(hashValue);
        if (it != stringTable_.end())
        {
            std::map<std::string, size_t> & sameHashStrings = it->second;
            auto innerIt = sameHashStrings.find(original);
            if (innerIt != sameHashStrings.end())
            {
                differentiator = innerIt->second;
            }
            else
            {
                differentiator = sameHashStrings.size();
                sameHashStrings[original] = differentiator;
            }
        }
        else
        {
            differentiator = 0;
            std::map<std::string, size_t> sameHashStrings;
            sameHashStrings[original] = differentiator;
            stringTable_[hashValue] = sameHashStrings;
        }

        return formatString("$(string.ns{0}.{1})", hashValue, differentiator);
    }

    void TraceManifestGenerator::Write(FileWriter & writer,
        Guid guid,
        std::string const & name,
        std::string const & message,
        std::string const & symbol,
        std::string const & targetFile)
    {
        bool forLease = false;
        if (name.find("Lease") != std::string::npos)
        {
            forLease = true;
        }

        writer << "<?xml version='1.0' encoding='utf-8' standalone='yes'?>\r\n";
        writer << "<assembly\r\n";
        writer << "    xmlns=\"urn:schemas-microsoft-com:asm.v3\"\r\n";
        writer << "    xmlns:test=\"http://manifests.microsoft.com/win/2004/08/test/events\"\r\n";
        writer << "    xmlns:win=\"http://manifests.microsoft.com/win/2004/08/windows/events\"\r\n";
        writer << "    xmlns:xs=\"http://www.w3.org/2001/XMLSchema\"\r\n";
        writer << "    xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\r\n";
        writer << "    manifestVersion=\"1.0\"\r\n";
        writer << "    >\r\n";
        writer << "  <assemblyIdentity\r\n";
        writer << "      buildType=\"$(build.buildType)\"\r\n";
        writer << "      language=\"neutral\"\r\n";
        if (forLease)
        {
            writer << "      name=\"Microsoft-ServiceFabric-LeaseEvents\"\r\n";
        }
        else
        {
            writer << "      name=\"Microsoft-ServiceFabric-Events\"\r\n";
        }
        writer << "      processorArchitecture=\"$(build.processorArchitecture)\"\r\n";
        writer << "      publicKeyToken=\"$(Build.WindowsPublicKeyToken)\"\r\n";
        writer << "      version=\"$(build.version)\"\r\n";
        writer << "      versionScope=\"nonSxS\"\r\n";
        writer << "      />\r\n";
        writer << "  <instrumentation>\r\n";
        writer << "    <events\r\n";
        writer << "        xmlns=\"http://schemas.microsoft.com/win/2004/08/events\"\r\n";
        writer << "        xmlns:auto-ns1=\"urn:schemas-microsoft-com:asm.v3\"\r\n";
        writer << "        >\r\n";
        writer << "      <provider\r\n";
        writer << "          guid=\"{" << guid << "}\"\r\n";
        writer << "          message=\"" << StringResource(message) << "\"\r\n";
        writer << "          messageFileName=\"" << targetFile << "\"\r\n";
        writer << "          name=\"" << name << "\"\r\n";
        writer << "          resourceFileName=\"" << targetFile << "\"\r\n";
        writer << "          symbol=\"" << symbol << "\"\r\n";
        writer << "          >\r\n";

        WriteChannels(writer, forLease);

        WriteContents(writer);

        writer << "      </provider>\r\n";
        writer << "    </events>\r\n";
        writer << "  </instrumentation>\r\n";

        WriteResources(writer);

        writer << "</assembly>\r\n";
    }

    void TraceManifestGenerator::WriteContents(FileWriter & writer)
    {
        writer << "        <events>\r\n";
        writer << events_;
        writer << "        </events>\r\n";

        writer << "        <tasks>\r\n";
        writer << tasks_;
        writer << "        </tasks>\r\n";

        writer << "        <templates>\r\n";
        writer << templates_;
        writer << "        </templates>\r\n";

        if (!maps_.empty())
        {
            writer << "        <maps>\r\n";
            writer << maps_;
            writer << "        </maps>\r\n";
        }

        if (!keyword_.empty())
        {
            writer << "        <keywords>\r\n";
            writer << keyword_;
            writer << "        </keywords>\r\n";
        }
    }

    void TraceManifestGenerator::WriteChannels(
        FileWriter & writer,
        bool isForLease)
    {
        writer << "        <channels>\r\n";
        string name;
        if (isForLease)
        {
            name = "Microsoft-ServiceFabric-Lease";
        }
        else
        {
            name = "Microsoft-ServiceFabric";
        }

        string provider_name = name;
        while (provider_name.find("-") != string::npos)
            provider_name.replace(provider_name.find("-"), 1, "_");

        for (TraceChannelType::Enum chnl = TraceChannelType::Admin; chnl < TraceChannelType::DeprecatedOperational; chnl = static_cast<TraceChannelType::Enum>(chnl + 1))
        {
            if (isForLease && chnl == TraceChannelType::DeprecatedOperational)
            {
                continue;
            }

            writer << "          <channel\r\n";
            writer << "              chid=\"" << chnl << "\"\r\n";
            if (chnl == TraceChannelType::Debug ||
                chnl == TraceChannelType::Analytic)
            {
                writer << "              enabled=\"false\"\r\n";
            }
            else
            {
                writer << "              enabled=\"true\"\r\n";
            }
            writer << "              isolation=\"Application\"\r\n";

            // Fabric operational is a new channel for external customers. It is important the old deprecated channel name stay the same.
            writer << "              name=\"" << (!isForLease && chnl == TraceChannelType::Operational ? "Microsoft-ServiceFabric" : name) << "/"
                   << (chnl == TraceChannelType::DeprecatedOperational ? TraceChannelType::Operational : chnl) << "\"\r\n";
            writer << "              symbol=\"CHANNEL_" << provider_name << "_" << chnl << "\"\r\n";
            writer << "              type=\"" << (chnl == TraceChannelType::DeprecatedOperational ? TraceChannelType::Operational : chnl) << "\"\r\n";
            writer << "              />\r\n";
        }

        writer << "        </channels>\r\n";
    }

    void TraceManifestGenerator::WriteResources(FileWriter & writer)
    {
        writer << "  <localization>\r\n";
        writer << "    <resources culture=\"en-US\">\r\n";
        writer << "      <stringTable>\r\n";

        for (auto iter = stringTable_.begin(); iter != stringTable_.end(); ++iter)
        {
            size_t hashValue = iter->first;
            std::map<std::string, size_t> & sameHashStrings = iter->second;
            for (auto iter1 = sameHashStrings.begin(); iter1 != sameHashStrings.end(); ++iter1)
            {
                std::string str = iter1->first;
                size_t differentiator = iter1->second;
                writer << "        <string" << "\r\n";
                writer << "            id=\"ns" << hashValue << "." << differentiator << "\"" << "\r\n";
                writer << "            value=\"" << str <<"\"" << "\r\n";
                writer << "            />\r\n";
            }
        }

        writer << "      </stringTable>\r\n";
        writer << "    </resources>\r\n";
        writer << "  </localization>\r\n";
    }
}
