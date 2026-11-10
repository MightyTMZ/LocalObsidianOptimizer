# Obsidian Vault Optimizer

A lightweight, automated media optimization utility for [Obsidian](https://obsidian.md/), built with modern C++.

Obsidian Vault Optimizer is designed to reduce the storage footprint of media assets across multiple Obsidian vaults while preserving **identical decoded media content, original metadata, and existing file references**.

Run the CLI manually or through cron/launchd to discover vault images and replace
them with smaller lossless versions. Compression preserves filenames and existing
Obsidian references.

> **Project Status:** In development. Compression relies on external lossless
> optimizers; independent verification of every image during a run is not implemented.

## Compression

Build with CMake 4.3 or newer and a C++20 compiler. Compression uses external command-line tools on macOS/Linux: `jpegtran` (libjpeg-turbo) for JPEG files and
[OptiPNG](https://optipng.sourceforge.net/) for PNG files. Install the tools and
make sure they are available on `PATH`. For example, with Homebrew:

```sh
brew install jpeg-turbo optipng
cmake -S . -B build
cmake --build build
./build/obsidian_optimizer --compress
```

Enter the path to your vault paths file when prompted; spaces are supported.
Without `--compress`, the program only scans for images.

`--compress` optimizes images **in place**, replacing the original only when the
optimizer succeeds and produces a smaller file. Filenames and Obsidian links
stay the same. Files that cannot be made smaller are left unchanged. Each image
reports bytes saved or the reason it was skipped or failed.

Compression works on a temporary snapshot beside the source. Before replacing
it, the program checks that the original content, permissions, and modification
time have not changed. It restores permissions and modification time on the
compressed file, then uses a same-filesystem atomic rename to replace the source.
On macOS, file metadata (including extended attributes and ACLs) is copied too.
Symlinks and files with multiple hard links are rejected. Temporary files are
cleaned up on handled failures; originals remain intact when an optimizer fails.
No backup copy is kept after successful replacement. Old `.optimized-media`
folders from earlier versions remain excluded from scans and are not migrated.

JPEG optimization uses `jpegtran -copy all -optimize` to retain metadata markers
and optimize entropy coding without re-encoding pixels. PNG optimization uses
OptiPNG with image reductions disabled (`-nx`) and without metadata stripping.
The program checks tool success and output size and relies on these lossless
tools for image-content preservation; it does not independently decode and compare
every image during a run. The JPEG integration test checks decoded pixels and
metadata retention. Routine runs can be scheduled as described below.

Run the JPEG integration check after building (requires `cjpeg` and `djpeg` from
libjpeg-turbo):

```sh
python3 tests/compression_integration.py ./build/obsidian_optimizer
```

## Unattended runs and scheduling

Pass the vault paths file explicitly to run without a prompt:

```sh
./build/obsidian_optimizer --vaults-file "/absolute/path/vaultpaths.txt" --compress
```

Omit `--compress` for a scan only. Use `--help` for usage. Each nonempty line in
the file is a root directory; relative paths resolve from the file's directory,
so scheduled runs do not depend on a shell's working directory. Blank lines and
Windows line endings are supported. Missing roots and empty configuration files
are errors. Exit codes are 0 for success (including already optimized
images), 1 for file or compression errors, and 2 for invalid CLI arguments.
Without a terminal, the vault paths file argument is required.

Example `vaultpaths.txt` (one folder per line, without quotes):

```text
/Users/yourname/Documents/MyVault
/Users/yourname/Documents/OtherVault
```

### Schedule with cron (macOS or Linux)

First build the executable and install the compression tools as shown above.
Use absolute paths for the executable, configuration, and log. Keep the executable
at that location after scheduling; if using CLion's build, replace
`build/obsidian_optimizer` with `cmake-build-debug/obsidian_optimizer`.

Test the same command with cron's minimal environment before scheduling it:

```sh
env -i HOME="$HOME" PATH=/opt/homebrew/bin:/usr/local/bin:/usr/bin:/bin \
  "/absolute/path/obsidian_optimizer/build/obsidian_optimizer" \
  --vaults-file "/absolute/path/obsidian_optimizer/vaultpaths.txt" --compress
```

This test performs real in-place compression. Omit `--compress` to check discovery
first. Then open your user's crontab (no `sudo` needed):

```sh
crontab -e
```

Add the following, replacing the example paths. The job runs daily at 03:00
in the machine's local timezone. Keep the scheduled command on one line:

```cron
PATH=/opt/homebrew/bin:/usr/local/bin:/usr/bin:/bin
0 3 * * * "/absolute/path/obsidian_optimizer/build/obsidian_optimizer" --vaults-file "/absolute/path/obsidian_optimizer/vaultpaths.txt" --compress >> "/absolute/path/obsidian_optimizer/cron.log" 2>&1
```

`PATH` lets cron find `jpegtran` and `optipng`; add their installation directories
if different on your system. Cron does not load your interactive shell profile.
The log's parent directory must exist and be writable. `>>` appends output and
`2>&1` includes errors. Rotate or truncate this log periodically.

The five schedule fields are minute, hour, day of month, month, and day of week.
Other examples: `0 */6 * * *` runs every six hours and `0 3 * * 0` runs Sundays
at 03:00. See the [crontab reference](https://man7.org/linux/man-pages/man5/crontab.5.html).
Choose an interval longer than a normal run; cron does not prevent overlapping
invocations. Enable only one scheduler for the same vaults.

After saving, check the installed entry and inspect the log after a run:

```sh
crontab -l
tail -n 50 "/absolute/path/obsidian_optimizer/cron.log"
```

To disable the job, run `crontab -e` and remove or comment out its command line.
Preserve any other jobs already in your crontab.

On macOS, cron misses runs while the Mac is asleep or off; it does not catch up
on wake. Choose a time when it will be awake, or use the launchd option below
for wake-time catch-up. See [Apple's scheduling documentation](https://developer.apple.com/library/archive/documentation/MacOSX/Conceptual/BPSystemStartup/Chapters/ScheduledJobs.html).

### macOS daily schedule with launchd

Generate a launchd configuration for your chosen local time. Python 3 is required
for the generator. Use a stable executable path; keep the binary there after
installing the schedule. The example runs daily at 03:00 and enables compression:

```sh
python3 tools/schedule.py \
  --binary "$PWD/build/obsidian_optimizer" \
  --vaults-file "$PWD/vaultpaths.txt" \
  --at 03:00 \
  --output local.obsidian-optimizer.plist
```

The generator validates paths, creates the log directory, and refuses to overwrite
an existing plist. Add `--scan-only` for scheduled scans without compression.
Generating the file does not enable the schedule. To enable it for your user:

```sh
mkdir -p "$HOME/Library/LaunchAgents"
cp -n local.obsidian-optimizer.plist "$HOME/Library/LaunchAgents/"
launchctl bootstrap "gui/$(id -u)" "$HOME/Library/LaunchAgents/local.obsidian-optimizer.plist"
```

Inspect the job, trigger a run immediately, or disable it:

```sh
launchctl print "gui/$(id -u)/local.obsidian-optimizer"
launchctl kickstart "gui/$(id -u)/local.obsidian-optimizer"
launchctl bootout "gui/$(id -u)" "$HOME/Library/LaunchAgents/local.obsidian-optimizer.plist"
```

Remove the installed plist after disabling it to prevent loading it at the next
login. To change the time, disable the job, generate a new plist under a new
filename, replace the installed file, and bootstrap it again.

Logs append to `~/Library/Logs/obsidian_optimizer/stdout.log` and `stderr.log`;
use `--log-directory PATH` to choose another location. Manage log rotation
separately. The generated job includes Homebrew directories and your current
`PATH` so it can locate `jpegtran` and `optipng`. Install both before enabling
compression. This is a user LaunchAgent, active while you are logged in.
`StartCalendarInterval` runs missed sleeping-time jobs on wake; see
[Apple's launchd documentation](https://developer.apple.com/library/archive/documentation/MacOSX/Conceptual/BPSystemStartup/Chapters/CreatingLaunchdJobs.html).

Verify CLI and schedule generation without installing a job:

```sh
python3 tests/scheduling_integration.py ./build/obsidian_optimizer
```
