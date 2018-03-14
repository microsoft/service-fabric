// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Federation
{
    using namespace std;
    using namespace Common;

    DEFINE_SINGLETON_COMPONENT_CONFIG(FederationConfig)

    VoteConfig::const_iterator VoteConfig::find(NodeId nodeId, wstring const & ringName) const
    {
        for (auto it = begin(); it != end(); ++it)
        {
            if (it->Id == nodeId && it->RingName == ringName)
            {
                return it;
            }
        }

        return end();
    }

    VoteConfig VoteConfig::Parse(StringMap const & entries)
    {
        VoteConfig result;

        // Votes are defined like this:
        // <proxy ID> = <configuration info>...
        //
        StringMap connectionStringsToKeyMap;
        for (auto entry = entries.begin(); entry != entries.end(); ++entry)
        {
            wstring const & key = entry->first;
            wstring const & value = entry->second;
            size_t index = value.find(L',');
            ASSERT_IF(index == wstring::npos, "VoteEntry for vote {0} is incorrectly formatted: {1}", key, value);
            wstring type = value.substr(0, index);
            wstring connectionString = value.substr(index + 1);
            wstring ringName;

            NodeId nodeId;
            if (type ==  Constants::SeedNodeVoteType || type == Constants::WindowsAzureVoteType)
            {
                wstring idString;
                index = key.find(L'@');
                if (index != wstring::npos)
                {
                    ringName = key.substr(index + 1);
                    idString = key.substr(0, index);
                }
                else
                {
                    idString = key;
                }

                ASSERT_IF(!NodeId::TryParse(idString, nodeId), "Could not parse '{0}' into a NodeId", key);
            }
            else if (type == Constants::SqlServerVoteType)
            {
                ErrorCode errorCode = NodeIdGenerator::GenerateFromString(key, nodeId, L"", false, L"");
                ASSERT_IFNOT(errorCode.IsSuccess(), "NodeIdGenerator failed");

                auto existing = connectionStringsToKeyMap.find(connectionString);
                if (existing != connectionStringsToKeyMap.end())
                {
                    Assert::CodingError(
                        "Connection string {0} already associated with key '{1}'",
                        connectionString,
                        existing->second);
                }

                connectionStringsToKeyMap.insert(pair<wstring, wstring>(connectionString, key));
            }
            else
            {
                Assert::CodingError("Unsupported proxy type {0}", type);
            }

            ASSERT_IF(result.find(nodeId, ringName) != result.end(), "Vote configuration for '{0}:{1}' already exists", key, value);
            result.push_back(VoteEntryConfig(nodeId, type, connectionString, ringName));
        }

        return result;
    }
    
    void VoteConfig::WriteTo(TextWriter& w, FormatOptions const&) const
    {
        for (auto it = begin(); it != end(); ++it)
        {
            w.WriteLine("{0}:{1}{2},{3}",
                it->Id,
                it->Type,
                it->RingName.empty() ? L"" : L"," + it->RingName,
                it->ConnectionString);
        }
    }

    bool ParseThresholds(vector<double> & thresholds, StringMap const & entries, wstring const & name, wstring const & defaultThresholds)
    {
        wstring data;

        auto it = entries.find(name + L"Threshold");
        if (it != entries.end())
        {
            data = it->second;
        }
        else if (defaultThresholds.size() == 0)
        {
            return false;
        }
        else
        {
            data = defaultThresholds;
        }

        vector<wstring> values;
        StringUtility::Split<wstring>(data, values, L",");

        ASSERT_IF(values.size() > 3, "Too many thresholds configured: {0}", data);
        
        for (wstring const & value : values)
        {
            double threshold;
            if (!Config::TryParse<double>(threshold, value))
            {
                Assert::CodingError("Invalid arbitration threshold value: {0}", value);
            }

            ASSERT_IF(thresholds.size() > 0 && thresholds[thresholds.size() - 1] > threshold,
                "Thresholds not in increasing order: {0}", data);

            thresholds.push_back(threshold);
        }

        return true;
    }
}
