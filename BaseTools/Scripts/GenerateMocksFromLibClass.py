### @file
#  Convert EDK II include files to mock libraries
#
# Copyright (C) 2024, Intel Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
# Known Limitations
# -----------------
# 1) Vararg functions are skipped by the parser and no mock function is
#    generated.  If '...' argument is replaced, then a mock function can be
#    generated, but the mock library can not be compiled because the mocked
#    function and the C declaration of the function do not match. The '...'
#    replacement can be done by calling CParser() with a replace argument set
#        replace = {r",\s*\.\.\.\s*\)": ")"}
#
# 2) const, CONST global variables are skipped because they require initial
#    values and this generator does not know what value to assign from the the
#    library include file alone. If unit tests depend on mocks of const global
#    variables, then the unit test must provide the const global variable.
#
# 3) Function with parameters that are function pointers declared inline are
#    skipped by the parser. The workaround is to declare a function pointer type
#    and use the function pointer type name in the parameter.  Example:
#
#      void qsort(void *,size_t,size_t,int (*)(const void *, const void *));
#
#    Change to:
#
#      typedef int (qsort_callback *)(const void *, const void *);
#      void qsort(void *,size_t,size_t,qsort_callback);
#
# 4) Packages that provide a gasket to a standard C library are not compatible
#    with host based build of mock libraries that use standard C includes.  The
#    standard C includes from the package is used instead of the host compiler.
#    The include path ordering is -I options are higher priority than the C
#    compiler standard include paths. The only way to resolve this issue is to
#    explicitly list the C compiler standard include paths ith -I options before
#    the -I options from the packages are added.  The packages impacted by this
#    limitation are:
#      * CryptoPkg
#      * RedfishPkg
###

import os
import sys
import argparse
import uuid
import yaml
import glob
from pyclibrary import CParser
from collections import OrderedDict

#
# Globals for help information
#
__prog__ = "GenerateMocks"
__copyright__ = "Copyright (c) 2024, Intel Corporation."
__description__ = "Generate Google Test mocks from EDK II include files.\n"

#
# All mock libraries depend on MdePkg and UnitTestFrameworkPkg
#
EDKII_DEFAULT_PACKAGES = [
    "MdePkg/MdePkg.dec",
    "UnitTestFrameworkPkg/UnitTestFrameworkPkg.dec",
]

#
# UEFI Specification calling convention keyword
#
UEFI_CALLING_CONVENTION = 'EFIAPI'

