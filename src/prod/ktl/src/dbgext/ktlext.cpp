/*++

Copyright (c) 2011  Microsoft Corporation

Module Name:

    Ktlext.cpp

Abstract:

    This file implements a debugger extension for KTL. 

Author:

    Peng Li (pengli) 24-March-2011

Environment:

    User mode

Notes:

Revision History:

--*/

#include <stdio.h>
#include <windows.h>

#include <EngExtCpp.hpp>
#include <string>
#include <memory>
#include <map>
#include <list>
#include <strsafe.h>

//
// The singleton EXT_CLASS class.
//

class EXT_CLASS;
typedef void (*KHeapCallback)(__in EXT_CLASS *ExtClass, __in ULONG64 LogStreamAddress, __in PVOID Context);
VOID DumpLogStreamCallback(
    __in EXT_CLASS *ExtClass,
    __in ULONG64 LogStreamAddress,
    __in PVOID Context
);


class EXT_CLASS : public ExtExtension
{
public:
    EXT_COMMAND_METHOD(listsptr);
    EXT_COMMAND_METHOD(walkcontext);
    EXT_COMMAND_METHOD(dumpkdom);    
    EXT_COMMAND_METHOD(dumpkvar);        
    EXT_COMMAND_METHOD(findalloc);        
    EXT_COMMAND_METHOD(findlogstreams);        
    EXT_COMMAND_METHOD(dumpstring);
    EXT_COMMAND_METHOD(findasync);        
    EXT_COMMAND_METHOD(asyncchildren);        
    EXT_COMMAND_METHOD(ktlsystemlist);        

private:    

    void 
    DumpKDomQName(
        __in ExtRemoteTyped& QName
        );

    void 
    DumpKHeap(
        __in ExtRemoteTyped& Allocator, 
        __in ULONG64 VtableAddress
        );

    void 
    DumpKHeap(
        __in ExtRemoteTyped& Allocator, 
        __in ULONG64 VtableAddress,
        __in KHeapCallback Callback,
        __in PVOID Context
         );

    void 
    DumpKVariant(
        __in ExtRemoteTyped& Variant
        );

    HRESULT
    GetVtableAddressForClass(
        __in PCSTR Class,
        __out PULONG64 VtableAddress
    );

    void
    DumpAsyncChildren(
        ULONG64 KtlSystemAddress,
        ULONG64 ParentAsyncAddress,
        ULONG64 IncludeIdle,
        ULONG64 VtableAddress,
        ULONG IndentSpaces
      );
    
};

    

//
// EXT_DECLARE_GLOBALS must be used to instantiate
// the framework's assumed globals.
//
EXT_DECLARE_GLOBALS();

EXT_COMMAND(listsptr,
            "List all reference allocation addresses for a KShared object of type T",
            "{;e;address;KShared object address}"
            "{;s;type name;Type name of T}")
{
    ULONG64 objectAddress = GetUnnamedArgU64(0);
    PCSTR typeName = GetUnnamedArgStr(1);
    
    //
    // Build a symbol name from the type.
    // For example, if the type is KAsyncContextBase, the symbol name 
    // is "(KAsyncContextBase*)@$extin".
    //
    std::string typeSymbolName("(");
    typeSymbolName += typeName;
    typeSymbolName += "*)@$extin";

    //
    // Build a KSharedPtr<> type name.
    // For example, if the type is KAsyncContextBase, 
    // sptrTypeName is "KSharedPtr<KAsyncContextBase>".
    //
    std::string sptrTypeName("KSharedPtr<");
    sptrTypeName += typeName;
    sptrTypeName += ">";

    ExtRemoteTyped object(typeSymbolName.c_str(), objectAddress);
    
    Out("Listing reference allocation addresses for %s at %p\n",
        typeName, objectAddress);
    Out("Ref count = %d\n\n", object.Field("_RefCount").GetLong());

    static const char sptrListFieldName[] = "_SharedPtrList";
    static const char linkageFieldName[] = "_Linkage";
    static const char allocationAddressFieldName[] = "_AllocationAddress";    

    bool hasField = object.HasField(sptrListFieldName);
    if (!hasField)
    {
        Out("Field %s does not exist. It is not a DBG build. \n", sptrListFieldName);
        return;
    }

    char fileName[MAX_PATH] = "Unknown file";
    ULONG line;        
    
    ExtRemoteTyped listHead = object.Field(sptrListFieldName);
    ExtRemoteTypedList sptrList(listHead,
                                sptrTypeName.c_str(),   // Type
                                linkageFieldName,       // LinkField
                                0,                      // TypeModeBase
                                0,                      // TypeId
                                nullptr,                // CacheCookie
                                true                    // Double
                                );

    ULONG count = 0;
    
    for (sptrList.StartHead(); sptrList.HasNode(); sptrList.Next())
    {
        count++;
        
        ExtRemoteTyped node = sptrList.GetTypedNode();
        ExtRemoteTyped allocAddress = node.Field(allocationAddressFieldName);

        Out("------------------------\n\n");

        Out("%s address: %p\n", sptrTypeName.c_str(), node.GetPointerTo().GetPtr());
        Out("Allocation address: %p\n", allocAddress.GetPtr());

        HRESULT hr = m_Symbols->GetLineByOffset(
            allocAddress.GetPtr(),
            &line,
            fileName,
            ARRAYSIZE(fileName),
            nullptr,
            nullptr
            );
        if (FAILED(hr))
        {
            Out("No file and line information: %x\n", hr);
        }
        else
        {
            Out("File: %s\n", fileName);
            Out("Line: %d\n\n", line);                            
        }                                
    }

    Out("------------------------\n\n");

    Out("Number of tracked %s objects: %d \n", sptrTypeName.c_str(), count);
    Out("Number of manual AddRef(): %d \n", object.Field("_RefCount").GetLong() - count);    
}

