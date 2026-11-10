"""Behavior checks for basic commands and a single redirection only."""
import os
from pathlib import Path
import subprocess
import tempfile

shell = str(Path(os.environ.get('S3', './build/s3')).resolve())
passed = 0


def run(text, cwd):
    return subprocess.run([shell], input=text, text=True, capture_output=True,
                          cwd=cwd, timeout=5)


def compare(text, cwd):
    global passed
    expected = subprocess.run(['bash'], input=text, text=True, capture_output=True,
                              cwd=cwd, timeout=5)
    actual = run(text, cwd)
    assert actual.stdout == expected.stdout, (text, actual.stdout, expected.stdout)
    assert actual.returncode == 0, (text, actual.stderr)
    assert not actual.stderr, (text, actual.stderr)
    passed += 1


with tempfile.TemporaryDirectory() as directory:
    for text in [
        'echo hello\nexit\n',
        'pwd\n',
        'whoami\n',
        'ls\n',
        'echo "hello world"\n',
        "echo 'hello world'\n",
        'echo pre"middle word"post\n',
        'printf "[%s][%s]\\n" "" end\n',
        'echo hello\\ world\n',
        'echo "< > >>"\n',
        'echo \\>\n',
        'echo\thello\tworld\n',
        '\n \t \n echo ok\n',
        'echo final',  # final line without newline
        '',  # normal EOF
        'echo first>out\necho second>>out\ncat<out\n',
        'echo "hello world" > "file with spaces"\ncat < "file with spaces"\n',
        'echo old > out\necho new > out\ncat < out\n',
        'echo > out hello world\ncat < out\n',
        'echo hi > ">"\ncat < ">"\n',
        'echo "a\\qb"\n',
        'exit\necho should-not-run\n',
    ]:
        compare(text, directory)
    for text in [
        'echo hi >\n', 'echo "unfinished\n', 'echo trailing\\\n',
        'echo hi > > out\n', 'echo hi > out > other\n',
        'echo ' + 'x ' * 130 + '\n',
        'echo ' + 'x' * 1100 + '\n',
        'cat < missing-file\n', 'missing-program-for-s3-test\n',
    ]:
        result = run(text + 'echo survived\n', directory)
        assert result.stderr, (text, 'expected error')
        assert result.stdout == 'survived\n', (text, result.stdout)
        passed += 1
print(f'{passed} checks passed')
