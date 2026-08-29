# Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

[CmdletBinding()]
param(
    [ValidateSet("Audit", "Scan", "Resave", "Remove")]
    [string]$Mode = "Audit",

    [string]$ProjectPath,
    [string]$ConfigPath,
    [string]$EngineRoot,
    [string]$EditorTarget,
    [string]$OutputRoot,

    [string[]]$RedirectId = @(),
    [string]$Match,
    [ValidateSet("Class", "Struct", "Enum", "Function", "Property", "Package")]
    [string[]]$Type = @(),
    [string[]]$PackagePath = @(),

    [switch]$Apply,
    [switch]$AllowMaps,
    [switch]$SkipBuild,
    [switch]$SkipEngineScan,
    [switch]$ForcePowerShellScan,
    [switch]$QuietSelection,
    [switch]$Force,
    [int]$MaxPackages = 0
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Find-ContainingUnrealProject {
    param([Parameter(Mandatory)][string]$StartPath)

    $directory = Get-Item -LiteralPath $StartPath
    while ($null -ne $directory) {
        $projects = @(Get-ChildItem -LiteralPath $directory.FullName -Filter "*.uproject" -File -ErrorAction SilentlyContinue)
        if ($projects.Count -eq 1) {
            return $projects[0].FullName
        }
        if ($projects.Count -gt 1) {
            throw "Multiple .uproject files were found in '$($directory.FullName)'. Supply -ProjectPath explicitly."
        }
        $directory = $directory.Parent
    }
    throw "No .uproject file was found above '$StartPath'. Supply -ProjectPath explicitly."
}

$PluginRoot = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($ProjectPath)) {
    $ProjectPath = Find-ContainingUnrealProject -StartPath $PluginRoot
}
$ProjectPath = [System.IO.Path]::GetFullPath($ProjectPath)
if (-not (Test-Path -LiteralPath $ProjectPath -PathType Leaf)) {
    throw "Unreal project was not found at '$ProjectPath'."
}

$RepoRoot = Split-Path -Parent $ProjectPath
if ([string]::IsNullOrWhiteSpace($ConfigPath)) {
    $ConfigPath = Join-Path $RepoRoot "Config\DefaultEngine.ini"
}
if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    $OutputRoot = Join-Path $RepoRoot "Saved\CoreRedirectAudit"
}

$ConfigPath = [System.IO.Path]::GetFullPath($ConfigPath)
$OutputRoot = [System.IO.Path]::GetFullPath($OutputRoot)
if (-not (Test-Path -LiteralPath $ConfigPath -PathType Leaf)) {
    throw "CoreRedirect config was not found at '$ConfigPath'."
}
$RunStamp = Get-Date -Format "yyyyMMdd-HHmmss"
$RunDirectory = Join-Path $OutputRoot $RunStamp
$null = New-Item -ItemType Directory -Force -Path $RunDirectory
$script:ResolvedRipgrepPath = $null

$RedirectId = @(
    $RedirectId |
        ForEach-Object { $_ -split ',' } |
        ForEach-Object { $_.Trim() } |
        Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
)
$PackagePath = @(
    $PackagePath |
        ForEach-Object { $_ -split ',' } |
        ForEach-Object { $_.Trim() } |
        Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
)

function Get-StableRedirectId {
    param(
        [Parameter(Mandatory)][string]$RedirectType,
        [Parameter(Mandatory)][string]$OldName
    )

    $sha = [System.Security.Cryptography.SHA256]::Create()
    try {
        $bytes = [System.Text.Encoding]::UTF8.GetBytes("$RedirectType|$OldName")
        $hash = $sha.ComputeHash($bytes)
        return ([System.Convert]::ToHexString($hash)).Substring(0, 12).ToLowerInvariant()
    }
    finally {
        $sha.Dispose()
    }
}

function Get-CoreRedirectEntries {
    param([Parameter(Mandatory)][string]$Path)

    $lines = [System.IO.File]::ReadAllLines($Path)
    $insideSection = $false
    $entries = [System.Collections.Generic.List[object]]::new()

    for ($index = 0; $index -lt $lines.Count; $index++) {
        $line = $lines[$index]
        if ($line -match '^\s*\[CoreRedirects\]\s*$') {
            $insideSection = $true
            continue
        }
        if ($insideSection -and $line -match '^\s*\[.+\]\s*$') {
            break
        }
        if (-not $insideSection) {
            continue
        }
        if ($line -notmatch '^\s*\+(?<Type>Class|Struct|Enum|Function|Property|Package)Redirects\s*=') {
            continue
        }

        $redirectType = $Matches.Type
        $oldName = if ($line -match 'OldName\s*=\s*"(?<Value>[^"]*)"') { $Matches.Value } else { "" }
        $newName = if ($line -match 'NewName\s*=\s*"(?<Value>[^"]*)"') { $Matches.Value } else { "" }
        $removed = $line -match 'Removed\s*=\s*true'
        $hasValueChanges = $line -match 'ValueChanges\s*='
        $isWildcard = $line -match '(MatchWildcard|MatchSubstring)\s*=\s*true' -or $oldName.Contains("...")
        $oldLeaf = if ($oldName.Contains('.')) {
            $oldName.Substring($oldName.LastIndexOf('.') + 1)
        }
        elseif ($oldName.Contains('/')) {
            $oldName.Substring($oldName.LastIndexOf('/') + 1)
        }
        else {
            $oldName
        }

        $entries.Add([pscustomobject]@{
            Id = Get-StableRedirectId -RedirectType $redirectType -OldName $oldName
            Line = $index + 1
            Type = $redirectType
            OldName = $oldName
            NewName = $newName
            OldLeaf = $oldLeaf
            Removed = $removed
            HasValueChanges = $hasValueChanges
            IsWildcard = $isWildcard
            Raw = $line
            IntroducedDate = $null
            IntroducedCommit = $null
            IntroducedSummary = $null
            StaticTextPaths = @()
            StaticPackagePaths = @()
            EnginePackages = @()
            Status = "Unscanned"
        })
    }

    if (-not $insideSection) {
        throw "[CoreRedirects] section was not found in '$Path'."
    }
    return @($entries)
}

