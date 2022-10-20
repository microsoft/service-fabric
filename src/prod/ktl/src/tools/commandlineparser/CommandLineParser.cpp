// CommandLineParser.cpp : Defines the entry point for the console application.
//

#pragma once
#include "CommandLineParser.h"

#if !defined(PLATFORM_UNIX)
#include "NtstrSafe.h"
#endif

     void Parameter::InsertValue(LPCWSTR Token)
     {
         NTSTATUS Status;

         if (!Token) return;
         size_t nLen = 0;
         Status = RtlStringCchLengthW(Token, MaxValueLength, &nLen);
         if (!NT_SUCCESS(Status ) || nLen == 0) return;

         HRESULT hr;
         size_t result;
         hr = SizeTAdd(nLen, 1, &result);
         KInvariant(SUCCEEDED(hr));
         wchar_t* pTmp = _new(KTL_TAG_CMDLINE, _Allocator) wchar_t [result];
         RtlStringCchCopyW(pTmp, nLen+1, Token);
         _Values[_ValueCount++] = pTmp;
     }

    CmdLineParser::CmdLineParser(KAllocator& Allocator) 
        :   _Allocator(Allocator)
    {
        Zero();
    }
       
    CmdLineParser::~CmdLineParser() 
    {
        Cleanup();
    }


    // Parse()
    //
    // This overload parses the type of command-line received from main()
    // 
    BOOLEAN CmdLineParser::Parse(int Argc, __in_ecount(Argc ) wchar_t* Argv[])
    {
        LPWSTR pszTemp;
        HRESULT hr;

        size_t TotalRequired = 0;
        for (int i = 0; i < Argc; i++)
        {
            NTSTATUS Status;

            size_t Len = 0;
            Status = RtlStringCchLengthW(Argv[i], 8192, &Len);
            if (!NT_SUCCESS(Status))
            {
                return(FALSE);
            }

            hr = SizeTAdd(TotalRequired, Len, &TotalRequired);
            KInvariant(SUCCEEDED(hr));
            hr = SizeTAdd(TotalRequired, 1, &TotalRequired);
            KInvariant(SUCCEEDED(hr));
            
        }

        hr = SizeTAdd(TotalRequired, 1, &TotalRequired);
        KInvariant(SUCCEEDED(hr));
        pszTemp = _new(KTL_TAG_CMDLINE, _Allocator) wchar_t [ TotalRequired];
        *pszTemp = 0;

        for (int i = 0; i < Argc; i++)
        {
            RtlStringCchCatW(pszTemp, TotalRequired, Argv[i]);
            RtlStringCchCatW(pszTemp, TotalRequired, L" ");
        }

        BOOLEAN Res = Parse(pszTemp);
        _delete(pszTemp);
        return Res;
    }
       
    // Parse()
    //
    // This overload parses a raw string.  The string must be
    // NULL-terminated.
    //
    //BOOLEAN Parse(wchar_t* RawString); 
    
    // Reset
    //
    // Empties the object and readies it for another parsing.
    //
    void CmdLineParser::Reset()
    {
        Cleanup();
    }

    // GetErrorInfo
    //
    // Used if Parse() return FALSE indicating an error
    // in the command line.  
    //
    // Return value:
    //   TRUE if there was an error and a message was returned.
    //       
    //   FALSE if there was nothing wrong with the command line.
    //
    // The message is statically allocated and does not need to be freed.
    //
    BOOLEAN CmdLineParser::GetErrorInfo(ULONG *CharPos, __out LPWSTR* Message)
    {
        *Message = _ErrorMsg;
        *CharPos = _CharPos;
        return TRUE;
    }

    // Get output from parsing.
    //
    // Returns the number of parameter objects.
    //
    ULONG CmdLineParser::ParameterCount() { return _ParamCount; }
       
    // GetParam()
    //
    // Retrieves the parameter at the specified index.
    // Returns NULL if the Index is invalid.
    //
    Parameter* CmdLineParser::GetParam(ULONG Index)
    {
        return _Params[Index];
    }

