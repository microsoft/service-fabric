---
Config packages contain a Settings.xml file, that can specify parameters for the service

Configuration packages describe user-defined, application-overridable configuration settings (sections of key-value pairs)
required for running service replicas/instances of service types specified in the ser-vice manifest. The configuration settings 
must be stored in Settings.xml in the config package folder. 

The service developer uses Windows Fabric APIs to locate the package folders and read applica-tion-overridable configuration settings.
The service developer can also register callbacks that are in-voked when any of the configuration packages specified in the
service manifest are upgraded and re-reads new configuration settings inside that callback. 

This means that Windows Fabric will not recycle EXEs and DLLHOSTs specified in the host and support packages when
configuration packages are up-graded. 

---
Data packages contain data files like custom dictionaries, 
non-overridable configuration files, custom initialized data files, etc. 

Windows Fabric will recycle all EXEs and DLLHOSTs specified in the host and support packages when any of the data packages
specified inside service manifest are upgraded.


