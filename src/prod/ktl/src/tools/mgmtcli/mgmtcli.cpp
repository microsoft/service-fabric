

#include <ktl.h>

#include <string.h>
#include <stdio.h>
#include <varargs.h>
#include <objbase.h>
#include <msxml6.h>

#include <CommandLineParser.h>

// Platform layer

#include <string>
#include <vector>
#include <memory>

using namespace std;

#define STD_TIMEOUT     10000

KAllocator* g_Allocator = nullptr;


LPWSTR GetTemplate =

L"<KtlHttpMessage"
L"    xmlns=\"http://schemas.microsoft.com/2012/03/rvd/adminmessage\"\n"
L"    xmlns:ktl=\"http://schemas.microsoft.com/2012/03/ktl\"\n"
L"    xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
L"    xmlns:xs=\"http://www.w3.org/2001/XMLSchema\"\n"
L"   >\n"
L"  <MessageHeader>\n"
L"    <RequestMessageHeader>\n"
L"      <To xsi:type=\"ktl:URI\">%1</To>\n"
L"      <ReplyTo xsi:type=\"ktl:URI\">ktl.convention:/replyToSender</ReplyTo>\n"
L"      <From xsi:type=\"ktl:URI\">%2</From>\n"
L"      <MessageId xsi:type=\"ktl:GUID\">%3</MessageId>\n"
L"      <OperationTimeout xsi:type=\"xs:duration\">PT30S</OperationTimeout>\n"
L"      <SentTimestamp xsi:type=\"ktl:DATETIME\">%4</SentTimestamp>\n"
L"      <ActivityId xsi:type=\"ktl:ULONGLONG\">0</ActivityId>\n"
L"      <OperationVerb xsi:type=\"ktl:URI\">ktl.verb:/Get</OperationVerb>\n"
L"      <ObjectUri xsi:type=\"ktl:URI\">%5</ObjectUri>\n"
L"    </RequestMessageHeader>\n"
L"  </MessageHeader>\n"
L"</KtlHttpMessage>\n";


NTSTATUS
GetXmlStringFromTemplate(
    __in LPCWSTR NetUrl,
    __in LPCWSTR ObjUri,
    __out KString::SPtr& NewStr)
{
    KString::SPtr Tmp = KString::Create(*g_Allocator, 8192);
    if (!Tmp)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    wchar_t CompName[MAX_PATH];
    *CompName = 0;
    DWORD Size = MAX_PATH;
    GetComputerName(CompName, &Size);

    ULONG ReplacementCount = 0;

    KGuid Guid;
    Guid.CreateNew();
    wchar_t GuidStr[128];
    KStringView GuidView(GuidStr, 128);
    GuidView.FromGUID(Guid);

    Tmp->CopyFrom(KStringView(GetTemplate));
    Tmp->Replace(KStringView(L"%1"), KStringView(NetUrl), ReplacementCount);
    Tmp->Replace(KStringView(L"%2"),  KStringView(CompName), ReplacementCount);
    Tmp->Replace(KStringView(L"%3"),  GuidView, ReplacementCount);

    KDateTime tm = KDateTime::Now();
    KString::SPtr TimeStr;
    tm.ToString(*g_Allocator, TimeStr);

    Tmp->Replace(KStringView(L"%4"), *TimeStr, ReplacementCount);
    Tmp->Replace(KStringView(L"%5"), KStringView(ObjUri), ReplacementCount);

    Tmp->EnsureBOM();

    NewStr = Tmp;

    printf("XML=\n%S\n", LPWSTR(*NewStr));

    return STATUS_SUCCESS;
}

static KAutoResetEvent* pResponseEvent = nullptr;

VOID
GetOpCallback(
   __in KAsyncContextBase* const Parent,
   __in KAsyncContextBase& Op
   )
{
    UNREFERENCED_PARAMETER(Parent);
    UNREFERENCED_PARAMETER(Op);

    // Called at first phase.
    //
    pResponseEvent->SetEvent();
}


