# Azure Pipelines PR Review Build Summary

## Overview
The edk2 project runs **3 main pipelines** for PR reviews to validate code changes across different platforms and toolchains.

---

## Main PR Review Pipelines

### 1. **Windows-VS.yml** (Visual Studio on Windows)
- **Trigger**: Pull requests to `master` and `stable/*` branches
- **Environment**: Windows 2022 with Visual Studio 2022
- **Architecture**: IA32, X64
- **Install**: OpenCppCoverage tool for code coverage analysis

### 2. **Ubuntu-GCC.yml** (GCC on Ubuntu)
- **Trigger**: Pull requests to `master` and `stable/*` branches
- **Environment**: Ubuntu 24.04 with Docker container (fedora-43-test)
- **Architecture**: IA32, X64, AARCH64, RISCV64, LOONGARCH64
- **Python**: Containerized Python (no explicit version specified)

### 3. **Ubuntu-PatchCheck.yml** (Patch Validation)
- **Trigger**: Pull requests to `master` and `stable/*` branches
- **Environment**: Ubuntu latest
- **Tool**: Python 3.12 running `BaseTools/Scripts/PatchCheck.py`
- **Purpose**: Validates the patch series format and quality

---

## Build Matrix Strategy

Each Windows-VS and Ubuntu-GCC pipeline runs a **matrix of 16 build jobs** in parallel to ensure comprehensive coverage:

| Build Job | Packages | Targets | Architectures |
|-----------|----------|---------|---|
| **Unit Tests** | All major packages (27 total) | NOOPT | X64 |
| **Arm** | ArmPkg, ArmPlatformPkg | DEBUG, RELEASE, NO-TARGET | Arch list |
| **MDE CPU** | MdePkg, UefiCpuPkg | DEBUG, RELEASE, NO-TARGET | Arch list |
| **MdeModule Debug** | MdeModulePkg | DEBUG | Arch list |
| **MdeModule Release** | MdeModulePkg | RELEASE, NO-TARGET | Arch list |
| **Network** | NetworkPkg, RedfishPkg | DEBUG, RELEASE, NO-TARGET | Arch list |
| **Other** | PcAtChipsetPkg, PrmPkg, ShellPkg, SourceLevelDebugPkg, StandaloneMmPkg, SignedCapsulePkg | DEBUG, RELEASE, NO-TARGET | Arch list |
| **FMP/FAT/Test** | FmpDevicePkg, FatPkg, UnitTestFrameworkPkg, DynamicTablesPkg | DEBUG, RELEASE, NO-TARGET | Arch list |
| **Crypto Debug** | CryptoPkg | DEBUG | Arch list |
| **Crypto Release** | CryptoPkg | RELEASE, NO-TARGET | Arch list |
| **FSP** | IntelFsp2Pkg, IntelFsp2WrapperPkg | DEBUG, RELEASE, NO-TARGET | Arch list |
| **Security** | SecurityPkg | DEBUG, RELEASE, NO-TARGET | Arch list |
| **UefiPayload IA32/X64** | UefiPayloadPkg | DEBUG, RELEASE, NO-TARGET | IA32, X64 |
| **UefiPayload ARM64 (GCC only)** | UefiPayloadPkg | DEBUG, RELEASE, NO-TARGET | AARCH64 |
| **EmbeddedPkg (GCC only)** | EmbeddedPkg | DEBUG, RELEASE, NO-TARGET | Arch list |
| **Platforms** | ArmVirtPkg, EmulatorPkg, OvmfPkg | NO-TARGET (code check only) | Arch list |

---

## Azure Pipelines: Detailed Package, Toolchain & Architecture Coverage

### **Complete Package List** (27 packages across all jobs)

**Arm Platform Packages**:
- ArmPkg - ARM architecture support
- ArmPlatformPkg - ARM platform support
- ArmVirtPkg - ARM virtualization platform (code-only check)

**Core Packages**:
- MdePkg - UEFI Platform Initialization DXE core services
- MdeModulePkg - UEFI Module implementations
- UefiCpuPkg - UEFI CPU-related modules

**Firmware Features**:
- CryptoPkg - Cryptography libraries (split into DEBUG/RELEASE due to size)
- NetworkPkg - Network stack and components
- RedfishPkg - Redfish protocol support
- SecurityPkg - Security and TPM support
- FmpDevicePkg - Firmware Management Protocol drivers

