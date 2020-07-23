// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#define DEFINE_RESERVED_ID_RANGE(name, low, high) Common::Global<ConsistencyUnitId::ReservedIdRange> ConsistencyUnitId::name = Common::make_global<ConsistencyUnitId::ReservedIdRange>(low, high)

namespace Reliability
{
    DEFINE_RESERVED_ID_RANGE(FMIdRange,                     0x00000001, 0x00000FFF);
    DEFINE_RESERVED_ID_RANGE(NamingIdRange,                 0x00001000, 0x00001FFF);
    DEFINE_RESERVED_ID_RANGE(CMIdRange,                     0x00002000, 0x00002FFF);
    DEFINE_RESERVED_ID_RANGE(FileStoreIdRange,              0x00003000, 0x00003FFF);
    DEFINE_RESERVED_ID_RANGE(RMIdRange,                     0x00004000, 0x00004FFF);
    DEFINE_RESERVED_ID_RANGE(FaultAnalysisServiceIdRange,   0x00005000, 0x00005FFF);
    DEFINE_RESERVED_ID_RANGE(UpgradeOrchestrationServiceIdRange, 0x00006000, 0x00006FFF);
    DEFINE_RESERVED_ID_RANGE(BackupRestoreServiceIdRange,   0x00007000, 0x00007FFF);
    DEFINE_RESERVED_ID_RANGE(CentralSecretServiceIdRange,   0x00008000, 0x00008FFF);
    DEFINE_RESERVED_ID_RANGE(EventStoreServiceIdRange,   0x00009000, 0x00009FFF);

    const ConsistencyUnitId ConsistencyUnitId::Zero(Common::Guid(L"00000000-0000-0000-0000-000000000000"));

    INITIALIZE_SIZE_ESTIMATION(ConsistencyUnitId)

    const ULONG bitsPerByte = 8;

    Common::Guid ConsistencyUnitId::CreateNonreservedGuid()
    {
        // CONSIDER: stamp part of time onto high bits of GUID so they are mostly sequential.
        // This can make database insertion more efficient.
        for (;;)
        {
            Common::Guid guid = Common::Guid::NewGuid();

            if (IsReserved(guid))
            {
                // Somehow we randomly allocated a GUID that had all 0's for its first 96 bits, so
                // loop back and retry.
                continue;
            }
            else
            {
                return guid;
            }
        }
    }

    Common::Guid ConsistencyUnitId::ReservedIdRange::CreateReservedGuid(ULONG index) const
    {
        ASSERT_IF(index > Count, "index cannot be greater than count");

        ::GUID guid;
        memset(&guid, 0, sizeof(guid));

        ULONG id = low_ + index;

        for (size_t i=0; i<sizeof(id); i++)
        {
            auto offset = sizeof(guid.Data4) - (i + 1);
            guid.Data4[offset] = static_cast<byte>(id >> (i * bitsPerByte));
        }

        return Common::Guid(guid);
    }

    ULONG ConsistencyUnitId::GetReservedId() const
    {
        ASSERT_IFNOT(IsReserved(guid_), "IsReserved(guid_)");
        ULONG id = 0;
        ::GUID const & innerGUID = guid_.AsGUID();

        for (size_t i=0; i<sizeof(id); i++)
        {
            auto offset = sizeof(innerGUID.Data4) - (i + 1);
            id += static_cast<ULONG>(innerGUID.Data4[offset]) << (i * bitsPerByte);
        }

        return id;
    }

    bool ConsistencyUnitId::IsReserved(Common::Guid guid)
    {
        ::GUID const & innerGUID = guid.AsGUID();

        return (
            (innerGUID.Data1 == 0) &&
            (innerGUID.Data2 == 0) &&
            (innerGUID.Data3 == 0) &&
            (innerGUID.Data4[0] == 0) &&
            (innerGUID.Data4[1] == 0) &&
            (innerGUID.Data4[2] == 0) &&
            (innerGUID.Data4[3] == 0));
    }

    std::wstring ConsistencyUnitId::ToString() const
    {   
        std::wstring product;
        Common::StringWriter writer(product);
        WriteTo(writer, Common::FormatOptions(0, false, ""));
        return product;
    }
}
