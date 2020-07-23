#pragma once

#include "ktl.h"

// 
// Pull in a few headers that users of CommandLineParer will need.
// 

#include "string.h"
extern "C" _Check_return_ _CRTIMP int __cdecl _wtoi(_In_z_ const wchar_t *_Str);


#undef iswspace
#undef iswalpha
#undef iswalnum
#define iswspace(c)  (c == L' ' || c == L'\t' || c == L'\r' || c == '\n')
#define iswalpha(c)  ((c >= L'a'  && c <= L'z') || (c >= L'A'  && c <= L'Z') || c == L'_')
#define iswalnum(c)  (iswalpha(c) || (c >= L'0' && c <= L'9'))


static const ULONG MaxParameterNameLength  = 32;
static const ULONG MaxParameters           = 32;
static const ULONG MaxValueLength          = 256;
static const ULONG MaxValuesPerParam       = 32;
static const ULONG MaxTokenBuffer          = 512;

class Parameter
{
    K_DENY_COPY(Parameter);

public:
    wchar_t  _Name[MaxParameterNameLength+1];
    ULONG    _ValueCount;
    LPCWSTR  _Values[MaxValuesPerParam];

    Parameter(KAllocator& Allocator) 
        :   _Allocator(Allocator)
    {
        _Name[0] = 0;
        _ValueCount = 0;
        for (ULONG i = 0; i < MaxValuesPerParam; i++)          
        {
            _Values[i] = 0;
        }
    }

    ~Parameter()
    {
        for (ULONG i = 0; i < MaxValuesPerParam; i++)
        {
            _delete((PVOID)_Values[i]);
        }
    }

    void InsertValue(LPCWSTR Token);

private:
    Parameter();

private:
    KAllocator&     _Allocator;
};

class CmdLineParser
{
    K_DENY_COPY(CmdLineParser);

public:
    CmdLineParser(KAllocator& Allocator);
    ~CmdLineParser();
    BOOLEAN Parse(int Argc, __in_ecount(Argc ) wchar_t* Argv[]);
    BOOLEAN Parse(__in_opt LPWSTR RawString);
    void Reset();
    BOOLEAN GetErrorInfo(ULONG *CharPos, __out LPWSTR* Message);
    ULONG ParameterCount();
    Parameter* GetParam(ULONG Index);

private:
    CmdLineParser();

private:
    static const wchar_t* _Msg_InvalidSwitchIdent;
    static const wchar_t* _Msg_ExpectingSwitch;
    static const wchar_t* _Msg_MissingSwitchIdent;
    static const wchar_t* _Msg_UnterminatedString;
    static const wchar_t* _Msg_NullString;
    static const wchar_t* _Msg_TooManyParameters;
    static const wchar_t* _Msg_TooManyValues;
    static const wchar_t* _Msg_ParameterNameTooLong;
    static const wchar_t* _Msg_ValueTooLong;
    
    KAllocator& _Allocator;
    LPWSTR    _Src;
    LPWSTR    _ErrorMsg;
    ULONG     _CharPos;
    Parameter* _Params[MaxParameters];
    ULONG      _ParamCount;
    wchar_t   _TokenBuffer[MaxTokenBuffer];
    ULONG     _TokenBufferPos;

    wchar_t   PeekChar();
    BOOLEAN   EndOfInput();
    wchar_t*  CurrentLoc();
    BOOLEAN EmitParam();
    void Cleanup();
    void Zero();
    void AppendValue(wchar_t c);
    BOOLEAN EmitValue();
    wchar_t NextChar();
    void ResetValueAccumulator();
};

#if defined(PLATFORM_UNIX)
NTSTATUS KtlTraceRegister();
VOID KtlTraceUnregister();
#endif
