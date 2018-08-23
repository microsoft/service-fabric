
/*++

    (c) 2012 by Microsoft Corp. All Rights Reserved.

    kdatetime.h

    Description:
      Kernel Tempate Library (KTL)

      ISO 8601 DateTime / Duration

    History:
      raymcc          6-June-2012         Initial version (no parser).

--*/

#pragma once


class KDuration
{
    friend class KDateTime;

public:
    KDuration()
    {
        _Raw = 0;
    }

    KDuration(__in LONGLONG Src)                // in terms of 100nsecs
    {
        _Raw = Src;
    }

    BOOLEAN
    IsValid() const
    {
        return _Raw == 0 ? FALSE : TRUE;
    }

    KDuration(
        __in KDuration const & Src
        )
    {
        _Raw = Src._Raw;
    }

    KDuration& operator =(KDuration const & Src)
    {
        _Raw = Src._Raw;
        return *this;
    }

    KDuration& operator =(LONGLONG const & Src)
    {
        _Raw = Src;
        return *this;
    }

    KDuration operator +(KDuration const & Addend)
    {
        return _Raw + Addend._Raw;
    }

    KDuration& operator +=(KDuration const & Addend)
    {
        _Raw += Addend._Raw;
        return *this;
    }

    BOOLEAN
    operator ==( KDuration const & Comparand)
    {
        return _Raw == Comparand._Raw;
    }

    BOOLEAN
    operator !=(KDuration const & Comparand)
    {
        return _Raw != Comparand._Raw;
    }

    BOOLEAN
    operator >(KDuration const & Comparand)
    {
        return _Raw > Comparand._Raw;
    }

    BOOLEAN
    operator >=(KDuration const & Comparand)
    {
        return _Raw >= Comparand._Raw;
    }

    BOOLEAN
    operator <(KDuration const & Comparand)
    {
        return _Raw < Comparand._Raw;
    }

    BOOLEAN
    operator <=(KDuration const & Comparand)
    {
        return _Raw <= Comparand._Raw;
    }


   ~KDuration()
    {
    }

    operator LONGLONG()
    {
        return _Raw;                // in terms of 100nsec ticks
    }

    VOID
    SetSeconds(__in LONG Seconds)
    {
        SetMilliseconds(Seconds * 1000);
    }

    VOID
    SetMilliseconds(__in LONG Milliseconds)
    {
        _Raw = Milliseconds * 10000;
    }

    LONGLONG
    Seconds() const
    {
        return Milliseconds() / 1000;
    }

    LONGLONG
    Milliseconds() const
    {
        return _Raw / 10000;
    }

    NTSTATUS
    ToString(__in  KAllocator& Alloc, __out KString::SPtr& Str)
    {
        KString::SPtr Tmp = KString::Create(Alloc, KStringView::MaxIso8601Duration);
        if (!Tmp)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        if (!Tmp->ToDurationFromTime(_Raw))
        {
            return STATUS_INTERNAL_ERROR;
        }
        Str = Tmp;
        return STATUS_SUCCESS;
    }

    BOOLEAN
    FromString(__in KStringView const & Src)
    {
        return Src.ToTimeFromDuration(_Raw);
    }

private:
    LONGLONG _Raw;                  // duration in terms of 100nsecs ticks
};

//** DateTime value imp
class KDateTime
{
    friend class KDuration;
public:
    KDateTime()
    {
        _Raw = 0;
    }

    KDateTime(__in LONGLONG Src)                // in terms of 100nsec ticks since January 1, 1601 (UTC)
    {
        _Raw = Src;
    }

    static
    KDateTime
    Now() 
    {
        return KDateTime(KNt::GetSystemTime());
    }

    BOOLEAN
    IsValid() const
    {
        return _Raw == 0 ? FALSE : TRUE;
    }

    VOID
    NormalizeToSeconds()
    {
        // Truncation is ok.
        _Raw = (_Raw / 10000000) * 10000000;
    }

    VOID
    NormalizeToMilliseconds()
    {
        // Truncation is ok.
        _Raw = (_Raw / 10000) * 10000;
    }

    KDateTime(__in KDateTime const & Src)
    {
        _Raw = Src._Raw;
    }

    KDateTime& operator =(KDateTime const & Src)
    {
        _Raw = Src._Raw;
        return *this;
    }

    KDateTime& operator =(LONGLONG const & Src)
    {
        _Raw = Src;
        return *this;
    }

    BOOLEAN
    operator ==(KDateTime const & Comparand) const
    {
        return _Raw == Comparand._Raw;
    }

    BOOLEAN
    operator !=(KDateTime const & Comparand)
    {
        return _Raw != Comparand._Raw;
    }

    BOOLEAN
    operator >(KDateTime const & Comparand)
    {
        return _Raw > Comparand._Raw;
    }

    BOOLEAN
    operator >=(KDateTime const & Comparand)
    {
        return _Raw >= Comparand._Raw;
    }

    BOOLEAN
    operator <(KDateTime const & Comparand)
    {
        return _Raw < Comparand._Raw;
    }

    BOOLEAN
    operator <=(KDateTime const & Comparand)
    {
        return _Raw <= Comparand._Raw;
    }

    KDuration operator -(KDateTime Other)
    {
        return KDuration(_Raw - Other._Raw);
    }

    KDateTime operator +(KDuration Interval)
    {
        return KDateTime(_Raw + Interval._Raw);
    }

    KDateTime& operator +=(KDuration const & Addend)
    {
        _Raw += Addend._Raw;
        return *this;
    }

   ~KDateTime()
    {
    }

    operator LONGLONG()             // in terms of 100nsec ticks since since January 1, 1601 (UTC)
    {
        return _Raw;
    }

    NTSTATUS
    ToString(__in  KAllocator& Allocator, __out KString::SPtr& Str)
    {
        Str = KString::Create(Allocator, KStringView::MaxIso8601DateTime);
        if (!Str)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        BOOLEAN Res = Str->FromSystemTimeToIso8601(_Raw);
        if (!Res)
        {
            return STATUS_INTERNAL_ERROR;
        }
        Str->SetNullTerminator();
        return STATUS_SUCCESS;
    }

private:
    LONGLONG _Raw;          // 100nsec ticks since since January 1, 1601 (UTC)
};

