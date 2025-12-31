#!/bin/bash
# Script to fetch and build third-party regex libraries for benchmarking
# Uses Ninja as the build system

set +e  # Don't exit on individual failures

THIRD_PARTY_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../third_party" && pwd)"
cd "$THIRD_PARTY_DIR"

echo "========================================="
echo "  Fetching Third-Party Libraries"
echo "========================================="
echo "Third-party directory: $THIRD_PARTY_DIR"
echo

# Count successes/failures
SUCCESS=0
FAILED=0

# Function to fetch RE2
fetch_re2() {
    echo "Fetching RE2..."
    if [ -d "re2" ]; then
        echo "  - RE2 directory already exists, skipping"
        return 0
    fi
    if command -v git &> /dev/null; then
        if git clone --depth 1 --branch 2023-11-01 https://github.com/google/re2.git re2; then
            echo "  - RE2 cloned successfully"
            ((SUCCESS++))
            return 0
        else
            echo "  - Failed to clone RE2"
            ((FAILED++))
            return 1
        fi
    else
        echo "  - Git not available"
        ((FAILED++))
        return 1
    fi
}

# Function to fetch PCRE2
fetch_pcre2() {
    echo "Fetching PCRE2..."
    if [ -d "pcre2" ]; then
        echo "  - PCRE2 directory already exists, skipping"
        return 0
    fi
    if command -v git &> /dev/null; then
        if git clone --depth 1 --branch pcre2-10.42 https://github.com/PCRE2Project/pcre2.git pcre2; then
            echo "  - PCRE2 cloned successfully"
            ((SUCCESS++))
            return 0
        else
            echo "  - Failed to clone PCRE2"
            ((FAILED++))
            return 1
        fi
    else
        echo "  - Git not available"
        ((FAILED++))
        return 1
    fi
}

# Function to fetch CTRE
fetch_ctre() {
    echo "Fetching CTRE..."
    if [ -d "ctre" ]; then
        echo "  - CTRE directory already exists, skipping"
        return 0
    fi
    if command -v git &> /dev/null; then
        if git clone --depth 1 https://github.com/hanickadot/compile-time-regular-expressions.git ctre 2>&1; then
            echo "  - CTRE cloned successfully"
            ((SUCCESS++))
            return 0
        else
            echo "  - Failed to clone CTRE"
            ((FAILED++))
            return 1
        fi
    else
        echo "  - Git not available"
        ((FAILED++))
        return 1
    fi
}

# Function to build RE2 with Ninja
build_re2_ninja() {
    echo "Building RE2 with CMake+Ninja..."
    if [ ! -d "re2" ]; then
        echo "  - RE2 not found, skipping build"
        return 1
    fi
    cd re2

    # Check if already built
    if [ -f "obj/libre2.a" ]; then
        echo "  - RE2 already built"
        cd "$THIRD_PARTY_DIR"
        return 0
    fi

    mkdir -p obj
    cd obj
    echo "  - Configuring with CMake..."
    if cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DCMAKE_FIND_FRAMEWORK=LAST -DCMAKE_FIND_APPBUNDLE=LAST -G Ninja > /dev/null 2>&1; then
        echo "  - Building with Ninja..."
        if ninja > /dev/null 2>&1; then
            echo "  - RE2 built successfully"
            cd "$THIRD_PARTY_DIR"
            ((SUCCESS++))
            return 0
        else
            echo "  - Ninja build failed"
            cd "$THIRD_PARTY_DIR"
            ((FAILED++))
            return 1
        fi
    else
        echo "  - CMake configuration failed"
        cd "$THIRD_PARTY_DIR"
        ((FAILED++))
        return 1
    fi
    cd ..
}