function Add-GitBlameMetadata {
    param(
        [Parameter(Mandatory)][object[]]$Entries,
        [Parameter(Mandatory)][string]$Path
    )

    $relativePath = [System.IO.Path]::GetRelativePath($RepoRoot, $Path)
    $blameLines = @(& git -C $RepoRoot blame --line-porcelain -- $relativePath 2>$null)
    if ($LASTEXITCODE -ne 0) {
        return
    }

    $metadataByLine = @{}
    $currentLine = 0
    $currentCommit = $null
    $currentTime = $null
    $currentSummary = $null
    foreach ($blameLine in $blameLines) {
        if ($blameLine -match '^(?<Commit>[0-9a-f]{40})\s+\d+\s+(?<FinalLine>\d+)') {
            $currentCommit = $Matches.Commit
            $currentLine = [int]$Matches.FinalLine
            $currentTime = $null
            $currentSummary = $null
        }
        elseif ($blameLine -match '^author-time\s+(?<Time>\d+)$') {
            $currentTime = [long]$Matches.Time
        }
        elseif ($blameLine -match '^summary\s+(?<Summary>.*)$') {
            $currentSummary = $Matches.Summary
        }
        elseif ($blameLine.StartsWith("`t") -and $currentLine -gt 0) {
            $date = if ($null -ne $currentTime) {
                [DateTimeOffset]::FromUnixTimeSeconds($currentTime).ToLocalTime().ToString("yyyy-MM-dd")
            }
            else {
                $null
            }
            $metadataByLine[$currentLine] = [pscustomobject]@{
                Commit = $currentCommit
                Date = $date
                Summary = $currentSummary
            }
        }
    }

    foreach ($entry in $Entries) {
        if ($metadataByLine.ContainsKey($entry.Line)) {
            $metadata = $metadataByLine[$entry.Line]
            $entry.IntroducedCommit = $metadata.Commit
            $entry.IntroducedDate = $metadata.Date
            $entry.IntroducedSummary = $metadata.Summary
        }
    }
}

function Get-StructuralFindings {
    param([Parameter(Mandatory)][object[]]$Entries)

    $findings = [System.Collections.Generic.List[object]]::new()
    foreach ($group in ($Entries | Group-Object Type, OldName | Where-Object Count -gt 1)) {
        $targets = @($group.Group.NewName | Sort-Object -Unique)
        $severity = if ($targets.Count -gt 1) { "Error" } else { "Warning" }
        $findings.Add([pscustomobject]@{
            Severity = $severity
            Code = if ($targets.Count -gt 1) { "ConflictingOldName" } else { "DuplicateOldName" }
            Message = "$($group.Name) appears $($group.Count) times at lines $($group.Group.Line -join ', ')."
        })
    }

    foreach ($entry in $Entries) {
        if ([string]::IsNullOrWhiteSpace($entry.OldName)) {
            $findings.Add([pscustomobject]@{ Severity = "Error"; Code = "MissingOldName"; Message = "Line $($entry.Line) has no OldName." })
        }
        if (-not $entry.Removed -and [string]::IsNullOrWhiteSpace($entry.NewName)) {
            $findings.Add([pscustomobject]@{ Severity = "Error"; Code = "MissingNewName"; Message = "Line $($entry.Line) has no NewName." })
        }
        if ($entry.OldName -eq $entry.NewName -and -not $entry.HasValueChanges) {
            $findings.Add([pscustomobject]@{ Severity = "Error"; Code = "IdentityRedirect"; Message = "Line $($entry.Line) redirects a name to itself without ValueChanges." })
        }
        if ($entry.IsWildcard) {
            $findings.Add([pscustomobject]@{ Severity = "Warning"; Code = "WildcardRedirect"; Message = "Line $($entry.Line) is a wildcard redirect and should remain temporary." })
        }
    }

    foreach ($entry in $Entries) {
        if ($entry.OldName -eq $entry.NewName) {
            continue
        }
        $next = @($Entries | Where-Object { $_.Type -eq $entry.Type -and $_.OldName -eq $entry.NewName })
        foreach ($nextEntry in $next) {
            $findings.Add([pscustomobject]@{
                Severity = "Warning"
                Code = "RedirectChain"
                Message = "Lines $($entry.Line) and $($nextEntry.Line) form a chain: $($entry.OldName) -> $($entry.NewName) -> $($nextEntry.NewName)."
            })
        }
    }
    return @($findings)
}

function Get-SearchTokens {
    param([Parameter(Mandatory)][object[]]$Entries)

    return @(
        foreach ($entry in $Entries) {
            if (-not [string]::IsNullOrWhiteSpace($entry.OldName)) { $entry.OldName }
            if ($entry.OldLeaf.Length -ge 4) { $entry.OldLeaf }
        }
    ) | Sort-Object -Unique
}

function New-TokenRegexes {
    param(
        [Parameter(Mandatory)][string[]]$Tokens,
        [switch]$WordMatch
    )

    $regexes = [System.Collections.Generic.List[System.Text.RegularExpressions.Regex]]::new()
    $sortedTokens = @($Tokens | Sort-Object Length -Descending -Unique)
    for ($offset = 0; $offset -lt $sortedTokens.Count; $offset += 100) {
        $end = [Math]::Min($offset + 99, $sortedTokens.Count - 1)
        $alternatives = @($sortedTokens[$offset..$end] | ForEach-Object { [System.Text.RegularExpressions.Regex]::Escape($_) })
        $body = "(?:$($alternatives -join '|'))"
        $pattern = if ($WordMatch) {
            "(?<![A-Za-z0-9_])$body(?![A-Za-z0-9_])"
        }
        else {
            $body
        }
        $regexes.Add([System.Text.RegularExpressions.Regex]::new(
            $pattern,
            [System.Text.RegularExpressions.RegexOptions]::CultureInvariant
        ))
    }
    return @($regexes)
}