NTSTATUS GetViaPost(
    __in LPCWSTR HttpUrl,
    __in LPCWSTR ObjUri
    )
{
    KHttpClient::SPtr Client;
    NTSTATUS Result;

    KStringView name(L"KTL HTTP Test Agent");
    Result = KHttpClient::Create(name, *g_Allocator, Client);
    if (!NT_SUCCESS(Result))
    {
        return Result;
    }

    // Convert Url to a network endpoint.
    //
    KNetworkEndpoint::SPtr Ep;
    KUriView url(HttpUrl);
    NTSTATUS Res = KHttpNetworkEndpoint::Create(url, *g_Allocator, Ep);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }


    // Create a new request and address
    //
    KHttpCliRequest::SPtr Req;
    Res = Client->CreateRequest(Req);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    Res = Req->SetTarget(Ep, KHttpUtils::eHttpPost);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    // Now get filled out "Get" template

    KString::SPtr Str;
    Res = GetXmlStringFromTemplate(HttpUrl, ObjUri, Str);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }


    Res = Req->SetContent(*Str);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    KStringView content(L"text/xml; charset=\"utf-16\"");
    Res = Req->AddHeaderByCode(KHttpUtils::HttpHeaderContentType, content);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }


    // Execute and block until the response is incoming
    //
    KHttpCliRequest::Response::SPtr Resp;

    KAutoResetEvent ResponseEvent;
    pResponseEvent = &ResponseEvent;

    Res = Req->Execute(Resp, nullptr, GetOpCallback);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    ResponseEvent.WaitUntilSet();


    // When here, we have a response or timeout
    //
    ULONG Code;
    NTSTATUS Status;

    Status = Resp->Status();

    if (!NT_SUCCESS(Status))
    {
        printf("Operation failed with 0x%X\n", Status);
    }

    Code = Resp->GetHttpResponseCode();

    printf("HTTP Response code is %u\n", Code);

    KString::SPtr ContentTypeHeader;
    Resp->GetHeaderByCode(KHttpUtils::HttpHeaderContentType, ContentTypeHeader);

    ULONG ContLength = Resp->GetContentLengthHeader();

    printf("Content type is        %S\n",   ContentTypeHeader ? PWSTR(*ContentTypeHeader) : L"(null)");
    printf("Content Length is      %u\n", ContLength);
    printf("Entity Body Length is  %u\n", Resp->GetEntityBodyLength());


    KString::SPtr AllHeaders;
    Resp->GetAllHeaders(AllHeaders);
    if (AllHeaders)
    {
        printf("\n-------------Begin headers:\n%S\n", PWSTR(*AllHeaders));
    }
    printf("\n--------End headers------------\n");

    KBuffer::SPtr Content;
    Status = Resp->GetContent(Content);
    if (!NT_SUCCESS(Status) && ContLength > 0)
    {
        printf("Failed to get content\n");
        return Status;
    }

    ULONG Pos;
    if (ContentTypeHeader->Search(KStringView(L"utf-16"), Pos))
    {
        LPCWSTR Buf = (LPCWSTR) Content->GetBuffer();
        Buf++;  // Move past BOM
        printf("Content (UTF-16):\n%S\n",  Buf);
    }
    else
    {
        printf("Content: (UTF-8)\n%s\n",  (char*)Content->GetBuffer());
    }

    return STATUS_SUCCESS;
}




NTSTATUS
Execute(
    __in int argc,
    __in wchar_t** args
    )
{
    CmdLineParser Parser(*g_Allocator);
    Parameter *Param;

    if (Parser.Parse( argc, args))
    {
        if (!Parser.ParameterCount())
        {
            printf("Missing parameters\n");
            return STATUS_UNSUCCESSFUL;
        }
        Param = Parser.GetParam(0);

        if (wcscmp(Param->_Name, L"getpost") == 0)
        {
            if (Param->_ValueCount != 2)
            {
                printf("Error: parameter count\n");
                return STATUS_UNSUCCESSFUL;
            }

            LPCWSTR HttpUrl = (wchar_t *) Param->_Values[0];
            LPCWSTR ObjUri = (wchar_t *) Param->_Values[1];

            printf("Executing a Get via POST to %S for object %S\n", HttpUrl, ObjUri);

            NTSTATUS Res = GetViaPost(HttpUrl, ObjUri);
            return   Res;
        }
        else if (wcscmp(Param->_Name, L"get") == 0)
        {
            if (Param->_ValueCount != 1)
            {
                printf("Error: parameter count\n");
                return STATUS_UNSUCCESSFUL;
            }
            LPWSTR HttpUrl = (wchar_t *) Param->_Values[0];

            printf("Executing an HTTP Get for %S\n", HttpUrl);
        }
    }
    printf("Invalid command line.\n");
    return STATUS_UNSUCCESSFUL;
}

int
wmain(int argc, wchar_t** args)
{
    NTSTATUS Result;

    printf("\n Pavilion Management Test Client [v1.01]\n\n");

    if (argc < 2)
    {
        printf("Usage:\n"
            "  -getpost     httpURL ObjUri      // Using POST, submits a GET request to the specified HTTP server and retrieves the object indicated via ObjUri\n"
            "  -get         httpURLwithUri      // Retrieve the item using a single large URL\n"
            "\n");
        return -1;
    }

    Result = KtlSystem::Initialize();
    if (!NT_SUCCESS(Result))
    {
        printf("KtlSystem::Initializer() failure\n");
        return Result;
    }
    KFinally([&](){ KtlSystem::Shutdown(); });
    g_Allocator = &KtlSystem::GlobalNonPagedAllocator();

    // Initialize COM for ATL

    HRESULT Status = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (! SUCCEEDED(Status))
    {
        printf("CoInitializeEx failure\n");
        return STATUS_UNSUCCESSFUL;
    }

    ULONGLONG StartingAllocs = KAllocatorSupport::gs_AllocsRemaining;

    if (argc > 0)
    {
        argc--;
        args++;
    }

    Result = Execute(argc, args);

    ULONGLONG Leaks = KAllocatorSupport::gs_AllocsRemaining - StartingAllocs;
    if (Leaks)
    {
        printf("Total memory leaks = %I64u\n", Leaks);
        Result = STATUS_UNSUCCESSFUL;
    }
    else
    {
        printf("No leaks.\n");
    }

    CoUninitialize();

    return Result;
}



