#!/bin/zsh

RED='\033[31m'
GREEN='\033[32m'
NC='\033[0m'

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

setvar()
{
    local name="$1"
    local value="$2"

    export "$name=$value"

    if [ -e "$value" ]; then
        echo -e "${GREEN}${name}=${value}${NC}"
    else
        echo -e "${RED}${name}=${value}${NC}"
    fi
}

setvar IMP_ROOT "$ROOT"
setvar IMP_ENGINE_ROOT "$ROOT/imp"
setvar IMP_TOOLS "$ROOT/tools"
setvar IMP_BUILD_DEB "$ROOT/build/mac-debug/engine"
setvar IMP_BUILD_REL "$ROOT/build/mac-release/engine"
setvar IMP_PROJ "$ROOT/sandbox"

echo
echo "Environment variables have been set."
read -r -p "Press Enter to continue..."