class IncludeFileParser:
    #
    # Regular expression to replace variable argument list declaration ... with
    # nothing so a variable argument function is converted to a mock function
    # with no variable argument parameters. This is only used when CheckUnparsed
    # is enabled to check for any additional unparsed content after variable
    # argument function are converted to non variable argument functions.
    #
    STRIP_VARARGS = {r",\s*\.\.\.\s*\)": ")"}

    #
    # Replace OPTIONAL keyword with an array of size __OPTIONAL__
    #
    OPTIONAL_ARRAY_SIZE = "___OPTIONAL___"

    #
    # Replace EDK II specific keywords with attributes to preserve the keywords
    # in the mocked interfaces.
    #
    EDKII_MACROS = {
        "RETURNS_TWICE": "__attribute__(RETURNS_TWICE)",
        UEFI_CALLING_CONVENTION: f"__attribute__({UEFI_CALLING_CONVENTION})",
        "STATIC": "__attribute__(STATIC)",
        "IN": "__attribute__(IN)",
        "OUT": "__attribute__(OUT)",
        "CONST": "__attribute__(CONST)",
        "OPTIONAL": f"[{OPTIONAL_ARRAY_SIZE}]",
        "EDKII_UNIT_TEST_FRAMEWORK_ENABLED": "1",
    }

    #
    # Default set of CPU Types to parse from include files
    #
    CPU_TYPE_LIST = ["IA32", "X64", "ARM", "AARCH64", "RISCV64", "LOONGARCH64"]

    def __init__(self, FileName, CpuTypeList=CPU_TYPE_LIST, Replace={}, Macros={}):
        self.FileName = FileName
        self.CpuTypeList = ["COMMON"]
        self.Replace = Replace
        self.Macros = IncludeFileParser.EDKII_MACROS.copy()
        self.Macros.update(Macros)
        self.Include = OrderedDict()
        self.Tokens = OrderedDict()
        self.MockVariables = OrderedDict()
        self.MockFunctions = OrderedDict()
        self.Unparsed = OrderedDict()

        self.CollectCpuTypes(CpuTypeList)

        for CpuType in self.CpuTypeList:
            if args.CheckUnparsed:
                if args.Verbose:
                    print("  Check Unparsed")
                Replace = IncludeFileParser.STRIP_VARARGS.copy()
                Replace.update(self.Replace)
                Unparsed = self.ParseCpuType(CpuType, Replace=Replace, Unparsed=True)
                Unparsed = Unparsed.strip()
                if Unparsed:
                    print("")
                    for Line in Unparsed.splitlines():
                        print("  ERROR: * ", Line)
                    self.Unparsed[CpuType] = Unparsed
                self.Tokens[CpuType] = self.ParseCpuType(
                    CpuType, Replace=Replace, Unparsed=False
                )
                MockFunctionsWithVarArgs = self.Include[CpuType].defs["functions"]
            self.Tokens[CpuType] = self.ParseCpuType(
                CpuType, Replace=self.Replace, Unparsed=False
            )
            if args.CheckUnparsed:
                Skipped = set(MockFunctionsWithVarArgs) - set(self.Include[CpuType].defs["functions"])
                if Skipped:
                    print("")
                for Item in Skipped:
                    print (f"  WARNING: * Skipped VARARG Function: {Item}")
        if self.Unparsed:
            sys.exit(f"  ERROR: Unparsed content in {self.FileName}")

        self.CollectVariables()
        self.CollectFunctions()

    def CollectCpuTypes(self, CpuTypeList):
        with open(self.FileName, "r") as f:
            Buffer = f.read()
            for CpuType in [f"MDE_CPU_{x}" for x in CpuTypeList]:
                if CpuType in Buffer:
                    self.CpuTypeList.append(CpuType)

    def ParseCpuType(self, CpuType, Replace={}, Unparsed=False):
        if args.Verbose:
            print(f"{__prog__}: Parse {self.FileName} for CPU Type {CpuType}")
        else:
            print(
                f"{__prog__}: Parse {self.FileName} for CPU Type {CpuType}",
                end="\033[K\r",
            )
        EdkiiMacros = self.Macros.copy()
        if CpuType != "COMMON":
            EdkiiMacros[CpuType] = ""
        if args.Verbose:
            print("  Create Parser")
        self.Include[CpuType] = CParser(
            self.FileName, replace=Replace, macros=EdkiiMacros, process_all=False
        )
        if args.Verbose:
            print("  Remove Comments")
        self.Include[CpuType].remove_comments(self.FileName)
        if args.Verbose:
            print("  Preprocess Macros")
        self.Include[CpuType].preprocess(self.FileName)
        if args.Verbose:
            print("  Parse Definitions")
        return self.Include[CpuType].parse_defs(self.FileName, return_unparsed=Unparsed)

    def CollectVariables(self):
        for CpuType in self.CpuTypeList:
            self.MockVariables[CpuType] = OrderedDict()
            for VariableName in self.Include[CpuType].defs["variables"]:
                if CpuType != "COMMON" and VariableName in self.MockVariables["COMMON"]:
                    continue
                Var = self.MockVariable(self, CpuType, VariableName)
                Skip = False
                for Qual in ["CONST", "const", "STATIC", "static"]:
                    if Qual in Var.Quals:
                        Skip = True
                if not Skip:
                    self.MockVariables[CpuType][VariableName] = Var
            if not self.MockVariables[CpuType]:
                self.MockVariables.pop(CpuType)
        # Determine column width for variable qualifiers and variable types
        for CpuType in self.MockVariables:
            QualsWidth = max(
                [x.QualsWidth for x in self.MockVariables[CpuType].values()]
            )
            TypeWidth = max([x.TypeWidth for x in self.MockVariables[CpuType].values()])
            for Variable in self.MockVariables[CpuType].values():
                Variable.QualsWidth = QualsWidth
                Variable.TypeWidth = TypeWidth

    def CollectFunctions(self):
        for CpuType in self.CpuTypeList:
            self.MockFunctions[CpuType] = OrderedDict()
            for FunctionName in self.Include[CpuType].defs["functions"]:
                if CpuType != "COMMON" and FunctionName in self.MockFunctions["COMMON"]:
                    continue
                F = self.MockFunction(self, CpuType, FunctionName)
                if len(F.ParameterList) > 15:
                    print(f"  WARNING: * Skipped Function: {FunctionName} with {len(F.ParameterList)} parameters")
                    continue
                self.MockFunctions[CpuType][FunctionName] = F
            if not self.MockFunctions[CpuType]:
                self.MockFunctions.pop(CpuType)

    def _ParseStrings(TypeQuals):
        def _ParseStringsRecursive(TypeQuals, Result, Exclude=[]):
            if isinstance(TypeQuals, int) or TypeQuals is None:
                TypeQuals = str(TypeQuals)
                if TypeQuals and TypeQuals not in Exclude:
                    Result.append(f"[{TypeQuals}]")
                return Result
            if isinstance(TypeQuals, str):
                TypeQuals = TypeQuals.strip()
                if TypeQuals and TypeQuals not in Exclude:
                    Result.append(TypeQuals)
                return Result
            try:
                for Item in TypeQuals:
                    Result = _ParseStringsRecursive(Item, Result, Exclude)
            except:
                pass
            return Result

        Quals = _ParseStringsRecursive(TypeQuals, [], ["__", "__attribute__"])
        return " ".join(Quals)

    def _GetArraySubscriptsFromItem(Item):
        Result = []
        if getattr(Item, 'arrays', ''):
            for Subscript in Item.arrays:
                if Subscript == "-1":
                    # Convert [-1] to []
                    Subscript = ""
                Result.append(Subscript)
        return Result

    def _ParseTokensForArraySubscripts(Tokens):
        Result = IncludeFileParser._GetArraySubscriptsFromItem (Tokens)
        for Item in Tokens:
            Result += IncludeFileParser._GetArraySubscriptsFromItem (Item)
        return Result

    def _ParseTypeQuals(Declaration, Tokens, ItemName, ParseType=''):
        SubscriptList = IncludeFileParser._ParseTokensForArraySubscripts(Tokens)
        Type = Declaration.type_spec
        CallingConvention = set()
        Optional = False
        Subscripts = ''
        Quals = []
        DecList = []
        PointerAtEnd = False
        DeclaratorsIndex = 0
        SubscriptIndex = 0
        for TypeQual in Declaration.type_quals:
            TypeQual = IncludeFileParser._ParseStrings(TypeQual).split()
            CallingConvention |= set(TypeQual) & set(args.CallingConventions)
            TypeQual = [item for item in TypeQual if item not in args.CallingConventions]
            if DeclaratorsIndex == 0:
                Quals += TypeQual
            else:
                DecList += TypeQual

            if DeclaratorsIndex < len(Declaration.declarators):
                Declarator = IncludeFileParser._ParseStrings(Declaration.declarators[DeclaratorsIndex]).strip()
                PointerAtEnd = Declarator == '*'
                if Declarator.startswith('[') and Declarator.endswith(']'):
                    if SubscriptIndex < len(SubscriptList):
                        if SubscriptList[SubscriptIndex] == IncludeFileParser.OPTIONAL_ARRAY_SIZE:
                            Optional = True
                        else:
                            Subscripts += f"[{SubscriptList[SubscriptIndex]}]"
                        SubscriptIndex += 1
                else:
                    DecList.append(Declarator)
            DeclaratorsIndex += 1

        if len(CallingConvention) > 1:
            sys.exit(f"{__prog__}: Multiple calling conventions detected {CallingConvention}")

        Result = f"{' '.join(DecList)} {ItemName}"
        if PointerAtEnd and Subscripts:
            if ParseType == "Return" or not Subscripts.endswith("[]"):
                Result = "(" + Result + ")"
        Result = (Result + Subscripts).replace('* ', '*')

        return ''.join(list(CallingConvention)), ' '.join(Quals), Type.strip(), Result.strip(), Optional

    class MockVariable:
        def __init__(self, Parser, CpuType, VariableName):
            self.VariableName = VariableName
            self.Variable = Parser.Include[CpuType].defs["variables"][VariableName]
            for Tokens in Parser.Tokens[CpuType]:
                try:
                    if Tokens[0].decl_list[0].name == VariableName:
                        self.VariableTokens = Tokens[0].decl_list[0]
                        break
                except:
                    pass
            Conv, self.Quals, self.Type, self.VariableName, Optional = IncludeFileParser._ParseTypeQuals(self.Variable[1], self.VariableTokens, VariableName, ParseType="Variable")
            self.QualsWidth = len(self.Quals)
            self.TypeWidth = len(self.Type)

        def GenerateDefinition(self):
            quals = ""
            if self.QualsWidth:
                quals = f"{self.Quals:<{self.QualsWidth + 1}}"
            return f"{quals}{self.Type:<{self.TypeWidth + 2}}{self.VariableName};"

    class MockFunction:
        class MockReturn:
            def __init__(self, Return, ReturnTokens):
                self.Return = Return
                self.ReturnTokens = ReturnTokens
                self.CallingConvention, Quals, Type, Name, Optional = IncludeFileParser._ParseTypeQuals(Return, ReturnTokens, "", ParseType="Return")
                self.ReturnName = f"{Quals} {Type} {Name}".strip()

        class MockParameter:
            def __init__(self, Parameter, ParameterTokens):
                self.Parameter = Parameter
                self.ParameterTokens = ParameterTokens
                Name = ""
                if Parameter[0]:
                    Name = self.Parameter[0]
                C, self.Quals,self.Type,self.ParameterName,self.Optional = IncludeFileParser._ParseTypeQuals(Parameter[1], ParameterTokens, Name, ParseType="Param")
                self.QualsWidth = len(self.Quals)
                self.TypeWidth = len(self.Type)

            def GenerateDeclaration(self):
                quals = ""
                if self.QualsWidth:
                    quals = f"{self.Quals:<{self.QualsWidth + 1}}"
                type = f"{self.Type:<{self.TypeWidth + 2}}"
                optional = " OPTIONAL" if self.Optional else ""
                return f"{quals}{type}{self.ParameterName}{optional}"

        def __init__(self, Parser, CpuType, FunctionName):
            self.FunctionName = FunctionName
            self.Function = Parser.Include[CpuType].defs["functions"][FunctionName]
            self.ReturnTokens = None
            self.ArgTokens = None
            for Tokens in Parser.Tokens[CpuType]:
                try:
                    if Tokens[0].decl_list[0].name == FunctionName:
                        self.ReturnTokens = Tokens[0].decl_list[0]
                        self.ArgTokens = Tokens[0].decl_list[0]
                        break
                except:
                    pass
                try:
                    if Tokens[0].decl_list[0].center[0].name == FunctionName:
                        self.ReturnTokens = Tokens[0].decl_list[0]
                        self.ArgTokens = Tokens[0].decl_list[0].center[0]
                        break
                except:
                    pass
            R = self.MockReturn(self.Function[0], self.ReturnTokens)
            self.ReturnType = R.ReturnName
            self.CallingConvention = R.CallingConvention
            self.ParameterList = []
            ArgIndex = 0
            for Parameter in self.Function[1]:
                P = self.MockParameter(Parameter, self.ArgTokens.args[ArgIndex])
                if not (P.ParameterName == "" and P.Type.lower() == "void"):
                    self.ParameterList.append(P)
                ArgIndex += 1
            if self.ParameterList:
                QualsWidth = max([x.QualsWidth for x in self.ParameterList])
                TypeWidth = max([x.TypeWidth for x in self.ParameterList])
                for Parameter in self.ParameterList:
                    Parameter.QualsWidth = QualsWidth
                    Parameter.TypeWidth = TypeWidth

        def GenerateDefinition(self):
            return f"MOCK_FUNCTION_DEFINITION (Mock{{LibName}}, {self.FunctionName}, {len(self.ParameterList)}, {self.CallingConvention});"

        def GenerateDeclaration(self):
            Parameters = [x.GenerateDeclaration() for x in self.ParameterList]
            Parameters = ",\n     ".join(Parameters)
            return f"""
  MOCK_FUNCTION_DECLARATION (
    {self.ReturnType},
    {self.FunctionName},
    ({Parameters})
    );"""


