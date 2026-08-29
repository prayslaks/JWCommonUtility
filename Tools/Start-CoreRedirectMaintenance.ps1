# Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

[CmdletBinding()]
param(
    [string]$ProjectPath,
    [string]$EngineRoot,
    [string]$EditorTarget
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
            throw "'$($directory.FullName)'에 .uproject가 여러 개 있습니다. -ProjectPath를 지정하세요."
        }
        $directory = $directory.Parent
    }
    throw "'$StartPath' 상위에서 .uproject를 찾지 못했습니다. -ProjectPath를 지정하세요."
}

$ManagerPath = Join-Path $PSScriptRoot "Manage-CoreRedirects.ps1"
if (-not (Test-Path -LiteralPath $ManagerPath -PathType Leaf)) {
    throw "작업 스크립트를 찾지 못했습니다: $ManagerPath"
}

$pwshCommand = Get-Command pwsh -ErrorAction SilentlyContinue
if ($null -eq $pwshCommand) {
    throw "PowerShell 7(pwsh)을 찾지 못했습니다. 설치 후 터미널을 다시 여세요."
}

$pluginRoot = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($ProjectPath)) {
    $ProjectPath = Find-ContainingUnrealProject -StartPath $pluginRoot
}
$ProjectPath = [System.IO.Path]::GetFullPath($ProjectPath)
$projectRoot = Split-Path -Parent $ProjectPath
$reportRoot = Join-Path $projectRoot "Saved\CoreRedirectAudit"

$commonManagerArguments = @("-ProjectPath", $ProjectPath)
if (-not [string]::IsNullOrWhiteSpace($EngineRoot)) {
    $commonManagerArguments += @("-EngineRoot", $EngineRoot)
}
if (-not [string]::IsNullOrWhiteSpace($EditorTarget)) {
    $commonManagerArguments += @("-EditorTarget", $EditorTarget)
}

$script:LastManagerExitCode = 0

function Invoke-Manager {
    param(
        [Parameter(Mandatory)][string[]]$Arguments,
        [string]$DisplayCommand
    )

    Write-Host ""
    $displayText = if ([string]::IsNullOrWhiteSpace($DisplayCommand)) {
        "pwsh Manage-CoreRedirects.ps1 $($Arguments -join ' ')"
    }
    else {
        $DisplayCommand
    }
    Write-Host "실행: $displayText" -ForegroundColor DarkGray
    & $pwshCommand.Source -NoProfile -ExecutionPolicy Bypass -File $ManagerPath @commonManagerArguments @Arguments
    $script:LastManagerExitCode = $LASTEXITCODE
    if ($script:LastManagerExitCode -ne 0) {
        Write-Warning "작업이 종료 코드 $script:LastManagerExitCode 로 끝났습니다. 위 오류와 최신 로그를 확인하세요."
    }
}

function Read-ReportFile {
    param([Parameter(Mandatory)][System.IO.FileInfo]$File)

    try {
        $json = [System.IO.File]::ReadAllText($File.FullName, [System.Text.UTF8Encoding]::new($false))
        return $json | ConvertFrom-Json
    }
    catch {
        Write-Warning "보고서를 읽지 못했습니다: $($File.FullName)"
        return $null
    }
}

function Get-LatestFullScanReport {
    if (-not (Test-Path -LiteralPath $reportRoot)) {
        return $null
    }

    $reportFiles = @(
        Get-ChildItem -LiteralPath $reportRoot -Filter "CoreRedirectAudit.json" -File -Recurse |
            Sort-Object LastWriteTime -Descending
    )
    foreach ($reportFile in $reportFiles) {
        $report = Read-ReportFile -File $reportFile
        if ($null -eq $report -or $report.Mode -ne "Scan" -or $null -eq $report.EngineScan) {
            continue
        }
        $exitCodeProperty = $report.EngineScan.PSObject.Properties["ExitCode"]
        $isPartialProperty = $report.EngineScan.PSObject.Properties["IsPartial"]
        if ($null -eq $exitCodeProperty -or $null -eq $isPartialProperty) {
            continue
        }
        if ($exitCodeProperty.Value -eq 0 -and -not $isPartialProperty.Value) {
            return [pscustomobject]@{ File = $reportFile; Data = $report }
        }
    }
    return $null
}

