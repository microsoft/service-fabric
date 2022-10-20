/*++

Copyright (c) Microsoft Corporation

Module Name:

    XmlTests.cpp

Abstract:

    This file contains test case implementations.

    To add a new unit test case:
    1. Declare your test case routine in XmlTests.h.
    2. Add an entry to the gs_KuTestCases table in XmlTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
    this file or any other file.

--*/
#pragma once
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#if KTL_USER_MODE
#if !defined(PLATFORM_UNIX)
#include <process.h>
#endif
#endif
#include <ktl.h>
#include <ktrace.h>
#include "XmlTests.h"
#include <CommandLineParser.h>

#if KTL_USER_MODE
#include <ktlevents.um.h>
#else
#include <ktlevents.km.h>
#endif

#if KTL_USER_MODE
    extern volatile LONGLONG gs_AllocsRemaining;
#endif

//** Generated XML file support
struct XmlOutput
{
    KBuffer::SPtr   _Buffer;
    ULONGLONG       _NextOffset;
};

XmlOutput*          gs_GenXmlStr = nullptr;
#define KFPrintf(...)   PrintXmlLine(__VA_ARGS__)

void
PrintXmlLine(XmlOutput* GenXmlStr, CHAR* Format, ...)
{
    va_list         args;
    va_start(args, Format);

    static const ULONG  maxXmlLineSize = 1024;
    static CHAR         dest[maxXmlLineSize];
    NTSTATUS            status;

    #if KTL_USER_MODE
    status = StringCchVPrintfA(STRSAFE_LPSTR(dest), maxXmlLineSize, Format, args);
    #else
    status = RtlStringCchVPrintfA(dest, maxXmlLineSize, Format, args);
    #endif

    KFatal(NT_SUCCESS(status));

    size_t      cb;

    #if KTL_USER_MODE
    status = StringCbLengthA(STRSAFE_LPSTR(dest), maxXmlLineSize, &cb);
    #else
    status = RtlStringCchLengthA(dest, maxXmlLineSize, &cb);
    #endif

    KFatal(NT_SUCCESS(status));

    if ((GenXmlStr->_NextOffset + cb + 1) > GenXmlStr->_Buffer->QuerySize())
    {
        status = GenXmlStr->_Buffer->SetSize(GenXmlStr->_Buffer->QuerySize() * 2, TRUE);
        KFatal(NT_SUCCESS(status));
    }

    KMemCpySafe(
        (UCHAR*)(GenXmlStr->_Buffer->GetBuffer()) + GenXmlStr->_NextOffset, 
        GenXmlStr->_Buffer->QuerySize() - GenXmlStr->_NextOffset, 
        &dest[0], 
        cb + 1);

    GenXmlStr->_NextOffset += cb;
}


//** Simple in-memory XML DOM implementation

//* XML DOM Attribute implementation
class XmlAttribute : public KObject<XmlAttribute>, public KShared<XmlAttribute>
{
    K_FORCE_SHARED(XmlAttribute);

public:
    KWString        _Namespace;
    KWString        _Name;
    KWString        _Value;

public:
    XmlAttribute(
        WCHAR* AttrNs,
        ULONG  AttrNsLength,
        WCHAR* AttrName,
        ULONG  AttrNameLength,
        WCHAR* AttrValue,
        ULONG  AttrValueLength);