class Template:
    def __init__(self, YamlFile):
        self.Replacements = {}
        self.yaml_doc = []
        try:
            with open(YamlFile) as f:
                yaml_file = yaml.load_all(f, Loader=yaml.loader.SafeLoader)
                for yaml_doc in yaml_file:
                    if args.Verbose:
                        print(__prog__, ": YAML file version: " + yaml_doc["version"])
                    self.yaml_doc.append(yaml_doc)
            if len(self.yaml_doc) != 1:
                raise
        except:
            sys.exit(f"{__prog__}: Invalid YAML Template file: {YamlFile}")

    def SetValue(self, Name, Value):
        self.Replacements[Name] = Value

    def ApplyValues(self, buffer):
        for Name in self.Replacements:
            buffer = buffer.replace(Name, self.Replacements[Name])
        return buffer

    def ApplyTemplate(self, templatetype):
        for file in self.yaml_doc[0]["filelist"]:
            filetype = file["type"]
            if templatetype not in filetype:
                continue
            return self.ApplyValues(file["content"])

    def WriteTemplate(self, templatetype, rootpath, filecontent):
        for file in self.yaml_doc[0]["filelist"]:
            filetype = file["type"]
            if templatetype not in filetype:
                continue
            filepath = self.ApplyValues(file["path"])
            filename = self.ApplyValues(file["name"])
            fullpath = os.path.normpath(os.path.join(rootpath, filepath))
            fullpathfile = os.path.normpath(os.path.join(fullpath, filename))
            if args.Verbose:
                print(f"{__prog__}: Output: {fullpathfile}")
            if not os.path.exists(fullpath):
                os.makedirs(fullpath, exist_ok=True)
            if os.path.exists(fullpathfile) and not args.Force:
                print(
                    f"{__prog__}: ERROR: {fullpathfile} already exists.  Use -f/--force to force overwrite."
                )
                return fullpathfile
            with open(fullpathfile, "w") as outputfile:
                outputfile.write(filecontent)
            return fullpathfile


