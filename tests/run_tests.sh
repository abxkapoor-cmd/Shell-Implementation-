
S3=./build/s3
passed=0
failed=0

check() {
    cmd="$1"
    expected=$(echo "$cmd" | bash 2>/dev/null)
    actual=$(echo "$cmd" | $S3 2>/dev/null | sed 's/\[s3\]\$ //g')

    if [ "$expected" == "$actual" ]; then
        echo "PASS: $cmd"
        passed=$((passed + 1))
    else
        echo "FAIL: $cmd"
        echo "  expected: $expected"
        echo "  got:      $actual"
        failed=$((failed + 1))
    fi
}

 # tests:
check "pwd"
check "ls"
check "whoami"
check "ls -l"
check "cat txt/phrases.txt"
check "sort txt/phrases.txt"
check "grep burn txt/phrases.txt"
check "wc txt/phrases.txt"

echo "$passed passed, $failed failed"
[ $failed -eq 0 ]
