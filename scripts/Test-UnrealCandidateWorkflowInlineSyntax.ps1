<# 
.SYNOPSIS
    Parse inline PowerShell scripts in the manual UE 5.8 workflow.
.DESCRIPTION
    Does not execute Unreal, the inline source, UAT or any .umap.
    Fail closed on malformed YAML run-block indentation/PowerShell syntax.
#>
[CmdletBinding()]
param(
    [string]$WorkflowPath = (Join-Path $PSScriptRoot '../.github/workflows/build-unreal-candidate.yml')
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if (-not (Test-Path -LiteralPath $WorkflowPath -PathType Leaf)) {
    throw "Cannot parse absent manual workflow: $WorkflowPath"
}

$lines = @(Get-Content -LiteralPath $WorkflowPath -ErrorAction Stop)
$blocks = 0
for ($i = 0; $i -lt $lines.Count; $i++) {
    $line = [string]$lines[$i]
    $match = [regex]::Match($line, '^(?<indent> *)run: *\| *$')
    if (-not $match.Success) {
        continue
    }
    $headerIndent = $match.Groups['indent'].Value.Length
    $blockLines = New-Object System.Collections.Generic.List[string]
    $contentIndent = -1
    for ($j = $i + 1; $j -lt $lines.Count; $j++) {
        $next = [string]$lines[$j]
        if ([string]::IsNullOrWhiteSpace($next)) {
            $blockLines.Add('')
            continue
        }
        $spaces = $next.Length - $next.TrimStart(' ').Length
        if ($spaces -le $headerIndent) {
            break
        }
        if ($contentIndent -lt 0) {
            $contentIndent = $spaces
        }
        if ($spaces -lt $contentIndent) {
            throw "Inconsistent YAML inline-script indentation at line $($j + 1)."
        }
        $blockLines.Add($next.Substring($contentIndent))
    }
    if ($contentIndent -lt 0 -or $blockLines.Count -eq 0) {
        throw "Empty YAML inline script near line $($i + 1)."
    }
    $code = $blockLines -join [Environment]::NewLine
    $tokens = $null
    $errors = $null
    [void][System.Management.Automation.Language.Parser]::ParseInput(
        $code, [ref]$tokens, [ref]$errors)
    if ($errors.Count -gt 0) {
        $errors | ForEach-Object {
            Write-Error "Unreal candidate inline PowerShell block $($blocks + 1): $_"
        }
        throw "Malformed workflow PowerShell block near YAML line $($i + 1)."
    }
    $blocks++
    $i = $j - 1
}
if ($blocks -lt 3) {
    throw "Expected at least 3 manually authorized inline PowerShell steps; got $blocks."
}
Write-Host "Verified $blocks inline PowerShell blocks from the manual UE workflow (not executed)."
