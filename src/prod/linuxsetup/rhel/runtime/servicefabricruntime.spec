%define		version 	0.0.0.0
%define 	release     1

BuildRoot:  BUILDROOT
Summary: Azure Service Fabric runtime package
Name: servicefabric
Version: %{version}
Release: %{release}
Requires: scl-utils >= 20130529, rssh >= 2.3.3, libcxx >= 3.8, lz4 >= 1.7.3, lttng-tools >= 2.4.1, lttng-ust >= 2.4.1, openssl >= 1.0.2, nodejs, npm, sshpass, atop, libcurl >= 7.16.2, libcgroup, zulu-8-azure-jdk, ebtables, libunwind, libicu, rh-dotnet20, rh-dotnet20-dotnet >= 2.0.7
#Conflicts: rh-dotnet20-lttng-ust # Enable in 6.5. Cannot add this due two-phase upgrade limitation in 6.4
License: see /usr/share/doc/servicefabric/copyright_runtime
Group: System Environment/Base
URL: https://docs.microsoft.com/en-us/azure/service-fabric/

%define _rpmfilename %%{NAME}_%%{VERSION}.rpm

%description
Microsoft Azure Service Fabric is a platform used for building and hosting distributed systems. This package contains the set of files required to host the Service Fabric runtime, version %{version}.

%pre

%post

%preun

%postun

exit 0
#METAPAYLOAD

%files
%defattr(-,root,root)
/opt/microsoft/servicefabric
/usr/share/servicefabric
/usr/share/doc/servicefabric