**Storage & File Systems**:
- FatPkg - FAT12/16/32 file system support

**Platform & Boot**:
- IntelFsp2Pkg - Intel Firmware Support Package (FSP)
- IntelFsp2WrapperPkg - FSP integration wrapper
- UefiPayloadPkg - UEFI payload (split testing for IA32/X64 and AARCH64)
- EmulatorPkg - Platform emulator (code-only check)
- OvmfPkg - OVMF platform (code-only check)

**Utilities & Tools**:
- PcAtChipsetPkg - PC AT chipset support
- PrmPkg - Platform Runtime Mechanism
- ShellPkg - UEFI Shell commands
- SourceLevelDebugPkg - Debug support
- StandaloneMmPkg - Standalone Memory Management
- SignedCapsulePkg - Capsule signing support
- EmbeddedPkg - Embedded platform support (GCC only)
- UnitTestFrameworkPkg - Unit test infrastructure
- DynamicTablesPkg - Dynamic ACPI table generation

### **Toolchain Support**

**Azure Pipelines** (Master/Stable branches):

| Toolchain | Windows | Linux | Architectures | Notes |
|-----------|---------|-------|---|---|
| **VS2022** | ✓ | ✗ | IA32, X64 | Visual Studio 2022 on Windows |
| **GCC** | ✗ | ✓ | IA32, X64, AARCH64, RISCV64, LOONGARCH64 | GNU toolchain on Linux (Fedora 43) |

**Target Build Types**:
- DEBUG - Debug builds with symbols
- RELEASE - Optimized release builds
- NO-TARGET - Code analysis only (no target binary generation)
- NOOPT - No optimization (for unit tests)

### **Architecture Support**

| Architecture | Windows VS | Linux GCC | Features |
|---|---|---|---|
| **IA32** | ✓ | ✓ | 32-bit x86 |
| **X64** | ✓ | ✓ | 64-bit x86 (primary testing arch) |
| **AARCH64** | ✗ | ✓ | ARM 64-bit (v8+) |
| **RISCV64** | ✗ | ✓ | RISC-V 64-bit |
| **LOONGARCH64** | ✗ | ✓ | Loongson 64-bit |

**Architecture Coverage**:
- **Full Firmware Build**: X64, IA32, AARCH64, RISCV64, LOONGARCH64
- **Unit Tests**: Primarily X64 (fastest builds)
- **EmulatorPkg**: IA32, X64 only

---

## PR-Specific Optimizations

The pipeline includes intelligent optimizations to reduce unnecessary builds:

- **PR Evaluation**: Uses `stuart_pr_eval` to determine which packages were changed by analyzing git diffs
- **Smart Package Detection**: Only runs affected builds based on code changes, reducing pipeline runtime
- **Spell Check Integration**: Integrated into the build pipeline (spell-check-prereq-steps)
- **Test Results**: Published to Azure Pipelines for visibility
- **Code Coverage**: Infrastructure exists but is currently disabled for PR gates to prevent pipeline delays

---

## Build Flow

For each build job:
1. **Setup**: Runs `stuart_setup` with the specified packages, targets, and architectures
2. **Update**: Runs `stuart_update` to fetch dependencies
3. **BaseTools Build**: Builds BaseTools if necessary
4. **Build and Test**: Runs `stuart_ci_build` with full compilation and unit test execution
5. **Results**: Publishes JUnit test results to Azure Pipelines

---

## References

- **Pipeline Files Location**: `.azurepipelines/`
- **Templates Location**: `.azurepipelines/templates/`
- **CI Settings**: `.pytool/CISettings.py`
- **Patch Validation Script**: `BaseTools/Scripts/PatchCheck.py`
- **Python Requirements**: `pip-requirements.txt`

---

# GitHub Actions PR Review Build Summary

## Overview
The edk2 project uses GitHub Actions with a **staged build** approach for PR reviews, triggered on PRs to specific branches (`sandbox/master`, `clang_ci_v5*`). The workflow orchestrates multiple build stages with automatic progression only when previous stages pass.

---

## PR Trigger Workflow

**File**: `.github/workflows/run-staged-build.yml`

**Triggers**:
- Pull requests to:
  - `sandbox/master`
  - `clang_ci_v5*` (pattern)
- Manual dispatch (`workflow_dispatch`)