    static void
    DisplayAll(KArray<XmlAttribute::SPtr>& Attrs)
    {
        for (ULONG i = 0; i < Attrs.Count(); i++)
        {
            if (Attrs[i]->_Namespace.Length() > 0)
            {
#if !defined(PLATFORM_UNIX)
                KTestPrintf(" %S:%S=\"%S\"", (WCHAR*)(Attrs[i]->_Namespace), (WCHAR*)(Attrs[i]->_Name), (WCHAR*)(Attrs[i]->_Value));
                KFPrintf(gs_GenXmlStr, " %S:%S=\"%S\"", (WCHAR*)(Attrs[i]->_Namespace), (WCHAR*)(Attrs[i]->_Name), (WCHAR*)(Attrs[i]->_Value));
#else
                KTestPrintf(" %s:%s=\"%s\"", Utf16To8((WCHAR*)(Attrs[i]->_Namespace)).c_str(), Utf16To8((WCHAR*)(Attrs[i]->_Name)).c_str(), Utf16To8((WCHAR*)(Attrs[i]->_Value)).c_str());
                KFPrintf(gs_GenXmlStr, " %s:%s=\"%s\"", Utf16To8((WCHAR*)(Attrs[i]->_Namespace)).c_str(), Utf16To8((WCHAR*)(Attrs[i]->_Name)).c_str(), Utf16To8((WCHAR*)(Attrs[i]->_Value)).c_str());
#endif
            }
            else
            {
#if !defined(PLATFORM_UNIX)
                KTestPrintf(" %S=\"%S\"", (WCHAR*)(Attrs[i]->_Name), (WCHAR*)(Attrs[i]->_Value));
                KFPrintf(gs_GenXmlStr, " %S=\"%S\"", (WCHAR*)(Attrs[i]->_Name), (WCHAR*)(Attrs[i]->_Value));
#else
                KTestPrintf(" %s=\"%s\"", Utf16To8((WCHAR*)(Attrs[i]->_Name)).c_str(), Utf16To8((WCHAR*)(Attrs[i]->_Value)).c_str());
                KFPrintf(gs_GenXmlStr, " %s=\"%s\"", Utf16To8((WCHAR*)(Attrs[i]->_Name)).c_str(), Utf16To8((WCHAR*)(Attrs[i]->_Value)).c_str());
#endif
            }
        }
    }
};

XmlAttribute::XmlAttribute(
    WCHAR* AttrNs,
    ULONG  AttrNsLength,
    WCHAR* AttrName,
    ULONG  AttrNameLength,
    WCHAR* AttrValue,
    ULONG  AttrValueLength)
    :   _Namespace(GetThisAllocator()),
        _Name(GetThisAllocator()),
        _Value(GetThisAllocator())
{
    UNICODE_STRING     us;

    us.Buffer = AttrNs;
    KFatal(AttrNsLength <= (MAXUSHORT / sizeof(WCHAR)));
    us.MaximumLength = (us.Length = (USHORT)AttrNsLength * sizeof(WCHAR));
    _Namespace = us;

    us.Buffer = AttrName;
    KFatal(AttrNameLength <= (MAXUSHORT / sizeof(WCHAR)));
    us.MaximumLength = (us.Length = (USHORT)AttrNameLength * sizeof(WCHAR));
    _Name = us;

    us.Buffer = AttrValue;
    KFatal(AttrValueLength <= (MAXUSHORT / sizeof(WCHAR)));
    us.MaximumLength = (us.Length = (USHORT)AttrValueLength * sizeof(WCHAR));
    _Value = us;
}

XmlAttribute::~XmlAttribute()
{
}


//* XML DOM Element implementation
class XmlElement : public KObject<XmlAttribute>, public KShared<XmlAttribute>
{
    K_FORCE_SHARED(XmlElement);

public:
    KWString        _Namespace;
    KWString        _Name;
    KWString        _Value;

    KArray<XmlAttribute::SPtr>  _Attributes;
    KArray<XmlElement::SPtr>    _Children;
    WCHAR*                      _StartElementSection;
    ULONG                       _ElementSectionLength;

public:
    XmlElement(
        WCHAR* ElementNs,
        ULONG  ElementNsLength,
        WCHAR* ElementName,
        ULONG  ElementNameLength,
        WCHAR* StartElementSection);

    void DisplayTabs(ULONG Tabs)
    {
        while (Tabs > 0)
        {
            KTestPrintf("\t");
            KFPrintf(gs_GenXmlStr, "\t");
            Tabs--;
        }
    }

