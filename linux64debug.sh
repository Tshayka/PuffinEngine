#!/usr/bin/env bash
set -euo pipefail

build_dir="build64d"
build_type="Debug"
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
app_exe="$repo_root/PuffinEngine/work/debug/PuffinEngine"

if [[ -z "${VULKAN_SDK:-}" ]]; then
    vulkan_root="$HOME/vulkan"
    if [[ -d "$vulkan_root" ]]; then
        latest_vulkan="$(find "$vulkan_root" -mindepth 1 -maxdepth 1 -type d | sort -Vr | head -n 1 || true)"
        if [[ -n "$latest_vulkan" && -d "$latest_vulkan/x86_64" ]]; then
            export VULKAN_SDK="$latest_vulkan/x86_64"
        fi
    fi
fi

if [[ -n "${VULKAN_SDK:-}" ]]; then
    export PATH="$VULKAN_SDK/bin:$PATH"
    export LD_LIBRARY_PATH="$VULKAN_SDK/lib:${LD_LIBRARY_PATH:-}"
    export VK_LAYER_PATH="$VULKAN_SDK/etc/explicit_layer.d:${VK_LAYER_PATH:-}"
fi

cmake --fresh -S "$repo_root" -B "$repo_root/$build_dir" -DCMAKE_BUILD_TYPE="$build_type" -Wdev
cmake --build "$repo_root/$build_dir" --target PuffinEngine -- -j"$(nproc)"

if [[ ! -x "$app_exe" ]]; then
    echo "Built executable was not found at $app_exe" >&2
    exit 1
fi

cd "$repo_root/$build_dir"
"$app_exe"
