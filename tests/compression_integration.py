"""Run with python3 tests/compression_integration.py /path/to/obsidian_optimizer.
Requires libjpeg-turbo command-line tools on PATH.
"""
import os
import pathlib
import shutil
import shlex
import subprocess
import sys
import tempfile

binary = str(pathlib.Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory(prefix='compression test ') as directory:
    root = pathlib.Path(directory)
    vault = root / 'vault with spaces'
    (vault / '.obsidian').mkdir(parents=True)
    pixels = bytes((x * 13 + y * 7 + channel * 50) % 256
                   for y in range(64) for x in range(64) for channel in range(3))
    ppm = b'P6\n64 64\n255\n' + pixels
    image = vault / 'photo.JPG'
    original = subprocess.run(['cjpeg', '-quality', '90'], input=ppm,
                              capture_output=True, check=True).stdout
    marker = b'Test image metadata'
    original = original[:2] + b'\xff\xfe' + (len(marker) + 2).to_bytes(2, 'big') + marker + original[2:]
    image.write_bytes(original)
    image.chmod(0o640)
    os.utime(image, ns=(1600000000000000000, 1600000000000000000))
    original_stat = image.stat()
    if sys.platform == 'darwin':
        subprocess.run(['xattr', '-w', 'user.optimizer-test', 'preserve me', str(image)], check=True)
    original_copy = root / 'original.jpg'
    original_copy.write_bytes(original)
    bad = vault / 'broken.jpg'
    bad.write_bytes(b'not a jpeg')
    paths = root / 'vaultpaths.txt'
    paths.write_text(str(vault) + '\n')
    def run():
        return subprocess.run([binary, '--compress', '--vaults-file', str(paths)],
                              text=True, capture_output=True).stdout
    output = run()
    assert image.stat().st_size < len(original), output
    assert image.stat().st_mtime_ns == original_stat.st_mtime_ns
    assert image.stat().st_mode == original_stat.st_mode
    assert marker in image.read_bytes()
    if sys.platform == 'darwin':
        assert subprocess.check_output(['xattr', '-p', 'user.optimizer-test', str(image)]).strip() == b'preserve me'
    def decode(path):
        return subprocess.run(['djpeg', str(path)], capture_output=True, check=True).stdout
    assert decode(image) == decode(original_copy)
    saved = image.read_bytes()
    output = run()
    assert 'Already optimized; original unchanged' in output
    assert 'Media files discovered: 2' in output, output
    assert image.read_bytes() == saved
    assert bad.read_bytes() == b'not a jpeg'
    assert not (vault / '.optimized-media').exists()
    assert not list(vault.rglob('.compression-*'))
    # A missing compressor must leave even an optimizable original intact.
    image.write_bytes(original)
    result = subprocess.run([binary, '--compress', '--vaults-file', str(paths)],
                            text=True, capture_output=True, env={**os.environ, 'PATH': '/nonexistent'})
    assert result.returncode == 1
    assert image.read_bytes() == original
    assert not list(vault.rglob('.compression-*'))
    # Simulate an editor modifying the source while the optimizer is running.
    tool_directory = root / 'tools'
    tool_directory.mkdir()
    wrapper = tool_directory / 'jpegtran'
    real_tool = shutil.which('jpegtran')
    wrapper.write_text('#!/bin/sh\n' + shlex.quote(real_tool) + ' "$@" || exit 1\n'
                       + 'printf edited > "$EDIT_SOURCE"\n')
    wrapper.chmod(0o755)
    result = subprocess.run([binary, '--compress', '--vaults-file', str(paths)],
                            text=True, capture_output=True,
                            env={**os.environ, 'PATH': str(tool_directory), 'EDIT_SOURCE': str(image)})
    assert result.returncode == 1
    assert 'Input changed during compression' in result.stdout
    assert image.read_bytes() == b'edited'
    assert not list(vault.rglob('.compression-*'))
    print('PASS: in-place JPEG, identical pixels, metadata/permissions/timestamp, repeat runs, invalid input, missing tool, concurrent edit, and cleanup')