    void DisplayFQName()
    {
        if (_Namespace.Length() > 0)
        {
#if !defined(PLATFORM_UNIX)
            KTestPrintf("%S:", (WCHAR*)_Namespace);
            KFPrintf(gs_GenXmlStr, "%S:", (WCHAR*)_Namespace);
#else
            KTestPrintf("%s:", Utf16To8((WCHAR*)_Namespace).c_str());
            KFPrintf(gs_GenXmlStr, "%s:", Utf16To8((WCHAR*)_Namespace).c_str());
#endif
        }
#if !defined(PLATFORM_UNIX)
        KTestPrintf("%S", (WCHAR*)_Name);
        KFPrintf(gs_GenXmlStr, "%S", (WCHAR*)_Name);
#else
        KTestPrintf("%s", Utf16To8((WCHAR*)_Name).c_str());
        KFPrintf(gs_GenXmlStr, "%s", Utf16To8((WCHAR*)_Name).c_str());
#endif
    }

    void DisplayAll(ULONG Level)
    {
        DisplayTabs(Level);
        KTestPrintf("<");
        KFPrintf(gs_GenXmlStr, "<");
        DisplayFQName();

        if (_Attributes.Count() > 0)
        {
            XmlAttribute::DisplayAll(_Attributes);
        }

        KTestPrintf(">");
        KFPrintf(gs_GenXmlStr, ">");
        if ((_Children.Count() == 0) && (_Value.Length() > 0))
        {
#if !defined(PLATFORM_UNIX)
            KTestPrintf("%S</", (WCHAR*)_Value);
            KFPrintf(gs_GenXmlStr, "%S</", (WCHAR*)_Value);
#else
            KTestPrintf("%s</", Utf16To8((WCHAR*)_Value).c_str());
            KFPrintf(gs_GenXmlStr, "%s</", Utf16To8((WCHAR*)_Value).c_str());
#endif
            DisplayFQName();
            KTestPrintf(">");
            KFPrintf(gs_GenXmlStr, ">");
            return;
        }

        if (_Value.Length() > 0)
        {
#if !defined(PLATFORM_UNIX)
            KTestPrintf("%S", (WCHAR*)_Value);
            KFPrintf(gs_GenXmlStr, "%S", (WCHAR*)_Value);
#else
            KTestPrintf("%s", Utf16To8((WCHAR*)_Value).c_str());
            KFPrintf(gs_GenXmlStr, "%s", Utf16To8((WCHAR*)_Value).c_str());
#endif
        }

        if (_Children.Count() > 0)
        {
            for(ULONG i = 0; i < _Children.Count(); i++)
            {
                KTestPrintf("\n");
                KFPrintf(gs_GenXmlStr, "\n");
                _Children[i]->DisplayAll(Level + 1);
            }

            KTestPrintf("\n");
            KFPrintf(gs_GenXmlStr, "\n");
            DisplayTabs(Level);
        }

        KTestPrintf("</");
        KFPrintf(gs_GenXmlStr, "</");
        DisplayFQName();
        KTestPrintf(">");
        KFPrintf(gs_GenXmlStr, ">");
    }
};

XmlElement::XmlElement(
    WCHAR* ElementNs,
    ULONG  ElementNsLength,
    WCHAR* ElementName,
    ULONG  ElementNameLength,
    WCHAR* StartElementSection)
    :   _Namespace(GetThisAllocator()),
        _Name(GetThisAllocator()),
        _Value(GetThisAllocator()),
        _Attributes(GetThisAllocator()),
        _Children(GetThisAllocator()),
        _StartElementSection(nullptr),
        _ElementSectionLength(0)
{
    UNICODE_STRING     us;

    us.Buffer = ElementNs;
    KFatal(ElementNsLength <= (MAXUSHORT / sizeof(WCHAR)));
    us.MaximumLength = (us.Length = (USHORT)ElementNsLength * sizeof(WCHAR));
    _Namespace = us;

    us.Buffer = ElementName;
    KFatal(ElementNameLength <= (MAXUSHORT / sizeof(WCHAR)));
    us.MaximumLength = (us.Length = (USHORT)ElementNameLength * sizeof(WCHAR));
    _Name = us;

    _StartElementSection = StartElementSection;
}

XmlElement::~XmlElement()
{
}


//** Implementation of KXmlParser::IHook that constructs an in-memory XML DOM
//   using XmlAttribute and XmlElement instances.
class TestHook : public KXmlParser::IHook
{
public:
    KArray<XmlAttribute::SPtr>  _HeaderAttrs;
    XmlElement::SPtr            _DomRoot;


