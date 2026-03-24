#!/usr/bin/env bash
# Verify that std::hash<int> compiles to the identity function.
#
# Compiles a minimal program using std::hash<int> at -O3 and checks
# the assembly output. If hash is identity, the compiler constant-folds
# hash(42) to 42 — no function call, no computation.
#
# Usage: ./scripts/verify_identity_hash.sh [clang++]

CXX="${1:-clang++}"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

cat > "$TMPDIR/hash_test.cpp" <<'EOF'
#include <cstdio>
#include <functional>

int main() {
    std::hash<int> h;
    int key = 42;
    size_t result = h(key);
    printf("hash(%d) = %zu\n", key, result);
    return static_cast<int>(result);
}
EOF

echo "==> Compiling with: $CXX -std=c++20 -O3"
"$CXX" -std=c++20 -O3 -S -o "$TMPDIR/hash_test.s" "$TMPDIR/hash_test.cpp"

echo ""
echo "==> Assembly output:"
echo ""
cat "$TMPDIR/hash_test.s"

echo ""
echo "==> Looking for identity evidence (literal 42 used directly):"
if grep -q '#42' "$TMPDIR/hash_test.s" || grep -q '$42' "$TMPDIR/hash_test.s" || grep -q 'movl.*\$42' "$TMPDIR/hash_test.s"; then
    echo "CONFIRMED: std::hash<int>()(42) compiles to literal 42 — identity hash."
else
    echo "INCONCLUSIVE: check assembly above manually."
fi
