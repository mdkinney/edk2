# Host UnitTest Compiler Plugin

A CiBuildPlugin that compiles the dsc for host based unit test apps.
An IUefiBuildPlugin may be attached to this plugin that will run the unit tests and collect the results after successful compilation.

When `BUILDMODULE` selects a host application and the Host-Based Unit Test
Runner is enabled, this plugin runs only that module after the single-module
build. Package builds continue to use the runner's post-build discovery.

## Configuration

The package relative path of the DSC file to build.

``` yaml
"HostUnitTestCompilerPlugin": {
    "DscPath": "<path to dsc from root of pkg>"
}
```

### DscPath

Package relative path to the DSC file to build.

## Copyright

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