    TestHook()
        :   _Dom(KtlSystem::GlobalNonPagedAllocator()),
            _HeaderAttrs(KtlSystem::GlobalNonPagedAllocator())
    {
    }

    ~TestHook()
    {
    }

    void
    DisplayAll()
    {
        if (_HeaderAttrs.Count() > 0)
        {
            KTestPrintf("<?xml");
            KFPrintf(gs_GenXmlStr, "<?xml");
            XmlAttribute::DisplayAll(_HeaderAttrs);
            KTestPrintf("?>\n");
            KFPrintf(gs_GenXmlStr, "?>\n");
        }

        if (_DomRoot != nullptr)
        {
            _DomRoot->DisplayAll(0);
        }
    }

private:
    KStack<XmlElement::SPtr>    _Dom;

private:    // IHook implementation
    NTSTATUS OpenElement(
        WCHAR* ElementNs,
        ULONG  ElementNsLength,
        WCHAR* ElementName,
        ULONG  ElementNameLength,
        const WCHAR* StartElementSection)
    {
        XmlElement::SPtr    newEle = _new(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator()) XmlElement(
            ElementNs,
            ElementNsLength,
            ElementName,
            ElementNameLength,
            (WCHAR*)StartElementSection);

        KFatal(newEle != nullptr);

        if (_Dom.Count() > 0)
        {
            // Not root node
            KFatal(NT_SUCCESS(_Dom.Peek()->_Children.Append(newEle)));
        }
        KFatal(_Dom.Push(newEle));

        return STATUS_SUCCESS;
    }

    NTSTATUS AddAttribute(
        BOOLEAN HeaderAttributes,    // Set to true if these are the <?xml header attributes
        WCHAR* AttributeNs,
        ULONG  AttributeNsLength,
        WCHAR* AttributeName,
        ULONG  AttributeNameLength,
        WCHAR* Value,
        ULONG  ValueLength)
    {
        XmlAttribute::SPtr  attr = _new(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator()) XmlAttribute(
                                        AttributeNs,
                                        AttributeNsLength,
                                        AttributeName,
                                        AttributeNameLength,
                                        Value,
                                        ValueLength);
        KFatal(attr != nullptr);

        if (HeaderAttributes)
        {
            KFatal(NT_SUCCESS(_HeaderAttrs.Append(Ktl::Move(attr))));
        }
        else
        {
            KFatal(!_Dom.IsEmpty());
            KFatal(NT_SUCCESS(_Dom.Peek()->_Attributes.Append(Ktl::Move(attr))));
        }

        return STATUS_SUCCESS;
    }

    NTSTATUS
    CloseAllAttributes()
    {
        return STATUS_SUCCESS;
    }

    NTSTATUS AddContent(
        ULONG  ContentIndex,
        ULONG  ContentType,
        WCHAR* ContentText,
        ULONG  ContentLength)
    {
        UNREFERENCED_PARAMETER(ContentIndex);
        UNREFERENCED_PARAMETER(ContentType);

        UNICODE_STRING     us;

        us.Buffer = ContentText;
        KFatal(ContentLength <= (MAXUSHORT / sizeof(WCHAR)));
        us.MaximumLength = (us.Length = (USHORT)ContentLength * sizeof(WCHAR));

        KFatal(!_Dom.IsEmpty());
        _Dom.Peek()->_Value = us;

        return STATUS_SUCCESS;
    }

    NTSTATUS
    CloseElement(ULONG ElementSectionLength)
    {
        XmlElement::SPtr    top;
        KFatal(_Dom.Pop(top));

        top->_ElementSectionLength = ElementSectionLength;
        if (_Dom.IsEmpty())
        {
            _DomRoot = Ktl::Move(top);
        }

        return STATUS_SUCCESS;
    }

    NTSTATUS
    Done(BOOLEAN Error)
    {
        UNREFERENCED_PARAMETER(Error);

        return STATUS_SUCCESS;
    }

    BOOLEAN
    ElementWhitespacePreserved()
    {
        // BUG: richhas; xxxxxx; consider adding tests for non-ignore whitespace option
        return FALSE;
    }
};


