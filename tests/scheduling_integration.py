"""Run with python3 tests/scheduling_integration.py /path/to/obsidian_optimizer."""
from pathlib import Path
import plistlib
import subprocess
import sys
import tempfile

binary = str(Path(sys.argv[1]).resolve())
generator = Path(__file__).resolve().parents[1] / 'tools/schedule.py'
with tempfile.TemporaryDirectory(prefix='schedule test ') as directory:
    root = Path(directory).resolve()
    vault = root / 'vault'
    (vault / '.obsidian').mkdir(parents=True)
    config = root / 'vault paths.txt'
    config.write_bytes(b'\r\nvault\r\n')
    def run(*args):
        return subprocess.run([binary, *args], input='', text=True, capture_output=True, cwd='/tmp')
    result = run('--vaults-file', str(config))
    assert result.returncode == 0, result.stderr
    assert 'Vaults discovered: 1' in result.stdout
    assert 'Enter absolute' not in result.stdout
    assert run().returncode == 2
    assert run('--vaults-file').returncode == 2
    assert run('--unknown').returncode == 2
    assert run('--help').returncode == 0
    assert run('--vaults-file', str(root / 'missing')).returncode == 1
    image = vault / 'broken.jpg'
    image.write_bytes(b'invalid')
    assert run('--compress', '--vaults-file', str(config)).returncode == 1
    config.write_text('missing-vault\n')
    assert run('--vaults-file', str(config)).returncode == 1
    config.write_text('')
    assert run('--vaults-file', str(config)).returncode == 1
    config.write_text('vault\n')
    output = root / 'local.obsidian-optimizer.plist'
    command = [sys.executable, str(generator), '--binary', binary,
               '--vaults-file', str(config), '--at', '03:15', '--output', str(output),
               '--log-directory', str(root / 'logs')]
    subprocess.run(command, check=True, capture_output=True)
    with output.open('rb') as file:
        job = plistlib.load(file)
    assert job['StartCalendarInterval'] == {'Hour': 3, 'Minute': 15}
    assert job['ProgramArguments'] == [binary, '--vaults-file', str(config), '--compress']
    result = subprocess.run(job['ProgramArguments'], input='', capture_output=True, cwd='/tmp')
    assert result.returncode == 1  # Invalid JPEG propagates to scheduler.
    assert subprocess.run(command, capture_output=True).returncode != 0
    command[command.index('03:15')] = '25:00'
    assert subprocess.run(command, capture_output=True).returncode != 0
    assert Path(job['StandardOutPath']).parent.is_dir()
    print('PASS: unattended CLI, config-relative paths, failure codes, daily plist, validation, and no overwrite')