class MockLibraryGenerator:
    def __init__(self, Templates, Include, OutputDirectory="."):
        self.Templates = Templates
        self.Include = Include
        self.OutputDirectory = OutputDirectory
        self.IncludeFileTemplateType = "library-h"
        self.CppFileTemplateType = "library-cpp"
        self.InfFileTemplateType = "library-inf"
        self.DscFileTemplateType = "library-dsc"

    def GenerateMockIncludeFile(self):
        FuncDecl = []
        for CpuType in self.Include.MockFunctions:
            if CpuType != "COMMON":
                FuncDecl.append(f"\n #if defined ({CpuType})")
            for Function in self.Include.MockFunctions[CpuType].values():
                FuncDecl.append(Function.GenerateDeclaration())
            if CpuType != "COMMON":
                FuncDecl.append(f" #endif // defined ({CpuType})")
        self.Templates.SetValue("{MockFunctionDeclarations}", "")
        if FuncDecl:
            self.Templates.SetValue(
                "{MockFunctionDeclarations}", "\n" + "\n".join(FuncDecl)
            )

        return self.Templates.WriteTemplate(
            self.IncludeFileTemplateType,
            self.OutputDirectory,
            self.Templates.ApplyTemplate(self.IncludeFileTemplateType),
        )

    def GenerateMockCppFile(self):
        VarDef = []
        for CpuType in self.Include.MockVariables:
            if CpuType != "COMMON":
                VarDef.append(f"\n#if defined ({CpuType})")
            for Variable in self.Include.MockVariables[CpuType].values():
                VarDef.append(Variable.GenerateDefinition())
            if CpuType != "COMMON":
                VarDef.append(f"#endif // defined ({CpuType})")
        self.Templates.SetValue("{GlobalVariables}", "")
        if VarDef:
            self.Templates.SetValue("{GlobalVariables}", "\n" + "\n".join(VarDef))

        FuncDef = []
        for CpuType in self.Include.MockFunctions:
            if CpuType != "COMMON":
                FuncDef.append(f"\n#if defined ({CpuType})")
            for Function in self.Include.MockFunctions[CpuType].values():
                FuncDef.append(Function.GenerateDefinition())
            if CpuType != "COMMON":
                FuncDef.append(f"#endif // defined ({CpuType})")
        FuncDef = self.Templates.ApplyValues("\n".join(FuncDef))
        self.Templates.SetValue("{MockFunctionDefinitions}", "")
        if FuncDef:
            self.Templates.SetValue("{MockFunctionDefinitions}", "\n\n" + FuncDef)

        return self.Templates.WriteTemplate(
            self.CppFileTemplateType,
            self.OutputDirectory,
            self.Templates.ApplyTemplate(self.CppFileTemplateType),
        )

    def GenerateMockInfFile(self):
        return self.Templates.WriteTemplate(
            self.InfFileTemplateType,
            self.OutputDirectory,
            self.Templates.ApplyTemplate(self.InfFileTemplateType),
        )

