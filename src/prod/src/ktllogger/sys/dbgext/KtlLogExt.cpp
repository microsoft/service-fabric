// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include <stdio.h>
#include <windows.h>

#include <EngExtCpp.hpp>
#include <string>
#include <memory>
#include <strsafe.h>

//
// The singleton EXT_CLASS class.
//

class EXT_CLASS : public ExtExtension
{
public:
    EXT_COMMAND_METHOD(dumpfot);

private:    

    VOID DumpFOTObject(
        __in ULONG64 FotEntry,
        __in ULONG LinksOffset
        );
    
    HRESULT
    GetVtableAddressForClass(
        __in PCSTR Class,
        __out PULONG64 VtableAddress
    );
    
};

//
// EXT_DECLARE_GLOBALS must be used to instantiate
// the framework's assumed globals.
//
EXT_DECLARE_GLOBALS();

VOID EXT_CLASS::DumpFOTObject(
    __in ULONG64 FotEntry,
    __in ULONG LinksOffset
    )
{
    std::string typeSymbolName("(FileObjectEntry*)@$extin");
    ExtRemoteTyped entry(typeSymbolName.c_str(), FotEntry);
    
    std::string objectPtrName("_ObjectPtr");
    ExtRemoteTyped object = entry.Field(objectPtrName.c_str());
    
    ULONG typeIndex = (object.Field("_ObjectType")).GetUlong();
    static PWCHAR typeNames[4] = { L"*NullObject*", L"KtlLogManagerKernel", L"KtlLogContainerKernel", L"KtlLogStreamKernel" };
    PWCHAR typeName;

    if (typeIndex > 3)
    {
        typeName = L"*Unknown*";
    } else {
        typeName = typeNames[typeIndex];
    }
    
    Out("%p : %p %ws\n", FotEntry, (object.Field("_ObjectId")).GetUlong64(), typeName);

    std::string tableEntryName("_TableEntry");
    ExtRemoteTyped tableEntry = entry.Field(tableEntryName.c_str());
    
    std::string leftOffsetName("_LeftOffset");
    ULONG64 left = tableEntry.Field(leftOffsetName.c_str()).GetPtr();
    
    std::string rightOffsetName("_RightOffset");
    ULONG64 right = tableEntry.Field(rightOffsetName.c_str()).GetPtr();

    // TODO: don't be so recursive
    if (left != 0)
    {
        DumpFOTObject(left - LinksOffset, LinksOffset);
    }

    if (right != 0)
    {
        DumpFOTObject(right - LinksOffset, LinksOffset);
    }
}

EXT_COMMAND(dumpfot,
            "Dump all entries in a file object table",
            "{;e;address;File Object Table address}"
           )
{
    ULONG64 objectAddress = GetUnnamedArgU64(0);

    std::string typeSymbolName("(FileObjectTable*)@$extin");
    ExtRemoteTyped object(typeSymbolName.c_str(), objectAddress);

    Out("File object table %p\n", objectAddress);
    
    std::string logManagerName("_LogManager");
    ExtRemoteTyped logManagerSptr = object.Field(logManagerName.c_str());
    
    std::string proxyName("_Proxy");
    ExtRemoteTyped logManager = logManagerSptr.Field(proxyName.c_str());
    
    Out("  KtlLogManagerKernel %p\n", logManager.GetPtr());

    Out("\n");
    
    std::string tableName("_Table");
    ExtRemoteTyped table = object.Field(tableName.c_str());
        
    std::string baseTableName("_BaseTable");
    ExtRemoteTyped baseTable = table.Field(baseTableName.c_str());
    
    std::string countName("_Count");
    ExtRemoteTyped baseTableCount = baseTable.Field(countName.c_str());
    Out("_Count %d\n", baseTableCount.GetUlong());
    
    std::string linksOffsetName("_LinksOffset");
    ULONG linksOffset = (table.Field(linksOffsetName.c_str())).GetUlong();
    Out("_Links 0x%x\n", linksOffset);
    
    std::string rootName("_Root");
    ULONG64 tableEntryAddress = baseTable.Field(rootName.c_str()).GetPtr();

//    Out("Table at %p has %d objects. Root %p, LinksOffset %d\n", baseTable.GetPtr(), baseTableCount.GetUlong(), tableEntryAddress, linksOffset);

    DumpFOTObject(tableEntryAddress - linksOffset, linksOffset);
}

HRESULT
EXT_CLASS::GetVtableAddressForClass(
    __in PCSTR Class,
    __out PULONG64 VtableAddress
)
{
    HRESULT hr;
    
    if (strcmp(Class, "0") != 0)
    {
        CHAR vtableSymbolName[512];
        
        StringCchCopy(vtableSymbolName, 512, Class);
        StringCchCat(vtableSymbolName, 512, "::`vftable'");

        hr = m_Symbols->GetOffsetByName(vtableSymbolName, VtableAddress);
        if (FAILED(hr))
        {
            Out("GetOffsetByName %s : failed %x\n\n", vtableSymbolName, hr);
            return(hr);
        }
    } else {
        *VtableAddress = 0;
        hr = S_OK;
    }

    return(hr);
}

