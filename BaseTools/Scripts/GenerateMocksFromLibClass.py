### @file
#  Convert EDK II include files to mock libraries
#
# Copyright (C) 2024, Intel Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
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

EDKII_DEFAULT_PACKAGES = [
    "MdePkg/MdePkg.dec",
    "UnitTestFrameworkPkg/UnitTestFrameworkPkg.dec",
]


class IncludeFileParser:
    #
    # Regular expression to replace variable argument list declaration ... with
    # nothing so a variable argument function is converted to a mock function
    # with no variable argument parameters.
    #
    STRIP_VARARGS = {r",\s*\.\.\.\s*\)": ")"}

    #
    # Replace OPTIONAL keyword with an array with size __OPTIONAL__
    #
    OPTIONAL_ARRAY_SIZE = "___OPTIONAL___"

    #
    # Replace EDK II specific keywords with attributes to preserve the keywords
    # in the mocked interfaces.
    #
    EDKII_MACROS = {
        "RETURNS_TWICE": "__attribute__(RETURNS_TWICE)",
        "EFIAPI": "__attribute__(EFIAPI)",
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

    def __init__(
        self,
        FileName,
        CpuTypeList=CPU_TYPE_LIST,
        Replace=STRIP_VARARGS,
        Macros=EDKII_MACROS,
        Verbose=False,
        CheckUnparsed=False,
    ):
        self.FileName = FileName
        self.CpuTypeList = ["COMMON"]
        self.Replace = Replace
        self.Macros = Macros
        self.Verbose = Verbose
        self.CheckUnparsed = CheckUnparsed
        self.Include = OrderedDict()
        self.Tokens = OrderedDict()
        self.MockVariables = OrderedDict()
        self.MockFunctions = OrderedDict()
        self.Unparsed = OrderedDict()

        self.CollectCpuTypes(CpuTypeList)

        for CpuType in self.CpuTypeList:
            self.ParseCpuType(CpuType)
            if self.CheckUnparsed:
                if self.Verbose:
                    print("  Check Unparsed")
                Unparsed = self.CollectUnparsed(CpuType)
                if Unparsed:
                    for Line in Unparsed.splitlines():
                        print("  * ", Line)
                    self.Unparsed[CpuType] = Unparsed
        if self.Unparsed:
            sys.exit(f"Error parsing {self.FileName}")

        self.CollectVariables()
        self.CollectFunctions()

    def CollectCpuTypes(self, CpuTypeList):
        with open(self.FileName, "r") as f:
            Buffer = f.read()
            for CpuType in [f"MDE_CPU_{x}" for x in CpuTypeList]:
                if CpuType in Buffer:
                    self.CpuTypeList.append(CpuType)

    def ParseCpuType(self, CpuType):
        print(f"Parse {self.FileName} for CPU Type {CpuType}")
        EdkiiMacros = self.Macros.copy()
        if CpuType != "COMMON":
            EdkiiMacros[CpuType] = "1"
        if self.Verbose:
            print("  Create Parser")
        self.Include[CpuType] = CParser(
            self.FileName, replace=self.Replace, macros=EdkiiMacros, process_all=False
        )
        if self.Verbose:
            print("  Remove Comments")
        self.Include[CpuType].remove_comments(self.FileName)
        if self.Verbose:
            print("  Preprocess Macros")
        self.Include[CpuType].preprocess(self.FileName)
        if self.Verbose:
            print("  Parse Definitions")
        self.Tokens[CpuType] = self.Include[CpuType].parse_defs(self.FileName)

    def CollectUnparsed(self, CpuType):
        Unparsed = self.Include[CpuType].parse_defs(self.FileName, return_unparsed=True)
        return Unparsed.strip()

    def CollectVariables(self):
        for CpuType in self.CpuTypeList:
            self.MockVariables[CpuType] = OrderedDict()
            for VariableName in self.Include[CpuType].defs["variables"]:
                if CpuType != "COMMON" and VariableName in self.MockVariables["COMMON"]:
                    continue
                self.MockVariables[CpuType][VariableName] = self.MockVariable(
                    self, CpuType, VariableName
                )
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
                self.MockFunctions[CpuType][FunctionName] = self.MockFunction(
                    self, CpuType, FunctionName
                )
            if not self.MockFunctions[CpuType]:
                self.MockFunctions.pop(CpuType)

    def _ParseTypeQuals(TypeQuals, Exclude=[]):
        def ParseTypeQualsRecursive(TypeQuals, Result, Exclude=[]):
            if isinstance(TypeQuals, str):
                TypeQuals = TypeQuals.strip()
                if TypeQuals and TypeQuals not in Exclude:
                    Result.append(TypeQuals)
                return Result
            try:
                for Item in TypeQuals:
                    Result = ParseTypeQualsRecursive(Item, Result, Exclude)
            except:
                pass
            return Result

        Exclude = ["__", "__attribute__"] + Exclude
        Quals = ParseTypeQualsRecursive(TypeQuals, [], Exclude)
        return " ".join(Quals)

    class MockVariable:
        def __init__(self, Parser, CpuType, VariableName):
            self.VariableName = VariableName
            self.Variable = Parser.Include[CpuType].defs["variables"][VariableName]
            if len(self.Variable[1]) > 1:
                Pointer = ""
                for Item in self.Variable[1][1:]:
                    if isinstance(Item, str):
                        Pointer += Item
                        continue
                    if isinstance(Item, list):
                        self.VariableName += str(Item)
                        continue
                    sys.exit(f"Unsupported variable type: {Item}")
                self.VariableName = Pointer + self.VariableName

            #
            # Remove static and const attributes from mocked variables so they can
            # be visible and modified by unit tests
            #
            Exclude = ["const", "CONST", "STATIC"]
            self.Quals = IncludeFileParser._ParseTypeQuals(
                self.Variable[1].type_quals, Exclude
            )
            self.QualsWidth = len(self.Quals)
            self.Type = self.Variable[1][0]
            self.TypeWidth = len(self.Type)

        def GenerateDefinition(self):
            quals = ""
            if self.QualsWidth:
                quals = f"{self.Quals:<{self.QualsWidth + 1}}"
            return f"{quals}{self.Type:<{self.TypeWidth + 2}}{self.VariableName};"

    class MockFunction:
        class MockParameter:
            def __init__(self, Parameter, ArraySubscripts=[]):
                self.Parameter = Parameter
                self.ParameterName = self.Parameter[0]
                self.Optional = False
                if len(self.Parameter[1]) > 1:
                    Pointer = ""
                    if ArraySubscripts:
                        if ArraySubscripts[-1] == IncludeFileParser.OPTIONAL_ARRAY_SIZE:
                            self.Optional = True
                            ArraySubscripts = ArraySubscripts[:-1]
                        if ArraySubscripts:
                            self.ParameterName += "[" + "][".join(ArraySubscripts) + "]"
                    for Item in Parameter[1][1:]:
                        if isinstance(Item, str):
                            Pointer += Item
                            continue
                        if isinstance(Item, list):
                            continue
                        sys.exit(f"Unsupported parameter type: {Item}")
                    self.ParameterName = Pointer + self.ParameterName
                self.Quals = IncludeFileParser._ParseTypeQuals(Parameter[1].type_quals)
                self.QualsWidth = len(self.Quals)
                self.Type = Parameter[1][0]
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
            self.Tokens = Parser.Tokens[CpuType]
            self.CallingConvention = IncludeFileParser._ParseTypeQuals(
                self.Function[0].type_quals
            )
            self.ReturnType = "".join(self.Function[0][:])
            self.ReturnType = " *".join(self.ReturnType.split("*", 1))
            self.ParameterList = OrderedDict()
            for Parameter in self.Function[1]:
                ParameterName = Parameter[0]
                if not ParameterName:
                    continue
                ArraySubscripts = self._FindArrayDeclaration(
                    FunctionName, ParameterName, self.Tokens
                )
                self.ParameterList[ParameterName] = self.MockParameter(
                    Parameter, ArraySubscripts
                )
            if self.ParameterList:
                QualsWidth = max([x.QualsWidth for x in self.ParameterList.values()])
                TypeWidth = max([x.TypeWidth for x in self.ParameterList.values()])
                for Parameter in self.ParameterList.values():
                    Parameter.QualsWidth = QualsWidth
                    Parameter.TypeWidth = TypeWidth

        def GenerateDefinition(self):
            return f"MOCK_FUNCTION_DEFINITION (Mock{{LibName}}, {self.FunctionName}, {len(self.ParameterList)}, {self.CallingConvention});"

        def GenerateDeclaration(self):
            Parameters = [x.GenerateDeclaration() for x in self.ParameterList.values()]
            Parameters = ",\n     ".join(Parameters)
            return f"""
  MOCK_FUNCTION_DECLARATION (
    {self.ReturnType},
    {self.FunctionName},
    ({Parameters})
    );"""

        def _FindArrayDeclaration(
            self, FunctionName, ParamName, t, value=None, depth=0, index=0
        ):
            if not t:
                return None
            if isinstance(t, (float, int, str)):
                if value:
                    res = self._FindArrayDeclaration(
                        FunctionName, ParamName, value, value=None, depth=depth + 1
                    )
                    if res:
                        return res
                return None
            for Attribute in [
                "pre_qual",
                "first_typequal",
                "ptrs",
                "name",
                "value",
                "arrays",
            ]:
                if hasattr(t, Attribute):
                    token = getattr(t, Attribute)
                    if token:
                        if Attribute == "arrays":
                            return token
                        # Hardcoded depth of 3.  May change for non library use cases.
                        if Attribute == "name" and depth == 3 and token != FunctionName:
                            return None
                        # Hardcoded depth of 6.  May change for non library use cases.
                        if Attribute == "name" and depth == 6 and token != ParamName:
                            return None
            if isinstance(t, dict):
                for item, value in t:
                    res = self._FindArrayDeclaration(
                        FunctionName, ParamName, item, value=value, depth=depth + 1
                    )
                    if res:
                        return res
                return None
            if isinstance(t, (list, tuple)) or len(t) > 0:
                index = 0
                for item in t:
                    res = self._FindArrayDeclaration(
                        FunctionName,
                        ParamName,
                        item,
                        value=None,
                        depth=depth + 1,
                        index=index,
                    )
                    if res:
                        return res
                    index = index + 1
                return None


class Template:
    def __init__(self, YamlFile):
        self.Replacements = {}
        self.yaml_doc = []
        try:
            with open(YamlFile) as f:
                yaml_file = yaml.load_all(f, Loader=yaml.loader.SafeLoader)
                for yaml_doc in yaml_file:
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

    def WriteTemplate(self, templatetype, rootpath, filecontent, force=False):
        for file in self.yaml_doc[0]["filelist"]:
            filetype = file["type"]
            if templatetype not in filetype:
                continue
            filepath = self.ApplyValues(file["path"])
            filename = self.ApplyValues(file["name"])
            fullpath = os.path.normpath(os.path.join(rootpath, filepath))
            fullpathfile = os.path.normpath(os.path.join(fullpath, filename))
            print(__prog__, ": Output: " + fullpathfile)
            if not os.path.exists(fullpath):
                os.makedirs(fullpath, exist_ok=True)
            if os.path.exists(fullpathfile) and not force:
                print("ERROR: File already exists.  Use --force to force overwrite.")
                return
            with open(fullpathfile, "w") as outputfile:
                outputfile.write(filecontent)


class MockLibraryGenerator:
    def __init__(self, Templates, Include, OutputDirectory=".", Force=False):
        self.Templates = Templates
        self.Include = Include
        self.OutputDirectory = OutputDirectory
        self.Force = Force
        self.IncludeFileTemplateType = "library-h"
        self.CppFileTemplateType = "library-cpp"
        self.InfFileTemplateType = "library-inf"

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
        self.Templates.WriteTemplate(
            self.IncludeFileTemplateType,
            self.OutputDirectory,
            self.Templates.ApplyTemplate(self.IncludeFileTemplateType),
            self.Force,
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
            self.Templates.SetValue("{GlobalVariables}", "\n  " + "\n  ".join(VarDef))

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

        self.Templates.WriteTemplate(
            self.CppFileTemplateType,
            self.OutputDirectory,
            self.Templates.ApplyTemplate(self.CppFileTemplateType),
            self.Force,
        )

    def GenerateMockInfFile(self):
        self.Templates.WriteTemplate(
            self.InfFileTemplateType,
            self.OutputDirectory,
            self.Templates.ApplyTemplate(self.InfFileTemplateType),
            self.Force,
        )


def ParseBaseName(FileName):
    BaseName = os.path.splitext(os.path.basename(FileName))[0]
    BASE_NAME = ""
    for c in BaseName:
        if c.isupper():
            BASE_NAME += "_"
        BASE_NAME += c.upper()
    BASE_NAME = BASE_NAME.lstrip("_")
    return BaseName, BASE_NAME


def ProcessFiles(
    Templates,
    FileList,
    OutputDirectory,
    Force,
    Verbose,
    CheckUnparsed,
):
    for FileName in FileList:
        LibName, LIB_NAME = ParseBaseName(FileName)

        Templates.SetValue("{LibName}", LibName)
        Templates.SetValue("{LIB_NAME}", LIB_NAME)
        Templates.SetValue("{FileGuid}", str(uuid.uuid5(uuid.NAMESPACE_DNS, LibName)))

        Include = IncludeFileParser(
            FileName, Verbose=Verbose, CheckUnparsed=CheckUnparsed
        )

        Generator = MockLibraryGenerator(Templates, Include, OutputDirectory, Force)
        Generator.GenerateMockIncludeFile()
        Generator.GenerateMockCppFile()
        Generator.GenerateMockInfFile()


def main():
    #
    # Load code templates
    #
    TemplateFileName = os.path.join(
        os.path.realpath(os.path.dirname(__file__)),
        "GenerateMocksFromLibClassTemplates.yaml",
    )
    Templates = Template(TemplateFileName)

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
        dest="InputFile",
        nargs="*",
        default=[],
        help="Input EDK II include file to generate mocks.",
    )
    group.add_argument(
        "-d",
        "--directory",
        dest="InputDirectory",
        nargs="*",
        help="Input EDK II directory of include files to generate mocks.",
    )
    group.add_argument(
        "-r",
        "--repository",
        dest="InputRepository",
        help="Input EDK II repository to scan for include files to generate mocks.",
    )
    parser.add_argument(
        "-o",
        "--output",
        dest="OutputDirectory",
        default="",
        help="Output EDK II Test directory for generate mocks.  Package relative path for -r/--repository",
    )
    parser.add_argument(
        "-c",
        "--copyright",
        dest="Copyright",
        required=True,
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
        help="Generate error if unparsable content is detected",
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

    Templates.SetValue("{Copyright}", args.Copyright)

    if args.InputRepository:
        SearchPath = os.path.join(args.InputRepository, "**", "*.dec")
        DecFiles = [f for f in glob.glob(SearchPath, recursive=True)]
        for DecFile in DecFiles:
            PackageDir = os.path.split(DecFile)[0]
            LibIncludeDir = os.path.join(PackageDir, "Include", "Library")
            if os.path.exists(LibIncludeDir):
                LibIncludeFiles = []
                for File in os.listdir(LibIncludeDir):
                    if os.path.splitext(File)[1] in [".h", ".H"]:
                        LibIncludeFiles.append(os.path.join(LibIncludeDir, File))
                if LibIncludeFiles:
                    if args.OutputDirectory:
                        MockOutputDir = os.path.join(PackageDir, args.OutputDirectory)
                    else:
                        MockOutputDir = os.path.join(PackageDir, "Test", "Mock")

                    PackageDecFiles = EDKII_DEFAULT_PACKAGES.copy()
                    DecFile = os.path.relpath(DecFile).replace('\\','/')
                    if DecFile not in PackageDecFiles:
                        PackageDecFiles.append(DecFile)
                    Templates.SetValue(
                        "{PackageDecFiles}", "\n  ".join(PackageDecFiles)
                    )

                    ProcessFiles(
                        Templates,
                        LibIncludeFiles,
                        MockOutputDir,
                        args.Force,
                        args.Verbose,
                        args.CheckUnparsed,
                    )
    else:
        Templates.SetValue("{PackageDecFiles}", "\n  ".join(EDKII_DEFAULT_PACKAGES))

        FileList = args.InputFile
        if args.InputDirectory:
            for Directory in args.InputDirectory:
                Files = os.listdir(Directory)
                for File in Files:
                    if os.path.splitext(File)[1] in [".h", ".H"]:
                        FileList.append(os.path.join(Directory, File))
        ProcessFiles(
            Templates,
            FileList,
            args.OutputDirectory,
            args.Force,
            args.Verbose,
            args.CheckUnparsed,
        )


if __name__ == "__main__":
    main()
