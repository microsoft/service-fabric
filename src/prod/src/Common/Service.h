// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Common/SecureString.h"

namespace Common
{
#if !defined(PLATFORM_UNIX)
    class ServiceHandle
    {
        DENY_COPY(ServiceHandle);
    private:
        ServiceHandle();

    public:
        __declspec( property( get=get_Handle ) ) ::SC_HANDLE Handle;
        ::SC_HANDLE get_Handle() { return serviceHandle; }

        ServiceHandle( ::SC_HANDLE handle ) :
        serviceHandle( handle )
        {
        }

        ~ServiceHandle()
        {
            CloseServiceHandle(serviceHandle);
        }

    protected:
        ::SC_HANDLE serviceHandle;
    };
#endif

    class ServiceBase
    {
        DENY_COPY( ServiceBase );

    protected:
        explicit ServiceBase() 
        { 
            Initialize();
        }

        explicit ServiceBase( std::wstring const & name ) 
            : serviceName_( name )
        {
            Initialize();
        }

    public:
#if defined(PLATFORM_UNIX)
        virtual ~ServiceBase() { }
#else
        virtual ~ServiceBase();
        virtual void OnPreshutdown() { }
#endif

        virtual void OnStart( Common::StringCollection const & args ) = 0;
        virtual void OnStop( bool shutdown ) = 0;
        virtual void OnPause() { };
        virtual void OnContinue() { };

        static void Run( ServiceBase& );

        void SetServiceState( DWORD currentState,
                              DWORD checkPoint = 0,
                              DWORD waitHint = 0,
                              DWORD errorCode = NO_ERROR );

        void  SetControlCode( DWORD newControlCode );
        DWORD GetControlCode();

        DWORD ServiceTimeout;

        // Invoked by worker thread. This is where we truly process the SCM control codes.
        DWORD ProcessControlCode( DWORD controlCode );

    protected:

        // 
        // this ugliness is because we can't possible pass in a per-service
        // ServiceMain to nt. It balks at the this pointer that's present. 
        // Polymorphism can't be done either since ServiceMain won't accept
        // a context
        // So we resort to this ugliness of having a global variable.
        // Any services written using Cxl can only have one service per 
        // executable.
        //
        // -- rdoshi
        // 
        static void WINAPI ServiceMain( DWORD argc, __in_ecount(argc) ::LPWSTR* argv);
        
        // Invoked by SCM. Accept the control code but don't do any work.
        static DWORD WINAPI ServiceHandler( DWORD, DWORD, ::LPVOID, ::LPVOID );

        static ServiceBase* storedService;

        std::wstring            serviceName_;
        DWORD                   checkPoint_;
        ::SERVICE_STATUS_HANDLE handle_;
        DWORD                   currentState_; 
        bool                    serviceStopped_;

    private:
        void Initialize()
        {
            handle_                   = nullptr;
            checkPoint_               = 0;
            serviceStopped_           = false;
            controlCodeHandlerThread_ = nullptr;
            controlCode_              = 0;
            ServiceTimeout            = 60000;
        }

        // Create and start the thread that processes control codes in the background
        void CreateHandlerThread();

        ::HANDLE        controlCodeHandlerThread_;
        DWORD           controlCode_;
        RwLock          controlCodeLock_;
    };

#if !defined(PLATFORM_UNIX)
    namespace ServiceControllerStatus{
        enum Enum {
            Stopped = SERVICE_STOPPED,
            StartPending = SERVICE_START_PENDING,
            StopPending = SERVICE_STOP_PENDING,
            Running = SERVICE_RUNNING,
            ContinuePending = SERVICE_CONTINUE_PENDING,
            PausePending = SERVICE_PAUSE_PENDING,
            Paused = SERVICE_PAUSED
        };
    };

    class ServiceController
    {
    private:
        //
        // Deny list
        //
        ServiceController( const ServiceController& );
        ServiceController& operator = ( const ServiceController& );

        DWORD GetNotifyMask(ServiceControllerStatus::Enum const status);

    public:
        explicit ServiceController() {}

        explicit ServiceController( std::wstring const & name ) : 
            serviceName_( name )
        {
        }

        void SetService( std::wstring const & name )
        {
            serviceName_.assign( name );
        }

        ErrorCode Install( const std::wstring& displayName, const std::wstring& path );

        ErrorCode InstallAuto( const std::wstring& displayName, const std::wstring& path );

        ErrorCode InstallKernelDriver(const std::wstring& displayName, const std::wstring& path);

        ErrorCode Uninstall();
        
        ErrorCode Start();

        ErrorCode Stop();

        ErrorCode GetServiceSIDType( SERVICE_SID_INFO& sidInfo );

        ErrorCode SetServiceSIDType( const SERVICE_SID_INFO& sidInfo );

        ErrorCode SetPreshutdownTimeout( DWORD preshutdownTimeout );

        ErrorCode SetStartupType (DWORD startType);

        ErrorCode SetStartupTypeDelayedAuto ();

        ErrorCode GetState(DWORD & state);

        ErrorCode SetServiceAccount( std::wstring const & accountName, Common::SecureString const & password );

        ErrorCode SetServiceDescription(std::wstring const & description);

        ErrorCode SetServiceFailureActionsForRestart(DWORD const & delayInSeconds, DWORD const & resetPeriodInDays);
        
        ErrorCode WaitForStatus(ServiceControllerStatus::Enum const expectedStatus, TimeSpan const & timeout=TimeSpan::MaxValue);

        ErrorCode WaitForServiceStop(TimeSpan const & timeout = TimeSpan::MaxValue);

        ErrorCode WaitForServiceStart(TimeSpan const & timeout = TimeSpan::MaxValue);

        bool IsServiceRunning();

        static ErrorCode StartServiceByName(std::wstring const & serviceName, bool waitForServiceStart = false, TimeSpan const & timeout = TimeSpan::MaxValue);

        static ErrorCode StopServiceByName(std::wstring const & serviceName, bool waitForServiceStart = false, TimeSpan const & timeout = TimeSpan::MaxValue);

        __declspec(property(get = get_ServiceName)) std::wstring const & ServiceName;
        std::wstring const & get_ServiceName() const { return serviceName_; }

    protected:

        //
        // Members
        //
        std::wstring serviceName_;
    };
#endif
};
