/*++

    (c) 2017 by Microsoft Corp. All Rights Reserved.

    palhandle.h

    Description:
        Platform abstraction layer for linux - abstract handle type

    History:

--*/



//* HANDLE support
class IHandleObject : public KRTT, public KShared<IHandleObject>, public KObject<IHandleObject>
{
    K_RUNTIME_TYPED(IHandleObject);
    K_FORCE_SHARED_WITH_INHERITANCE(IHandleObject);

public:
    HANDLE      _kxmHandle = INVALID_HANDLE_VALUE;

    virtual BOOL Close() = 0;
};


template<typename TDerived>
static typename KSharedPtr<TDerived> ResolveHandle(HANDLE Handle)
{
    //
    // TODO: Put this back in when the issue found in FabricRM.test is
    //       fixed. This issue is due to the caller having allocated
    //       its KXMFileHandle from the KTL system in libFabricRuntime
    //       but the check is done by the KTL system in the static
    //       library.
    //
    KInvariant(is_type<TDerived>((IHandleObject*)Handle));          // Could blow with a AV - very small chance of false success

    
    typename TDerived::SPtr result = (TDerived*)Handle;
    return Ktl::Move(result);
}

static HANDLE AcquireKxmHandle();
static void ReleaseKxmHandle();

// End HANDLE support
/////////////////////////////
