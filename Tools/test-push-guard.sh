#!/bin/sh
cd "$(dirname "$0")/.." || exit 1

head=$(git rev-parse HEAD)
parent=$(git rev-parse HEAD~1)
zero=$(echo "$head" | tr '0-9a-f' '0')
failed=0

expect() {
	echo "$3" | GIT_LFS_SKIP_PUSH=1 sh Tools/hooks/pre-push origin test >/dev/null 2>&1
	got=$?
	[ "$got" -eq "$1" ] || { echo "FAIL: $2 (exit $got, expected $1)"; failed=1; }
}

expect 1 "deleting main is refused"           "(delete) $zero refs/heads/main $head"
expect 1 "rewriting main is refused"          "refs/heads/main $parent refs/heads/main $head"
expect 0 "fast-forwarding main is allowed"    "refs/heads/main $head refs/heads/main $parent"
expect 0 "the first push of main is allowed"  "refs/heads/main $head refs/heads/main $zero"
expect 0 "rewriting a task branch is allowed" "refs/heads/cam/x $parent refs/heads/cam/x $head"

[ "$failed" -eq 0 ] && echo "PASS: the push guard refuses what it should."
exit $failed
