%define		version 	0.0.0.0
%define 	release     1

BuildRoot:  BUILDROOT
Summary: Azure Service Fabric SDK Common
Name: servicefabricsdkcommon
Version: %{version}
Release: %{release}
Requires: servicefabric >= REPLACE_RUNTIME_VERSION
License: see /usr/share/doc/servicefabric/copyright_common
Group: System Environment/Base
URL: https://docs.microsoft.com/en-us/azure/service-fabric/

%define _rpmfilename servicefabric_sdkcommon_%%{VERSION}.rpm

%description
Microsoft Azure Service Fabric is a platform used for building and hosting distributed systems. This is the SDK package used for developing and interacting with Service Fabric and Service Fabric applications, version %{version}.

%pre

%post

%preun

%postun

%files
%defattr(-,root,root)
/opt/microsoft/sdk/servicefabric
/usr/share/doc/servicefabric