function Invoke-PowerShellTokenScan {
    param(
        [Parameter(Mandatory)][string[]]$Tokens,
        [Parameter(Mandatory)][string[]]$ExistingRoots,
        [Parameter(Mandatory)][string[]]$Globs,
        [switch]$Binary
    )

    $extensions = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
    foreach ($glob in $Globs) {
        if ($glob -match '^\*(?<Extension>\.[A-Za-z0-9]+)$') {
            $null = $extensions.Add($Matches.Extension)
        }
    }

    $files = [System.Collections.Generic.List[System.IO.FileInfo]]::new()
    foreach ($root in $ExistingRoots) {
        $absoluteRoot = Join-Path $RepoRoot $root
        foreach ($file in Get-ChildItem -LiteralPath $absoluteRoot -Recurse -File -ErrorAction SilentlyContinue) {
            if ($extensions.Contains($file.Extension)) {
                $files.Add($file)
            }
        }
    }

    $simpleTokens = @($Tokens | Where-Object { $_ -match '^[A-Za-z_][A-Za-z0-9_]*$' })
    $qualifiedTokens = @($Tokens | Where-Object { $_ -notmatch '^[A-Za-z_][A-Za-z0-9_]*$' })
    $regexes = @(
        if ($qualifiedTokens.Count -gt 0) { New-TokenRegexes -Tokens $qualifiedTokens }
        if ($simpleTokens.Count -gt 0) { New-TokenRegexes -Tokens $simpleTokens -WordMatch }
    )
    $uniqueTokenCount = @($Tokens | Sort-Object -Unique).Count
    $maxTokenLength = [Math]::Max(1, ($Tokens | ForEach-Object Length | Measure-Object -Maximum).Maximum)
    $encoding = if ($Binary) { [System.Text.Encoding]::Latin1 } else { [System.Text.UTF8Encoding]::new($false, $false) }
    $detectByteOrderMark = -not $Binary
    $contentKind = if ($Binary) { "package" } else { "text" }
    $fallbackRegexes = $regexes
    $fallbackEncoding = $encoding
    $fallbackDetectByteOrderMark = $detectByteOrderMark
    $fallbackUniqueTokenCount = $uniqueTokenCount
    $fallbackMaxTokenLength = $maxTokenLength
    $fallbackRepoRoot = $RepoRoot
    $throttleLimit = [Math]::Min(8, [Math]::Max(2, [Environment]::ProcessorCount))
    $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()

    Write-Warning "rg를 찾지 못해 PowerShell fallback으로 $contentKind 파일 $($files.Count)개를 병렬 검사합니다. 결과는 같지만 더 느릴 수 있습니다."
    $results = @($files | ForEach-Object -ThrottleLimit $throttleLimit -Parallel {
        $file = $_
        $localRegexes = $using:fallbackRegexes
        $localEncoding = $using:fallbackEncoding
        $localDetectByteOrderMark = $using:fallbackDetectByteOrderMark
        $localUniqueTokenCount = $using:fallbackUniqueTokenCount
        $localMaxTokenLength = $using:fallbackMaxTokenLength
        $localRepoRoot = $using:fallbackRepoRoot
        $buffer = [char[]]::new(1024 * 1024)
        $foundTokens = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)
        $reader = $null
        try {
            $reader = [System.IO.StreamReader]::new($file.FullName, $localEncoding, $localDetectByteOrderMark, $buffer.Length)
            $carry = ""
            while (($readCount = $reader.Read($buffer, 0, $buffer.Length)) -gt 0) {
                $chunk = $carry + [string]::new($buffer, 0, $readCount)
                foreach ($regex in $localRegexes) {
                    foreach ($match in $regex.Matches($chunk)) {
                        $null = $foundTokens.Add($match.Value)
                    }
                }
                if ($foundTokens.Count -ge $localUniqueTokenCount) {
                    break
                }
                $carryLength = [Math]::Min($localMaxTokenLength + 1, $chunk.Length)
                $carry = $chunk.Substring($chunk.Length - $carryLength, $carryLength)
            }
        }
        catch {
            Write-Warning "'$($file.FullName)' 검사 실패: $($_.Exception.Message)"
        }
        finally {
            if ($null -ne $reader) { $reader.Dispose() }
        }

        $relativePath = [System.IO.Path]::GetRelativePath($localRepoRoot, $file.FullName)
        foreach ($token in $foundTokens) {
            [pscustomobject]@{ Path = $relativePath; Token = $token }
        }
    })
    $stopwatch.Stop()
    Write-Host ("PowerShell fallback {0} scan completed in {1:N1}s." -f $contentKind, $stopwatch.Elapsed.TotalSeconds)
    return $results
}

function Resolve-RipgrepPath {
    if (-not [string]::IsNullOrWhiteSpace($script:ResolvedRipgrepPath)) {
        return $script:ResolvedRipgrepPath
    }

    $pathCommand = Get-Command rg -ErrorAction SilentlyContinue
    if ($null -ne $pathCommand) {
        $script:ResolvedRipgrepPath = $pathCommand.Source
        return $script:ResolvedRipgrepPath
    }

    $candidates = [System.Collections.Generic.List[string]]::new()
    $candidates.Add((Join-Path $PSScriptRoot "rg.exe"))
    if (-not [string]::IsNullOrWhiteSpace($env:LOCALAPPDATA)) {
        $candidates.Add((Join-Path $env:LOCALAPPDATA "Programs\Microsoft VS Code\resources\app\node_modules.asar.unpacked\@vscode\ripgrep\bin\rg.exe"))
        $codexBinRoot = Join-Path $env:LOCALAPPDATA "OpenAI\Codex\bin"
        if (Test-Path -LiteralPath $codexBinRoot) {
            foreach ($codexRipgrep in Get-ChildItem -LiteralPath $codexBinRoot -Filter "rg.exe" -File -Recurse -ErrorAction SilentlyContinue | Sort-Object LastWriteTime -Descending) {
                $candidates.Add($codexRipgrep.FullName)
            }
        }
    }

    foreach ($candidate in $candidates | Select-Object -Unique) {
        if (Test-Path -LiteralPath $candidate -PathType Leaf) {
            Write-Host "Using discovered ripgrep: $candidate"
            $script:ResolvedRipgrepPath = $candidate
            return $script:ResolvedRipgrepPath
        }
    }
    return $null
}

function Invoke-TokenScan {
    param(
        [Parameter(Mandatory)][string[]]$Tokens,
        [Parameter(Mandatory)][string[]]$Roots,
        [Parameter(Mandatory)][string[]]$Globs,
        [switch]$Binary
    )

    $existingRoots = @($Roots | Where-Object { Test-Path -LiteralPath (Join-Path $RepoRoot $_) })
    if ($Tokens.Count -eq 0 -or $existingRoots.Count -eq 0) {
        return @()
    }

    $rgPath = if ($ForcePowerShellScan) { $null } else { Resolve-RipgrepPath }
    if ([string]::IsNullOrWhiteSpace($rgPath)) {
        return @(Invoke-PowerShellTokenScan -Tokens $Tokens -ExistingRoots $existingRoots -Globs $Globs -Binary:$Binary)
    }

    $simpleTokens = @($Tokens | Where-Object { $_ -match '^[A-Za-z_][A-Za-z0-9_]*$' })
    $qualifiedTokens = @($Tokens | Where-Object { $_ -notmatch '^[A-Za-z_][A-Za-z0-9_]*$' })
    $output = [System.Collections.Generic.List[string]]::new()

    $scanGroups = @(
        [pscustomobject]@{ Name = "qualified"; Tokens = $qualifiedTokens; WordMatch = $false },
        [pscustomobject]@{ Name = "symbol"; Tokens = $simpleTokens; WordMatch = $true }
    )
    foreach ($scanGroup in $scanGroups) {
        if ($scanGroup.Tokens.Count -eq 0) {
            continue
        }

        $contentKind = if ($Binary) { "package" } else { "text" }
        $patternFile = Join-Path $RunDirectory "$contentKind-$($scanGroup.Name)-patterns.txt"
        [System.IO.File]::WriteAllLines($patternFile, $scanGroup.Tokens, [System.Text.UTF8Encoding]::new($false))

        $arguments = @("-o", "-F", "-f", $patternFile, "--no-heading", "--no-line-number", "--with-filename")
        if ($scanGroup.WordMatch) {
            $arguments += "-w"
        }
        if ($Binary) {
            $arguments += "-a"
        }
        foreach ($glob in $Globs) {
            $arguments += @("-g", $glob)
        }
        $arguments += "--"
        $arguments += $existingRoots

        Push-Location $RepoRoot
        try {
            $groupOutput = @(& $rgPath @arguments 2>$null)
            if ($LASTEXITCODE -notin @(0, 1)) {
                throw "rg scan failed with exit code $LASTEXITCODE."
            }
            $output.AddRange([string[]]$groupOutput)
        }
        finally {
            Pop-Location
        }
    }

    $matches = [System.Collections.Generic.List[object]]::new()
    foreach ($outputLine in $output) {
        $separator = $outputLine.LastIndexOf(':')
        if ($separator -le 0) {
            continue
        }
        $matches.Add([pscustomobject]@{
            Path = $outputLine.Substring(0, $separator)
            Token = $outputLine.Substring($separator + 1)
        })
    }
    return @($matches)
}

