#!/usr/bin/env python3
"""Generate a daily macOS launchd job. Enable it using launchctl bootstrap."""
import argparse
import datetime
import os
from pathlib import Path
import plistlib
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', required=True, type=Path)
    parser.add_argument('--vaults-file', required=True, type=Path)
    parser.add_argument('--at', required=True, metavar='HH:MM',
                        help='daily local time (24-hour clock)')
    parser.add_argument('--output', required=True, type=Path,
                        help='new plist file; existing files are never overwritten')
    parser.add_argument('--log-directory', type=Path,
                        default=Path.home() / 'Library/Logs/obsidian_optimizer')
    parser.add_argument('--scan-only', action='store_true')
    args = parser.parse_args()
    try:
        if len(args.at) != 5 or args.at[2] != ':':
            raise ValueError('Expected HH:MM')
        time = datetime.datetime.strptime(args.at, '%H:%M').time()
    except ValueError:
        parser.error('--at must be a valid 24-hour time, e.g. 03:00')
    binary = args.binary.resolve()
    config = args.vaults_file.resolve()
    if not binary.is_file() or not os.access(binary, os.X_OK):
        parser.error('--binary must be an executable file')
    if not config.is_file():
        parser.error('--vaults-file must be an existing file')
    logs = args.log_directory.resolve()
    arguments = [str(binary), '--vaults-file', str(config)]
    if not args.scan_only:
        arguments.append('--compress')
    job = {
        'Label': 'local.obsidian-optimizer',
        'ProgramArguments': arguments,
        'StartCalendarInterval': {'Hour': time.hour, 'Minute': time.minute},
        'EnvironmentVariables': {
            'PATH': '/opt/homebrew/bin:/usr/local/bin:/usr/bin:/bin:' + os.environ.get('PATH', '')
        },
        'StandardOutPath': str(logs / 'stdout.log'),
        'StandardErrorPath': str(logs / 'stderr.log'),
    }
    try:
        logs.mkdir(parents=True, exist_ok=True)
        # Exclusive creation avoids replacing an active job's configuration.
        with args.output.open('xb') as output:
            plistlib.dump(job, output)
    except OSError as error:
        parser.error(str(error))
    print(f'Created {args.output.resolve()}. Schedule is not enabled yet.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