def MergeDefines(Defines, OverrideDefines):
    Dict = OrderedDict()
    for Item in Defines:
        Item = Item.split()
        Dict[Item[1]] = Item[2]
    OverrideDict = OrderedDict()
    for Item in OverrideDefines:
        Item = Item.split()
        OverrideDict[Item[1]] = Item[2]
    Dict.update(OverrideDict)
    Result = []
    for Item in Dict:
        Result.append(f"#define {Item} {Dict[Item]}")
    return Result

def FindParentDecFile(File):
    if not os.path.isfile(File):
        return ''
    Split = os.path.split(File)
    while Split[0]:
        if not os.path.isdir(Split[0]):
            return ''
        for File in os.listdir(Split[0]):
            if os.path.splitext(File)[1] in [".dec"]:
                return os.path.join(Split[0], File)
        Split = os.path.split(Split[0])
    return ''

def NormalizePackageDecFilePath(DecFile):
    if not DecFile:
        return ''
    DecFile = os.path.split(DecFile)
    PackageDir = os.path.split(DecFile[0])[1]
    return PackageDir + '/' + DecFile[1]

def FindParentPackage(File):
    return NormalizePackageDecFilePath(FindParentDecFile(File))

def ParseBaseName(FileName):
    BaseName = os.path.splitext(os.path.basename(FileName))[0]
    BASE_NAME = ""
    for c in BaseName:
        if c.isupper():
            BASE_NAME += "_"
        BASE_NAME += c.upper()
    BASE_NAME = BASE_NAME.lstrip("_")
    return BaseName, BASE_NAME


def ProcessFiles(Templates, FileList, OutputDirectory):
    InfFileList = []
    for FileName in FileList:
        LibName, LIB_NAME = ParseBaseName(FileName)

        Templates.SetValue("{LibName}", LibName)
        Templates.SetValue("{LIB_NAME}", LIB_NAME)
        Templates.SetValue("{FileGuid}", str(uuid.uuid5(uuid.NAMESPACE_DNS, FileName)))

        Include = IncludeFileParser(FileName, Macros=args.Defines)
        if not Include.MockVariables and not Include.MockFunctions:
            print(f"{__prog__}: Skip {FileName} with no functions or global variables")
            continue

        Generator = MockLibraryGenerator(Templates, Include, OutputDirectory)
        Generator.GenerateMockIncludeFile()
        Generator.GenerateMockCppFile()
        InfFile = Generator.GenerateMockInfFile()
        InfFileList.append(InfFile)
    return InfFileList