EXT_COMMAND(walkcontext,
            "Display the parent chain of AsyncContext blocks and their callback delegates.",
            "{;e;address;KAsyncContextBase}" )
{
    ULONG64 objectAddress = GetUnnamedArgU64(0);
    
    //
    // Build a symbol name from the type.
    //

    std::string typeSymbolName("(KAsyncContextBase*)@$extin");

    ExtRemoteTyped object(typeSymbolName.c_str(), objectAddress);
    
    Out("Parent Context\t    Callback Object\tCallback Function\n");

    static const char parentFieldName[] = "_ParentAsyncContextPtr";
    static const char callbackFieldName[] = "_CallbackPtr";

    char functionName[MAX_PATH] = "";
    ExtRemoteTyped callback;
        
    do
    {
        ExtRemoteTyped parent = object.Field("_ParentAsyncContextPtr._Proxy");
        ExtRemoteTyped nextObject = object.Field("_CallbackPtr._Obj" );
        callback = object.Field("_CallbackPtr._U._MethodPtr" );
    
        parent.OutSimpleValue(); Out(" ");
        nextObject.OutSimpleValue(); Out(" ");
    
        HRESULT hr = m_Symbols->GetNameByOffset(
            callback.GetPtr(),
            functionName,
            ARRAYSIZE(functionName),
            nullptr,
            nullptr
            );
    
        if (FAILED(hr))
        {
            callback.OutSimpleValue();
        }
        else
        {
            Out(functionName);                            
        }       
        
        Out("\n");                         

        object = parent;
    }
    while( object.GetPtr() != 0);

}

//
// This method dumps the content of a NamedDomValue::FQN object.
//

void 
EXT_CLASS::DumpKDomQName(
    __in ExtRemoteTyped& QName
    )
{
    std::unique_ptr<WCHAR[]> buffer;    
    ULONG size = 0;
    
    ExtRemoteTyped nameSpace = QName.Field("_Namespace._Proxy._Data");
    size = QName.Field("_Namespace._Proxy._Size").GetUshort();

    buffer.reset(new WCHAR[size + 1]);
    Out("Namespace: %S\n", nameSpace.GetString(buffer.get(), size + 1, size, false));    

    ExtRemoteTyped name = QName.Field("_Name._Proxy._Data");
    size = QName.Field("_Name._Proxy._Size").GetUshort();    

    buffer.reset(new WCHAR[size + 1]);
    Out("Name: %S\n", name.GetString(buffer.get(), size + 1, size, false));
}

//
// This method dumps the content of a KVariant object.
//

