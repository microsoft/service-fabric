// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

HostedServiceParameters::HostedServiceParameters()
    : serviceName_(),
    exeName_(),
    arguments_(),
    workingDirectory_(),
    environment_(),
    ctrlCSpecified_(),
    serviceNodeName_(),
    port_(),
    parentServiceName_(),
    protocol_(),
    runasSpecified_(),
    runasAccountName_(),
    runasAccountType_(),
    runasPassword_(),
    runasPasswordEncrypted_(),
    sslCertificateFindValue_(),
    sslCertStoreLocation_(),
    sslCertificateFindType_(),
    cpusetCpus_(),
    cpuShares_(),
    memoryInMB_(),
    memorySwapInMB_()
{
}

HostedServiceParameters::HostedServiceParameters(
    wstring const & serviceName,
    wstring const & exeName,
    wstring const & arguments,
    wstring const & workingDirectory,
    wstring const & environment, 
    bool ctrlCSpecified,
    wstring const & serviceNodeName,
    wstring const & parentServiceName,
    UINT port,
    wstring const & protocol,
    bool runasSpecified,
    wstring const & runasAccountName,
    ServiceModel::SecurityPrincipalAccountType::Enum runasAccountType,
    wstring const & runasPassword,
    bool runasPasswordEncrypted,
    wstring const & sslCertificateFindValue,
    wstring const & sslCertStoreLocation,
    X509FindType::Enum sslCertificateFindType,
    std::wstring const & cpusetCpus,
    uint cpuShares,
    uint memoryInMB,
    uint memorySwapInMB
    )
    : serviceName_(serviceName),
    exeName_(exeName),
    arguments_(arguments),
    workingDirectory_(workingDirectory),
    environment_(environment),
    ctrlCSpecified_(ctrlCSpecified),
    serviceNodeName_(serviceNodeName),
    parentServiceName_(parentServiceName),
    port_(port),
    protocol_(protocol),
    runasSpecified_(runasSpecified),
    runasAccountName_(runasAccountName),
    runasAccountType_(runasAccountType),
    runasPassword_(runasPassword),
    runasPasswordEncrypted_(runasPasswordEncrypted),
    sslCertificateFindValue_(sslCertificateFindValue),
    sslCertStoreLocation_(sslCertStoreLocation),
    sslCertificateFindType_(sslCertificateFindType),
    cpusetCpus_(cpusetCpus),
    cpuShares_(cpuShares),
    memoryInMB_(memoryInMB),
    memorySwapInMB_(memorySwapInMB)
{
}

void HostedServiceParameters::WriteTo(__in TextWriter & w, FormatOptions const &) const
{
    w.Write("ActivateHostedServiceRequest { ");
    w.Write("ServiceName = {0}", serviceName_);
    w.Write("ExeName = {0}", exeName_);
    w.Write("Arguments = {0}", arguments_);
    w.Write("WorkingDirectory = {0}", workingDirectory_);
    w.Write("Environment = {0}", environment_);
    w.Write("CtrlCSPecified = {0}", ctrlCSpecified_);
    w.Write("ServiceNodeName = {0}", serviceNodeName_);
    w.Write("ParentServiceName = {0}", parentServiceName_);
    w.Write("Port = {0}", port_);
    w.Write("Protocol = {0}", protocol_);
    w.Write("RunasSpecified = {0}", runasSpecified_);
    w.Write("RunasAccountName = {0}", runasAccountName_);
    w.Write("RunasAccountType = {0}", runasAccountType_);
    w.Write(runasPassword_.empty() ? "RunasPassword not specified" : "RunasPassword is specified");
    w.Write("RunasPasswordEncrypted = {0}", runasPasswordEncrypted_);
    w.Write("SslCertificateFindValue = {0}", sslCertificateFindValue_);
    w.Write("SslCertificateStoreLocation = {0}", sslCertStoreLocation_);
    w.Write("SslCertificateFindType = {0}", sslCertificateFindType_);
    w.Write("CpusetCpus = {0}", cpusetCpus_);
    w.Write("CpuShares = {0}", cpuShares_);
    w.Write("MemoryInMB = {0}", memoryInMB_);
    w.Write("MemorySwapInMB = {0}", memorySwapInMB_);
    w.Write("}");
}
