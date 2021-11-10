## @file
# Compare builds from different git branches/refs.
#
# Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

'''
CompareBuild
'''
from __future__ import print_function
import os
import sys
import argparse
from git import Repo
import filecmp
from shutil import copytree
from shutil import copy
from shutil import rmtree
from shutil import move
import subprocess

#
# Globals for help information
#
__prog__        = 'CompareBuild'
__copyright__   = 'Copyright (c) 2021, Intel Corporation. All rights reserved.'
__description__ = 'Compare two builds from different git branches/refs.\n'

class dircmp(filecmp.dircmp):
    """
    Compare the content of dir1 and dir2. In contrast with filecmp.dircmp, this
    subclass compares the content of files with the same path.
    """
    def phase3(self):
        """
        Find out differences between common files.
        Ensure we are using content comparison with shallow=False.
        """
        fcomp = filecmp.cmpfiles(self.left, self.right, self.common_files,
                                 shallow=False)
        self.same_files, self.diff_files, self.funny_files = fcomp

def is_same(dir1, dir2, verbose = False):
    NumSame = 0
    NumIgnored = 0;
    NumDifferent = 0
    compared = dircmp(dir1, dir2)
    NumSame += len(compared.same_files)
    NumDifferent += (len(compared.left_only) + len(compared.right_only) + len(compared.funny_files))
    if verbose:
        for file in compared.same_files:
            print ('Identical file:', os.path.join (dir1, file))
    for file in compared.left_only:
        print ('Left-Only file:', os.path.join (dir1, file))
    for file in compared.right_only:
        print ('Right-only file:', os.path.join (dir1, file))
    for file in compared.funny_files:
        print ('Funny file:', os.path.join (dir1, file))
    for file in compared.diff_files:
        if file == 'Build.log':
            if verbose:
                print ('Ignore differences in file:', os.path.join (dir1, file))
            NumIgnored += 1
            continue
        if file.startswith('GlobalVar_') and file.endswith('.bin'):
            if verbose:
                print ('Ignore differences in file:', os.path.join (dir1, file))
            NumIgnored += 1
            continue
        if dir1.endswith('PcdValueInit'):
            if verbose:
                print ('Ignore differences in file:', os.path.join (dir1, file))
            NumIgnored += 1
            continue
        Extension = os.path.splitext(file)[1]
        #
        # Ignore differences in intermediate builds files that may have
        # CPP related line number information
        #
        if Extension in ['.i', '.ii', '.iii', '.iiii']:
            if verbose:
                print ('Ignore differences in file:', os.path.join (dir1, file))
            NumIgnored += 1
            continue
        print ('Difference detected in file:', os.path.join (dir1, file))
        NumDifferent += 1
    for subdir in compared.common_dirs:
        SubNumSame, SunNumIgnored, SubNumDifferent = is_same(os.path.join(dir1, subdir), os.path.join(dir2, subdir), verbose=verbose)
        NumSame      += SubNumSame
        NumIgnored   += SunNumIgnored
        NumDifferent += SubNumDifferent
    return NumSame, NumIgnored, NumDifferent

def Cleanup (args, BuildDir):
    if BuildDir:
        if args.Cleanup and os.path.exists (os.path.join (BuildDir, 'Build.Ref1')):
            print ('rmtree:', os.path.join (BuildDir, 'Build.Ref1'))
            rmtree (os.path.join (BuildDir, 'Build.Ref1'))
        if args.Cleanup and os.path.exists (os.path.join (BuildDir, 'Build.Ref2')):
            print ('rmtree:', os.path.join (BuildDir, 'Build.Ref2'))
            rmtree (os.path.join (BuildDir, 'Build.Ref2'))
        if os.path.exists (BuildDir + '.temp'):
            if os.path.exists (BuildDir):
                rmtree (BuildDir)
            os.rename(BuildDir + '.temp', BuildDir)