void 
EXT_CLASS::DumpKVariant(
    __in ExtRemoteTyped& Variant
    )
{
    ULONG type = Variant.Field("_Type").GetUlong();
    ExtRemoteTyped value = Variant.Field("_Value");
    switch (type)
    {
        case 0:
        {
            Out("Type: Type_ERROR\n");
            Out("Value: 0x%08x\n", value.Field("_Status").GetLong());
            break;
        }

        case 1:
        {
            Out("Type: Type_EMPTY\n");
            Out("Value: N/A\n");
            break;
        }

        case 2:
        {
            Out("Type: Type_BOOLEAN\n");
            Out("Value: %s\n", value.Field("_BOOLEAN_Val").GetBoolean() ? "TRUE" : "FALSE");
            break;
        }
            
        case 3:
        {
            Out("Type: Type_LONG\n");
            Out("Value: 0x%08x\n", value.Field("_LONG_Val").GetLong());
            break;
        }

        case 4:
        {
            Out("Type: Type_ULONG\n");
            Out("Value: 0x%08x\n", value.Field("_ULONG_Val").GetUlong());
            break;
        }

        case 5:
        {
            Out("Type: Type_LONGLONG\n");
            Out("Value: 0x%I64x\n", value.Field("_LONGLONG_Val").GetLong64());
            break;
        }
        
        case 6:
        {
            Out("Type: Type_ULONGLONG\n");
            Out("Value: 0x%I64x\n", value.Field("_ULONGLONG_Val").GetUlong64());
            break;
        }

        case 7:
        {
            Out("Type: Type_GUID\n");
            
            GUID guid;
            value.Field("_GUID_Val").ReadBuffer(&guid, sizeof(guid), true);   

            Out(
                "Value: {%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                guid.Data1, guid.Data2, guid.Data3, 
                guid.Data4[0], guid.Data4[1],
                guid.Data4[2], guid.Data4[3],
                guid.Data4[4], guid.Data4[5],
                guid.Data4[6], guid.Data4[7]
                );
            break;
        }
        
        case 8:
        {
            Out("Type: Type_KMemRef\n");
            Out(
                "Value: {_Size = %d; _Param = %d; _Address = %p}\n", 
                value.Field("_KMemRef_Val._Size").GetUlong(),
                value.Field("_KMemRef_Val._Param").GetUlong(),                
                value.Field("_KMemRef_Val._Address").GetPtr()
                );
            break;
        }
        
        case 9:
        {
            Out("Type: Type_KDateTime\n");
            Out("Value: 0x%I64x\n", value.Field("_LONGLONG_Val").GetLong64());            
            break;
        }

        case 10:
        {
            Out("Type: Type_KDuration\n");
            Out("Value: 0x%I64x\n", value.Field("_LONGLONG_Val").GetLong64());            
            break;
        }
        
        case 0x80000000:
        {
            Out("Type: Type_SPtr\n");
            Out("Value: KShared<> pointer = %p\n", value.Field("_LONGLONG_Val").GetPtr());
            break;
        }

        case 0x80000002:
        {
            Out("Type: Type_KBuffer_SPtr\n");
            Out("Value: KBuffer pointer = %p\n", value.Field("_LONGLONG_Val").GetPtr());
            break;
        }                    

        case 0x80000003:
        {
            Out("Type: Type_KString_SPtr\n");

            //
            // Get the _Proxy value. _Proxy field is always at the beginning of a KSharedPtr<> object. 
            // KVariant._Value is a union of LONGLONG and KSharedPtr<>. This is essentially type casting.
            //

            if (value.Field("_LONGLONG_Val").GetPtr() == 0)
            {
                Out("Value: <NULL string>\n");
                break;
            }
            
            ExtRemoteTyped stringValue("(KString*)@$extin", value.Field("_LONGLONG_Val").GetPtr());

            ExtRemoteTyped sizeObject = stringValue.Field("_LenInChars");
            ULONG size = sizeObject.GetUlong();

            //
            // The real string buffer is pointed to by _Buffer. Since we will make a copy, untyped ExtRemoteData 
            // is fine.
            //
            ExtRemoteData bufferObject(stringValue.Field("_Buffer").GetPtr(), size * 2);

            std::unique_ptr<WCHAR[]> buffer(new WCHAR[size + 1]);
            bufferObject.ReadBuffer(buffer.get(), (size + 1)*2, true);
            *(buffer.get() + size) = 0;

            Out("Value: %S\n", buffer.get());                        
            break;
        }                    

        case 0x80000004:
        {
            Out("Type: Type_KUri_SPtr\n");

            //
            // Get the _Proxy value. _Proxy field is always at the beginning of a KSharedPtr<> object. 
            // KVariant._Value is a union of LONGLONG and KSharedPtr<>. This is essentially type casting.
            //

            if (value.Field("_LONGLONG_Val").GetPtr() == 0)
            {
                Out("Value: <NULL string>\n");
                break;
            }
            
            ExtRemoteTyped stringValue("(KUri*)@$extin", value.Field("_LONGLONG_Val").GetPtr());

            ExtRemoteTyped sizeObject = stringValue.Field("_src_Raw._LenInChars");
            ULONG size = sizeObject.GetUlong();

            //
            // The real string buffer is pointed to by _Buffer. Since we will make a copy, untyped ExtRemoteData 
            // is fine.
            //
            ExtRemoteData bufferObject(stringValue.Field("_src_Raw._Buffer").GetPtr(), size * 2);

            std::unique_ptr<WCHAR[]> buffer(new WCHAR[size + 1]);
            bufferObject.ReadBuffer(buffer.get(), (size + 1)*2, true);
            *(buffer.get() + size) = 0;

            Out("Value: %S\n", buffer.get());            
            break;
        }                            

        case 0x80000005:
        {
            Out("Type: Type_KIMutableDomNode_SPtr\n");

            //
            // Get the _Proxy value. _Proxy field is always at the beginning of a KSharedPtr<> object. 
            // KVariant._Value is a union of LONGLONG and KSharedPtr<>. This is essentially type casting.
            //            

            Out("Value: KIMutableDomNode pointer = %p\n", value.Field("_LONGLONG_Val").GetPtr());
            break;
        }                                    

        default:
        {
            Out("Type: 0x%08x\n", type);
            break;
        }                            
    }
}

EXT_COMMAND(dumpkdom,
            "Dump the content of a KIDomNode, KIMutableDomNode, or DomNode object",
            "{;e;address;address of a KIDomNode, KIMutableDomNode, or DomNode object}"
            "{;s;type;type name of the object, KIDomNode, KIMutableDomNode, or DomNode}")
{
    ULONG64 objectAddress = GetUnnamedArgU64(0);
    PCSTR typeName = GetUnnamedArgStr(1);    

    //
    // Get the backing DomNode object.
    //
    
    ExtRemoteTyped backingNode;    
    if (_stricmp(typeName, "KIDomNode") == 0 || _stricmp(typeName, "KIMutableDomNode") == 0)
    {
        //
        // Both KIDomNode and KIMutableDomNode are implemented by DomNodeApi.
        //
        
        ExtRemoteTyped dom("(DomNodeApi*)@$extin", objectAddress);        
        backingNode = dom.Field("_BackingNode");
    }
    else if (_stricmp(typeName, "DomNode") == 0 )
    {
        backingNode.Set("(DomNode*)@$extin", objectAddress);
    }
    else
    {
        Out("ERROR: Unknown type name \"%s\"\n", typeName);
        return;
    }

    if (backingNode.GetPtr() == 0)
    {
        Out("This DOM node contains a null object pointer\n");
        return;
    }

    //
    // Dump the name and value of this DOM.
    //

    ExtRemoteTyped domQName = backingNode.Field("_FQN");
    ExtRemoteTyped domValue = backingNode.Field("_Value");    

    DumpKDomQName(domQName);
    DumpKVariant(domValue);

    //
    // Dump children of this DOM. We only dump names of the child DOMs. The user is expected to recursively 
    // use this command to get details of individual child DOM nodes.
    //

    ExtRemoteTyped children = backingNode.Field("_Children");
    
    const ULONG numberOfChildren = children.Field("_Count").GetUlong();
    Out("\nNumber of child DomNode objects: %d\n\n", numberOfChildren);
    
    for (ULONG i = 0; i < numberOfChildren; i++)
    {
        Out("--------------------\n");
        ExtRemoteTyped childProxy = children.Field("_Array").ArrayElement(i).Field("_Proxy");
        Out("Child DomNode %d: address = %p\n", i, childProxy.GetPtr());

        ExtRemoteTyped domQName1 = childProxy.Field("_FQN");        
        DumpKDomQName(domQName1);
    }
}

