"""Behavior checks for commands, redirection, and pipelines."""
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
    # Run each shell from the same clean path, and compare file contents too.
    with tempfile.TemporaryDirectory(dir=cwd) as case_directory:
        expected = subprocess.run(['bash'], input=text, text=True,
                                  capture_output=True, cwd=case_directory, timeout=5)
        expected_files = {p.name: p.read_bytes()
                          for p in Path(case_directory).iterdir() if p.is_file()}
        for path in Path(case_directory).iterdir():
            path.unlink()

        actual = run(text, case_directory)
        actual_files = {p.name: p.read_bytes()
                        for p in Path(case_directory).iterdir() if p.is_file()}
        assert actual.stdout == expected.stdout, (text, actual.stdout, expected.stdout)
        assert actual.returncode == 0, (text, actual.stderr)
        assert not actual.stderr, (text, actual.stderr)
        assert actual_files == expected_files, (text, actual_files, expected_files)
    passed += 1


with tempfile.TemporaryDirectory() as directory:
    for text in [
        'echo hello\nexit\n',
        'echo hello | wc -c\n',
        'echo hello|cat\n',
        'echo hello | cat | cat\n',
        'printf "b\\na\\n" | sort | head -n 1 | wc -l\n',
        'seq 1 50000 | cat | wc -l\n',
        'yes | cat | head -n 1\n',
        'echo hello' + ' | cat' * 30 + '\n',
        'echo hello | cat > out\n',
        'echo hello|cat>out\n',
        'echo old > out\necho new | cat > out\n',
        'echo old > out\necho new | cat >> out\n',
        'echo hello | cat >> new-file\n',
        'echo hello > input\ncat < input | cat | wc -l\n',
        'echo hello > input\ncat < input | cat > output\n',
        'echo hello > middle | wc -c\n',
        'echo hello | cat > middle | wc -c\n',
        'echo file > input\necho pipe | cat < input\n',
        'echo hello | cat > "file with spaces"\n',
        'echo hello | echo > out trailing arguments\n',
        'echo ">" | cat > ">"\n',
        'echo "a|b" | cat | wc -c\n',

        'printf "b\\na\\n" | sort\n',
        'echo "|" | cat\n',
        "echo '|'\n",
        'echo \\| | cat\n',
        'seq 1 50000 | wc -l\n',  # exceeds pipe capacity
        'yes | head -n 1\n',  # reader exits before the writer
        'echo hello | cat\necho finished\n',

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
        'echo hi >\n',
        '| cat\n', 'echo hello |\n',
        'echo hello | | cat\n',
        'echo hello | cat |\n',
        'echo hello | cat >\n',
        'echo hello | cat > > out\n',
        'echo hello | cat > out >> other\n',
        'echo hello | cat < missing-file\n',
        'echo hello | cat > missing-directory/out\n',
        'echo hello | cat | echo "unfinished\n',

        'nonexistent-pipeline-program | cat\n',
        'echo "unfinished\n', 'echo trailing\\\n',
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
