
/*++

    (c) 2012 by Microsoft Corp. All Rights Reserved.

    kdompath.h

    Description:
      Kernel Tempate Library (KTL): KDomPath

      Path helper/parser for KDom-related objects.

    History:
      raymcc          26-Jul-2012         Initial version.

--*/

#pragma once

class KDomPath
{
    K_NO_DYNAMIC_ALLOCATE();
    K_DENY_COPY(KDomPath);

public:
    static const ULONG MaxIndices = 3;

    // KDomPath constructor
    //
    // This is the only usable/public function in the class.  The rest is hidden and for private
    // use by DOM implementations.
    //
    // The caller can specifiy a literal path, or a parameterized path for arrays.
    //
    //  Dom->GetValue(KDomPath(L"a/b/c"), Val);                 Value at element
    //  Dom->GetValue(KDomPath(L"a/b/c[]", ix), Val);           Value from an array of 'c'
    //  Dom->GetValue(KDomPath(L"a/b/c[]/d[]", ix, ix2), Val);  Value from two-level array of 'c' and 'd'
    //  Dom->GetValue(KDomPath(L"a/b/c/@x"), Val);              Value from an attribute
    //  Dom->GetValue(KDomPath(L"ns1:a/ns2:b/c/@ns3:x"), Val);  Value based on namespace prefixes
    //
    //  Notes:
    //      (a) Arrays can exist up to three levels in this implementation.
    //      (b) Namespace prefixes are tested literally as tokens and do not do real-time namespace lookup/matching.
    //
    //
    KDomPath(__in LPCWSTR RawPath, LONG Index1 = -1, LONG Index2 = -1, LONG Index3 = -1)
    {
        _RawPath = RawPath;
        _Indices[0] = Index1;
        _Indices[1] = Index2;
        _Indices[2] = Index3;
    }

   ~KDomPath(){}


    // The content below this is only used by code that needs to invoke the parser (DOM etc.), not end-user callers.
    //
    enum { eNone = 0, eElement = 1, eArrayElement = 2, eAttribute = 3 };

    struct PToken
    {
        KString::SPtr _Ident;
        KString::SPtr _NsPrefix;
        ULONG         _IdentType;
        LONG          _LiteralIndex;

        PToken()
        {
            Clear();
        }

        VOID
        Clear()
        {
            _Ident = 0;
            _NsPrefix = 0;
            _IdentType = eNone;
            _LiteralIndex = -1;
        }
    };


    NTSTATUS
    Parse(
        __in  KAllocator& Alloc,
        __out KArray<PToken>& Output
        ) const;

    LPCWSTR _RawPath;
    LONG    _Indices[MaxIndices];

private:
    KDomPath(){} // No access
};