# Function to build PCRE2 with Ninja
build_pcre2_ninja() {
    echo "Building PCRE2 with CMake+Ninja..."
    if [ ! -d "pcre2" ]; then
        echo "  - PCRE2 not found, skipping build"
        return 1
    fi
    cd pcre2

    # Check if already built
    if [ -f "build/libpcre2-8.a" ] || [ -f "build/libpcre2-8.so" ]; then
        echo "  - PCRE2 already built"
        cd "$THIRD_PARTY_DIR"
        return 0
    fi

    if command -v ninja &> /dev/null && command -v cmake &> /dev/null; then
        echo "  - Configuring with CMake..."
        mkdir -p build
        cd build

        # Patch CMakeLists.txt for modern CMake
        cd ..
        if [ -f "CMakeLists.txt" ]; then
            cp CMakeLists.txt CMakeLists.txt.bak
            if command -v perl &> /dev/null; then
                perl -pi -e 's/CMAKE_MINIMUM_REQUIRED\(VERSION [0-9.]+\)/cmake_minimum_required(VERSION 3.15)/g' CMakeLists.txt
            else
                sed 's/CMAKE_MINIMUM_REQUIRED(VERSION [0-9.]+)/cmake_minimum_required(VERSION 3.15)/g' CMakeLists.txt > CMakeLists.txt.tmp
                mv CMakeLists.txt.tmp CMakeLists.txt
            fi
        fi
        cd build

        if cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DBUILD_TESTS=OFF -DPCRE2_SUPPORT_JIT=OFF -G Ninja > /dev/null 2>&1; then
            echo "  - Building with Ninja..."
            if ninja > /dev/null 2>&1; then
                echo "  - PCRE2 built successfully"
                cd "$THIRD_PARTY_DIR/pcre2"
                if [ -f "CMakeLists.txt.bak" ]; then
                    mv CMakeLists.txt.bak CMakeLists.txt
                fi
                cd "$THIRD_PARTY_DIR"
                ((SUCCESS++))
                return 0
            else
                echo "  - Ninja build failed"
                cd "$THIRD_PARTY_DIR/pcre2"
                if [ -f "CMakeLists.txt.bak" ]; then
                    mv CMakeLists.txt.bak CMakeLists.txt
                fi
                cd "$THIRD_PARTY_DIR"
                ((FAILED++))
                return 1
            fi
        else
            echo "  - CMake configuration failed"
            cd "$THIRD_PARTY_DIR/pcre2"
            if [ -f "CMakeLists.txt.bak" ]; then
                mv CMakeLists.txt.bak CMakeLists.txt
            fi
            cd "$THIRD_PARTY_DIR"
            ((FAILED++))
            return 1
        fi
        cd ..
    fi

    echo "  - Need both cmake and ninja to build PCRE2"
    cd "$THIRD_PARTY_DIR"
    ((FAILED++))
    return 1
}

# CTRE is header-only, no build needed

# Parse arguments
FETCH_ONLY=false
BUILD_ONLY=false
FETCH_RE2=""
FETCH_PCRE2=""
FETCH_CTRE=""

while [[ $# -gt 0 ]]; do
    case $1 in
        --fetch-only)
            FETCH_ONLY=true
            shift
            ;;
        --build-only)
            BUILD_ONLY=true
            shift
            ;;
        --re2)
            FETCH_RE2=true
            shift
            ;;
        --pcre2)
            FETCH_PCRE2=true
            shift
            ;;
        --ctre)
            FETCH_CTRE=true
            shift
            ;;
        *)
            echo "Unknown option: $1"
            echo "Usage: $0 [--fetch-only] [--build-only] [--re2] [--pcre2] [--ctre]"
            exit 1
            ;;
    esac
done

# Set what to fetch
FETCH_ALL=true
if [ -n "$FETCH_RE2" ] || [ -n "$FETCH_PCRE2" ] || [ -n "$FETCH_CTRE" ]; then
    FETCH_ALL=false
fi

# Fetch libraries
if [ "$BUILD_ONLY" = false ]; then
    if [ "$FETCH_ALL" = true ] || [ -n "$FETCH_RE2" ]; then
        fetch_re2
    fi
    if [ "$FETCH_ALL" = true ] || [ -n "$FETCH_PCRE2" ]; then
        fetch_pcre2
    fi
    if [ "$FETCH_ALL" = true ] || [ -n "$FETCH_CTRE" ]; then
        fetch_ctre
    fi
fi

# Build libraries (CTRE is header-only, no build)
if [ "$FETCH_ONLY" = false ]; then
    if [ "$FETCH_ALL" = true ] || [ -n "$FETCH_RE2" ]; then
        build_re2_ninja
    fi
    if [ "$FETCH_ALL" = true ] || [ -n "$FETCH_PCRE2" ]; then
        build_pcre2_ninja
    fi
fi

echo
echo "========================================="
echo "  Summary"
echo "========================================="
echo "Successful fetches/builds: $SUCCESS"
echo "Failed: $FAILED"
echo

if [ $SUCCESS -gt 0 ]; then
    echo "Now reconfigure cmake to detect libraries:"
    echo "  rm -rf build && cmake -S . -B build -G Ninja"
    echo "  ninja -C build"
fi