function Show-ReportSummary {
    $scanReport = Get-LatestFullScanReport
    if ($null -eq $scanReport) {
        Write-Warning "성공한 전체 Scan 보고서가 없습니다. 먼저 메뉴 2 또는 5를 실행하세요."
        return
    }

    Write-Host ""
    Write-Host "최근 전체 Scan: $($scanReport.File.FullName)" -ForegroundColor Cyan
    foreach ($statusCount in $scanReport.Data.Counts.ByStatus) {
        Write-Host ("  {0,-18} {1,4}" -f $statusCount.Status, $statusCount.Count)
    }
    if ($scanReport.Data.EngineScan.Warnings.Count -gt 0 -or $scanReport.Data.EngineScan.Errors.Count -gt 0) {
        Write-Host ("  Engine warning/error: {0}/{1}" -f $scanReport.Data.EngineScan.Warnings.Count, $scanReport.Data.EngineScan.Errors.Count) -ForegroundColor Yellow
    }
}

function Select-RedirectsFromLatestScan {
    param([Parameter(Mandatory)][string]$Status)

    $scanReport = Get-LatestFullScanReport
    if ($null -eq $scanReport) {
        Write-Warning "선택 근거가 될 성공한 전체 Scan이 없습니다. 먼저 메뉴 2 또는 5를 실행하세요."
        return @()
    }

    $candidates = @($scanReport.Data.Redirects | Where-Object Status -eq $Status)
    if ($candidates.Count -eq 0) {
        Write-Host "최근 전체 Scan에 $Status 항목이 없습니다." -ForegroundColor Green
        return @()
    }

    Write-Host ""
    Write-Host "$Status 후보 — 최근 보고서: $($scanReport.File.Directory.Name)" -ForegroundColor Cyan
    for ($index = 0; $index -lt $candidates.Count; ++$index) {
        $candidate = $candidates[$index]
        $packageCount = @($candidate.StaticPackagePaths).Count + @($candidate.EnginePackages).Count
        Write-Host ("[{0,2}] {1}  {2,-8} packages:{3,3}  {4} -> {5}" -f ($index + 1), $candidate.Id, $candidate.Type, $packageCount, $candidate.OldName, $candidate.NewName)
    }

    Write-Host ""
    $selection = Read-Host "작업할 번호를 쉼표로 입력하세요(예: 1,3). 취소는 Enter"
    if ([string]::IsNullOrWhiteSpace($selection)) {
        return @()
    }

    $selected = [System.Collections.Generic.List[object]]::new()
    foreach ($token in ($selection -split ',')) {
        $trimmed = $token.Trim()
        if ($trimmed -notmatch '^\d+$') {
            Write-Warning "올바르지 않은 번호입니다: $trimmed"
            return @()
        }
        $selectedIndex = [int]$trimmed - 1
        if ($selectedIndex -lt 0 -or $selectedIndex -ge $candidates.Count) {
            Write-Warning "범위를 벗어난 번호입니다: $trimmed"
            return @()
        }
        if (-not $selected.Contains($candidates[$selectedIndex])) {
            $selected.Add($candidates[$selectedIndex])
        }
    }
    return @($selected)
}

function Show-SelectedPackages {
    param([Parameter(Mandatory)][object[]]$Entries)

    $packages = @(
        foreach ($entry in $Entries) {
            $entry.StaticPackagePaths
            $entry.EnginePackages
        }
    ) | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Sort-Object -Unique
    $packages = @($packages)

    Write-Host ""
    Write-Host "영향 패키지:" -ForegroundColor Cyan
    if ($packages.Count -eq 0) {
        Write-Host "  보고서에 기록된 패키지가 없습니다. 실행 시 재스캔 결과를 사용합니다."
    }
    else {
        foreach ($package in $packages) {
            Write-Host "  $package"
        }
    }
}

function Invoke-NeedsResaveStep {
    $scanReport = Get-LatestFullScanReport
    if ($null -eq $scanReport) {
        Write-Warning "작업 목록의 근거가 될 성공한 전체 Scan이 없습니다. 먼저 메뉴 2 또는 5를 실행하세요."
        return
    }

    $selected = @($scanReport.Data.Redirects | Where-Object Status -eq "NeedsResave")
    if ($selected.Count -eq 0) {
        Write-Host "최근 전체 Scan에 NeedsResave 항목이 없습니다." -ForegroundColor Green
        return
    }

    Write-Host ""
    Write-Host "최근 전체 Scan의 NeedsResave $($selected.Count)개를 모두 수동 작업 목록으로 만듭니다." -ForegroundColor Cyan
    Write-Host "자동 저장은 실행하지 않습니다." -ForegroundColor Yellow
    $manualPackages = @(
        foreach ($entry in $selected) {
            $entry.StaticPackagePaths
            $entry.EnginePackages
        }
    ) | Where-Object { -not [string]::IsNullOrWhiteSpace($_) -and $_ -ne "<startup>" } | Sort-Object -Unique
    $manualPackages = @($manualPackages)
    $arguments = @("-Mode", "Resave", "-RedirectId", (($selected.Id) -join ','), "-SkipBuild", "-SkipEngineScan", "-QuietSelection")
    if ($manualPackages.Count -gt 0) {
        $arguments += @("-PackagePath", ($manualPackages -join ','))
    }
    Invoke-Manager -Arguments $arguments -DisplayCommand "NeedsResave $($selected.Count)개 수동 계획 생성"
}

