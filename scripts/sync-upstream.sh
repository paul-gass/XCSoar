#!/bin/bash
# Sync the OpenGlide Nav fork with upstream XCSoar.
#
# Usage:
#   ./scripts/sync-upstream.sh              # fetch and show what changed
#   ./scripts/sync-upstream.sh --merge      # fetch and merge
#   ./scripts/sync-upstream.sh --rebase     # fetch and rebase (cleaner history)
#
# Prerequisites:
#   git remote add upstream https://github.com/XCSoar/XCSoar.git
#
# The script:
# 1. Fetches the latest from upstream
# 2. Shows what changed since our last sync
# 3. Optionally merges or rebases
#
# After merging, check that:
# - src/ProductName.hpp still has our branding
# - src/OpenGlide/ was not touched by upstream
# - The build still works (CI will verify)

set -euo pipefail

UPSTREAM_REMOTE="upstream"
UPSTREAM_BRANCH="master"

# Ensure the upstream remote exists
if ! git remote | grep -q "^${UPSTREAM_REMOTE}$"; then
    echo "Adding upstream remote..."
    git remote add "$UPSTREAM_REMOTE" https://github.com/XCSoar/XCSoar.git
fi

echo "Fetching upstream..."
git fetch "$UPSTREAM_REMOTE" --tags

# Show what changed
LAST_MERGE=$(git merge-base HEAD "${UPSTREAM_REMOTE}/${UPSTREAM_BRANCH}" 2>/dev/null || echo "")
if [ -n "$LAST_MERGE" ]; then
    BEHIND=$(git rev-list --count "HEAD..${UPSTREAM_REMOTE}/${UPSTREAM_BRANCH}" 2>/dev/null || echo "?")
    AHEAD=$(git rev-list --count "${UPSTREAM_REMOTE}/${UPSTREAM_BRANCH}..HEAD" 2>/dev/null || echo "?")
    echo ""
    echo "Status:"
    echo "  ${BEHIND} commits behind upstream"
    echo "  ${AHEAD} commits ahead (our changes)"
    echo ""

    if [ "$BEHIND" = "0" ]; then
        echo "Already up to date with upstream."
        exit 0
    fi

    echo "Upstream changes since last sync:"
    git log --oneline "${LAST_MERGE}..${UPSTREAM_REMOTE}/${UPSTREAM_BRANCH}" | head -20
    echo ""
fi

# Check for --merge or --rebase flag
case "${1:-}" in
    --merge)
        echo "Merging upstream/${UPSTREAM_BRANCH}..."
        git merge "${UPSTREAM_REMOTE}/${UPSTREAM_BRANCH}" \
            -m "chore: sync with upstream XCSoar $(git describe --tags ${UPSTREAM_REMOTE}/${UPSTREAM_BRANCH} 2>/dev/null || echo '')"

        echo ""
        echo "✓ Merged. Now verify:"
        echo "  1. grep 'OpenGlide Nav' src/ProductName.hpp"
        echo "  2. make -j\$(nproc) TARGET=UNIX VFB=y"
        echo "  3. git push"
        ;;
    --rebase)
        echo "Rebasing onto upstream/${UPSTREAM_BRANCH}..."
        git rebase "${UPSTREAM_REMOTE}/${UPSTREAM_BRANCH}"

        echo ""
        echo "✓ Rebased. Now verify:"
        echo "  1. grep 'OpenGlide Nav' src/ProductName.hpp"
        echo "  2. make -j\$(nproc) TARGET=UNIX VFB=y"
        echo "  3. git push --force-with-lease"
        ;;
    *)
        echo "To merge: $0 --merge"
        echo "To rebase: $0 --rebase"
        ;;
esac
