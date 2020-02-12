## @file
# Use smptlib to send an email message
#
# Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

'''
SendMail
'''
from __future__ import print_function

import sys
import argparse
import smtplib

#
# Globals for help information
#
__prog__        = 'SendMail'
__copyright__   = 'Copyright (c) 2020, Intel Corporation. All rights reserved.'
__description__ = 'Use smptlib to send an email message.\n'

if __name__ == '__main__':
    #
    # Create command line argument parser object
    #
    parser = argparse.ArgumentParser (prog = __prog__,
                                      description = __description__ + __copyright__,
                                      conflict_handler = 'resolve')
    parser.add_argument ("-i", "--input", dest = 'InputFile', type = argparse.FileType ('r'), action='append', required = True,
                         help = "Input email text filename.  Multiple input files send multiple emails.")
    parser.add_argument ("-u", "--user-name", dest = 'UserName', required = True,
                         help = "User name to login to SMTP server.")
    parser.add_argument ("-p", "--password", dest = 'Password', required = True,
                         help = "Password to login to SMTP server.")
    parser.add_argument ("-s", "--server", dest = 'SmtpAddress', required = True,
                         help = "SMTP server address.")
    parser.add_argument ("-o", "--port", dest = 'SmtpPortNumber', type = int, required = True,
                         help = "SMTP server port number.")
    parser.add_argument ("-t", "--to", dest = 'To', required = True,
                         help = "Email to address.")
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

    #
    # Connect and login to SMTP server
    #
    try:
        if args.Verbose:
            print ('SendMail: Open SMTP server %s port %d' % (args.SmtpAddress, args.SmtpPortNumber))
        SmtpServer = smtplib.SMTP(args.SmtpAddress, args.SmtpPortNumber)
        SmtpServer.starttls()
        SmtpServer.ehlo()
        if args.Verbose:
            print ('SendMail: SMTP Login')
        SmtpServer.login(args.UserName, args.Password)
    except:
        print ('SendMail: error: can not connect or login.')
        raise
        sys.exit (1)

    #
    # Read email text input files
    #
    for File in args.InputFile:
        try:
            Message = File.read ()
            File.close ()
        except:
            print ('SendMail: error: can not read input file {File}'.format (File = File))
            break

        try:
            if args.Verbose:
                print ('SendMail: SMTP send email')
                print ('-------------------------')
                print ('From: %s' % (args.UserName))
                print ('To: %s' % (args.To))
                print (Message)
                print ('-------------------------')
            SmtpServer.sendmail(args.UserName, args.To, Message)
        except:
            print ('SendMail: error: can not send email')
            raise
            break

        if not args.Quiet:
            print ('SendMail: SMTP email sent')

    if args.Verbose:
        print ('SendMail: SMTP Quit')
    SmtpServer.quit()