**Dispatcher Configuration** (for manual runs):
- Python 3.12
- pip-requirements.txt
- Container images for Ubuntu and Windows builds

---

## Staged Build Architecture

The staged build is orchestrated by `.github/workflows/staged-build.yml` with 3 progressive stages:

### **Stage 0: Package Detection and Patch Validation**

**Job**: `set_package_lists`
- **Tool**: Ubuntu Set Package Lists and Patch Check workflow
- **Purpose**:
  - Determines which packages need testing based on PR file changes
  - Optionally runs patch format and commit validation (if enabled)
- **Condition**: Always runs to determine package scope
- **Output**: Package lists passed to downstream jobs

---

---

# GitHub Actions PR Review Build Summary
## (Assuming All Default Packages Tested)

## Overview

When a PR is submitted to `sandbox/master` or `clang_ci_v5*` branches with all default packages included, the workflow executes **3 progressive stages** with automatic advancement only on success:

- **Stage 0**: Package detection & patch validation (1 job)
- **Stage 1**: Code quality & Basic Acceptance Tests (10 parallel jobs)
- **Stage 2**: Full builds across all toolchains (8 parallel jobs)

**Total Build Jobs**: ~19 major jobs + sub-jobs per matrix configuration

---

## Stage 0: Package Detection & Patch Validation

| Job | Purpose | Duration | Blocks PR |
|-----|---------|----------|----------|
| **set_package_lists** | Analyzes git diff to determine which packages changed; optionally validates patch format | Fast (< 2 min) | Yes |

**Output**: Package lists for downstream jobs

---

## Stage 1: Code Quality & Basic Acceptance Tests (BAT)

Runs **10 parallel jobs** after Stage 0 succeeds. Each tests all 25 packages across multiple architectures with QEMU firmware validation:

### Code Quality
| Job | Toolchain | Builds | Architectures | Scope |
|-----|-----------|--------|---|---|
| **build_linux_clang_no_target_test** | CLANGPDB | NO-TARGET only | X64 | Code analysis, no firmware generation |

### Linux BAT (4 jobs)
| Job | Toolchain | Builds | Architectures | Tests |
|-----|-----------|--------|---|---|
| **build_linux_clangdwarf_basic_acceptance_test** | CLANGDWARF | DEBUG, RELEASE, NOOPT | IA32, X64, AARCH64, RISCV64 | Firmware + Unit Tests + Emulator |
| **build_linux_clangpdb_basic_acceptance_test** | CLANGPDB | DEBUG, RELEASE, NOOPT | IA32, X64 | Firmware only |
| **build_linux_gcc_basic_acceptance_test** | GCC | DEBUG, RELEASE, NOOPT | IA32, X64, AARCH64, RISCV64 | Firmware + Unit Tests + Emulator |
| **build_linux_gccnolto_basic_acceptance_test** (optional) | GCCNOLTO | DEBUG, RELEASE, NOOPT | IA32, X64, AARCH64 | Firmware + Unit Tests + Emulator |

### Windows MinGW EDK2 BAT (2 jobs)
| Job | Toolchain | Builds | Architectures | Tests |
|-----|-----------|--------|---|---|
| **build_windows_mingw_edk2_clangdwarf_basic_acceptance_test** | CLANGDWARF | DEBUG, RELEASE, NOOPT | IA32, X64 | Firmware + Unit Tests |
| **build_windows_mingw_edk2_clangpdb_basic_acceptance_test** | CLANGPDB | DEBUG, RELEASE, NOOPT | IA32, X64 | Firmware only |

### Windows MinGW LLVM BAT (2 jobs)
| Job | Toolchain | Builds | Architectures | Tests |
|-----|-----------|--------|---|---|
| **build_windows_mingw_llvm_20_clang_basic_acceptance_test** | CLANGDWARF | DEBUG, RELEASE, NOOPT | IA32, X64 | Firmware + Unit Tests + Emulator |
| **build_windows_vs_llvm_20_clang_basic_acceptance_test** | CLANGPDB | DEBUG, RELEASE, NOOPT | IA32, X64 | Firmware + Unit Tests + Emulator |

### Windows Visual Studio BAT (1 job)
| Job | Toolchain | Builds | Architectures | Tests |
|-----|-----------|--------|---|---|
| **build_windows_vs_basic_acceptance_test** | VS2022 | DEBUG, RELEASE, NOOPT | IA32, X64 | Firmware + Unit Tests + Emulator |

