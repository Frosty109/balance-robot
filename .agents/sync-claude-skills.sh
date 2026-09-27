#!/usr/bin/env sh
# Regenerates .claude/skills/ from .agents/skills/; with --check, reports drift instead.
set -eu

repo=$(cd "$(dirname "$0")/.." && pwd)
src="$repo/.agents/skills"
dst="$repo/.claude/skills"
check=0
[ "${1:-}" = "--check" ] && check=1

# Codex's skill is implementation-plan; Claude keeps the /plan name used across projects.
claude_name() {
    case "$1" in
        implementation-plan) echo plan ;;
        *) echo "$1" ;;
    esac
}

render() {
    name=$1
    out=$(claude_name "$name")
    awk -v src=".agents/skills/$name/SKILL.md" '
        { print }
        /^---$/ && ++fences == 2 { print ""; print "<!-- Generated from " src " by .agents/sync-claude-skills.sh. Edit the source. -->" }
    ' "$src/$name/SKILL.md" |
        sed -e "s/^name: $name\$/name: $out/" \
            -e 's/\$implementation-plan/\/plan/g' \
            -e 's/\$\([a-z][a-z-]*\)/\/\1/g'
}

status=0
for dir in "$src"/*/; do
    name=$(basename "$dir")
    [ -f "$src/$name/SKILL.md" ] || continue
    target="$dst/$(claude_name "$name")/SKILL.md"
    if [ "$check" -eq 1 ]; then
        if ! render "$name" | cmp -s - "$target"; then
            echo "out of date: $target"
            status=1
        fi
    else
        mkdir -p "$(dirname "$target")"
        render "$name" > "$target"
        echo "wrote $target"
    fi
done
exit $status
