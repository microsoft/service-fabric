# About

Use these props files to set the appropriate configuration for each Visual C++
that requires the Windows SDK or the Windows WDK toolset. This also includes
the Razzle-based Windows libs.

To use these files, determine first the type of artifact you're trying to build.
This can be either kernel or user mode, and a driver (.sys extension), static 
library (.lib extension) or dynamic library (DLL). Then, import the
corresponding file under the `props/` folder. This will also add the correct
Windows SDK include directory and additional libs, from the property files under
the `props/imports/` path.
*Note, the files in `props/imports/` should not be imported directly by a
project.*

The following sections describe the individual imports.

## KtlKernelDriver.props
A kernel driver. An example is the KTL Logger itself. This will produce a `.sys` 
file.

## TAEFTest.props
A user mode test that requires the use of `TAEF.exe`. This is commonly the
logger tests.

## Tools.props
User mode tools that run as an executable.

## UserBasic.props
Commonly a static lib, but can be any configuration type. Always user mode. Use 
this if you only require the standard headers and libraries.

## UserTest.props
A project that can describe either a user mode console test or a user mode
dynamic library. Use this if you're writing a new test that does not require
`TAEF.exe`.