**Stage 1 Summary**:
- ✓ 25 packages (from default lists) tested across **10 different configurations**
- ✓ **QEMU firmware validation** for each configuration
- ✓ **Host-based unit tests** where applicable
- ✓ **EmulatorPkg testing** on supported platforms
- ✓ All 25 packages run in each job (no package filtering)

---

## Stage 2: Full Package Builds (After Stage 1 Success)

Runs **8 parallel jobs**. Each builds all 25 packages in parallel sub-jobs:

### Linux Clang
| Job | Sub-Jobs | Architectures | Build Types | Total Builds |
|-----|----------|---|---|---|
| **ubuntu_clang_build** | 2 parallel jobs (IA32/X64, AARCH64/RISCV64) | IA32, X64, AARCH64, RISCV64 | DEBUG, RELEASE, NOOPT | 4 arch × 3 types + unit tests |

### Windows MinGW Clang
| Job | Sub-Jobs | Architectures | Build Types |
|-----|----------|---|---|
| **windows_mingw_edk2_clang_20_build** | 1 job | IA32, X64 | DEBUG, RELEASE, NOOPT + unit tests |

### Windows MinGW LLVM 20
| Job | Sub-Jobs | Architectures | Build Types |
|-----|----------|---|---|
| **windows_mingw_llvm_20_clang_build** | 1 job | IA32, X64 | DEBUG, RELEASE, NOOPT + unit tests + emulator |

### Windows VS LLVM 20
| Job | Sub-Jobs | Architectures | Build Types |
|-----|----------|---|---|
| **windows_vs_llvm_20_clang_build** | 1 job | IA32, X64 | DEBUG, RELEASE, NOOPT + unit tests + emulator |

### Linux GCC
| Job | Sub-Jobs | Architectures | Build Types | Total Builds |
|-----|----------|---|---|---|
| **ubuntu_gcc_build** | 4 parallel jobs (IA32/X64, AARCH64/RISCV64, unit tests, emulator) | IA32, X64, AARCH64, RISCV64 | DEBUG, RELEASE, NOOPT | 4 arch × 3 types + unit tests + emulator |

### Linux GCCNOLTO (Optional)
| Job | Sub-Jobs | Architectures | Build Types | Total Builds |
|-----|----------|---|---|---|
| **ubuntu_gccnolto_build** | 4 parallel jobs (IA32/X64, AARCH64/RISCV64, unit tests, emulator) | IA32, X64, AARCH64 | DEBUG, RELEASE, NOOPT | 3 arch × 3 types + unit tests + emulator |

### Windows Visual Studio
| Job | Sub-Jobs | Architectures | Build Types |
|-----|----------|---|---|
| **windows_vs_build** | 1 job | IA32, X64 | DEBUG, RELEASE, NOOPT + unit tests |

**Stage 2 Summary**:
- ✓ **8 toolchain configurations** tested in parallel
- ✓ All **25 default packages** included in each configuration
- ✓ Multiple **build type combinations** per architecture
- ✓ **Unit tests** run for applicable architectures
- ✓ **EmulatorPkg explicit builds** where applicable

---

## Complete Package Test Matrix

### All 25 Packages Tested:

**Batch 1** (9 packages):
1. ArmPkg
2. ArmPlatformPkg
3. ArmVirtPkg
4. OvmfPkg
5. DynamicTablesPkg
6. EmbeddedPkg
7. MdeModulePkg
8. MdePkg
9. CryptoPkg

**Batch 2** (16 packages):
10. IntelFsp2Pkg
11. IntelFsp2WrapperPkg
12. UefiCpuPkg
13. PrmPkg
14. NetworkPkg
15. SecurityPkg
16. ShellPkg
17. FmpDevicePkg
18. FatPkg
19. RedfishPkg
20. PcAtChipsetPkg
21. SignedCapsulePkg
22. SourceLevelDebugPkg
23. UnitTestFrameworkPkg
24. StandaloneMmPkg
25. EmulatorPkg

---

## Comprehensive Toolchain & Architecture Coverage

### Stage 1 + Stage 2 Toolchain Matrix

