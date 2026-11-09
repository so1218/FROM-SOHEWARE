<#
エンジン (Engine/) を指定した版に更新する。

使い方 (リポジトリのどこからでもよい):
    .\Engine\UpdateEngine.ps1 v1.1.0

使える版は、引数なしで実行すると一覧が出る。
#>
param(
    [string]$Version
)

$ErrorActionPreference = "Stop"

# 途中で移動するので、終わったら (止まったときも) 実行した場所に戻す
$originalLocation = Get-Location
function Exit-Script([int]$exitCode)
{
    Set-Location $originalLocation
    exit $exitCode
}
trap
{
    Set-Location $originalLocation
    break
}

# エンジンのリポジトリ
$engineRepositoryUrl = "https://github.com/so1218/FROM-SOHEWARE-Engine.git"

function Stop-Update([string]$message)
{
    Write-Host $message -ForegroundColor Red
    Exit-Script 1
}

function Invoke-Git
{
    $output = & git @args
    if ($LASTEXITCODE -ne 0)
    {
        Stop-Update "git $($args -join ' ') が失敗した"
    }
    return $output
}

function Get-CurrentVersion([string]$changelogPath)
{
    $heading = Get-Content $changelogPath -Encoding UTF8 | Where-Object { $_ -match '^## v\d+\.\d+\.\d+' } | Select-Object -First 1
    if ($heading) { return ($heading -replace '^## (v\d+\.\d+\.\d+).*', '$1') }
    return "不明"
}

if ($engineRepositoryUrl.StartsWith("<"))
{
    Stop-Update "UpdateEngine.ps1 にエンジンのリポジトリの URL が書かれていない。エンジンの担当者に連絡する"
}

# git が出す絶対パスは日本語のフォルダ名が文字化けするので、相対パスだけを使う
Set-Location $PSScriptRoot
$enginePrefix = (Invoke-Git rev-parse --show-prefix).TrimEnd('/')
if (-not $enginePrefix)
{
    Stop-Update "このスクリプトはゲームのリポジトリの中の Engine フォルダに置いて実行する"
}
Set-Location (Invoke-Git rev-parse --show-cdup)

$tags = Invoke-Git ls-remote --tags --refs $engineRepositoryUrl |
    ForEach-Object { ($_ -split "refs/tags/")[1] } |
    Where-Object { $_ -match '^v\d+\.\d+\.\d+$' } |
    Sort-Object { [version]($_.Substring(1)) }

$currentVersion = Get-CurrentVersion "$enginePrefix/CHANGELOG.md"

if (-not $Version)
{
    Write-Host "今の版: $currentVersion"
    Write-Host "使える版: $($tags -join ', ')"
    Write-Host "更新するときは .\$enginePrefix\UpdateEngine.ps1 <版> で実行する"
    Exit-Script 0
}

if ($tags -notcontains $Version)
{
    Stop-Update "$Version という版はない。使える版: $($tags -join ', ')"
}

# 取り込みはマージなので、Git が管理しているファイルの変更があると混ざって戻せなくなる
# まだ登録していない新しいファイルは取り込みに影響しないので見ない
$uncommitted = Invoke-Git status --porcelain --untracked-files=no
if ($uncommitted)
{
    Write-Host ($uncommitted -join "`n")
    Stop-Update "コミットしていない変更がある。コミットしてからやり直す"
}

Write-Host "$currentVersion から $Version に切り替える..."
$headBeforeUpdate = Invoke-Git rev-parse HEAD

& git subtree pull --prefix=$enginePrefix $engineRepositoryUrl $Version --squash -m "Update engine to $Version"
if ($LASTEXITCODE -ne 0)
{
    Write-Host "更新に失敗した。$enginePrefix の中を直接直したところがぶつかっている可能性が高い" -ForegroundColor Red
    Write-Host "元に戻すには: git merge --abort"
    Write-Host "戻したら、エンジンの担当者に相談する"
    Exit-Script 1
}

# 最後に取り込んだ版と同じ版を指定すると、git は何もせずに成功する
if ((Invoke-Git rev-parse HEAD) -eq $headBeforeUpdate)
{
    Write-Host "$Version はすでに取り込んであるので、何も変わらなかった" -ForegroundColor Yellow
    Exit-Script 0
}

Write-Host "$Version に切り替えた" -ForegroundColor Green
Write-Host "1. $enginePrefix/CHANGELOG.md の $Version の「対応が必要」を読んで、ゲームのコードを直す"
Write-Host "2. Visual Studio でリビルドして、起動を確かめる"
Write-Host "3. git push する。うまく動かないときは .\$enginePrefix\UpdateEngine.ps1 $currentVersion で前の版に戻せる"
Set-Location $originalLocation
