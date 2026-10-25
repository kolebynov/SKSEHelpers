param(
    [string]$ClangFormat = "clang-format",
    [switch]$Check
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$sourceRoots = @("src", "tests") | ForEach-Object { Join-Path $repoRoot $_ }

$clangFormatCommand = Get-Command $ClangFormat -ErrorAction SilentlyContinue
if ($null -eq $clangFormatCommand) {
    Write-Error "clang-format was not found. Install clang-format or pass -ClangFormat <path>."
    exit 1
}

$sourceFiles = @(Get-ChildItem -Path $sourceRoots -Recurse -File -Include *.h, *.cpp | Sort-Object FullName)

if ($sourceFiles.Count -eq 0) {
    Write-Host "No .h or .cpp files found."
    exit 0
}

Write-Host "Found $($sourceFiles.Count) .h/.cpp files."

$failed = $false
foreach ($sourceFile in $sourceFiles) {
    $relativePath = $sourceFile.FullName.Substring($repoRoot.TrimEnd('\', '/').Length + 1)

    Write-Host $relativePath
    if ($Check) {
        & $clangFormatCommand.Source --style=file --dry-run --Werror $sourceFile.FullName
        if ($LASTEXITCODE -ne 0) {
            Write-Host "Needs formatting: $relativePath"
            $failed = $true
        }

        continue
    }

    & $clangFormatCommand.Source --style=file -i $sourceFile.FullName
    if ($LASTEXITCODE -ne 0) {
        Write-Error "clang-format failed for $relativePath"
        exit 1
    }
}

if ($failed) {
    Write-Error "clang-format check failed. Run .\Format-Code.ps1 to update formatting."
    exit 1
}

if ($Check) {
    Write-Host "clang-format check passed."
}
