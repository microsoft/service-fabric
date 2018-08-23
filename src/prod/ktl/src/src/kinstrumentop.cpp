/*++

    (c) 2016 by Microsoft Corp. All Rights Reserved.

    kinstrumentop.cpp

    Description:
      Kernel Template Library (KTL): Operation instrumentation framework

      Class that can be used by a component to track and report information
      that is used to provide instrumentation information about the
      operations that it performs. The class tracks things such as

          * Min, max and average time for an operation over a period
          * Min, max and average size for an operation over a period
          * Min, max and average time between operations over a period
      
      The component calls BeginOperation() with the size of the
      operation right before the operation is started and then
      EndOperation() right when the operation is completed. KInstr

      The component will call Create() and pass a name for the
      operation type and an optional reporting timer. 



      An example would be KBlockFile using this to 


    History:
      alanwar          25-APR-2017         Initial version.

--*/

#include <ktl.h>
#include <ktrace.h>
#include <kinstrumentop.h>

KInstrumentedOperation::~KInstrumentedOperation()
{
    _InstrumentedComponent = nullptr;
}

KInstrumentedOperation::KInstrumentedOperation()
{
}

KInstrumentedOperation::KInstrumentedOperation(
    __in KInstrumentedComponent& InstrumentedComponent
    )
{
    _InstrumentedComponent = &InstrumentedComponent; 
}


NTSTATUS KInstrumentedComponent::Create(
    __out KInstrumentedComponent::SPtr& IC,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    )
{
    NTSTATUS status;
    KInstrumentedComponent::SPtr ic;

    ic = _new(AllocationTag, Allocator) KInstrumentedComponent();
    if (ic == nullptr)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    status = ic->Status();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    IC = Ktl::Move(ic);
    return(STATUS_SUCCESS); 
}



KInstrumentedComponent::~KInstrumentedComponent()
{
}

KInstrumentedComponent::KInstrumentedComponent(
    )
{
    NTSTATUS status;
    
    KStringView name(L"Unspecified");   
    status = SetComponentName(name);
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }
    
    SetReportFrequency(MAXULONGLONG);
    Reset();
    StartSampling();
}

NTSTATUS KInstrumentedComponent::SetComponentName(
    __in KStringView& Component
    )
{
    NTSTATUS status;
    PCHAR componentNameBuffer;
    ULONG lengthInChar = Component.Length();

    //
    // Tracing only supports ANSI strings and so we need to create the
    // component name as ANSI
    //
    status = KStringA::Create(_ComponentName, GetThisAllocator(), lengthInChar+1);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    componentNameBuffer = (PCHAR)(*_ComponentName);
    Component.CopyToAnsi(componentNameBuffer, lengthInChar);
    componentNameBuffer[lengthInChar] = 0;
	
	return(STATUS_SUCCESS);
}
        

NTSTATUS KInstrumentedComponent::SetComponentName(
    __in KStringView& ComponentPrefix,
    __in GUID Guid
    )
{
    NTSTATUS status;
    BOOLEAN b;
    KStringView separator(L":");
    ULONG sizeNeeded = ComponentPrefix.Length() + 1 + KStringView::GuidLengthInChars + 1;
    KString::SPtr local;

    status = KString::Create(local, GetThisAllocator(), sizeNeeded);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    b = local->CopyFrom(ComponentPrefix);
    if (! b)
    {
        return(STATUS_UNSUCCESSFUL);
    }

    b = local->Concat(separator);
    if (! b)
    {
        return(STATUS_UNSUCCESSFUL);
    }

    b = local->FromGUID(Guid, TRUE);
    if (! b)
    {
        return(STATUS_UNSUCCESSFUL);
    }

    status = SetComponentName(*local);
	return(status);
}

NTSTATUS KInstrumentedComponent::SetComponentName(
    __in KStringView& ComponentPrefix,
    __in GUID Guid1,
    __in GUID Guid2
    )
{
    NTSTATUS status;
    BOOLEAN b;
    KStringView separator(L":");
    ULONG sizeNeeded = ComponentPrefix.Length() + 2 + (2*KStringView::GuidLengthInChars) + 1;
    KString::SPtr local;

    status = KString::Create(local, GetThisAllocator(), sizeNeeded);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    b = local->CopyFrom(ComponentPrefix);
    if (! b)
    {
        return(STATUS_UNSUCCESSFUL);
    }

    b = local->Concat(separator);
    if (! b)
    {
        return(STATUS_UNSUCCESSFUL);
    }

    b = local->FromGUID(Guid1, TRUE);
    if (! b)
    {
        return(STATUS_UNSUCCESSFUL);
    }

    b = local->Concat(separator);
    if (! b)
    {
        return(STATUS_UNSUCCESSFUL);
    }

    b = local->FromGUID(Guid2, TRUE);
    if (! b)
    {
        return(STATUS_UNSUCCESSFUL);
    }

    status = SetComponentName(*local);
	return(status);
}


