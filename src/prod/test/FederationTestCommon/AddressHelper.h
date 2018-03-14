// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FederationTestCommon
{
    class AddressHelper
    {
    public:

        AddressHelper();

        void GetAvailablePorts(std::wstring const& nodeId, std::vector<USHORT> & ports, int numberOfPorts = 3);
        static std::queue<USHORT> GetAvailableSeedNodePorts();
        static std::wstring GetLoopbackAddress();
        static std::wstring BuildAddress(std::wstring const &);
        static std::wstring BuildAddress(std::wstring const & , std::wstring const &);

        static USHORT SeedNodeRangeStart;
        static USHORT SeedNodeRangeEnd;

        static USHORT StartPort;
        static USHORT EndPort;
        static USHORT ServerListenPort;

        static std::wstring GetServerListenAddress();

    private:

        void RefreshAvailablePorts();

        static int MaxRetryCount;
        static double RetryWaitTimeInSeconds;

        Common::ExclusiveLock portsLock_;
        std::set<USHORT> availablePorts_;
        std::map<std::wstring, std::vector<USHORT>> usedPorts_;
    };
};