/*  Basic unit test for KXmlParser.

    This test parses testfile0.xml, a baseline test file, into a simple in-memory DOM (see XmlElement and XmlAttribute)
    thru a simple implementation of KXmlParser.IHook.

    It then displays an XML text stream from that DOM. In UM this output is echoed into a text file on disk.

    In in user mode the test then spawns XmlDiff to compare the original xml file with the generated. Any differences
    will cause failure of the test.
*/
NTSTATUS
XmlBasicTest(int Argc, WCHAR* Args[])
{
    // BUG: richhas; xxxxx; add tests for CDATA
#if !defined(PLATFORM_UNIX)
    KFatal(Argc == 1);
#else
    UNREFERENCED_PARAMETER(Argc);
    UNREFERENCED_PARAMETER(Args);
#endif
    EventRegisterMicrosoft_Windows_KTL();
    KtlSystem::Initialize();

    KFinally([](){
        KtlSystem::Shutdown();
        EventUnregisterMicrosoft_Windows_KTL();
    });

    KTestPrintf("XmlBasicTest: STARTED\n");

    {
        KAllocator&     allocator = KtlSystem::GlobalNonPagedAllocator();
        NTSTATUS        status = STATUS_SUCCESS;
        KBuffer::SPtr   xmlDocBuffer;

#if !defined(PLATFORM_UNIX)
        WCHAR           driveStr[]  = L"\\??\\x:";
                        driveStr[4] = *Args[0];
        KWString        fileName(allocator, driveStr);
                        fileName += L"\\temp\\Ktl\\XmlTest\\testfile0.xml";
#else
        KWString        fileName(allocator, L"/tmp/TestFile0.xml");
#endif
        KWString        genFileName(fileName);
                        genFileName += L".XmlBasicTest.xml";

        //* Read in the reference XML file; converting if needed to unicode; then extract the entire
        //  file into xmlDocBuffer.
        {
            KTextFile::SPtr testFile;
            status = KTextFile::OpenSynchronous(fileName, 10000, testFile, KTL_TAG_TEST, &allocator);
            if (!NT_SUCCESS(status))
            {
                KTestPrintf("XmlBasicTest: OpenSynchronous failed @%u with 0x%08X\n", __LINE__, status);
                return status;
            }

            status = testFile->ReadBinary(xmlDocBuffer);
            if (!NT_SUCCESS(status))
            {
                KTestPrintf("XmlBasicTest: ReadBinary failed @%u with 0x%08X\n", __LINE__, status);
                return status;
            }
        }

        //* Parse xmlDocBuffer into an in-memory DOM. Then display that DOM via DbgPrintf (and into a file in UM).
        {
            KFatal(xmlDocBuffer.RawPtr() != nullptr);

            TestHook            testHook;
            KXmlParser::SPtr    iut;

            status = KXmlParser::Create(iut, allocator, KTL_TAG_TEST);
            if (!NT_SUCCESS(status))
            {
                KTestPrintf("XmlBasicTest: Create failed @%u with 0x%08X\n", __LINE__, status);
                return status;
            }

            status = iut->Parse((WCHAR*)xmlDocBuffer->GetBuffer(), xmlDocBuffer->QuerySize() / sizeof(WCHAR), testHook);
            if (!NT_SUCCESS(status))
            {
                KTestPrintf("XmlBasicTest: Parse failed @%u with 0x%08X\n", __LINE__, status);
                return status;
            }

            // BUG: richhas/jhavens, xxxxx; add text output to KTextFile (both KM/UM). We alos need a means to validate
            //                              KM generated files like this. Updating the unit test FW to support the notion
            //                              of post test validation steps (executed in user mode) would be a good thing.

            // In user mode:
            //      Create generated xml file: fileName + "XmlBasicTest.xml"
            XmlOutput           outputXml;
            status = KBuffer::Create(1024, outputXml._Buffer, allocator);
            KFatal(NT_SUCCESS(status));
            gs_GenXmlStr = &outputXml;
            outputXml._NextOffset = 0;

            testHook.DisplayAll();

            // Create and write the generated xml file
            HANDLE              genXmlFile = nullptr;
            OBJECT_ATTRIBUTES   objAttributes;
            IO_STATUS_BLOCK     ioStatus;
            ULONG               attributes = OBJ_CASE_INSENSITIVE;

            InitializeObjectAttributes(&objAttributes, genFileName, attributes, NULL, NULL);

            status = KNt::CreateFile(
                &genXmlFile,
                FILE_GENERIC_WRITE,
                &objAttributes,
                &ioStatus,
                nullptr,
                FILE_ATTRIBUTE_NORMAL,
                0,
                FILE_SUPERSEDE,
                FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                nullptr,
                0);
            if (!NT_SUCCESS(status))
            {
                KTestPrintf("XmlBasicTest: CreateFile failed @%u with 0x%08X\n", __LINE__, status);
                return status;
            }

            KFatal(outputXml._NextOffset <= MAXULONG);

            status = KNt::WriteFile(
                genXmlFile,
                nullptr,
                nullptr,
                nullptr,
                &ioStatus,
                outputXml._Buffer->GetBuffer(),
                (ULONG)(outputXml._NextOffset),
                nullptr,
                nullptr);
            if (!NT_SUCCESS(status))
            {
                KTestPrintf("XmlBasicTest: CreateFile failed @%u with 0x%08X\n", __LINE__, status);
                return status;
            }

            KFatal(NT_SUCCESS(KNt::Close(genXmlFile)));
            genXmlFile = nullptr;

            #if CONSOLE_TEST
                // Pass the reference and post parse generated xml files to WinDiff to validate they are eqv
                KWString        xmlDiffFilePath(allocator);
                xmlDiffFilePath += L".\\xmldiff.exe";

            #if !defined(PLATFORM_UNIX)
                intptr_t spawnStatus = _wspawnl(
                    _P_WAIT,
                    xmlDiffFilePath,
                    xmlDiffFilePath,
                    ((WCHAR*)fileName) + 4,                 // Don't pass FQN prefixes to CLR
                    ((WCHAR*)genFileName) + 4,
                    NULL);
            #else
                    intptr_t spawnStatus = 0;
            #endif
                if (spawnStatus == -1)
                {
                    KTestPrintf("XmlBasicTest: _wspawnl failed @%u with errno: 0x%04x\n", __LINE__, errno);
                    return STATUS_UNSUCCESSFUL;
                }

                if (spawnStatus != 0)
                {
                    KTestPrintf("XmlBasicTest: XmlDiff failed @%u with: 0x%08x\n", __LINE__, spawnStatus);
                    return STATUS_UNSUCCESSFUL;
                }
            #endif
        }
    }

    KTestPrintf("XmlBasicTest: COMPLETED\n");
    return STATUS_SUCCESS;
}


