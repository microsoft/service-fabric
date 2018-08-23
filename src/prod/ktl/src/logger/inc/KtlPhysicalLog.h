// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include <ktl.h>
#include <limits.h>

class RvdLogManager;
class RvdLogStreamType;
class RvdLogStreamId;
class RvdLogId;
class RvdLogAsn;



//**
// A log stream is a logical sequence of log records. A client can use an RvdLogStream
// object to write and read log records. It can also use it to truncate a log stream.
// Log streams are physically multiplexed into a single log container, but they are
// logically separate.
//
class RvdLogStream abstract : public KObject<RvdLogStream>, public KShared<RvdLogStream>
{
    K_FORCE_SHARED_WITH_INHERITANCE(RvdLogStream);

    #include "PhysLogStream.h"
};

//
// An RvdLog object is a container of many logically separated log streams.
// It manages the log container in terms of physical space, cache, etc.
//
// Once a client gets an SPtr to a RvdLog object, the log is already mounted.
// It does not have to and cannot call mount(). Only the RvdLogManager
// can mount a physical log.
//
class RvdLog abstract : public KObject<RvdLog>, public KShared<RvdLog>
{
    K_FORCE_SHARED_WITH_INHERITANCE(RvdLog);

    #include "PhysLogContainer.h"
};

//
// The RvdLogManager object is the root entry into the RVD log system instance.
// All other RVD log objects are created directly or indirectly
// through the RvdLogManager interface.
//
// There is typically one global RvdLogManager per machine/process.
// This is to ensure singleton properties of all other objects
// created through the RvdLogManager interface. This singleton property
// is important to coordicate all I/Os from different log users
// to the same physical log, maintain log LSN consistency etc. It is the
// responsibility of the user of the RvdLog facility to manage the lifetime
// of RvdLogManager instances in order to maintain this property.
//
class RvdLogManager abstract : public KAsyncContextBase
{
    K_FORCE_SHARED_WITH_INHERITANCE(RvdLogManager);

    #include "PhysLogManager.h"
    
protected:
    using KAsyncContextBase::Cancel;
};

// Strong unique ID and scalar types used in public API
class RvdLogStreamType : public KStrongType<RvdLogStreamType, KGuid>
{
    K_STRONG_TYPE(RvdLogStreamType, KGuid);

public:
    RvdLogStreamType() {}
    ~RvdLogStreamType() {}
};

class RvdLogStreamId : public KStrongType<RvdLogStreamId, KGuid>
{
    K_STRONG_TYPE(RvdLogStreamId, KGuid);

public:
    RvdLogStreamId() {}
    ~RvdLogStreamId() {}
};

class RvdLogId : public KStrongType<RvdLogId, KGuid>
{
    K_STRONG_TYPE(RvdLogId, KGuid);

public:
    RvdLogId() {}
    ~RvdLogId() {}
};

class RvdLogAsn : public KStrongType<RvdLogAsn, ULONGLONG volatile>
{
    K_STRONG_TYPE(RvdLogAsn, ULONGLONG volatile);

public:
    RvdLogAsn() : KStrongType<RvdLogAsn, ULONGLONG volatile>(RvdLogAsn::Null()) {}

    // All valid log ASNs must be between Null and Max, exclusively.
    static RvdLogAsn const&
    Null()
    {
        static RvdLogAsn const nullAsn((ULONGLONG)0);
        return nullAsn;
    }

    static RvdLogAsn const&
    Min()
    {
        static RvdLogAsn const minAsn((ULONGLONG)1);
        return minAsn;
    }

    static RvdLogAsn const&
    Max()
    {
        static RvdLogAsn const maxAsn(_UI64_MAX);
        return maxAsn;
    }

    BOOLEAN
    IsNull()
    {
        return (*this) == Null();
    }

    __inline BOOLEAN
    SetIfLarger(ULONGLONG newValue)
    {
        ULONGLONG volatile  oldValue;
        while (newValue > (oldValue = _Value))
        {
            if (InterlockedCompareExchange64(
                (LONGLONG volatile*)&_Value,
                (LONGLONG)newValue,
                (LONGLONG)oldValue) == (LONGLONG)oldValue)
            {
                return TRUE;
            }
        }

        return FALSE;
    }

    __inline BOOLEAN
    SetIfLarger(RvdLogAsn& NewSuggestedAsn)
    {
        return SetIfLarger(NewSuggestedAsn.Get());
    }
};

struct RvdLogStream::RecordMetadata
{
    RvdLogAsn                       Asn;
    ULONGLONG                       Version;
    RvdLogStream::RecordDisposition Disposition;
    ULONG                           Size;
    LONGLONG                        Debug_LsnValue;
};
