# PowerShell script to fetch third-party regex libraries for benchmarking

$ErrorActionPreference = "Stop"

$THIRD_PARTY_DIR = $PSScriptRoot + "\..\third_party"
$THIRD_PARTY_DIR = (Resolve-Path -Path $THIRD_PARTY_DIR).Path

Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "  Fetching Third-Party Libraries" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "Third-party directory: $THIRD_PARTY_DIR"
Write-Host ""

# Function to fetch RE2
function Fetch-RE2 {
    Write-Host "Fetching RE2..." -ForegroundColor Yellow
    if (Test-Path "$THIRD_PARTY_DIR\re2") {
        Write-Host "  - RE2 directory already exists, skipping" -ForegroundColor Gray
        return
    }
    try {
        git clone https://github.com/google/re2.git "$THIRD_PARTY_DIR\re2"
        Set-Location "$THIRD_PARTY_DIR\re2"
        git checkout 2023-11-01 2>$null
        Write-Host "  - RE2 checked out" -ForegroundColor Green
        Set-Location $THIRD_PARTY_DIR
    }
    catch {
        Write-Host "  - Failed to fetch RE2: $_" -ForegroundColor Red
    }
}

# Function to fetch PCRE2
function Fetch-PCRE2 {
    Write-Host "Fetching PCRE2..." -ForegroundColor Yellow
    if (Test-Path "$THIRD_PARTY_DIR\pcre2") {
        Write-Host "  - PCRE2 directory already exists, skipping" -ForegroundColor Gray
        return
    }
    try {
        git clone https://github.com/PCRE2Project/pcre2.git "$THIRD_PARTY_DIR\pcre2"
        Set-Location "$THIRD_PARTY_DIR\pcre2"
        git checkout pcre2-10.42 2>$null
        Write-Host "  - PCRE2 checked out" -ForegroundColor Green
        Set-Location $THIRD_PARTY_DIR
    }
    catch {
        Write-Host "  - Failed to fetch PCRE2: $_" -ForegroundColor Red
    }
}

# Function to fetch Hyperscan
function Fetch-Hyperscan {
    Write-Host "Fetching Hyperscan..." -ForegroundColor Yellow
    if (Test-Path "$THIRD_PARTY_DIR\hyperscan") {
        Write-Host "  - Hyperscan directory already exists, skipping" -ForegroundColor Gray
        return
    }
    try {
        git clone https://github.com/01org/hyperscan.git "$THIRD_PARTY_DIR\hyperscan"
        Set-Location "$THIRD_PARTY_DIR\hyperscan"
        git checkout v5.4.0 2>$null
        Write-Host "  - Hyperscan checked out" -ForegroundColor Green
        Set-Location $THIRD_PARTY_DIR
    }
    catch {
        Write-Host "  - Failed to fetch Hyperscan: $_" -ForegroundColor Red
    }
}

# Check what to fetch
$FETCH_ALL = $true
$FETCH_RE2 = $false
$FETCH_PCRE2 = $false
$FETCH_HYPERSCAN = $false

for ($i = 0; $i -lt $args.Count; $i++) {
    switch ($args[$i]) {
        "--with-re2" { $FETCH_ALL = $false; $FETCH_RE2 = $true }
        "--with-pcre2" { $FETCH_ALL = $false; $FETCH_PCRE2 = $true }
        "--with-hyperscan" { $FETCH_ALL = $false; $FETCH_HYPERSCAN = $true }
    }
}

# Fetch libraries
if ($FETCH_ALL -or $FETCH_RE2) { Fetch-RE2 }
if ($FETCH_ALL -or $FETCH_PCRE2) { Fetch-PCRE2 }
if ($FETCH_ALL -or $FETCH_HYPERSCAN) { Fetch-Hyperscan }

Write-Host ""
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "  Fetch Complete!" -ForegroundColor Green
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Next steps:"
Write-Host "  1. Build each library in their respective directories"
Write-Host "  2. Re-run cmake to detect the libraries"
Write-Host "  3. Run: cmake --build ."
Write-Host ""
