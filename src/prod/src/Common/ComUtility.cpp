// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#ifdef PLATFORM_UNIX

interface
IErrorInfo : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE GetGUID( 
        /* [out] */ GUID *pGUID) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE GetSource( 
        /* [out] */ BSTR *pBstrSource) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE GetDescription( 
        /* [out] */ BSTR *pBstrDescription) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE GetHelpFile( 
        /* [out] */ BSTR *pBstrHelpFile) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE GetHelpContext( 
        /* [out] */ DWORD *pdwHelpContext) = 0;
    
};

#endif

namespace Common
{
    Common::StringLiteral const ComUtility::ComUtilitySource("Common.ComUtility");

    namespace
    {
        typedef HRESULT (WINAPI *CoGetApartmentTypeFPtr)(
            __out APTTYPE * pAptType,
            __out APTTYPEQUALIFIER *pAptQualifier);

        typedef HRESULT (WINAPI *SetErrorInfoFPtr)(
            __in DWORD dwReserved,
            __in IErrorInfo * perrinfo);

        static HMODULE ole32Module = NULL;
        static CoGetApartmentTypeFPtr coGetApartmentTypeFPtr = NULL;
        static SetErrorInfoFPtr setErrorInfoFPtr = NULL;
    }

    bool ComUtility::Initialize()
    {
#if !defined(PLATFORM_UNIX)
        if (ole32Module == NULL)
        {
            BOOL getModuleResult = ::GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN, L"ole32.dll", &ole32Module);
            if (!getModuleResult) { return false; }
        }

        if (coGetApartmentTypeFPtr == NULL)
        {
            FARPROC rawCoGetApartmentTypeFPtr = ::GetProcAddress(ole32Module, "CoGetApartmentType");
            if (!rawCoGetApartmentTypeFPtr) { return false; }
            coGetApartmentTypeFPtr = reinterpret_cast<CoGetApartmentTypeFPtr>(rawCoGetApartmentTypeFPtr);
        }

        if (setErrorInfoFPtr == NULL)
        {
            FARPROC rawFPtr = ::GetProcAddress(ole32Module, "SetErrorInfo");
            if (!rawFPtr) { return false; }
            setErrorInfoFPtr = reinterpret_cast<SetErrorInfoFPtr>(rawFPtr);
        }

        return true;
#else
        return false;
#endif
    }

    ErrorCode ComUtility::CheckMTA()
    {
        if (!ComUtility::Initialize())
        {
            return ErrorCode::Success();
        }

        APTTYPE apartmentType;
        APTTYPEQUALIFIER apartmentTypeQualifier;
        HRESULT getApartmentTypeResult = (*coGetApartmentTypeFPtr)(&apartmentType, &apartmentTypeQualifier);
        if (getApartmentTypeResult != S_OK)
        {
            return ErrorCode::Success();
        }

        switch (apartmentType)
        {
        case APTTYPE_STA:
        case APTTYPE_MAINSTA:
        case APTTYPE_NA:
            Trace.WriteError(
                ComUtility::ComUtilitySource,
                "COM apartment state must be uninitialized or MTA.  Current apartment state is {0}.",
                static_cast<int>(apartmentType));
            return ErrorCode(ErrorCodeValue::InvalidState); // TODO: return correct error code

        case APTTYPE_MTA:
        default:
            return ErrorCode::Success();
        }
    }

    
    // CLR sets ErrorInfo to an Exception object to propagate Exceptions through COM.
    //
    // Unfortunately, the thread may be returning from COM with an Exception from an AppDomain that
    // has been unloaded.  This results in the user seeing an AppDomainUnloadedException where they
    // were expecting a FabricException.
    //
    // To work around this, we clear error info whenever we return a failure HRESULT.
    HRESULT ComUtility::OnPublicApiReturn(HRESULT hr)
    { 
        return OnPublicApiReturn(hr, std::wstring());
    }

    HRESULT ComUtility::OnPublicApiReturn(HRESULT hr, std::wstring && description)
    {
        if (FAILED(hr)) 
        { 
            ClearErrorInfo(); 

            if (!description.empty())
            {
                DWORD threadId = 0;
                ::FabricSetLastErrorMessage(description.c_str(), &threadId);
            }
        } 
        
        return hr; 
    }

    HRESULT ComUtility::OnPublicApiReturn(Common::ErrorCode const & error)
    {
        return OnPublicApiReturn(error.ToHResult(), wstring(error.Message));
    }

    HRESULT ComUtility::OnPublicApiReturn(Common::ErrorCode && error)
    {
        return OnPublicApiReturn(error.ToHResult(), error.TakeMessage());
    }

    void ComUtility::ClearErrorInfo()
    {
        if (ComUtility::Initialize())
        {
            (*setErrorInfoFPtr)(0U, NULL);
        }
    }
}