EXT_COMMAND(dumpkvar,
            "Dump the content of a KVariant object",
            "{;e;address;address of a KVariant object}")
{
    ULONG64 objectAddress = GetUnnamedArgU64(0);

    ExtRemoteTyped object("(KVariant*)@$extin", objectAddress);
    DumpKVariant(object);
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

void
EXT_CLASS::DumpKHeap(__in ExtRemoteTyped& Allocator, __in ULONG64 VtableAddress)
{
    DumpKHeap(Allocator, VtableAddress, NULL, NULL);
}

class AllocatedEntry
{
    public:
        AllocatedEntry() {};
        ~AllocatedEntry() {};

        BOOLEAN operator<(const AllocatedEntry& Right) const
        {
            return Size < Right.Size ? TRUE : FALSE;
        }
    
        ULONG64 VtableAddress;
        ULONG64 Size;
        ULONG Count;
        BOOLEAN IsVtable;
};

void
EXT_CLASS::DumpKHeap(
        __in ExtRemoteTyped& Allocator,
        __in ULONG64 VtableAddress,
        __in KHeapCallback Callback,
        __in PVOID Context
    )
{
    ExtRemoteTyped allocatedEntries = Allocator.Field("_AllocatedEntries");
    ExtRemoteTyped count = allocatedEntries.Field("_Count");
    ExtRemoteTyped head = allocatedEntries.Field("_Head");

    ULONG64 headAddress = head.m_Offset;
    
    BOOLEAN showSummary = FALSE;
    std::map<ULONG64, AllocatedEntry> summaryMap;
    ULONG entriesProcessed = 0;

    if (VtableAddress == (ULONG64)-1)
    {
        VtableAddress = 0;
        showSummary = TRUE;
    }

    Out("Allocator has 0x%x entries\n", count.GetUlong());
    Out("Head of list is %p\n\n", headAddress);

    if (count.GetUlong() > 0)
    {
        ULONG64 totalSize = 0;

       //
        // Now lets walk the list and see if we can match the vtable.
        //
        ULONG64 listAddress = head.Field("Flink").GetPtr();
        while (listAddress != headAddress)
        {
            ExtRemoteData entry;
            ULONG64 entryVtableAddress;
            ULONG64 entryCreatorAddress;
            ULONG64 entrySize;

            //
            // define offsets from the beginning of the allocation to
            // the location of the default vtable and the caller that
            // initially allocated the memory
            //
            ULONG vtableOffset = 0x30;
            ULONG creatorOffset = 0x18;
            ULONG sizeOffset = 0x20;

            entry.Set(listAddress + vtableOffset, IsPtr64() ? sizeof(ULONG64) : sizeof(ULONG));
            entryVtableAddress = entry.GetPtr();

            entry.Set(listAddress + creatorOffset, IsPtr64() ? sizeof(ULONG64) : sizeof(ULONG));
            entryCreatorAddress = entry.GetPtr();
                        
            entry.Set(listAddress + sizeOffset, sizeof(ULONG64));
            entrySize = entry.GetUlong64();
            totalSize += entrySize;
                        
            if ((VtableAddress == 0) || (VtableAddress == entryVtableAddress))
            {
                CHAR name[512];
                ULONG line;
                ULONG64 displacement;

                *name = 0;
                HRESULT hr = m_Symbols->GetNameByOffset(
                    entryVtableAddress,
                    name,
                    sizeof(name),
                    NULL,
                    &displacement);

                if (showSummary)
                {
                    entriesProcessed++;
                    if ((entriesProcessed % 500) == 499)
                    {
                        Out("Processed %d entries....\n", entriesProcessed);
                    }

                    BOOLEAN isVtable = TRUE;
                    if ((! SUCCEEDED(hr)) || (*name == 0) || (displacement != 0))
                    {
                        //
                        // In this case the allocation is not an object
                        // so we track it by the allocation address
                        //
                        entryVtableAddress = entryCreatorAddress;
                        isVtable = FALSE;
                    }
                    
                    if (summaryMap.find(entryVtableAddress) == summaryMap.end())
                    {
                        AllocatedEntry allocatedEntry;
                        allocatedEntry.VtableAddress = entryVtableAddress;
                        allocatedEntry.Count = 1;
                        allocatedEntry.Size = entrySize;
                        allocatedEntry.IsVtable = isVtable;
                        summaryMap[entryVtableAddress] = allocatedEntry;
                    }
                    else 
                    {
                        summaryMap[entryVtableAddress].Count++;
                        summaryMap[entryVtableAddress].Size += entrySize;
                    }
                } else {
                    //
                    // we have a candidate to dump
                    //
                    Out("%p %s+0x%x (%N)\n", (PVOID)(listAddress + vtableOffset), name, displacement, entrySize);

                    hr = m_Symbols->GetNameByOffset(entryCreatorAddress,
                        name,
                        sizeof(name),
                        NULL,
                        &displacement);

                    Out("    Allocated by: %p - %s+0x%x\n", entryCreatorAddress, name, displacement);

                    hr = m_Symbols->GetLineByOffset(entryCreatorAddress,
                        &line,
                        name,
                        sizeof(name),
                        NULL,
                        &displacement);

                    Out("        %s(%d)+0x%x\n\n", name, line, displacement);
                    if (Callback != NULL)
                    {
                        (*Callback)(this, (listAddress + vtableOffset), Context);
                    }
                }
            }

            //
            // advance to the next entry in the list
            //
            entry.Set(listAddress, sizeof(ULONG64));
            listAddress = entry.GetPtr();       
        }

        if (showSummary)
        {
            std::list<AllocatedEntry> sortedSummary;
            std::map<ULONG64, AllocatedEntry>::iterator it;
            for (it = summaryMap.begin(); it != summaryMap.end(); it++)
            {
                sortedSummary.push_back(it->second);
            }
            sortedSummary.sort();
            
            Out("VtableAddress   Name                            Count    Total Size\n");
            std::list<AllocatedEntry>::iterator itList;
            for (itList = sortedSummary.begin(); itList != sortedSummary.end(); itList++)
            {
                AllocatedEntry allocatedEntry = *itList;

                CHAR name[512];
                ULONG64 displacement;

                m_Symbols->GetNameByOffset(
                    allocatedEntry.VtableAddress,
                    name,
                    sizeof(name),
                    NULL,
                    &displacement);

                if (allocatedEntry.IsVtable)
                {
                    Out("%16p %91s %8d %8d\n", (PVOID)allocatedEntry.VtableAddress, name, allocatedEntry.Count, allocatedEntry.Size);
                } else {
                    Out("%16p %80s+0x%08x %8d %8d\n", (PVOID)allocatedEntry.VtableAddress, name, displacement, allocatedEntry.Count, allocatedEntry.Size);
                }
            }
        }

        Out("Total size: %N\n\n", totalSize);
    }
}


EXT_COMMAND(findalloc,
            "find all allocations with a KTL system non paged allocator by matching the vtable of the object",
            "{;e,r;ktlsystembase;KTL system base in which to search. Use !ktlsystemlist to find KTL systems}"
            "{;s,r;typename;Type of object for which to search, ie SsReplicaStandard. Use 0 for all types or"
            " 1 for summary of all allocs}")
{
    HRESULT hr;
    
    ULONG64 ktlSystemAddress = GetUnnamedArgU64(0);
    PCSTR typeName = GetUnnamedArgStr(1);

    ULONG64 vtableAddress;

    Out("\n");

    if (*typeName == '1')
    {
        vtableAddress = (ULONG64)-1;
    } else {    
        //
        // first step is to find the vtable address
        //
        hr = GetVtableAddressForClass(typeName, &vtableAddress);
        if (FAILED(hr))
        {
            return;
        }
    }
    
    //
    // Next is to get the address of the non paged allocator from the
    // KTL system. This would always be the  _NonPagedAllocator
    //
    ExtRemoteTyped ktlSystem("(KtlSystemBase*)@$extin", ktlSystemAddress);

    Out("***_NonPagedAllocator:\n");
    ExtRemoteTyped allocator = ktlSystem.Field("_NonPagedAllocator");
    DumpKHeap(allocator, vtableAddress);

    Out("***_PagedAllocator:\n");
    allocator = ktlSystem.Field("_PagedAllocator");
    DumpKHeap(allocator, vtableAddress);
}

VOID DumpLogStreamCallback(
    __in EXT_CLASS* ExtClass,                          
    __in ULONG64 LogStreamAddress,
    __in PVOID
    )
{
    ExtRemoteTyped overlayStream("(OverlayStream*)@$extin", LogStreamAddress);
    ExtRemoteTyped pathProxy = overlayStream.Field("_Path");
    ExtRemoteTyped path = pathProxy.Field("_Proxy");
    
    ExtRemoteTyped sizeObject = path.Field("_LenInChars");
    ULONG size = sizeObject.GetUlong();

    //
    // The real string buffer is pointed to by _Buffer. Since we will make a copy, untyped ExtRemoteData 
    // is fine.
    //
    ExtRemoteData bufferObject(path.Field("_IntBuffer").GetPtr(), size * 2);

    std::unique_ptr<WCHAR[]> buffer(new WCHAR[size + 1]);
    bufferObject.ReadBuffer(buffer.get(), (size + 1)*2, true);
    *(buffer.get() + size) = 0;

    ExtClass->Out("\n");
    ExtClass->Out("    Path: %S\n", buffer.get());
    
    ExtRemoteTyped a = overlayStream.Field("_IsOpen");
    ExtClass->Out("    _IsOpen: ");
    a.OutSimpleValue();
    ExtClass->Out("\n");

    a = overlayStream.Field("_OpenCompleted");
    ExtClass->Out("    _OpenCompleted: ");
    a.OutSimpleValue();
    ExtClass->Out("\n");

    a = overlayStream.Field("_State");
    ExtClass->Out("    _State: ");
    a.OutSimpleValue();
    ExtClass->Out("\n");
    
    a = overlayStream.Field("_FailureStatus");
    ExtClass->Out("    _FailureStatus: ");
    a.OutSimpleValue();
    ExtClass->Out("\n");
    
    a = overlayStream.Field("_WriteOnlyToDedicated");
    ExtClass->Out("    _WriteOnlyToDedicated: ");
    a.OutSimpleValue();
    ExtClass->Out("\n");
    
    a = overlayStream.Field("_GateAcquired");
    ExtClass->Out("    _GateAcquired: ");
    a.OutSimpleValue();
    ExtClass->Out("\n");    

    ExtClass->Out("\n");    
}

EXT_COMMAND(findlogstreams,
            "find all KTLLogger log streams and show information about them",
            "{;e,r;ktlsystembase;KTL system base in which to search. Use !ktlsystemlist to find KTL systems}"
            )
{
    HRESULT hr;
    
    ULONG64 ktlSystemAddress = GetUnnamedArgU64(0);
    ULONG64 flags = 0;
    PCSTR typeName = "OverlayStream";

    ULONG64 vtableAddress;

    Out("\n");
    
    //
    // first step is to find the vtable address
    //
    hr = GetVtableAddressForClass(typeName, &vtableAddress);
    if (FAILED(hr))
    {
        return;
    }
    
    //
    // Next is to get the address of the non paged allocator from the
    // KTL system. This would always be the  _NonPagedAllocator
    //
    ExtRemoteTyped ktlSystem("(KtlSystemBase*)@$extin", ktlSystemAddress);

    KHeapCallback callback = DumpLogStreamCallback;
    
    Out("***_NonPagedAllocator:\n");
    ExtRemoteTyped allocator = ktlSystem.Field("_NonPagedAllocator");
    DumpKHeap(allocator, vtableAddress, callback, (PVOID)flags);

    Out("***_PagedAllocator:\n");
    allocator = ktlSystem.Field("_PagedAllocator");
    DumpKHeap(allocator, vtableAddress, callback, (PVOID)flags);
}


EXT_COMMAND(dumpstring,
    "Dump the contents of a KBuffer as a unicode string",
    "{;e;address;}")
{
        ULONG64 objectAddress = GetUnnamedArgU64(0);
        ExtRemoteTyped node("(KBuffer *)@$extin", objectAddress);
        
        ULONG length = node.Field("_Size").GetUlong();
        ExtRemoteData bufferObject(node.Field("_Content").GetPtr(), length);

        std::unique_ptr<WCHAR[]> buffer(new WCHAR[(length/2) + 1]);
        bufferObject.ReadBuffer(buffer.get(), length, true);
        *(buffer.get() + (length/2)) = 0;

        if (*(buffer.get()) == 0xFEFF )
        {
            Out("%S", buffer.get()+1);   
            
        }
        else
        {
            Out("%S", buffer.get());   
        }

}

typedef struct 
{
    PCSTR Symbol;
    ULONG64 Value;
    PCSTR Text;
} SymbolValues, *PSymbolValues;

static PCSTR Unknown = "Unknown";

PCSTR LookupSymbolText(
    PSymbolValues SymbolValues,
    ULONG SymbolValuesCount,
    ULONG64 Value
    )
{
    for (ULONG i = 0; i < SymbolValuesCount; i++)
    {
        if (Value == SymbolValues[i].Value)
        {
            return(SymbolValues[i].Text);
        }
    }
    
    return(Unknown);
}

SymbolValues symbolValues[5] = {
    { "KAsyncContextBase::InitializedStateDispatcher",       0, "Initialized/Reused" },
    { "KAsyncContextBase::OperatingStateDispatcher",         0, "Operating" },
    { "KAsyncContextBase::CompletingStateDispatcher",        0, "Completing" },
    { "KAsyncContextBase::CompletionPendingStateDispatcher", 0, "Completion Pending" },
    { "KAsyncContextBase::CompletedStateDispatcher",         0, "Completed" }
};

EXT_COMMAND(findasync,
            "find all KAsyncContextBase within a KTL by matching the vtable of the object",
            "{;e,r;ktlsystembase;KTL system base in which to search. Use !ktlsystemlist to find KTL systems}"
            "{;s,r;typename;Type of object for which to search, ie SsExeCache::FlushNeededNotifyContext. Use 0 for all types}"
            "{;e,r;flags;Specify 1 to include idle asyncs}")
{
    HRESULT hr = S_OK;
    
    ULONG64 ktlSystemAddress = GetUnnamedArgU64(0);
    PCSTR typeName = GetUnnamedArgStr(1);
    ULONG64 includeIdle = GetUnnamedArgU64(2);

    ULONG64 vtableAddress;
    PCSTR vtableText = "::`vftable'";

    ULONG i;
    
    Out("\n");

    PULONG64 initializedState = &symbolValues[0].Value;
//  PULONG64 operatingState = &symbolValues[1].Value;
//  PULONG64 completingState = &symbolValues[2].Value;
//  PULONG64 completionPendingState = &symbolValues[3].Value;
//  PULONG64 completedState = &symbolValues[4].Value;   
        
    for (i = 0; SUCCEEDED(hr) && (i < 5); i++)
    {
        hr = m_Symbols->GetOffsetByName(symbolValues[i].Symbol, &symbolValues[i].Value);
    }

    if (FAILED(hr))
    {
        Out("GetOffsetByName %s : failed %x\n\n", symbolValues[i].Symbol, hr);
        return;
    }
    
    //
    // first step is to find the vtable address
    //
    hr = GetVtableAddressForClass(typeName, &vtableAddress);
    if (FAILED(hr))
    {
        return;
    }
    
    //
    // Next is to get the address of the non paged allocator from the
    // KTL system. This would always be the  _NonPagedAllocator
    //
    ExtRemoteTyped ktlSystem("(KtlSystemBase*)@$extin", ktlSystemAddress);

    ExtRemoteTyped allocator = ktlSystem.Field("_NonPagedAllocator");
    ExtRemoteTyped allocatedEntries = allocator.Field("_AllocatedEntries");
    ExtRemoteTyped count = allocatedEntries.Field("_Count");
    ExtRemoteTyped head = allocatedEntries.Field("_Head");

    ULONG64 headAddress = head.m_Offset;
    
    Out("Allocator has 0x%x entries\n", count.GetUlong());
    Out("Head of list is %p\n\n", headAddress);

    if (count.GetUlong() > 0)
    {
        //
        // Now lets walk the list and see if we can match the vtable.
        //
        ULONG64 listAddress = head.Field("Flink").GetPtr();
        while (listAddress != headAddress)
        {
            ExtRemoteData entry;
            ULONG64 entryVtableAddress;
            ULONG vtableTextLen = (ULONG)strlen(vtableText);

            //
            // define offsets from the beginning of the allocation to
            // the location of the default vtable and the caller that
            // initially allocated the memory
            //
            ULONG vtableOffset = 0x30;

            entry.Set(listAddress + vtableOffset, IsPtr64() ? sizeof(ULONG64) : sizeof(ULONG));
            entryVtableAddress = entry.GetPtr();

            if ((vtableAddress == 0) || (vtableAddress == entryVtableAddress))
            {
                //
                // we have a candidate to dump
                //
                CHAR name[512];
                CHAR expression[512];
                CHAR *pname;
                ULONG64 displacement;
                ULONG l;

                hr = m_Symbols->GetNameByOffset(entryVtableAddress,
                                                name,
                                                sizeof(name),
                                                NULL,
                                                &displacement);

                l = (ULONG)strlen(name);
                if ((l < vtableTextLen) || displacement != 0)
                {
                    //
                    // if there isn't room for the vtable text or the
                    // address doesn't resolve cleanly then it can't be
                    // an object
                    //
                    goto next;
                }

                pname = name + (l - vtableTextLen);
                if (strcmp(pname, vtableText) != 0)
                {
                    //
                    // if this isn't a vtable address then it can't be
                    // an object
                    //
                    goto next;
                }
                
                //
                // convert vtable name back into type by chopping off
                // the ending "::`vftable'" and removing the module!
                // prefix
                //
                name[l - vtableTextLen] = 0;

                pname = name;
                while (*pname != '!') pname++;
                pname++;

                expression[0] = '(';
                expression[1] = 0;
                StringCchCat(expression, 512, pname);
                StringCchCat(expression, 512, "*)@$extin");

                //
                // Do this in a try...catch since some symbols don't
                // resolve properly and we do not want to cause the
                // whole listing to stop just because of a bad symbol.
                //
                try
                {
                    ExtRemoteTyped async(expression, listAddress + vtableOffset);
                
                    if (async.HasField("_CurrentStateDispatcher"))
                    {
                        //
                        // NOTE that this is only valid on 64 bit
                        // architectures. _CurrentStateDispatcher is
                        // defined in a way that doesn't allow it to be
                        // accessed via GetPtr(), so this is the
                        // workaround
                        //
                        ULONG64 currentStateDispatcher;
                        async.Field("_CurrentStateDispatcher").ReadBuffer(&currentStateDispatcher, 8, TRUE);
                        
                        if (includeIdle || (currentStateDispatcher != *initializedState))
                        {
                            PCSTR state = LookupSymbolText(symbolValues,
                                                           5,
                                                           currentStateDispatcher);

                            // TODO: more fields
                            // KAsyncContextBase::SPtr     _ParentAsyncContextPtr;
                            // CompletionCallback          _CallbackPtr;

                            Out("%25s %p %s\n", state, (PVOID)(listAddress + vtableOffset), name);
                        }                       
                    }
                } catch (...) {
                    Out("\nExtRemoteTyped Exception for %s\n\n", expression);
                    goto next;
                }
                
            }

next:
            //
            // advance to the next entry in the list
            //
            entry.Set(listAddress, sizeof(ULONG64));
            listAddress = entry.GetPtr();       
        }
    }
}


void EXT_CLASS::DumpAsyncChildren(
    ULONG64 KtlSystemAddress,
    ULONG64 ParentAsyncAddress,
    ULONG64 IncludeIdle,
    ULONG64 VtableAddress,
    ULONG IndentSpaces
  )
{
    HRESULT hr = S_OK;    
    PCSTR vtableText = "::`vftable'";
    
    PULONG64 initializedState = &symbolValues[0].Value;
//  PULONG64 operatingState = &symbolValues[1].Value;
//  PULONG64 completingState = &symbolValues[2].Value;
//  PULONG64 completionPendingState = &symbolValues[3].Value;
//  PULONG64 completedState = &symbolValues[4].Value;   
        
    //
    // Next is to get the address of the non paged allocator from the
    // KTL system. This would always be the  _NonPagedAllocator
    //
    ExtRemoteTyped ktlSystem("(KtlSystemBase*)@$extin", KtlSystemAddress);

    ExtRemoteTyped allocator = ktlSystem.Field("_NonPagedAllocator");
    ExtRemoteTyped allocatedEntries = allocator.Field("_AllocatedEntries");
    ExtRemoteTyped count = allocatedEntries.Field("_Count");
    ExtRemoteTyped head = allocatedEntries.Field("_Head");

    ULONG64 headAddress = head.m_Offset;
    
    if (count.GetUlong() > 0)
    {
        //
        // Now lets walk the list and see if we can match the vtable.
        //
        ULONG64 listAddress = head.Field("Flink").GetPtr();
        while (listAddress != headAddress)
        {
            ExtRemoteData entry;
            ULONG64 entryVtableAddress;
            ULONG vtableTextLen = (ULONG)strlen(vtableText);

            //
            // define offsets from the beginning of the allocation to
            // the location of the default vtable and the caller that
            // initially allocated the memory
            //
            ULONG vtableOffset = 0x30;

            entry.Set(listAddress + vtableOffset, IsPtr64() ? sizeof(ULONG64) : sizeof(ULONG));
            entryVtableAddress = entry.GetPtr();

            if ((VtableAddress == 0) || (VtableAddress == entryVtableAddress))
            {
                //
                // we have a candidate to dump
                //
                CHAR name[512];
                CHAR expression[512];
                CHAR *pname;
                ULONG64 displacement;
                ULONG l;

                hr = m_Symbols->GetNameByOffset(entryVtableAddress,
                                                name,
                                                sizeof(name),
                                                NULL,
                                                &displacement);

                l = (ULONG)strlen(name);
                if ((l < vtableTextLen) || displacement != 0)
                {
                    //
                    // if there isn't room for the vtable text or the
                    // address doesn't resolve cleanly then it can't be
                    // an object
                    //
                    goto next;
                }

                pname = name + (l - vtableTextLen);
                if (strcmp(pname, vtableText) != 0)
                {
                    //
                    // if this isn't a vtable address then it can't be
                    // an object
                    //
                    goto next;
                }
                
                //
                // convert vtable name back into type by chopping off
                // the ending "::`vftable'" and removing the module!
                // prefix
                //
                name[l - vtableTextLen] = 0;

                pname = name;
                while (*pname != '!') pname++;
                pname++;

                expression[0] = '(';
                expression[1] = 0;
                StringCchCat(expression, 512, pname);
                StringCchCat(expression, 512, "*)@$extin");

                //
                // Do this in a try...catch since some symbols don't
                // resolve properly and we do not want to cause the
                // whole listing to stop just because of a bad symbol.
                //
                try
                {
                    ExtRemoteTyped async(expression, listAddress + vtableOffset);
                
                    if (async.HasField("_CurrentStateDispatcher"))
                    {

                        //
                        // extract the ParentAsync field
                        //
                        ExtRemoteTyped sptr = async.Field("_ParentAsyncContextPtr");
                        ULONG64 address = sptr.Field("_Proxy").GetPtr();
                        
                        //
                        // NOTE that this is only valid on 64 bit
                        // architectures. _CurrentStateDispatcher is
                        // defined in a way that doesn't allow it to be
                        // accessed via GetPtr(), so this is the
                        // workaround
                        //
                        ULONG64 currentStateDispatcher;
                        async.Field("_CurrentStateDispatcher").ReadBuffer(&currentStateDispatcher, 8, TRUE);
                        
                        if ((address == ParentAsyncAddress) && (IncludeIdle || (currentStateDispatcher != *initializedState)))
                        {
                            PCSTR state = LookupSymbolText(symbolValues,
                                                           5,
                                                           currentStateDispatcher);

                            // TODO: more fields
                            // KAsyncContextBase::SPtr     _ParentAsyncContextPtr;
                            // CompletionCallback          _CallbackPtr;

                            if (IndentSpaces == 0) Out("\n");
                            for (ULONG i = 0; i < IndentSpaces; i++) Out(" ");
                            Out("%s %p %s\n", state, (PVOID)(listAddress + vtableOffset), name);

                            DumpAsyncChildren(KtlSystemAddress,
                                              listAddress + vtableOffset,
                                              IncludeIdle,
                                              VtableAddress,
                                              IndentSpaces + 2);
                        }                       
                    }
                } catch (...) {
                    Out("\nExtRemoteTyped Exception for %s\n\n", expression);
                    goto next;
                }
                
            }

next:
            //
            // advance to the next entry in the list
            //
            entry.Set(listAddress, sizeof(ULONG64));
            listAddress = entry.GetPtr();       
        }
    }
}

EXT_COMMAND(asyncchildren,
            "find all KAsyncContextBase within a KTL by matching the parent async of the object",
            "{;e,r;ktlsystembase;KTL system base in which to search. Use !ktlsystemlist to find KTL systems}"
            "{;e,r;parentasync;Parent async whose children are to be found}"
            "{;s,r;typename;Type of object for which to search, ie SsExeCache::FlushNeededNotifyContext. Use 0 for all types}"
            "{;e,r;flags;Specify 1 to include idle asyncs}")
{
    HRESULT hr = S_OK;
    
    ULONG64 ktlSystemAddress = GetUnnamedArgU64(0);
    ULONG64 parentAsyncAddress = GetUnnamedArgU64(1);
    PCSTR typeName = GetUnnamedArgStr(2);
    ULONG64 includeIdle = GetUnnamedArgU64(3);

    ULONG64 vtableAddress;
    ULONG i;
    
    for (i = 0; i < 5; i++)
    {
        hr = m_Symbols->GetOffsetByName(symbolValues[i].Symbol, &symbolValues[i].Value);
        if (FAILED(hr))
        {
            Out("GetOffsetByName %s : failed %x\n\n", symbolValues[i].Symbol, hr);
            return;
        }
    }
    
    //
    // first step is to find the vtable address
    //
    hr = GetVtableAddressForClass(typeName, &vtableAddress);
    if (FAILED(hr))
    {
        return;
    }

    DumpAsyncChildren(ktlSystemAddress,
                      parentAsyncAddress,
                      includeIdle,
                      vtableAddress,
                      0);
}


EXT_COMMAND(ktlsystemlist,
            "find all ktlsystembase", 
            "{;e,o;flags;Not defined yet}")
{
    HRESULT hr = S_OK;
    PCSTR name;
    ULONG64 ktlSystemListHead;

    //
    // get the head of the KTL system list
    //
    name = "KtlSystemListStorage";
    hr = m_Symbols->GetOffsetByName("KtlSystemListStorage", &ktlSystemListHead);
    if (FAILED(hr))
    {
        Out("GetOffsetByName %s : failed %x\n\n", name, hr);
        return;
    }

    Out("KtlSystemList\n");
    ExtRemoteTyped ktlSystem("(LIST_ENTRY*)@$extin", ktlSystemListHead);
    ExtRemoteTyped ktlSystemZero("(KtlSystemBase*)@$extin", 0);
    ULONG ktlSystemListEntryOffset = ktlSystemZero.GetFieldOffset("_KtlSystemsListEntry");
    
    ULONG64 listAddress = ktlSystem.Field("Flink").GetPtr();
    while (listAddress != ktlSystemListHead)
    {
        ExtRemoteData entry;
        
        Out("    %p    KtlSystemBase\n", (PVOID)(listAddress - ktlSystemListEntryOffset));
        
        //
        // advance to the next entry in the list
        //
        entry.Set(listAddress, sizeof(ULONG64));
        listAddress = entry.GetPtr();       
    }
}