if __name__ == '__main__':
    Workspace = ''
    NewConfDir = ''
    BuildDir = ''
    ActiveCommit = ''

    #
    # Create command line argument parser object
    #
    parser = argparse.ArgumentParser (prog = __prog__,
                                      description = __description__ + __copyright__,
                                      conflict_handler = 'resolve')
    parser.add_argument ("--ref1", dest = 'Ref1', required = True,
                         help = "First GIT branch/reference.")
    parser.add_argument ("--ref2", dest = 'Ref2', required = True,
                         help = "Second GIT branch/reference.")
    parser.add_argument ("--skip-build", dest = 'SkipBuild', action = "store_true",
                         help = "Skip build and only redo compare from most recent build.")
    parser.add_argument ("--cleanup", dest = 'Cleanup', action = "store_true",
                         help = "Cleanup temporary Build and Conf folders.")
    parser.add_argument ("--verbose", dest = 'Verbose', action = "store_true",
                         help = "Increase output messages")
    parser.add_argument('rest', nargs=argparse.REMAINDER)
    #
    # Parse command line arguments
    #
    args = parser.parse_args ()
    if not args.Ref1 or not args.Ref2:
        sys.exit('FAIL: Invalid ref')
    BuildCommand = ' '.join(args.rest)
    if BuildCommand == '':
        sys.exit('FAIL: No build command provided')
    if args.Ref1 == args.Ref2:
        print('PASS: Builds identical.  Same refs:' + BuildCommand)
        sys.exit(0)

    try:
        repo = Repo('.', search_parent_directories=True)
    except:
        sys.exit('FAIL: Not in a git repo: ' + BuildCommand)
    if repo.is_dirty():
        sys.exit('FAIL: Repo has uncommitted changes: ' + BuildCommand)
    if repo.head.is_detached:
        ActiveCommit = repo.head.commit
    else:
        ActiveCommit = repo.active_branch.name

    if 'WORKSPACE' not in os.environ:
        sys.exit('FAIL: WORKSPACE is not set: ' + BuildCommand)
    Workspace = os.environ['WORKSPACE']
    if not os.path.exists (Workspace):
        sys.exit('FAIL: WORKSPACE %s is invalid: %s' % (Workspace, BuildCommand))
    print ('WORKSPACE:', Workspace)
    
    if 'EDK_TOOLS_PATH' not in os.environ:
        sys.exit('FAIL: EDK_TOOLS_PATH is not set: ' + BuildCommand)
    EdkToolsPath = os.environ['EDK_TOOLS_PATH']
    if not os.path.exists (EdkToolsPath):
        sys.exit('FAIL: EDK_TOOLS_PATH %s is invalid: %s' % (EdkToolsPath, BuildCommand))
    print ('EDK_TOOLS_PATH:', EdkToolsPath)
    
    BuildDir = os.path.join (Workspace, 'Build')
    print ('Build Directory:', BuildDir)
    ConfDir = os.path.join (Workspace, 'Conf')
    if not os.path.exists (ConfDir):
        Cleanup (args, BuildDir)
        sys.exit('FAIL: No Conf directory: ' + BuildCommand)

    # cleanup from previous run
    if os.path.exists (BuildDir + '.temp'):
        if os.path.exists (BuildDir):
            rmtree (BuildDir)
        os.rename(BuildDir + '.temp', BuildDir)
    if os.path.exists (BuildDir + '.Ref1'):
        print ('rmtree:', BuildDir + '.Ref1')
        rmtree (BuildDir + '.Ref1')
    if os.path.exists (BuildDir + '.Ref2'):
        print ('rmtree:', BuildDir + '.Ref2')
        rmtree (BuildDir + '.Ref2')

    BuildFailed = False
    if not args.SkipBuild:
        for (Ref,Ext) in [(args.Ref1,'.Ref1'), (args.Ref2,'.Ref2')]:
            # Checkout Ref and run build
            try:
                print ('Checkout:', Ref)
                repo.git.checkout(Ref)
                repo.git.submodule('update', '--init')
            except:
                Cleanup (args, BuildDir)
                if ActiveCommit:
                    print ('Checkout:', ActiveCommit)
                    repo.git.checkout(ActiveCommit)
                sys.exit('FAIL: Checkout ref %s failed: %s' % (Ref, BuildCommand))

            # Preserve exisiting Build dir
            if os.path.exists (BuildDir):
                os.rename(BuildDir, BuildDir + '.temp')
            # Create Build dir
            os.mkdir(BuildDir)

            # Create modified Conf directory
            NewConfDir = os.path.join (Workspace, 'Build', 'Conf.temp')
            os.mkdir(NewConfDir)
            copy (os.path.join (EdkToolsPath, 'Conf', 'target.template'), os.path.join (Workspace, 'Build', 'Conf.temp', 'target.txt'))
            BuildRuleTxt = os.path.join (NewConfDir, 'build_rule.txt')
            copy (os.path.join (EdkToolsPath, 'Conf', 'build_rule.template'), BuildRuleTxt)
            if not os.path.exists (BuildRuleTxt):
                Cleanup (args, BuildDir)
                sys.exit('FAIL: No build_rules.txt file: ' + BuildCommand)
            ToolsDefTxt = os.path.join (NewConfDir, 'tools_def.txt')
            copy (os.path.join (EdkToolsPath, 'Conf', 'tools_def.template'), ToolsDefTxt)
            if not os.path.exists (ToolsDefTxt):
                Cleanup (args, BuildDir)
                sys.exit('FAIL: No tools_def.txt file: ' + BuildCommand)

            # Create modified build_rule.txt file
            File = open(BuildRuleTxt, 'r')
            BuildRuleLines = File.read().splitlines(keepends = False)
            File.close()
            NewBuildRuleLines = []
            for Line in BuildRuleLines:
                if Line.strip() == '':
                    NewBuildRuleLines.append(Line)
                    continue
                Line = Line.rstrip()
                if Line.strip().startswith ('#'):
                    NewBuildRuleLines.append(Line)
                    continue
                if '"$(SLINK)"' in Line and ' cr ' in Line and ' cdD ' not in Line:
                    # GCC
                    Line = Line.replace(' cr ', ' crD ', 1)
                if '"$(CC)"' in Line:
                    if '-o' in Line:
                        # GCC or CLANGPDB
                        ExtraFlags = ' -g0 -D__TIME__=\'"00:00:00"\' -D__DATE__=\'"Jan 1 2021"\' -Wno-builtin-macro-redefined -frandom-seed=CONSTANTSEEDSTRING -DDEBUG_LINE_NUMBER=1 -DDEBUG_EXPRESSION_STRING_VALUE=\'"Hello"\''
                    else:
                        # MSFT
                        ExtraFlags = ' -D__TIME__="00:00:00" -D__DATE__="Jan 1 2021" /wd4117 -DDEBUG_LINE_NUMBER=1 -DDEBUG_EXPRESSION_STRING_VALUE=\\"Hello\\"'
                        ExtraFlags += ' -Brepro'
                    ExtraFlags += ' -UBUILD_EPOCH -DBUILD_EPOCH=0'
                    Line = Line.replace ('$(CC_FLAGS)', '$(CC_FLAGS)' + ExtraFlags)
                if '"$(ASLCC)"' in Line:
                    if '-o' in Line:
                        # GCC or CLANGPDB or XCODE
                        ExtraFlags = ' -g0 -D__TIME__=\'"00:00:00"\' -D__DATE__=\'"Jan 1 2021"\' -Wno-builtin-macro-redefined -frandom-seed=CONSTANTSEEDSTRING -DDEBUG_LINE_NUMBER=1 -DDEBUG_EXPRESSION_STRING_VALUE=\'"Hello"\''
                    else:
                        # MSFT
                        ExtraFlags = ' -D__TIME__="00:00:00" -D__DATE__="Jan 1 2021" /wd4117 -DDEBUG_LINE_NUMBER=1 -DDEBUG_EXPRESSION_STRING_VALUE=\\"Hello\\"'
                        ExtraFlags += ' -Brepro'
                    ExtraFlags += ' -UBUILD_EPOCH -DBUILD_EPOCH=0'
                    Line = Line.replace ('$(ASLCC_FLAGS)', '$(ASLCC_FLAGS)' + ExtraFlags)
                if '"$(NASM)"' in Line:
                    ExtraFlags = ' --reproducible'
                    Line = Line.replace ('$(NASM_FLAGS)', '$(NASM_FLAGS)' + ExtraFlags)
                if '"$(DLINK)"' in Line:
                    if '-o' in Line or '-Wl' in Line:
                        # GCC or CLANGPDB
                        ExtraFlags = ''
                    else:
                        # MSFT
                        ExtraFlags = ' /DEBUG:NONE -Brepro'
                    Line = Line.replace ('$(DLINK_FLAGS)', '$(DLINK_FLAGS)' + ExtraFlags)
                if '"$(ASLDLINK)"' in Line:
                    if '-o' in Line or '-Wl' in Line:
                        # GCC or CLANGPDB
                        ExtraFlags = ''
                    else:
                        # MSFT
                        ExtraFlags = ' /DEBUG:NONE -Brepro'
                    Line = Line.replace ('$(ASLDLINK_FLAGS)', '$(ASLDLINK_FLAGS)' + ExtraFlags)
                if '"$(SLINK)"' in Line and '/OUT' in Line:
                    # MSFT
                    ExtraFlags = ' -Brepro'
                    Line = Line.replace ('$(SLINK_FLAGS)', '$(SLINK_FLAGS)' + ExtraFlags)
                if '"$(ASM)"' in Line and '/Fo' in Line:
                    # MSFT
                    ExtraFlags = ' -Brepro'
                    Line = Line.replace ('$(ASM_FLAGS)', '$(ASM_FLAGS)' + ExtraFlags)
                NewBuildRuleLines.append(Line)
            File = open(BuildRuleTxt, 'w')
            File.write('\n'.join(NewBuildRuleLines) + '\n')
            File.close()

            # Create modified tools_def.txt file
            File = open(ToolsDefTxt, 'r')
            ToolsDefLines = File.read().splitlines(keepends = False)
            File.close()
            NewToolsDefLines = []
            for Line in ToolsDefLines:
                if Line.strip() == '':
                    NewToolsDefLines.append(Line)
                    continue
                Line = Line.rstrip()
                if Line.strip().startswith ('#'):
                    NewToolsDefLines.append(Line)
                    continue
                if 'DEFINE' in Line:
                    NewToolsDefLines.append(Line)
                    continue
                if '_VS20' in Line and '_CC_FLAGS' in Line:
                    Line = Line.replace (' /Zi','')
                    Line = Line.replace (' /Zd','')
                    Line = Line.replace (' /Z7','')
                    Line = Line.replace (' /ZI','')
                if '_VFR_FLAGS' in Line:
                    Line = Line.replace ('-l', '')
                if '_CLANGPDB_' in Line and ('_CC_FLAGS' in Line or '_SLINK_FLAGS' in Line or '_DLINK_FLAGS' in Line or '_ASM_FLAGS' in Line):
                    Line += ' -Brepro'
                NewToolsDefLines.append(Line)
            File = open(ToolsDefTxt, 'w')
            File.write('\n'.join(NewToolsDefLines) + '\n')
            File.close()

            # Run build command using customized Conf directory
            print ('Run:', BuildCommand + ' --conf=' + NewConfDir)
            try:
                subprocess.run(BuildCommand + ' --conf=' + NewConfDir, shell=True, check=True)
            except:
                # Build command failed
                BuildFailed = True
                print ('FAIL: Build command: ' + BuildCommand)
            if os.path.exists (BuildDir):
                os.rename(BuildDir, BuildDir + Ext)
            if os.path.exists (BuildDir + '.temp'):
                os.rename(BuildDir + '.temp', BuildDir)
            if os.path.exists (os.path.join (BuildDir, 'Build' + Ext)):
                rmtree (os.path.join (BuildDir, 'Build' + Ext))
            if os.path.exists (BuildDir + Ext):
                move (BuildDir + Ext, os.path.join (BuildDir, 'Build' + Ext))

        # Restore active git branch
        if ActiveCommit:
            print ('Checkout:', ActiveCommit)
            repo.git.checkout(ActiveCommit)

    # Determine if build output dirs are the same
    if BuildFailed:
        Cleanup (args, BuildDir)
        print('One or more builds failed')
        sys.exit('FAIL: Builds not identical.  Build failed: ' + BuildCommand)
    if not os.path.exists (os.path.join (BuildDir, 'Build.Ref1')) or not os.path.exists (os.path.join (BuildDir, 'Build.Ref2')):
        Cleanup (args, BuildDir)
        print('Build did not generate expected build output directories')
        sys.exit('FAIL: Builds not identical. Build output directory not found: ' + BuildCommand)
    NumSame, NumIgnored, NumDifferent = is_same(os.path.join (BuildDir, 'Build.Ref1'), os.path.join (BuildDir, 'Build.Ref2'), args.Verbose)
    if NumSame > 0 and NumDifferent == 0:
        Cleanup (args, BuildDir)
        print('PASS: Builds identical: ' + BuildCommand)
        print('  Number same      =', NumSame)
        print('  Number ignored   =', NumIgnored)
        print('  Number different =', NumDifferent)
        sys.exit(0)
    else:
        Cleanup (args, BuildDir)
        print('FAIL: Builds not identical: ' + BuildCommand)
        print('  Number same      =', NumSame)
        print('  Number ignored   =', NumIgnored)
        print('  Number different =', NumDifferent)
        sys.exit('FAIL: Builds not identical: ' + BuildCommand)
