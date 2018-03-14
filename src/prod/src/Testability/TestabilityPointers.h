// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Testability
{ 
    class ITestabilitySubsystem;
    class TestabilitySubsystem;
    typedef std::shared_ptr<ITestabilitySubsystem> ITestabilitySubsystemSPtr;
    typedef std::weak_ptr<ITestabilitySubsystem> ITestabilitySubsystemWPtr;
    typedef std::shared_ptr<TestabilitySubsystem> TestabilitySubsystemSPtr;
    typedef Common::RootedObjectHolder<TestabilitySubsystem> TestabilitySubsystemHolder;
    typedef Common::RootedObjectWeakHolder<TestabilitySubsystem> TestabilitySubsystemWeakHolder;
}