| Toolchain | Platform | Stage 1 BAT | Stage 2 Full | Architectures |
|-----------|----------|:---:|:---:|---|
| **CLANGPDB** | Linux | ✓ | ✓ | IA32, X64, AARCH64, RISCV64 |
| **CLANGDWARF** | Linux | ✓ | ✓ | IA32, X64, AARCH64, RISCV64 |
| **GCC** | Linux | ✓ | ✓ | IA32, X64, AARCH64, RISCV64 |
| **GCCNOLTO** | Linux | ✓ (opt) | ✓ (opt) | IA32, X64, AARCH64 |
| **MinGW EDK2 CLANGDWARF** | Windows | ✓ | ✓ | IA32, X64 |
| **MinGW EDK2 CLANGPDB** | Windows | ✓ | ✗ | IA32, X64 |
| **MinGW LLVM 20 CLANGDWARF** | Windows | ✓ | ✓ | IA32, X64 |
| **VS LLVM 20 CLANGPDB** | Windows | ✓ | ✓ | IA32, X64 |
| **VS2022** | Windows | ✓ | ✓ | IA32, X64 |

### Build Type Coverage Per Toolchain

| Build Type | Stage 1 | Stage 2 | Purpose |
|-----------|--------|--------|---------|
| **DEBUG** | ✓ | ✓ | Debug symbols, no optimization |
| **RELEASE** | ✓ | ✓ | Optimized for production |
| **NOOPT** | ✓ | ✓ | No optimization, used for unit tests |
| **NO-TARGET** | ✓ | ✗ | Code analysis only (Stage 1 quality check) |

### Architecture Deployment by Toolchain

| Arch | Linux CLANG | Linux GCC | Windows MinGW | Windows VS | QEMU Tested |
|-----|---|---|---|---|---|
| **IA32** | ✓ | ✓ | ✓ | ✓ | ✓ |
| **X64** | ✓ | ✓ | ✓ | ✓ | ✓ |
| **AARCH64** | ✓ | ✓ | ✗ | ✗ | ✓ |
| **RISCV64** | ✓ | ✓ | ✗ | ✗ | ✓ |

---

## Smart Filtering & Optimization

**Skip Filters** (not tested):
- GCCNOLTO + RISCV64 (not supported by GCCNOLTO)

