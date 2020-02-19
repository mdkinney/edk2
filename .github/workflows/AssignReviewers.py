## @file
# Assign reviewers to commits in a GitHub pull request based on assignments
# documented in Maintainers.txt
#
# Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

'''
AssignReviewers
'''
from __future__ import print_function

import sys
import argparse
import subprocess
from github import Github

#
# Globals for help information
#
__prog__        = 'AssignReviewers'
__copyright__   = 'Copyright (c) 2020, Intel Corporation. All rights reserved.'
__description__ = 'Assign reviewers to commits in a GitHub pull request based on assignments documented in Maintainers.txt.\n'

if __name__ == '__main__':
    #
    # Create command line argument parser object
    #
    parser = argparse.ArgumentParser (prog = __prog__,
                                      description = __description__ + __copyright__,
                                      conflict_handler = 'resolve')
    parser.add_argument ("-a", "--access-token", dest = 'AccessToken', required = True,
                         help = "GitHub access token.")
    parser.add_argument ("-r", "--base-repo", dest = 'BaseRepo', required = True,
                         help = "GitHub pull request base repisitory.")
    parser.add_argument ("-n", "--pull-request-number", dest = 'PullRequestNumber', type = int, required = True,
                         help = "GitHub pull request base repisitory.")
    parser.add_argument ("-v", "--verbose", dest = 'Verbose', action = "store_true",
                         help = "Increase output messages")
    parser.add_argument ("-q", "--quiet", dest = 'Quiet', action = "store_true",
                         help = "Reduce output messages")
    parser.add_argument ("--debug", dest = 'Debug', type = int, metavar = '[0-9]', choices = range (0, 10), default = 0,
                         help = "Set debug level")

    #
    # Parse command line arguments
    #
    args = parser.parse_args ()

    Hub = Github(args.AccessToken)
    HubRepo = Hub.get_repo(args.BaseRepo)
    HubPullRequest = HubRepo.get_pull(args.PullRequestNumber)

    GitHubIdList = []
    EmailList = []
    for Commit in HubPullRequest.get_commits():
        try:
            if args.Verbose:
                print (__prog__ + ': GetMaintainer for sha ', Commit.sha)
            Output = subprocess.check_output (['python','BaseTools/Scripts/GetMaintainer.py', '--tag', Commit.sha]).decode()
            if args.Verbose:
                print ('    ' + '\n    '.join(Output.splitlines()))
        except:
            print (__prog__ + ': error: Cannot run GetMaintainer.py')
            sys.exit(1)

        SingleCommitGitHubIdList = []
        for Line in Output.splitlines():
            Line = Line.strip()
            if Line.split(':')[0] not in ['R', 'M']:
                continue
            if '[' in Line and ']' in Line:
                if args.Verbose:
                    print (__prog__ + ': Parse GitHub ID from: ', Line)
                GitHubId = Line.split('[')[1].split(']')[0].strip()
                if GitHubId:
                    SingleCommitGitHubIdList.append(GitHubId)
                    GitHubIdList.append(GitHubId)
                else:
                    print (__prog__ + ': error: Missing GitHub ID: ' + Line)
                    sys.exit(1)
            else:
                print (__prog__ + ': error: Missing GitHub ID: ' + Line)
                sys.exit(1)
            if '<' in Line and '>' in Line:
                if args.Verbose:
                    print (__prog__ + ': Parse email from: ', Line)
                Email = Line.split('<')[1].split('>')[0].strip()
                if '@' in Email:
                    EmailList.append(Email)
                else:
                    print (__prog__ + ': error: Invalid email address: ' + Line)
                    sys.exit(1)
            else:
                print (__prog__ + ': error: Missing email address: ' + Line)
                sys.exit(1)
        SingleCommitGitHubIdList = list(set(SingleCommitGitHubIdList))

        Body = ''
        Comments = Commit.get_comments()
        for Comment in Comments:
            Body = Body + Comment.body
        Body = Body.splitlines()

        AddReviewers = []
        for Reviewer in SingleCommitGitHubIdList:
            if 'Assign Review @' + Reviewer not in Body:
                AddReviewers.append(Reviewer)
        if AddReviewers != []:
            print ('Assign Reviewers to commit', Commit.sha, AddReviewers)
            Commit.create_comment ('Assign Review @' + '\nAssign Review @'.join(AddReviewers))

    GitHubIdList = list(set(GitHubIdList))
    EmailList    = list(set(EmailList))

    if args.Verbose:
        print (GitHubIdList)
        print (EmailList)

    Assignees = HubPullRequest.assignees

    for Assignee in Assignees:
        if Assignee.login not in GitHubIdList:
            print ('Remove Assignee  :', Assignee.login)
            HubPullRequest.remove_from_assignees (Assignee)

    for Login in GitHubIdList:
        Assignee = Hub.get_user(Login)
        if Assignee not in Assignees:
            print ('Add Assignee     :', Assignee.login)
            HubPullRequest.add_to_assignees (Assignee)
        else:
            print ('Already Assigned :', Assignee.login)
