#!/usr/bin/env python3
"""Find merges that dropped one side's changes to a file.

For every merge commit on the first-parent history of a branch, looks at the
files both sides changed since their merge base. If the merged file is exactly
one side's version while the other side changed it differently, the other
side's changes to that file were lost (usually a conflict resolved by taking
one side's whole file).

    python tools/check_merges.py                 # merges of the last 50 commits of HEAD
    python tools/check_merges.py acd5b93..main   # merges in a range
    python tools/check_merges.py --pr 123        # the merge of PR #123

Exits with 1 when something looks lost, 0 otherwise. Files under bindings/ are
generated: regenerate them rather than merging them by hand.
"""
import argparse
import subprocess
import sys


def git(*args):
    return subprocess.run(["git", *args], capture_output=True, text=True, check=False).stdout.strip()


def blob(commit, path):
    return git("rev-parse", "-q", "--verify", f"{commit}:{path}")


def check(merge):
    p1, p2 = merge + "^1", merge + "^2"
    base = git("merge-base", p1, p2)
    if not base:
        return []
    ours = set(git("diff", "--name-only", base, p1).splitlines())
    theirs = set(git("diff", "--name-only", base, p2).splitlines())
    lost = []
    for path in sorted(ours & theirs):
        a, b, r = blob(p1, path), blob(p2, path), blob(merge, path)
        if a == b:
            continue
        if r == a:
            lost.append(("branch side", path))
        elif r == b:
            lost.append(("main side", path))
    return lost


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("range", nargs="?", default="HEAD~50..HEAD", help="revision range (default HEAD~50..HEAD)")
    ap.add_argument("--pr", type=int, help="only the merge of this pull request")
    ap.add_argument("--all", action="store_true", help="include the generated files under bindings/")
    args = ap.parse_args()

    merges = git("rev-list", "--merges", "--first-parent", args.range).splitlines()
    found = False
    for merge in merges:
        subject = git("log", "-1", "--format=%s", merge)
        if args.pr and f"#{args.pr} " not in subject + " ":
            continue
        lost = [(s, p) for s, p in check(merge) if args.all or not p.startswith("bindings/")]
        if not lost:
            continue
        found = True
        print(f"{merge[:9]} {subject}")
        for side, path in lost:
            print(f"    {side} lost: {path}")
    if not found:
        print("No merge dropped one side's changes.")
    return 1 if found else 0


if __name__ == "__main__":
    sys.exit(main())
