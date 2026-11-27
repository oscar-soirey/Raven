#!/bin/sh
# Installs the Homebrew formula of Raven from this working tree (not from a
# release), runs its test and its audit, then removes everything it made.
#
#   packaging/homebrew/test-local.sh [work folder]
#
# Homebrew installs formulae only from a tap: a temporary tap (local/raven-test,
# made with `brew tap-new` and trusted with `brew trust`, both undone at the
# end) holds a copy of the formula whose url is `git archive HEAD` (file://)
# and its sha256. Commit first: what is not committed is not in the archive.
# Uninstalling the formula at the end lets Homebrew remove the dependencies it
# installed for it alone (cmake, for instance).
set -eu

HERE=$(cd "$(dirname "$0")" && pwd)
RAVEN=$(cd "$HERE/../.." && pwd)
WORK=${1:-"$RAVEN/build/homebrew-test"}
TAP=local/raven-test
export HOMEBREW_NO_AUTO_UPDATE=1 HOMEBREW_NO_INSTALL_CLEANUP=1 HOMEBREW_NO_ANALYTICS=1

mkdir -p "$WORK"
WORK=$(cd "$WORK" && pwd)
VERSION=$(sed -n 's|^  url ".*/v\(.*\)\.tar\.gz"$|\1|p' "$HERE/raven-gui.rb")
ARCHIVE="$WORK/Raven-$VERSION.tar.gz"
git -C "$RAVEN" archive --format=tar.gz --prefix="Raven-$VERSION/" -o "$ARCHIVE" HEAD
SHA256=$(shasum -a 256 "$ARCHIVE" | cut -d' ' -f1)

cleanup() {
    brew uninstall --formula "$TAP/raven-gui" >/dev/null 2>&1 || true
    brew untrust --tap "$TAP" >/dev/null 2>&1 || true
    brew untap "$TAP" >/dev/null 2>&1 || true
}
trap cleanup EXIT

brew tap-new --no-git "$TAP" >/dev/null
# (Homebrew loads the formulae of a tap that is not official only once it is trusted)
brew trust --tap "$TAP"
FORMULA="$(brew --repository "$TAP")/Formula/raven-gui.rb"
sed -e "s|^  url \".*\"$|  url \"file://$ARCHIVE\"|" -e "s|^  sha256 \".*\"$|  sha256 \"$SHA256\"|" "$HERE/raven-gui.rb" > "$FORMULA"

brew style "$TAP/raven-gui"
brew audit --new --strict --formula "$TAP/raven-gui"
brew install --build-from-source "$TAP/raven-gui"
brew linkage --test "$TAP/raven-gui"
brew test --verbose "$TAP/raven-gui"
echo "== the Homebrew formula of $(git -C "$RAVEN" rev-parse --short HEAD) works"