function Invoke-RemovalStep {
    $selected = @(Select-RedirectsFromLatestScan -Status "RemovalCandidate")
    if ($selected.Count -eq 0) {
        return
    }

    Write-Host ""
    Write-Host "제거 대상:" -ForegroundColor Yellow
    foreach ($entry in $selected) {
        Write-Host "  [$($entry.Id)] $($entry.Type): $($entry.OldName) -> $($entry.NewName)"
    }
    Write-Host "전체 재스캔, Config 백업, 패키지 로드, Blueprint 컴파일 검증을 다시 수행합니다."
    $confirmation = Read-Host "실제 제거하려면 REMOVE를 입력하세요"
    if ($confirmation -cne "REMOVE") {
        Write-Host "취소했습니다."
        return
    }

    Invoke-Manager -Arguments @("-Mode", "Remove", "-RedirectId", (($selected.Id) -join ','), "-SkipBuild", "-Apply")
}

function Show-AssetCommitCheckpoint {
    Write-Host ""
    Write-Host "에셋 커밋 체크포인트" -ForegroundColor Cyan
    Write-Host "재저장된 에셋만 검토·커밋한 뒤 메뉴 5의 전체 재스캔으로 진행하세요."
    & git -C $projectRoot status --short -- Content Plugins
    Write-Host ""
    Write-Host "예시: git add <검토한 에셋>; git commit" -ForegroundColor DarkGray
    Write-Host "메뉴는 의도하지 않은 파일을 자동 커밋하지 않습니다."
}

function Show-ConfigCommitCheckpoint {
    Write-Host ""
    Write-Host "Config 커밋 체크포인트" -ForegroundColor Cyan
    & git -C $projectRoot diff -- Config/DefaultEngine.ini
    & git -C $projectRoot status --short -- Config/DefaultEngine.ini
    Write-Host ""
    Write-Host "예시: git add Config/DefaultEngine.ini; git commit" -ForegroundColor DarkGray
    Write-Host "에셋 재저장 커밋과 Redirect 제거 커밋을 분리하세요."
}

:MenuLoop while ($true) {
    Write-Host ""
    Write-Host "============================================================" -ForegroundColor DarkCyan
    Write-Host " CoreRedirect Maintenance — 권장 작업 순서" -ForegroundColor Cyan
    Write-Host " Project: $ProjectPath" -ForegroundColor DarkGray
    Write-Host "============================================================" -ForegroundColor DarkCyan
    Write-Host " [1] Audit                       구조 검사"
    Write-Host " [2] 전체 Scan                   최초 사용처 조사"
    Write-Host " [3] NeedsResave 전체 목록 생성  선택 없이 계획서 생성"
    Write-Host " [4] 에셋 커밋 체크포인트        상태 확인(자동 커밋 안 함)"
    Write-Host " [5] 전체 Scan                   재저장 후 재검증"
    Write-Host " [6] RemovalCandidate 선택 제거  REMOVE + 사후 검증"
    Write-Host " [7] Config 커밋 체크포인트      diff 확인(자동 커밋 안 함)"
    Write-Host " [8] 최근 전체 Scan 요약"
    Write-Host " [0] 종료"
    Write-Host ""

    $choice = Read-Host "선택"
    switch ($choice) {
        "1" { Invoke-Manager -Arguments @("-Mode", "Audit") }
        "2" { Invoke-Manager -Arguments @("-Mode", "Scan") }
        "3" { Invoke-NeedsResaveStep }
        "4" { Show-AssetCommitCheckpoint }
        "5" { Invoke-Manager -Arguments @("-Mode", "Scan") }
        "6" { Invoke-RemovalStep }
        "7" { Show-ConfigCommitCheckpoint }
        "8" { Show-ReportSummary }
        "0" { break MenuLoop }
        default { Write-Warning "0~8 중에서 선택하세요." }
    }
}