#if CONSOLE_TEST
#if !defined(PLATFORM_UNIX)
int __cdecl
wmain(__in int argc, __in_ecount(argc) WCHAR* args[])
{
    NTSTATUS    status = STATUS_INVALID_PARAMETER_MIX;
    if (argc == 0)
    {
        WCHAR*          a[] = {L"C:"};
        status = XmlBasicTest(1, a);
    }
    else
    {
        argc--;
        args++;

        if (_wcsicmp(args[0], L"XmlBasicTest") == 0)
        {
            argc--;
            args++;

            if (argc != 0)
            {
                status = XmlBasicTest(argc, args);
            }
        }

        else if (_wcsicmp(args[0], L"DomBasicTest") == 0)
        {
            status = DomBasicTest(argc, args);
        }
    }

    return RtlNtStatusToDosError(status);
}
#else
int main(int argc, char* cargs[])
{
    NTSTATUS status;

#if defined(PLATFORM_UNIX)
    status = KtlTraceRegister();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Failed to KtlTraceRegister\n");
        return(status);
    }
#endif
    
    status = XmlBasicTest(0, nullptr);
    KFatal(NT_SUCCESS(status));

    status = DomBasicTest(0, nullptr);
    KFatal(NT_SUCCESS(status));

#if defined(PLATFORM_UNIX)
    KtlTraceUnregister();
#endif  
    
    return 0;
}
#endif
#endif
