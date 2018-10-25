//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#pragma once
#include "ThreadErrorMessages.h"
#include "ConfigStoreDescription.h"
#include "FabricCrashLeasingApplicationCallback.h"
#include "ThreadCounter.h"

namespace Common
{
    /*
        FabricGlobals provides a mechanism to ensure that all components
        agree on a specific instance of the singleton for the entire process.

        The issue being solved is that the singleton itself is defined in 
        common.lib which is very low level and included by all modules and hence
        different dlls when loaded have their own instance of the singleton.

        In order to make all instances agree, by convention the instance in 
        FabricCommon is chosen to be the master. In production code, FabricCommon 
        is the only place where the Initialize method is called and all production code
        depends on FabricCommon.

        The first thing the module that has the master copy must do is call InitializeAsMaster.
        This tells the globals object that it is the master and must return itself 
        when the singleton is accessed in other places. If the object is not the master it will
        dynamically load fabric common and get the instance from there which ensures the 
        same instance of the singleton (FabricCommon's) is used everywhere. A dynamic load is done
        to prevent a build and runtime dependency on FabricCommon for layers that are below 
        FabricCommon. 

        Test code for layers under FabricCommon (such as unit tests for Common itself) 
        can Initialize and set their copy as the master in their main function

        FabricGlobals supports two patterns:
        - Holding on to singleton instances of the object itself
        - Holding on to a factory which will create the singleton instance on first use

        As of now the dynamic loading optimization is not done on linux.

        Today, bringing in FabricGlobals will bring in all the associated components. 
        This should be fine because there are very few globals. As it grows 
        it should be refactored and pieces should be hidden away by indirection
    */
    class FabricGlobals
    {
    public:
        /*
            Factory that defines how to load the configuration. Configuration is loaded
            i.e. the factory is invoked only when the config is first accessed. IT is 
            guaranteed that the factory is invoked only once in the entire execution
            of the module.

            NOTE: This is not a std::function because the FabricGlobals may need to
            be created from a DllMain where it is not allowed to allocate memory
            using the CRT and construction of a std::function may allocate
        */
        typedef ConfigStoreDescriptionUPtr(*ConfigLoaderFnPtr)();

        typedef std::string(*InitializeSymbolPathFnPtr)();

        static FabricGlobals & Get();
        
        static void InitializeAsMaster(ConfigLoaderFnPtr configLoaderFunction);

        ThreadCounter & GetThreadCounter()
        {
            return threadCounter_;
        }

        ThreadErrorMessages & GetThreadErrorMessages()
        {
            return threadErrorMessages_;
        }

        FabricCrashLeasingApplicationCallback & GetCrashLeasingApplicationCallback()
        {
            return crashLeasingApplicationCallback_;
        }

        std::string const & GetSymbolPath() const;

        ConfigStoreDescription const & GetConfigStore() const;

    private:
        static Common::Global<FabricGlobals> singleton_;
        static FabricGlobals *accessor_;

        // Default constructed thread error messages
        ThreadErrorMessages threadErrorMessages_;
        
        // Default constructed leasing application callback holder
        FabricCrashLeasingApplicationCallback crashLeasingApplicationCallback_;

        // Lazily initialized configuration 
        ConfigLoaderFnPtr configLoader_ = nullptr;
        mutable std::unique_ptr<ConfigStoreDescription> configStore_;

        // Lazily initialized symbol paths
        // No need for a factory because there is only one way to do this
        // which is defined in Common
        mutable std::string symbolPath_;

        // Default constructed thread counter
        ThreadCounter threadCounter_;
    };
}