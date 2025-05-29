param (
    [string]$FilePath,
    [string]$MacroName,
    [bool]$CommentOut
)

if (-not (Test-Path $FilePath)) {
    Write-Error "File not found: $FilePath"
    exit 1
}

$content = Get-Content $FilePath

$newContent = $content | ForEach-Object {
    if ($CommentOut) {
        # COMMENT OUT: if not already commented
        if ($_ -match "^\s*#define\s+$MacroName") {
            $_ -replace "^\s*#define\s+$MacroName", "//$&"
        } else {
            $_
        }
    } else {
        # UNCOMMENT: if commented
        if ($_ -match "^\s*//\s*#define\s+$MacroName") {
            $_ -replace "^\s*//\s*#define\s+$MacroName", "#define $MacroName"
        } else {
            $_
        }
    }
}

Set-Content -Path $FilePath -Value $newContent