//private:

    // Functions

    wchar_t   CmdLineParser::PeekChar() { return *_Src; }
    BOOLEAN   CmdLineParser::EndOfInput() { return *_Src ? FALSE: TRUE; }
    wchar_t*  CmdLineParser::CurrentLoc() { return _Src; }
    
    BOOLEAN CmdLineParser::EmitParam() 
    {
        NTSTATUS Status;
        size_t Len;
        Status = RtlStringCchLengthW(_TokenBuffer, MaxTokenBuffer, &Len);
        if (!NT_SUCCESS(Status )|| Len > MaxParameterNameLength)
        {
            _ErrorMsg = (LPWSTR) _Msg_ParameterNameTooLong;
            return FALSE;
        }

        if (_ParamCount == MaxParameters)
        {
            _ErrorMsg = (LPWSTR) _Msg_TooManyParameters;
            return FALSE;
        }
        _Params[_ParamCount] = _new(KTL_TAG_CMDLINE, _Allocator) Parameter(_Allocator);
        RtlStringCchCopyW(_Params[_ParamCount]->_Name, MaxParameterNameLength, _TokenBuffer);
        _ParamCount++;
        return TRUE;
    }


    void CmdLineParser::Cleanup() 
    {
        for (ULONG i = 0; i < MaxParameters; i++)
        {
            _delete(_Params[i]);
        }
        Zero();
    }

    void CmdLineParser::Zero()
    {
        for (ULONG i = 0; i < MaxParameters; i++)
        {
            _Params[i] = nullptr;
        }
        _ParamCount = 0;
        _Src = 0;
        _ErrorMsg = 0;
        _CharPos = 0;
    }


    void CmdLineParser::AppendValue(wchar_t c) 
    {
       _TokenBuffer[_TokenBufferPos++] = c;
       _TokenBuffer[_TokenBufferPos] = 0;
    }

    BOOLEAN CmdLineParser::EmitValue()
    {
        NTSTATUS Status;
        size_t Len;
        Status = RtlStringCchLengthW(_TokenBuffer, MaxTokenBuffer, &Len);
        if (!NT_SUCCESS(Status ) || Len > MaxValueLength)
        {
            _ErrorMsg = (LPWSTR) _Msg_ValueTooLong;
            return FALSE;
        }

        if (_Params[_ParamCount-1]->_ValueCount == MaxValuesPerParam)
        {
            _ErrorMsg = (LPWSTR) _Msg_TooManyValues;
            return FALSE;
        }

        _Params[_ParamCount-1]->InsertValue(_TokenBuffer);

        return TRUE;
    }

    wchar_t CmdLineParser::NextChar()
    {
        if (!*_Src) return 0; // End of input
        _CharPos++;
        _Src++;
        return *_Src;
    }


    void CmdLineParser::ResetValueAccumulator()
    {
        _TokenBufferPos = 0;
        _TokenBuffer[0] = 0;
    }


const wchar_t* CmdLineParser::_Msg_InvalidSwitchIdent   = L"Invalid switch character : must be alphanumeric or _";
const wchar_t* CmdLineParser::_Msg_MissingSwitchIdent   = L"Missing switch identifier character : must be alphanumeric or _";
const wchar_t* CmdLineParser::_Msg_ExpectingSwitch      = L"Expecting a parameter switch: - or /";
const wchar_t* CmdLineParser::_Msg_UnterminatedString   = L"Unterminated string";
const wchar_t* CmdLineParser::_Msg_NullString           = L"Null String";
const wchar_t* CmdLineParser::_Msg_TooManyParameters    = L"Too many parameters";
const wchar_t* CmdLineParser::_Msg_TooManyValues        = L"Too many values for the parameter";
const wchar_t* CmdLineParser::_Msg_ParameterNameTooLong = L"Parameter name too long";
const wchar_t* CmdLineParser::_Msg_ValueTooLong         = L"Parameter value too long";