**Continue-on-Error Filters** (tested but don't fail PR):
- All GCCNOLTO combinations (known issues, marked as optional)

**Parallelization Strategy**:
- Stage 1: 10 jobs run in parallel
- Stage 2: 8 jobs run in parallel
- Within each job: package batches and architecture splits parallelize further

---

## Estimated Job Count (All Packages Tested)

**Stage 0**: 1 job
**Stage 1**: 10 jobs (all run in parallel)
**Stage 2**: 8 main jobs + ~30+ sub-jobs (batched by architecture/build-type)

**Total Parallel Execution**: ~18-20 concurrent jobs in peak load
**Expected Pipeline Duration**: 30-60 minutes (depending on runner availability and compilation times)

This represents the **most comprehensive PR validation**, testing all 25 packages across 9 different toolchains with 4 architecture targets and multiple build types.

---

## Build Workflow Components

### **Reusable Workflows** (in `.github/workflows/`)

1. **ubuntu-gcc-build.yml** - GCC-based package builds
2. **ubuntu-clang-build.yml** - Clang-based package builds
3. **ubuntu-package-build-test.yml** - Generic Ubuntu package builder
4. **windows-vs-build.yml** - Visual Studio package builds
5. **windows-package-build.yml** - Generic Windows package builder
6. **ubuntu-basic-acceptance-test.yml** - Ubuntu firmware validation with QEMU
7. **windows-basic-acceptance-test.yml** - Windows firmware validation with QEMU
8. **ubuntu-emulator-build.yml** - EmulatorPkg for Ubuntu
9. **windows-emulator-build.yml** - EmulatorPkg for Windows
10. **ubuntu-set-package-lists-and-patch-check.yml** - Package detection & patch validation

### **Smart Build Filtering**

- **PR Evaluation**: Analyzes git diff to determine affected packages
- **Skip List**: Excludes specific combinations (e.g., GCCNOLTO with RISCV64)
- **Continue-on-Error List**: Allows known issues to not block PR (e.g., GCCNOLTO issues)
- **Default Package Lists** (split into 2 batches for parallelization):
  - Batch 1: ArmPkg, ArmPlatformPkg, ArmVirtPkg, OvmfPkg, DynamicTablesPkg, EmbeddedPkg, MdeModulePkg, MdePkg, CryptoPkg
  - Batch 2: IntelFsp2Pkg, IntelFsp2WrapperPkg, UefiCpuPkg, PrmPkg, NetworkPkg, SecurityPkg, ShellPkg, FmpDevicePkg, FatPkg, RedfishPkg, PcAtChipsetPkg, SignedCapsulePkg, SourceLevelDebugPkg, UnitTestFrameworkPkg, StandaloneMmPkg, EmulatorPkg

---

## Environment Configuration

**Runners & Images**:
- **Linux**: ubuntu-24.04 runner with custom container `ghcr.io/mdkinney/containers/ubuntu-24-build:latest`
- **Linux (QEMU Tests)**: Custom container `ghcr.io/mdkinney/containers/ubuntu-24-test:latest`
- **Windows**: windows-2025 runner

**Build Tools**:
- Python 3.12
- Toolchains: VS2022, GCC, CLANGPDB, CLANGDWARF, GCCNOLTO, MinGW variants, LLVM 20

---

## GitHub Actions vs Azure Pipelines

| Aspect | GitHub Actions | Azure Pipelines |
|--------|---------|---------|
| **Trigger** | PR to specific branches (sandbox/master, clang_ci_v5*) | PR to master, stable/* |
| **Stages** | 3-stage progressive (Stage 0→1→2) | Matrix-based parallel |
| **Toolchains** | 9 toolchains (multiple CLANG, GCCNOLTO variants, LLVM 20) | 2 toolchains (VS2022, GCC) |
| **Architecture Support** | IA32, X64, AARCH64, RISCV64 | IA32, X64, AARCH64, RISCV64, LOONGARCH64 |
| **Packages Tested** | 25 packages (all default) | 27 packages (split by category) |
| **BAT (QEMU Testing)** | Yes (Stage 1) - all platforms | No |
| **EmulatorPkg Testing** | Explicit builds in Stage 1 & 2 | Separate jobs |
| **Primary Focus** | Sandbox branches, experimental/extended toolchain validation | Master branch rapid validation |
| **Test Duration** | 30-60 minutes | 15-30 minutes |

---

## Package Scope: Feature Comparison

### **What Azure Pipelines Tests That GitHub Actions Does Not**

#### 1. **Categorical Package Isolation**
- Azure Pipelines builds packages in logically grouped categories (Arm, MDE, Network, etc.), ensuring category-level integration
- Allows isolated testing of package groups and their interdependencies
- Example: MdeModulePkg tested separately in DEBUG and RELEASE jobs

#### 2. **LOONGARCH64 Architecture**
- **Only in Azure Pipelines** via Linux GCC
- GitHub Actions limited to IA32, X64, AARCH64, RISCV64
- Critical for LOONGSON platform support validation

#### 3. **UefiPayloadPkg Architecture Split Testing**
- Azure Pipelines splits UefiPayloadPkg testing:
  - Dedicated IA32/X64 job
  - Dedicated AARCH64 job (GCC only)
- GitHub Actions tests it as part of batched packages
- Enables targeted debugging of architecture-specific issues

#### 4. **Package-Specific Build Target Tuning**
- CryptoPkg: Split into separate DEBUG and RELEASE jobs (due to size/build time)
- MdeModulePkg: Split into DEBUG and RELEASE jobs
- Other packages: Grouped by functionality
- Allows fine-grained control and faster failure identification

#### 5. **Code-Only (NO-TARGET) Validation at Package Level**
- Azure Pipelines explicitly tests NO-TARGET for many packages
- Validates syntax/structure without full compilation
- GitHub Actions uses NO-TARGET only for Stage 1 code quality (single X64 job)

---

### **What GitHub Actions Tests That Azure Pipelines Does Not**

#### 1. **QEMU-Based Firmware Validation (Stage 1 BAT)**
- **Only in GitHub Actions** - integrated into Stage 1
- QEMU emulation tests actual firmware execution behavior
- Tests across all 10 BAT configurations (Linux CLANG, GCC, Windows variants)
- Validates:
  - IA32/X64/AARCH64/RISCV64 firmware boots and runs
  - Unit tests execute correctly
  - EmulatorPkg functionality
- Azure Pipelines performs no firmware validation

#### 2. **Extended Toolchain Coverage**
| Toolchain | GitHub Actions | Azure Pipelines |
|-----------|:---:|:---:|
| **VS2022** | ✓ | ✓ |
| **GCC** | ✓ | ✓ |
| **GCCNOLTO** | ✓ (optional) | ✗ |
| **CLANGPDB** | ✓ (Linux + Windows) | ✗ |
| **CLANGDWARF** | ✓ (Linux + Windows) | ✗ |
| **MinGW EDK2** | ✓ | ✗ |
| **MinGW LLVM 20** | ✓ | ✗ |
| **VS LLVM 20** | ✓ | ✗ |

- GitHub Actions validates 7 additional toolchains
- Tests experimental and alternative compiler configurations
- Enables early detection of compiler compatibility issues

#### 3. **Integrated Host-Based Unit Testing at Package Level**
- GitHub Actions Stage 1 BAT includes unit test execution for applicable packages
- Built into firmware validation pipeline (not separate)
- Tests on all supported architectures (IA32, X64)
- Azure Pipelines runs unit tests as part of full matrix, not in BAT phase

#### 4. **EmulatorPkg Active Validation**
- GitHub Actions: EmulatorPkg explicitly built and tested in Stage 1 and Stage 2
- Azure Pipelines: EmulatorPkg only analyzed for code correctness (NO-TARGET)
- GitHub Actions actually runs EmulatorPkg with host-based tests

#### 5. **Full Package Integration Testing**
- GitHub Actions tests all 25 packages together in each toolchain configuration
- Single job builds entire package set (Batch 1 + Batch 2) with same toolchain
- Validates inter-package dependencies across entire ecosystem
- Azure Pipelines tests packages in isolated groups per job
- Cannot detect cross-category integration issues as easily

#### 6. **RISCV64 Architecture Coverage**
- GitHub Actions: Full validation on RISCV64 across 3 Linux toolchains (CLANG variants, GCC)
- Azure Pipelines: Only GCC on RISCV64, no alternative toolchain validation
- GitHub Actions provides better RISCV64 ecosystem confidence

#### 7. **Progressive Stage Validation**
- GitHub Actions Stage 0: Detects which packages actually changed (skip unnecessary builds)
- GitHub Actions Stage 1: Fast BAT feedback before full builds (catch issues early)
- GitHub Actions Stage 2: Only runs full tests after BAT passes
- Azure Pipelines: Runs all matrix jobs regardless, no early-exit capability

#### 8. **Continue-on-Error Semantics**
- GitHub Actions allows marking known issues (e.g., GCCNOLTO) as non-blocking
- GitHub Actions GCCNOLTO tests run but don't fail the PR
- Azure Pipelines has no mechanism to continue on specific toolchain failures

---

## Ideal Coverage: Combined Approach

For **maximum assurance**, a PR should ideally pass both:

**Azure Pipelines** provides:
- ✓ Rapid validation (15-30 min)
- ✓ Focused master/stable branch criteria
- ✓ LOONGARCH64 support validation
- ✓ Package category isolation verification
- ✓ Code-only syntax checks

**GitHub Actions** provides:
- ✓ Extended toolchain validation (7 additional compilers)
- ✓ QEMU-based firmware correctness (executable validation)
- ✓ Complete package integration testing
- ✓ RISCV64 multi-toolchain support
- ✓ Early-exit stage validation
- ✓ Alternative compiler ecosystem confidence

**PR Recommendations**:
- **master/stable branches**: Must pass Azure Pipelines (fastest feedback)
- **sandbox/experimental branches**: Should pass GitHub Actions (comprehensive validation)
- **Critical changes**: Both pipelines recommended for maximum confidence

---

## References

**GitHub Actions Files**:
- Trigger: `.github/workflows/run-staged-build.yml`
- Orchestration: `.github/workflows/staged-build.yml`
- Build workflows: `.github/workflows/ubuntu-gcc-build.yml`, `ubuntu-clang-build.yml`, `windows-vs-build.yml`, etc.
- CI Settings: `.pytool/CISettings.py`

**Azure Pipelines Files**:
- Trigger: `.azurepipelines/Windows-VS.yml`, `.azurepipelines/Ubuntu-GCC.yml`, `.azurepipelines/Ubuntu-PatchCheck.yml`
- Templates: `.azurepipelines/templates/pr-gate-build-job.yml`, `pr-gate-steps.yml`
- CI Settings: `.pytool/CISettings.py`
- Patch Check: `BaseTools/Scripts/PatchCheck.py`
- Python Requirements: `pip-requirements.txt`