def main():
    global args

    #
    # Create command line argument parser object
    #
    parser = argparse.ArgumentParser(
        prog=__prog__,
        description=__description__ + __copyright__,
        conflict_handler="resolve",
    )
    group = parser.add_mutually_exclusive_group()
    group.add_argument(
        "-i",
        "--input",
        dest="InputPath",
        action="extend",
        nargs="*",
        default=[],
        help="Input EDK II include file or directory paths to generate mocks.",
    )
    group.add_argument(
        "-r",
        "--repository",
        dest="RepositoryPath",
        help="Input EDK II repository to scan for include files to generate mocks.",
    )
    parser.add_argument(
        "-y",
        "--yaml",
        dest="InputYaml",
        help="Input YAML files that provides all options.",
    )
    parser.add_argument(
        "-s",
        "--skip-package",
        dest="SkipPackages",
        action="extend",
        nargs="*",
        default=[],
        help="Package DEC files to skip when generating mocks for a repository.",
    )
    parser.add_argument(
        "-D",
        dest="Defines",
        action="extend",
        nargs="*",
        default=[],
        help="Defines to set when parsing EDK II include files.",
    )
    parser.add_argument(
        "-g",
        "--global-package-dependency",
        dest="GlobalPackageDependencies",
        action="extend",
        nargs="*",
        default=EDKII_DEFAULT_PACKAGES,
        help="Extra package dependencies for all EDK II INF [Packages] sections.",
    )
    parser.add_argument(
        "-p",
        "--package-dependency",
        dest="PackageDependencies",
        action="extend",
        nargs="*",
        default=[],
        help="Extra package dependencies for package specific EDK II INF [Packages] sections.",
    )
    parser.add_argument(
        "-e",
        "--extra-define",
        dest="ExtraDefines",
        action="extend",
        nargs="*",
        default=[],
        help="Extra defines for generated mock include files.",
    )
    parser.add_argument(
        "-a",
        "--calling-convention",
        dest="CallingConventions",
        action="extend",
        nargs="*",
        default=[UEFI_CALLING_CONVENTION],
        help="Calling convention attribute keywords for generated mock libraries.",
    )
    parser.add_argument(
        "-o",
        "--output",
        dest="OutputDirectory",
        default="",
        help="Output EDK II Test directory for generate mocks.  Package relative path for -r/--repository",
    )
    parser.add_argument(
        "-t",
        "--test-dsc",
        dest="TestDsc",
        default="",
        help="Output DSC file to test build of generated mocks.",
    )
    parser.add_argument(
        "-c",
        "--copyright",
        dest="Copyright",
        help="Copyright to apply to generated output files.",
    )
    parser.add_argument(
        "-f",
        "--force",
        dest="Force",
        action="store_true",
        help="Force writing to output files that already exist",
    )
    parser.add_argument(
        "-u",
        "--unparsed",
        dest="CheckUnparsed",
        action="store_true",
        help="Print warning if unparsable content is detected. VARAGS are not parsable",
    )
    parser.add_argument(
        "-v",
        "--verbose",
        dest="Verbose",
        action="store_true",
        help="Increase output messages",
    )
    parser.add_argument(
        "-q",
        "--quiet",
        dest="Quiet",
        action="store_true",
        help="Reduce output messages",
    )
    parser.add_argument(
        "--debug",
        dest="Debug",
        type=int,
        metavar="[0-9]",
        choices=range(0, 10),
        default=0,
        help="Set debug level",
    )

    #
    # Parse command line arguments
    #
    args = parser.parse_args()

    #
    # Convert args.PackageDependencies to match parsed YAML format so the YAML
    # and command line settings can be merged
    #
    if args.PackageDependencies:
        Deps = []
        for Package in args.PackageDependencies:
            Package = Package.split("=", 1)
            if len(Package) == 1:
                sys.exit(f"{__prog__}: Invalid package dependency {Package}")
            if not Package[1]:
                sys.exit(f"{__prog__}: Invalid package dependency {Package}")
            Item = {}
            Item["PackageName"] = Package[0]
            Item["Dependencies"] = Package[1].split(",")
            Item["ExtraDefines"] = []
            Deps.append(Item)
        args.PackageDependencies = Deps

    if args.Verbose:
        #
        # Print arguments from command line
        #
        print("Command line options")
        for Option in vars(args):
            print(f"  {Option}: {getattr(args, Option)}")

    if args.InputYaml:
        #
        # Parse settings from YAML file
        #
        config = []
        try:
            with open(args.InputYaml) as f:
                yaml_file = yaml.load_all(f, Loader=yaml.loader.SafeLoader)
                for doc in yaml_file:
                    config.append(doc)
        except Exception as exc:
            sys.exit(f"{__prog__}: Error parsing YAML file {args.InputYaml}\n{exc}")
        if len(config) != 1:
            sys.exit(f"{__prog__}: Invalid YAML file {args.InputYaml}")
        config = config[0]

        #
        # Update args with settings from YAML file.
        # Settings from command line override YAML settings
        #
        for Option in config:
            if Option not in vars(args):
                sys.exit(f"{__prog__}: Unsupported option {Option} in {args.InputYaml}.")
        for Option in vars(args):
            if Option in config:
                if not getattr(args, Option):
                    setattr(args, Option, config[Option])
                    continue
                if isinstance(getattr(args, Option), list):
                    setattr(args, Option, config[Option] + getattr(args, Option))

    #
    # Copyright argument is always required to generate any files
    #
    if not args.Copyright:
        parser.print_help()
        sys.exit(f"{__prog__}: -c/--copyright option not specified.")

    #
    # OutputDirectory argument is always required to generate any files
    #
    if not args.OutputDirectory:
        parser.print_help()
        sys.exit(f"{__prog__}: -o/--output option not specified.")

    #
    # Convert YAML and command line defines settings into a dictionary for use
    # by CParser
    #
    Macros = {}
    for Define in args.Defines:
        Define = Define.split("=", 1)
        if len(Define) == 1:
            Macros[Define[0]] = ""
        else:
            Macros[Define[0]] = Define[1]
    args.Defines = Macros

    #
    # Convert YAML and command line extra defines settings into a list of
    # #define statements
    #
    ExtraDefines = []
    for Define in args.ExtraDefines:
        Define = Define.split("=", 1)
        if len(Define) == 1:
            sys.exit(f"{__prog__}: Extra define missing value {Define}")
        if not Define[1]:
            sys.exit(f"{__prog__}: Extra define missing value {Define}")
        ExtraDefines.append(f"#define {Define[0]}  {Define[1]}")
    args.ExtraDefines = ExtraDefines

    #
    # Convert YAML package specific extra defines settings into a list of
    # #define statements
    #
    for Package in args.PackageDependencies:
        if "ExtraDefines" not in Package:
            continue
        ExtraDefines = []
        for Define in Package["ExtraDefines"]:
            Define = Define.split("=", 1)
            if len(Define) == 1:
                sys.exit(f"{__prog__}: Extra define missing value {Define}")
            if not Define[1]:
                sys.exit(f"{__prog__}: Extra define missing value {Define}")
            ExtraDefines.append(f"#define {Define[0]}  {Define[1]}")
        Package["ExtraDefines"] = ExtraDefines

    #
    # Remove duplicate GlobalPackageDependencies attribute keywords
    #
    args.GlobalPackageDependencies = list(dict.fromkeys(args.GlobalPackageDependencies))

    #
    # Remove duplicate CallingConvention attribute keywords
    #
    args.CallingConventions = list(dict.fromkeys(args.CallingConventions))

    if args.Verbose:
        #
        # Print arguments from command line
        #
        print("Command line options merged with YAML settings")
        for Option in vars(args):
            print(f"  {Option}: {getattr(args, Option)}")

    #
    # Load code templates
    #
    TemplateFileName = os.path.join(
        os.path.realpath(os.path.dirname(__file__)),
        "GenerateMocksFromLibClassTemplates.yaml",
    )
    Templates = Template(TemplateFileName)

    #
    # Set the Copyright for template processing
    #
    Templates.SetValue("{Copyright}", args.Copyright)

    #
    # Set the extra defines for template processing
    #
    Templates.SetValue("{ExtraDefines}", "")
    if args.ExtraDefines:
        Templates.SetValue("{ExtraDefines}", "\n  ".join(args.ExtraDefines) + "\n  ")

    MockLibraryInfFiles = []
    if args.RepositoryPath:
        if not os.path.isdir(args.RepositoryPath):
            sys.exit(
                f"{__prog__}: ERROR: Repository path {args.RepositoryPath} does not exist."
            )

        #
        # Recursively search for all DEC file in RepositoryPath
        #
        SearchPath = os.path.join(args.RepositoryPath, "**", "*.dec")
        DecFiles = {}
        for DecFile in glob.glob(SearchPath, recursive=True):
            DecFiles[NormalizePackageDecFilePath(DecFile)] = DecFile

        #
        # Verify all GlobalPackageDependencies map to DEC files in the
        # repository
        #
        for Package in args.GlobalPackageDependencies:
            Package = NormalizePackageDecFilePath(Package)
            if Package not in DecFiles:
                if args.Verbose:
                    print(f"{__prog__}: WARNING: Global package dependency {Package} not into repository.")

        #
        # Verify all PackageDependencies map to DEC files in the repository
        #
        PackageDependencyDict = {}
        PackageExtraDefinesDict = {}
        for Item in args.PackageDependencies:
            Package = NormalizePackageDecFilePath(Item["PackageName"])
            if Package not in DecFiles:
                sys.exit(f"{__prog__}: Invalid package dependency {Package}")
            if "Dependencies" in Item:
                PackageDependencyList = []
                for Dependency in Item["Dependencies"]:
                    Dependency = NormalizePackageDecFilePath(Dependency)
                    if Dependency not in DecFiles:
                        if args.Verbose:
                            print(f"{__prog__}: WARNING: Dependent package {Dependency} not into repository.")
                    PackageDependencyList.append(Dependency)
                PackageDependencyDict[Package] = PackageDependencyList
            if "ExtraDefines" in Item:
                PackageExtraDefinesDict[Package] = Item["ExtraDefines"]

        #
        # Verify all SkipPackages map to DEC files in the repository
        #
        for Skip in args.SkipPackages:
            Skip = NormalizePackageDecFilePath(Skip)
            if Skip not in DecFiles:
                sys.exit(f"{__prog__}: Invalid skip package {Skip}")
            DecFiles.pop(Skip)

        #
        # Process all unskipped packages in the repository
        #
        for DecFile in DecFiles:
            PackageDir = os.path.split(DecFiles[DecFile])[0]
            PackageDir = os.path.join(args.RepositoryPath, PackageDir)
            LibIncludeDir = os.path.join(PackageDir, "Include", "Library")
            if not os.path.exists(LibIncludeDir):
                continue
            LibIncludeFiles = []
            for File in os.listdir(LibIncludeDir):
                if os.path.splitext(File)[1] in [".h", ".H"]:
                    LibIncludeFilePath = os.path.join(LibIncludeDir, File)
                    if LibIncludeFilePath not in LibIncludeFiles:
                        LibIncludeFiles.append(LibIncludeFilePath)
            if not LibIncludeFiles:
                continue
            MockOutputDir = os.path.join(PackageDir, args.OutputDirectory)

            #
            # [Packages] is EDKII_DEFAULT_PACKAGES with additional dependencies
            # for that specific package from YAML file and command line followed
            # by the current package with no duplicates added.
            #
            PackageDecFiles = []
            PackageDecFiles.append(DecFile)
            if DecFile in PackageDependencyDict:
                for Package in PackageDependencyDict[DecFile]:
                    if Package not in PackageDecFiles:
                        PackageDecFiles.append(Package)
            PackageDecFiles += args.GlobalPackageDependencies

            Templates.SetValue("{PackageDecFiles}", "\n  ".join(PackageDecFiles))

            Templates.SetValue("{ExtraDefines}", "")
            ExtraDefines = args.ExtraDefines
            if DecFile in PackageExtraDefinesDict:
                ExtraDefines = MergeDefines(ExtraDefines, PackageExtraDefinesDict[DecFile])
            if ExtraDefines:
                Templates.SetValue("{ExtraDefines}", "\n  ".join(ExtraDefines) + "\n  ")

            InfFiles = ProcessFiles(Templates, LibIncludeFiles, MockOutputDir)
            MockLibraryInfFiles += InfFiles
    else:
        SkipPackages = []
        for Skip in args.SkipPackages:
            SkipPackages.append(NormalizePackageDecFilePath(Skip))

        PackageDependencyDict = {}
        PackageExtraDefinesDict = {}
        for Item in args.PackageDependencies:
            Package = NormalizePackageDecFilePath(Item["PackageName"])
            if "Dependencies" in Item:
                PackageDependencyList = []
                for Dependency in Item["Dependencies"]:
                    Dependency = NormalizePackageDecFilePath(Dependency)
                    PackageDependencyList.append(Dependency)
                PackageDependencyDict[Package] = PackageDependencyList
            if "ExtraDefines" in Item:
                PackageExtraDefinesDict[Package] = Item["ExtraDefines"]

        FileList = []
        for Path in args.InputPath:
            if os.path.isfile(Path):
                FileList.append(Path)
            elif os.path.isdir(Path):
                for File in os.listdir(Path):
                    if os.path.splitext(File)[1] in [".h", ".H"]:
                        FileList.append(os.path.join(Path, File))
            else:
                sys.exit(f"{__prog__}: ERROR: Input path {Path} does not exist.")

        for File in FileList:
            DecFile = FindParentPackage(File)
            if DecFile in SkipPackages:
                continue

            #
            # [Packages] is EDKII_DEFAULT_PACKAGES with additional dependencies
            # for that specific package from YAML file and command line followed
            # by the current package with no duplicates added.
            #
            PackageDecFiles = []
            PackageDecFiles.append(DecFile)
            if DecFile in PackageDependencyDict:
                for Package in PackageDependencyDict[DecFile]:
                    if Package not in PackageDecFiles:
                        PackageDecFiles.append(Package)
            PackageDecFiles += args.GlobalPackageDependencies

            Templates.SetValue("{PackageDecFiles}", "\n  ".join(PackageDecFiles))

            Templates.SetValue("{ExtraDefines}", "")
            ExtraDefines = args.ExtraDefines
            if DecFile in PackageExtraDefinesDict:
                ExtraDefines = MergeDefines(ExtraDefines, PackageExtraDefinesDict[DecFile])
            if ExtraDefines:
                Templates.SetValue("{ExtraDefines}", "\n  ".join(ExtraDefines) + "\n  ")

            InfFiles = ProcessFiles(Templates, [File], args.OutputDirectory)
            MockLibraryInfFiles += InfFiles

    if args.TestDsc:
        Templates.SetValue("{MockLibrariesTestDsc}", args.TestDsc)
        MockLibraryInfFiles = [x.replace("\\", "/") for x in MockLibraryInfFiles]
        Templates.SetValue("{MockLibraryInfPaths}", "\n  ".join(MockLibraryInfFiles))
        Templates.WriteTemplate(
            "library-dsc", ".", Templates.ApplyTemplate("library-dsc")
        )


if __name__ == "__main__":
    main()