function Add-StaticScanResults {
    param([Parameter(Mandatory)][object[]]$Entries)

    $tokens = @(Get-SearchTokens -Entries $Entries)
    $textMatches = @(Invoke-TokenScan -Tokens $tokens -Roots @("Source", "Config", "Docs", "Plugins", "Tools") -Globs @("*.h", "*.cpp", "*.cs", "*.ini", "*.md", "*.ps1", "*.py", "*.json"))
    $packageMatches = @(Invoke-TokenScan -Tokens $tokens -Roots @("Content", "Plugins") -Globs @("*.uasset", "*.umap") -Binary)

    $configRelativePath = [System.IO.Path]::GetRelativePath($RepoRoot, $ConfigPath).Replace('/', '\')
    foreach ($entry in $Entries) {
        $entryTokens = @($entry.OldName, $entry.OldLeaf) | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Sort-Object -Unique
        $entry.StaticTextPaths = @(
            $textMatches |
                Where-Object { $_.Token -in $entryTokens -and $_.Path.Replace('/', '\') -ne $configRelativePath } |
                Select-Object -ExpandProperty Path -Unique |
                Sort-Object
        )
        $entry.StaticPackagePaths = @(
            $packageMatches |
                Where-Object { $_.Token -in $entryTokens } |
                Select-Object -ExpandProperty Path -Unique |
                Sort-Object
        )
    }
}

function Get-ProjectPackageRoots {
    $roots = [System.Collections.Generic.List[string]]::new()
    $roots.Add("/Game")
    $pluginsPath = Join-Path $RepoRoot "Plugins"
    if (Test-Path -LiteralPath $pluginsPath) {
        foreach ($pluginFile in Get-ChildItem -LiteralPath $pluginsPath -Recurse -Filter "*.uplugin") {
            try {
                $descriptor = Get-Content -Raw -LiteralPath $pluginFile.FullName | ConvertFrom-Json
                if ($descriptor.CanContainContent -eq $true) {
                    $roots.Add("/$($pluginFile.BaseName)")
                }
            }
            catch {
                Write-Warning "Could not parse plugin descriptor '$($pluginFile.FullName)': $($_.Exception.Message)"
            }
        }
    }
    return @($roots | Sort-Object -Unique)
}

function Get-UnrealEngineRoot {
    if (-not [string]::IsNullOrWhiteSpace($script:EngineRoot)) {
        $explicitRoot = [System.IO.Path]::GetFullPath($script:EngineRoot)
        if (-not (Test-Path -LiteralPath (Join-Path $explicitRoot "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"))) {
            throw "UnrealEditor-Cmd.exe was not found below -EngineRoot '$explicitRoot'."
        }
        $script:EngineRoot = $explicitRoot
        return $explicitRoot
    }

    $projectDescriptor = Get-Content -Raw -LiteralPath $ProjectPath | ConvertFrom-Json
    $engineAssociation = [string]$projectDescriptor.EngineAssociation
    if ([string]::IsNullOrWhiteSpace($engineAssociation)) {
        throw "'$ProjectPath' has no EngineAssociation. Supply -EngineRoot explicitly."
    }

    $candidates = [System.Collections.Generic.List[string]]::new()
    $customBuildsKey = "HKCU:\Software\Epic Games\Unreal Engine\Builds"
    if (Test-Path -LiteralPath $customBuildsKey) {
        $customBuilds = Get-ItemProperty -LiteralPath $customBuildsKey
        $customBuild = $customBuilds.PSObject.Properties[$engineAssociation]
        if ($null -ne $customBuild -and -not [string]::IsNullOrWhiteSpace([string]$customBuild.Value)) {
            $candidates.Add([string]$customBuild.Value)
        }
    }

    $installedBuildKey = "HKLM:\SOFTWARE\EpicGames\Unreal Engine\$engineAssociation"
    if (Test-Path -LiteralPath $installedBuildKey) {
        $installedBuild = Get-ItemProperty -LiteralPath $installedBuildKey
        if (-not [string]::IsNullOrWhiteSpace([string]$installedBuild.InstalledDirectory)) {
            $candidates.Add([string]$installedBuild.InstalledDirectory)
        }
    }

    if ($engineAssociation -match '^\d+(\.\d+)?$') {
        $programFiles = [Environment]::GetFolderPath([Environment+SpecialFolder]::ProgramFiles)
        $candidates.Add((Join-Path $programFiles "Epic Games\UE_$engineAssociation"))
    }

    foreach ($candidate in $candidates | Select-Object -Unique) {
        $candidateRoot = [System.IO.Path]::GetFullPath($candidate)
        if (Test-Path -LiteralPath (Join-Path $candidateRoot "Engine\Binaries\Win64\UnrealEditor-Cmd.exe")) {
            $script:EngineRoot = $candidateRoot
            return $candidateRoot
        }
    }

    throw "Could not resolve EngineAssociation '$engineAssociation'. Supply -EngineRoot explicitly."
}

function Get-ProjectEditorTarget {
    if (-not [string]::IsNullOrWhiteSpace($script:EditorTarget)) {
        return $script:EditorTarget
    }

    $sourceRoot = Join-Path $RepoRoot "Source"
    $targetFiles = @(Get-ChildItem -LiteralPath $sourceRoot -Filter "*Editor.Target.cs" -File -ErrorAction SilentlyContinue)
    if ($targetFiles.Count -ne 1) {
        throw "Expected exactly one Source/*Editor.Target.cs below '$RepoRoot', found $($targetFiles.Count). Supply -EditorTarget explicitly."
    }

    $script:EditorTarget = $targetFiles[0].Name -replace '\.Target\.cs$', ''
    return $script:EditorTarget
}

function Invoke-ProjectEditorBuild {
    $resolvedEngineRoot = Get-UnrealEngineRoot
    $resolvedEditorTarget = Get-ProjectEditorTarget
    $buildScript = Join-Path $resolvedEngineRoot "Engine\Build\BatchFiles\Build.bat"
    if (-not (Test-Path -LiteralPath $buildScript)) {
        throw "Unreal Build.bat was not found at '$buildScript'."
    }

    $buildLog = Join-Path $RunDirectory "editor-build.log"
    Write-Host "Building $resolvedEditorTarget..."
    & $buildScript $resolvedEditorTarget Win64 Development "-Project=$ProjectPath" -WaitMutex -FromMsBuild 2>&1 |
        Tee-Object -FilePath $buildLog
    if ($LASTEXITCODE -ne 0) {
        throw "$resolvedEditorTarget build failed. See '$buildLog'."
    }
}

function Invoke-EditorCommandlet {
    param(
        [Parameter(Mandatory)][string[]]$Arguments,
        [Parameter(Mandatory)][string]$LogName
    )

    $resolvedEngineRoot = Get-UnrealEngineRoot
    $editorCommand = Join-Path $resolvedEngineRoot "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
    if (-not (Test-Path -LiteralPath $editorCommand)) {
        throw "UnrealEditor-Cmd.exe was not found at '$editorCommand'."
    }

    $engineLog = Join-Path $RunDirectory $LogName
    $stdoutLog = Join-Path $RunDirectory ([System.IO.Path]::GetFileNameWithoutExtension($LogName) + "-stdout.log")
    $commonArguments = @(
        $ProjectPath,
        "-unattended",
        "-nop4",
        "-nosplash",
        "-nullrhi",
        "-stdout",
        "-FullStdOutLogOutput",
        "-abslog=$engineLog"
    )

    $processStartInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $processStartInfo.FileName = $editorCommand
    $processStartInfo.UseShellExecute = $false
    $processStartInfo.CreateNoWindow = $true
    $processStartInfo.RedirectStandardOutput = $true
    $processStartInfo.RedirectStandardError = $true
    foreach ($argument in ($commonArguments + $Arguments)) {
        $null = $processStartInfo.ArgumentList.Add($argument)
    }

    $process = [System.Diagnostics.Process]::new()
    $process.StartInfo = $processStartInfo
    $null = $process.Start()
    $stdoutTask = $process.StandardOutput.ReadToEndAsync()
    $stderrTask = $process.StandardError.ReadToEndAsync()
    $process.WaitForExit()
    $stdout = $stdoutTask.GetAwaiter().GetResult()
    $stderr = $stderrTask.GetAwaiter().GetResult()
    [System.IO.File]::WriteAllText($stdoutLog, $stdout + $stderr, [System.Text.UTF8Encoding]::new($false))
    Write-Host "Commandlet exit code $($process.ExitCode). Engine log: $engineLog"
    return [pscustomobject]@{
        ExitCode = $process.ExitCode
        EngineLog = $engineLog
        StdoutLog = $stdoutLog
    }
}

function Invoke-CoreRedirectEngineScan {
    param([Parameter(Mandatory)][object[]]$Entries)

    $packageRoots = (Get-ProjectPackageRoots) -join "+"
    $arguments = @(
        "-run=JWCU_CoreRedirectAudit",
        "-PackageRoots=$packageRoots",
        "-GarbageCollectionFrequency=100",
        "-DebugCoreRedirects"
    )
    if ($MaxPackages -gt 0) {
        $arguments += "-MaxPackages=$MaxPackages"
    }

    Write-Host "Loading project packages with CoreRedirect diagnostics..."
    $result = Invoke-EditorCommandlet -Arguments $arguments -LogName "core-redirect-engine-scan.log"
    $currentPackage = $null
    $applications = [System.Collections.Generic.List[object]]::new()
    $warnings = [System.Collections.Generic.List[string]]::new()
    $errors = [System.Collections.Generic.List[string]]::new()
    if (Test-Path -LiteralPath $result.EngineLog) {
        foreach ($logLine in Get-Content -LiteralPath $result.EngineLog) {
            if ($logLine -match 'Log[^:]+:\s+Warning:\s+(?<Message>.+)$') {
                $warningMessage = $Matches.Message.Trim()
                if (-not $warnings.Contains($warningMessage)) {
                    $warnings.Add($warningMessage)
                }
            }
            elseif ($logLine -match 'Log[^:]+:\s+Error:\s+(?<Message>.+)$') {
                $errorMessage = $Matches.Message.Trim()
                if (-not $errors.Contains($errorMessage)) {
                    $errors.Add($errorMessage)
                }
            }
            if ($logLine -match 'JWCU_CORE_REDIRECT_PACKAGE_BEGIN\|(?<Package>[^\r\n]+)$') {
                $currentPackage = $Matches.Package.Trim()
            }
            elseif ($logLine -match 'JWCU_CORE_REDIRECT_PACKAGE_END\|') {
                $currentPackage = $null
            }
            elseif ($logLine -match 'RedirectNameAndValues\((?<Old>.+?)\) replaced by (?<New>.+?)$') {
                $applications.Add([pscustomobject]@{
                    Package = if ($currentPackage) { $currentPackage } else { "<startup>" }
                    OldName = $Matches.Old.Trim()
                    NewName = $Matches.New.Trim()
                })
            }
        }
    }

    foreach ($entry in $Entries) {
        $entry.EnginePackages = @(
            $applications |
                Where-Object { $_.OldName -eq $entry.OldName -or $_.OldName.EndsWith(".$($entry.OldLeaf)") -or $_.OldName -eq $entry.OldLeaf } |
                Select-Object -ExpandProperty Package -Unique |
                Sort-Object
        )
    }

    return [pscustomobject]@{
        ExitCode = $result.ExitCode
        LogPath = $result.EngineLog
        IsPartial = $MaxPackages -gt 0
        Applications = @($applications)
        Warnings = @($warnings)
        Errors = @($errors)
    }
}

function Update-RedirectStatuses {
    param(
        [Parameter(Mandatory)][object[]]$Entries,
        [Parameter(Mandatory)][AllowEmptyCollection()][object[]]$Findings,
        [switch]$ScanCompleted,
        [switch]$EngineScanCompleted
    )

    foreach ($entry in $Entries) {
        $hasError = @($Findings | Where-Object { $_.Severity -eq "Error" -and $_.Message -match "Line $($entry.Line)\b" }).Count -gt 0
        if ($hasError) {
            $entry.Status = "Invalid"
        }
        elseif (-not $ScanCompleted) {
            $entry.Status = "Unscanned"
        }
        elseif ($entry.StaticPackagePaths.Count -gt 0 -or $entry.EnginePackages.Count -gt 0) {
            $entry.Status = "NeedsResave"
        }
        elseif ($entry.HasValueChanges -or $entry.Removed -or $entry.Type -in @("Enum", "Package")) {
            $entry.Status = "ManualReview"
        }
        elseif ($EngineScanCompleted) {
            $entry.Status = "RemovalCandidate"
        }
        else {
            $entry.Status = "StaticCandidate"
        }
    }
}

function Write-AuditReports {
    param(
        [Parameter(Mandatory)][object[]]$Entries,
        [Parameter(Mandatory)][AllowEmptyCollection()][object[]]$Findings,
        [object]$EngineScan
    )

    $jsonPath = Join-Path $RunDirectory "CoreRedirectAudit.json"
    $markdownPath = Join-Path $RunDirectory "CoreRedirectAudit.md"
    $report = [ordered]@{
        GeneratedAt = (Get-Date).ToString("o")
        Mode = $Mode
        ProjectPath = $ProjectPath
        ConfigPath = $ConfigPath
        Counts = [ordered]@{
            Total = $Entries.Count
            ByType = @($Entries | Group-Object Type | Sort-Object Name | ForEach-Object { [pscustomobject]@{ Type = $_.Name; Count = $_.Count } })
            ByStatus = @($Entries | Group-Object Status | Sort-Object Name | ForEach-Object { [pscustomobject]@{ Status = $_.Name; Count = $_.Count } })
        }
        Findings = $Findings
        EngineScan = $EngineScan
        Redirects = $Entries
    }
    $report | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $jsonPath -Encoding utf8NoBOM

    $builder = [System.Text.StringBuilder]::new()
    $null = $builder.AppendLine("# CoreRedirect Audit")
    $null = $builder.AppendLine()
    $null = $builder.AppendLine("- 생성 시각: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')")
    $null = $builder.AppendLine("- 모드: ``$Mode``")
    $null = $builder.AppendLine("- Config: ``$ConfigPath``")
    $null = $builder.AppendLine("- 총 CoreRedirect: $($Entries.Count)개")
    $null = $builder.AppendLine()
    $null = $builder.AppendLine("## 요약")
    $null = $builder.AppendLine()
    $null = $builder.AppendLine("| 상태 | 개수 |")
    $null = $builder.AppendLine("| --- | ---: |")
    foreach ($group in $Entries | Group-Object Status | Sort-Object Name) {
        $null = $builder.AppendLine("| $($group.Name) | $($group.Count) |")
    }
    $null = $builder.AppendLine()
    $null = $builder.AppendLine("## 구조 검사")
    $null = $builder.AppendLine()
    if ($Findings.Count -eq 0) {
        $null = $builder.AppendLine("- 중복, 충돌, 체인 또는 형식 오류를 찾지 못했습니다.")
    }
    else {
        foreach ($finding in $Findings) {
            $null = $builder.AppendLine("- **$($finding.Severity) / $($finding.Code)**: $($finding.Message)")
        }
    }
    if ($null -ne $EngineScan) {
        $null = $builder.AppendLine()
        $null = $builder.AppendLine("## 엔진 스캔")
        $null = $builder.AppendLine()
        $null = $builder.AppendLine("- 종료 코드: $($EngineScan.ExitCode)")
        $null = $builder.AppendLine("- 부분 스캔: $($EngineScan.IsPartial)")
        $null = $builder.AppendLine("- CoreRedirect 실제 적용: $($EngineScan.Applications.Count)건")
        $null = $builder.AppendLine("- 로드 경고/오류: $($EngineScan.Warnings.Count) / $($EngineScan.Errors.Count)")
        $null = $builder.AppendLine("- 로그: ``$($EngineScan.LogPath)``")
    }
    $null = $builder.AppendLine()
    $null = $builder.AppendLine("## 항목")
    $null = $builder.AppendLine()
    $null = $builder.AppendLine("| ID | Line | Type | Status | Static packages | Engine packages | OldName | NewName |")
    $null = $builder.AppendLine("| --- | ---: | --- | --- | ---: | ---: | --- | --- |")
    foreach ($entry in $Entries) {
        $safeOld = $entry.OldName.Replace('|', '\|')
        $safeNew = $entry.NewName.Replace('|', '\|')
        $null = $builder.AppendLine("| ``$($entry.Id)`` | $($entry.Line) | $($entry.Type) | $($entry.Status) | $($entry.StaticPackagePaths.Count) | $($entry.EnginePackages.Count) | ``$safeOld`` | ``$safeNew`` |")
    }
    [System.IO.File]::WriteAllText($markdownPath, $builder.ToString(), [System.Text.UTF8Encoding]::new($false))

    Write-Host "JSON report: $jsonPath"
    Write-Host "Markdown report: $markdownPath"
    return [pscustomobject]@{ Json = $jsonPath; Markdown = $markdownPath }
}

function Select-CoreRedirectEntries {
    param([Parameter(Mandatory)][object[]]$Entries)

    if ($RedirectId.Count -eq 0 -and $Type.Count -eq 0 -and [string]::IsNullOrWhiteSpace($Match)) {
        throw "$Mode mode requires at least one selector: -RedirectId, -Type, or -Match."
    }

    return @($Entries | Where-Object {
        $idMatches = $RedirectId.Count -gt 0 -and $_.Id -in $RedirectId
        $typeMatches = $Type.Count -gt 0 -and $_.Type -in $Type
        $regexMatches = -not [string]::IsNullOrWhiteSpace($Match) -and ($_.OldName -match $Match -or $_.NewName -match $Match)
        $idMatches -or $typeMatches -or $regexMatches
    })
}

function Resolve-LongPackagePath {
    param([Parameter(Mandatory)][string]$LongPackageName)

    $mounts = @{
        "/Game" = Join-Path $RepoRoot "Content"
    }
    foreach ($pluginFile in Get-ChildItem -Path (Join-Path $RepoRoot "Plugins") -Recurse -Filter "*.uplugin" -ErrorAction SilentlyContinue) {
        $mounts["/$($pluginFile.BaseName)"] = Join-Path $pluginFile.Directory.FullName "Content"
    }

    foreach ($mount in $mounts.GetEnumerator()) {
        if ($LongPackageName -eq $mount.Key -or $LongPackageName.StartsWith("$($mount.Key)/")) {
            $suffix = $LongPackageName.Substring($mount.Key.Length).TrimStart('/').Replace('/', [System.IO.Path]::DirectorySeparatorChar)
            $basePath = Join-Path $mount.Value $suffix
            foreach ($extension in @(".uasset", ".umap")) {
                $candidate = "$basePath$extension"
                if (Test-Path -LiteralPath $candidate) {
                    return $candidate
                }
            }
        }
    }
    return $null
}

function Convert-ToLongPackageName {
    param([Parameter(Mandatory)][string]$Path)

    if ($Path.StartsWith('/')) {
        return ($Path -replace '\.(?:uasset|umap)$', '')
    }

    $fullPath = if ([System.IO.Path]::IsPathRooted($Path)) {
        [System.IO.Path]::GetFullPath($Path)
    }
    else {
        [System.IO.Path]::GetFullPath((Join-Path $RepoRoot $Path))
    }

    $mounts = @{
        "/Game" = Join-Path $RepoRoot "Content"
    }
    foreach ($pluginFile in Get-ChildItem -Path (Join-Path $RepoRoot "Plugins") -Recurse -Filter "*.uplugin" -ErrorAction SilentlyContinue) {
        $mounts["/$($pluginFile.BaseName)"] = Join-Path $pluginFile.Directory.FullName "Content"
    }

    foreach ($mount in $mounts.GetEnumerator()) {
        $contentRoot = [System.IO.Path]::GetFullPath($mount.Value).TrimEnd([System.IO.Path]::DirectorySeparatorChar) + [System.IO.Path]::DirectorySeparatorChar
        if ($fullPath.StartsWith($contentRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
            $relativePath = [System.IO.Path]::GetRelativePath($contentRoot, $fullPath)
            $withoutExtension = ($relativePath -replace '\.(?:uasset|umap)$', '').Replace([System.IO.Path]::DirectorySeparatorChar, '/')
            return "$($mount.Key)/$withoutExtension"
        }
    }
    return $null
}

function Get-OwningMapPackageName {
    param([Parameter(Mandatory)][string]$LongPackageName)

    if ($LongPackageName -match '^/Game/__(?:ExternalActors|ExternalObjects)__/(?<MapPackage>.+)/[A-Z0-9]{1,2}/[A-Z0-9]{1,2}/[A-Z0-9]+$') {
        return "/Game/$($Matches.MapPackage)"
    }
    return $null
}

function Assert-EditorClosedForMutation {
    if (Get-Process -Name "UnrealEditor", "UnrealEditor-Cmd" -ErrorAction SilentlyContinue) {
        throw "UnrealEditor is running. Close the editor before Resave or Remove."
    }
}

function Write-ManualResavePlan {
    param(
        [Parameter(Mandatory)][object[]]$SelectedEntries,
        [Parameter(Mandatory)][AllowEmptyCollection()][string[]]$AdditionalPackages
    )

    $packageNames = [System.Collections.Generic.List[string]]::new()
    foreach ($entry in $SelectedEntries) {
        foreach ($path in $entry.StaticPackagePaths) {
            $longPackageName = Convert-ToLongPackageName -Path $path
            if ($longPackageName) { $packageNames.Add($longPackageName) }
        }
        foreach ($longPackageName in $entry.EnginePackages) {
            if ($longPackageName -ne "<startup>") {
                $packageNames.Add($longPackageName)
            }
        }
    }
    foreach ($path in $AdditionalPackages) {
        $longPackageName = Convert-ToLongPackageName -Path $path
        if ($longPackageName -and $longPackageName -ne "<startup>") { $packageNames.Add($longPackageName) }
    }

    $uniquePackageNames = @($packageNames | Sort-Object -Unique)
    if ($uniquePackageNames.Count -eq 0) {
        throw "No packages were found for the selected redirects. Run a full Scan first."
    }

    $owningMaps = @(
        $uniquePackageNames |
            ForEach-Object { Get-OwningMapPackageName -LongPackageName $_ } |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) } |
            Sort-Object -Unique
    )
    $regularPackages = @($uniquePackageNames | Where-Object { [string]::IsNullOrWhiteSpace((Get-OwningMapPackageName -LongPackageName $_)) })

    $planPath = Join-Path $RunDirectory "ManualResavePlan.md"
    $packageListPath = Join-Path $RunDirectory "ManualResavePackages.txt"
    $builder = [System.Text.StringBuilder]::new()
    $null = $builder.AppendLine("# Manual CoreRedirect Resave Plan")
    $null = $builder.AppendLine()
    $null = $builder.AppendLine("- 생성 시각: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')")
    $null = $builder.AppendLine("- 대상 Redirect: $($SelectedEntries.Count)개")
    $null = $builder.AppendLine("- 영향 패키지: $($uniquePackageNames.Count)개")
    $null = $builder.AppendLine()
    $null = $builder.AppendLine("## 작업 순서")
    $null = $builder.AppendLine()
    $null = $builder.AppendLine("1. ``[CoreRedirects]``를 유지한 상태로 Unreal Editor를 연다.")
    $null = $builder.AppendLine("2. 아래 일반 패키지를 Content Browser에서 찾아 열고 해당 에셋만 저장한다.")
    $null = $builder.AppendLine("3. External Actor/Object 항목은 파일을 직접 저장하지 않고 아래 owning map을 열어 현재 레벨을 저장한다.")
    $null = $builder.AppendLine("4. Git 변경 목록에서 의도한 에셋만 바뀌었는지 확인하고 별도 커밋한다.")
    $null = $builder.AppendLine("5. 에디터를 닫은 뒤 CLI 메뉴 5번 전체 Scan을 실행한다.")
    $null = $builder.AppendLine()
    $null = $builder.AppendLine("## 대상 Redirect")
    $null = $builder.AppendLine()
    foreach ($entry in $SelectedEntries) {
        $null = $builder.AppendLine("- ``$($entry.Id)`` $($entry.Type): ``$($entry.OldName)`` → ``$($entry.NewName)``")
        $entryPackageNames = @(
            @($entry.StaticPackagePaths) + @($entry.EnginePackages) |
                Where-Object { -not [string]::IsNullOrWhiteSpace($_) -and $_ -ne "<startup>" } |
                ForEach-Object { Convert-ToLongPackageName -Path $_ } |
                Where-Object { -not [string]::IsNullOrWhiteSpace($_) } |
                Sort-Object -Unique
        )
        foreach ($entryPackageName in $entryPackageNames) {
            $owningMap = Get-OwningMapPackageName -LongPackageName $entryPackageName
            $displayPackage = if ($owningMap) { "$entryPackageName (owning map: $owningMap)" } else { $entryPackageName }
            $null = $builder.AppendLine("  - ``$displayPackage``")
        }
    }
    $null = $builder.AppendLine()
    $null = $builder.AppendLine("## 일반 패키지")
    $null = $builder.AppendLine()
    foreach ($packageName in $regularPackages) {
        $resolvedPath = Resolve-LongPackagePath -LongPackageName $packageName
        $relativePath = if ($resolvedPath) { [System.IO.Path]::GetRelativePath($RepoRoot, $resolvedPath) } else { "경로 확인 필요" }
        $null = $builder.AppendLine("- ``$packageName`` — ``$relativePath``")
    }
    $null = $builder.AppendLine()
    $null = $builder.AppendLine("## External Actor/Object owning map")
    $null = $builder.AppendLine()
    if ($owningMaps.Count -eq 0) {
        $null = $builder.AppendLine("- 없음")
    }
    else {
        foreach ($owningMap in $owningMaps) { $null = $builder.AppendLine("- ``$owningMap``") }
    }

    [System.IO.File]::WriteAllText($planPath, $builder.ToString(), [System.Text.UTF8Encoding]::new($false))
    [System.IO.File]::WriteAllLines($packageListPath, $uniquePackageNames, [System.Text.UTF8Encoding]::new($false))
    Write-Host "Manual resave plan: $planPath"
    Write-Host "Package list: $packageListPath"
    if ($Apply) {
        Write-Warning "Automatic ResavePackages execution is disabled. -Apply was ignored; use the manual plan in Unreal Editor."
    }
}

function Invoke-BlueprintValidation {
    return Invoke-EditorCommandlet -Arguments @("-run=CompileAllBlueprints") -LogName "compile-all-blueprints.log"
}

$entries = @(Get-CoreRedirectEntries -Path $ConfigPath)
Add-GitBlameMetadata -Entries $entries -Path $ConfigPath
$findings = @(Get-StructuralFindings -Entries $entries)
$scanCompleted = $false
$engineScanCompleted = $false
$engineScan = $null

if ($Mode -in @("Scan", "Resave", "Remove")) {
    Write-Host "Scanning text and package files for old reflected names..."
    Add-StaticScanResults -Entries $entries
    $scanCompleted = $true

    if (-not $SkipEngineScan) {
        if (-not $SkipBuild) {
            Invoke-ProjectEditorBuild
        }
        $engineScan = Invoke-CoreRedirectEngineScan -Entries $entries
        $engineScanCompleted = $engineScan.ExitCode -eq 0 -and -not $engineScan.IsPartial
        if ($engineScan.IsPartial) {
            Write-Warning "-MaxPackages produced a partial engine scan. It is valid for smoke testing but cannot qualify removal candidates."
        }
        if ($engineScan.ExitCode -ne 0) {
            Write-Warning "The engine scan reported failures. Removal candidates will not be trusted."
        }
    }
}

Update-RedirectStatuses -Entries $entries -Findings $findings -ScanCompleted:$scanCompleted -EngineScanCompleted:$engineScanCompleted
$reports = Write-AuditReports -Entries $entries -Findings $findings -EngineScan $engineScan

if ($Mode -eq "Audit") {
    $errorCount = @($findings | Where-Object Severity -eq "Error").Count
    Write-Host "Audit completed: $($entries.Count) redirects, $errorCount structural error(s)."
    if ($errorCount -gt 0) { exit 1 }
    exit 0
}

if ($Mode -eq "Scan") {
    $needsResave = @($entries | Where-Object Status -eq "NeedsResave").Count
    $staticCandidates = @($entries | Where-Object Status -eq "StaticCandidate").Count
    $removalCandidates = @($entries | Where-Object Status -eq "RemovalCandidate").Count
    Write-Host "Scan completed: $needsResave need resave; $staticCandidates static candidate(s); $removalCandidates engine-qualified removal candidate(s)."
    if ($null -ne $engineScan -and $engineScan.ExitCode -ne 0) { exit $engineScan.ExitCode }
    exit 0
}

$selectedEntries = @(Select-CoreRedirectEntries -Entries $entries)
if ($selectedEntries.Count -eq 0) {
    throw "No redirects matched the supplied selectors."
}
Write-Host "Target redirects: $($selectedEntries.Count)"
if (-not $QuietSelection) {
    foreach ($entry in $selectedEntries) {
        Write-Host "  [$($entry.Id)] $($entry.Type) line $($entry.Line): $($entry.OldName) -> $($entry.NewName) [$($entry.Status)]"
    }
}

if ($Mode -eq "Resave") {
    Write-ManualResavePlan -SelectedEntries $selectedEntries -AdditionalPackages $PackagePath
    exit 0
}

if ($Mode -eq "Remove") {
    $referencedEntries = @($selectedEntries | Where-Object { $_.StaticPackagePaths.Count -gt 0 -or $_.EnginePackages.Count -gt 0 })
    if ($referencedEntries.Count -gt 0 -and -not $Force) {
        throw "$($referencedEntries.Count) selected redirect(s) still have package references. Resave them first or use -Force only after manual verification."
    }
    if ((-not $engineScanCompleted -or $SkipEngineScan) -and -not $Force) {
        throw "A successful engine scan is required before removal. Re-run without -SkipEngineScan, or use -Force after manual verification."
    }

    Write-Host "Config lines proposed for removal:"
    foreach ($entry in $selectedEntries | Sort-Object Line) {
        Write-Host "  L$($entry.Line): $($entry.Raw)"
    }
    if (-not $Apply) {
        Write-Warning "Preview only. Re-run with -Apply to remove and validate these redirects."
        exit 0
    }

    Assert-EditorClosedForMutation
    $configRelativePath = [System.IO.Path]::GetRelativePath($RepoRoot, $ConfigPath)
    $existingConfigDiff = @(& git -C $RepoRoot diff -- $configRelativePath 2>$null)
    $existingStagedConfigDiff = @(& git -C $RepoRoot diff --cached -- $configRelativePath 2>$null)
    if (($existingConfigDiff.Count -gt 0 -or $existingStagedConfigDiff.Count -gt 0) -and -not $Force) {
        throw "'$configRelativePath' already has uncommitted changes. Commit/stash them first or use -Force after review."
    }

    $backupPath = Join-Path $RunDirectory "DefaultEngine.ini.before-remove"
    [System.IO.File]::Copy($ConfigPath, $backupPath, $true)
    $allConfigLines = [System.IO.File]::ReadAllLines($ConfigPath)
    $removeLineNumbers = [System.Collections.Generic.HashSet[int]]::new()
    foreach ($entry in $selectedEntries) { $null = $removeLineNumbers.Add($entry.Line) }
    $remainingLines = for ($index = 0; $index -lt $allConfigLines.Count; $index++) {
        if (-not $removeLineNumbers.Contains($index + 1)) { $allConfigLines[$index] }
    }
    [System.IO.File]::WriteAllLines($ConfigPath, [string[]]$remainingLines, [System.Text.UTF8Encoding]::new($false))

    try {
        $postRemovalEntries = @(Get-CoreRedirectEntries -Path $ConfigPath)
        $postRemovalScan = Invoke-CoreRedirectEngineScan -Entries $postRemovalEntries
        if ($postRemovalScan.ExitCode -ne 0) {
            throw "Post-removal package load validation failed with exit code $($postRemovalScan.ExitCode)."
        }
        $blueprintValidation = Invoke-BlueprintValidation
        if ($blueprintValidation.ExitCode -ne 0) {
            throw "Post-removal Blueprint validation failed with exit code $($blueprintValidation.ExitCode)."
        }
    }
    catch {
        [System.IO.File]::Copy($backupPath, $ConfigPath, $true)
        throw "Removal validation failed and DefaultEngine.ini was restored. $($_.Exception.Message)"
    }

    Write-Host "Removed $($selectedEntries.Count) redirect(s) and passed package-load/Blueprint validation."
    Write-Host "Review and commit the Config diff separately from any earlier asset resave."
}