BOOLEAN CmdLineParser::Parse(__in_opt LPWSTR RawString)
{
    // This parser models a simple DFA, and the enum holds the current 'state'.
    //
    enum { eWaitingSwitch,       // Waiting for a - or /
           eSwitch,              // Recognized precisely one - or /
           eSwitchIdentStart,    // Waiting for switch identifier first character [a-zA-Z_]
           eSwitchIdent,         // In the middle of a switch identifier [a-zA-Z_0-9]
           ePostIdent,           // Whitespace after switch identifer is completed; either another switch or value of previous switch
           eContiguousValue,     // Found an non-switch, so we must be recognizing a value for the previous switch
           eSingleQuotedString,  // Found a string beginning with ', so we look for a closing '
           eDoubleQuotedString,  // Found a string beginning with ", so we look for a closing ", except for escaped string
           ePossibleEscape,      // Found an \ in the middle of a string, so allow a quote.  If a double quote not found, emit \ plus whatever was found.
           eError,               // An error in the sequence
           eDone                 // Finished, all is well
         } 
         eState = eWaitingSwitch;

    if (!RawString)
    {
        _ErrorMsg = (LPWSTR) _Msg_NullString;
        return FALSE;
    }

    _Src = RawString;
    _CharPos = 0;

    while (eState != eError && eState != eDone)
    {
        switch (eState)
        {
        case eWaitingSwitch:
            if (EndOfInput())
            {
                eState = eDone;
                continue;
            }
            if (iswspace(PeekChar()))  
            {
                NextChar();
                continue; 
            }
            eState = eSwitch;
            continue;

        // Recognize a command line switch start
        case eSwitch:
            if (PeekChar() == L'/' || PeekChar() == '-')
            {
                eState = eSwitchIdentStart;
                NextChar();
                continue;
            }
            else
            {
                eState = eError;
                _ErrorMsg = (LPWSTR) _Msg_ExpectingSwitch;
                continue;
            }

        // We have recognized the switch char, so extract the first char of identifier
        case eSwitchIdentStart:
            if (EndOfInput())
            {
                eState = eError;
                _ErrorMsg = (LPWSTR) _Msg_MissingSwitchIdent;
                continue;
            }
            if (! (iswalpha(PeekChar()) || PeekChar() == '_'))
            {
                eState = eError;
                _ErrorMsg = (LPWSTR) _Msg_InvalidSwitchIdent;
                continue;
            }
            eState = eSwitchIdent;
            ResetValueAccumulator();
            AppendValue(PeekChar());
            NextChar();
            continue;

        // We have recognized the first switch ident character; keep going
        case eSwitchIdent:
            if (EndOfInput())
            {
                // End of input, finish up the current switch and exit.
                
                if (!EmitParam())
                {
                    eState = eError;
                    break;
                }
                eState = ePostIdent;
                continue;
            }
            // If still in the ident, add it to the token 
            if (iswalnum(PeekChar()) || PeekChar() == '_')
            {
                AppendValue(PeekChar());
                NextChar();
                continue;
            }
            
            // Done with the switch
            if (iswspace(PeekChar()))
            {
                // Emit new parameter object

                if (!EmitParam())
                {
                    eState = eError;
                    break;
                }
                eState = ePostIdent;
                NextChar();
                continue;
            }

            // Otherwise, some kind of error
            eState = eError;
            _ErrorMsg = (LPWSTR) _Msg_InvalidSwitchIdent;
            continue;

        // We have recognized a full switch and its name, but don't know
        // whether we are going for a value or a new switch
        //
        case ePostIdent:
            if (EndOfInput())
            {
                eState = eDone;
                continue;
            }

            // If we are back to recognizing switches
            if (PeekChar() == L'/' || PeekChar() == L'-')
            {
                eState = eSwitch;
                continue;
            }

            // Still waiting to find out...
            if (iswspace(PeekChar()))
            {
                NextChar();
                continue;  // Still Don't KNow
            }

            // Ah, we are entering a quoted string
            if (PeekChar() == L'"')
            {
                eState = eDoubleQuotedString;
                ResetValueAccumulator();
                NextChar();
                continue;
            }

            // Entering a singly quoted string
            if (PeekChar() == L'\'')
            {
                NextChar();
                ResetValueAccumulator();
                eState = eSingleQuotedString;
                continue;
            }
            else
            {
                ResetValueAccumulator();
                AppendValue(PeekChar());
                NextChar();
                eState = eContiguousValue;
                continue;
            }
        
        case eContiguousValue:
            if (EndOfInput())
            {
                if (!EmitValue())
                {
                    eState = eError;
                    break;
                }
                eState = eDone;
                continue;
            }
            if (iswspace(PeekChar()))
            {
                if (!EmitValue())
                {
                    eState = eError;
                    break;
                }       
                eState = ePostIdent;
                continue;
            }
            AppendValue(PeekChar());
            NextChar();
            continue;
            

        case eDoubleQuotedString:
            if (EndOfInput())
            {
                eState = eError;
                _ErrorMsg = (LPWSTR) _Msg_UnterminatedString;
                break;;
            }

            if (PeekChar() == L'"')
            {
                eState = ePostIdent;
                if (!EmitValue())
                {
                    eState = eError;
                    break;
                }
                NextChar();
                continue;
            }
            AppendValue(PeekChar());
            NextChar();
            continue;

        case eSingleQuotedString:
            if (EndOfInput())
            {
                eState = eError;
                _ErrorMsg = (LPWSTR) _Msg_UnterminatedString;
                break;;
            }

            if (PeekChar() == L'\'')
            {
                eState = ePostIdent;
                if (!EmitValue())
                {
                    eState = eError;
                    break;
                }
                
                NextChar();
                continue;
            }
            AppendValue(PeekChar());
            NextChar();
            continue;


        case ePossibleEscape:
            if (PeekChar() == L'"')
            {
                eState = ePostIdent;
                if (!EmitValue())
                {
                    eState = eError;
                    break;
                }
                NextChar();
                continue;
            }
            AppendValue(PeekChar());
            NextChar();
            continue;

        default:
            eState = eError;
            break;
        }
    }

    if (eState == eDone) return TRUE;
    else return FALSE;
}



