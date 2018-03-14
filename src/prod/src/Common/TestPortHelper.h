// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class TestPortHelper
    {
    public:
        typedef std::map<USHORT/*port*/, int/*owner process id*/> PortMap;

        static void GetPorts(USHORT numberOfPorts, USHORT & startPort);
        static USHORT GetNextLeasePort();
        static PortMap GetPortsInUse();
        static ErrorCode GetTcpPortOwnerInfo(USHORT port, _Out_ std::wstring & ownerInfo);
        static ErrorCode GetTcp6PortOwnerInfo(USHORT port, _Out_ std::wstring & ownerInfo);
        static ErrorCode GetTcpPortOwnerInfo(USHORT port, _Out_ std::string & ownerInfo);
        static std::string GetTcpPortOwnerInfo(USHORT port);
        static std::string GetNetStatTcp();

    private:
        TestPortHelper();

        static Common::GlobalWString EnvVarName_WinFabTestPorts;

        static const USHORT DefaultStartPort;
        static const USHORT DefaultEndPort;
        static const USHORT DefaultEndLeasePort;

        static USHORT startPort_;
        static USHORT endPort_;
        static USHORT endLeasePort_;
        static USHORT currentIndex_;
        static USHORT leaseIndex_;

        static ExclusiveLock portLock_;
    };
}