//
// Support for redirecting KTL traces to a file when running under a
// unit test
//
#if defined(PLATFORM_UNIX)

static const ULONG rootPathLengthInChar = 512;
#define TraceFileName "/ktl.trace"
static const char* traceFileName = TraceFileName; 
static FILE* fp;

//
// Format of a trace line in a .trace file:
// date time,Type,ThreadId,ProcessId,TraceType,TraceText
// example: 2018-8-17 09:05:43.233,Informational,4284,4,.CHECKPOINT_WDATA_INFORMATIONAL,Checkpoint (Activity Id:0) (Message:=CloseServiceFSM) (Status:=0) (D1:=2) (D2:=ffffe0006b17d870) (D3:=0) (D4:=0) (Function:OverlayManager::CloseServiceFSM) (Line:407) (File:x:\bt\961135\repo\src\prod\src\ktllogger\sys\ktlshim\overlaymanager.cpp)
//
static char* levels[] =
{
    "Silent",
    "Critical",
    "Error",
    "Warning",
    "Informational",
    "Noise"
};

VOID KtlTraceCallbackRoutine(
    __in LPCWSTR TypeText,
    __in USHORT Level,
    __in LPCSTR Text
   )
{
    KAssert(fp != NULL);

    time_t rawtime;
    struct tm* timeInfo;
    char timeBuf[80];

    time(&rawtime);
    timeInfo = localtime(&rawtime);
    strftime(timeBuf, 80, "%F %T", timeInfo);

    char *level;
    if (Level > 5)
    {
        level = "Unknown";
    } else {
        level = levels[Level];
    }

    char* p = (char*)Text;
    while(*p != 0)
    {
        if (*p == (char)0x0a)
        {
            *p = ' ';
        }
        p++;
    }
    
    fprintf(fp, "%s,%s,%d,%d,%s,%s\n", timeBuf, level, GetCurrentThreadId(), GetCurrentProcess(), Utf16To8(TypeText).c_str(), Text);
    fflush(fp);
}


NTSTATUS KtlTraceRegister()
{
    char currentPath[rootPathLengthInChar+1];
    char *path;
	HRESULT hr;
	size_t cchMax;

    path = getcwd(currentPath, rootPathLengthInChar);
    if (path == NULL)
    {
        printf("Ktl traces path is too long\n");
        return(STATUS_UNSUCCESSFUL);
    }

	hr = StringCchLengthA(currentPath, rootPathLengthInChar, &cchMax);
	if (! SUCCEEDED(hr))
	{
        printf("Ktl traces path is too long\n");
        return(STATUS_UNSUCCESSFUL);
	}
	
    ULONG lenNeeded = cchMax + sizeof(TraceFileName);
    if (lenNeeded > rootPathLengthInChar)
    {
        printf("Ktl traces path is too long\n");
        return(STATUS_UNSUCCESSFUL);
    }
	
    char* psz = currentPath + cchMax;
	hr = StringCchCopyA(psz, (rootPathLengthInChar - cchMax), traceFileName);
	if (! SUCCEEDED(hr))
	{
        printf("Ktl traces path is too long\n");
        return(STATUS_UNSUCCESSFUL);
	}
	
    printf("Ktl traces will be saved to %s\n", path);

    fp = fopen(path, "w+");
    if (fp == NULL)
    {
        return(STATUS_UNSUCCESSFUL);
    }

    RegisterKtlTraceCallback(KtlTraceCallbackRoutine);
    return(STATUS_SUCCESS);
}

VOID KtlTraceUnregister()
{
    fclose(fp);
}

#endif
