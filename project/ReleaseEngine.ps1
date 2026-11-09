<#
エンジン (project/Engine) の新しい版を、エンジンのリポジトリに出す。

使い方:
    .\ReleaseEngine.ps1 v1.1.0

前もってやっておくこと:
    - エンジンのリポジトリを remote "engine" として登録しておく (最初の 1 回だけ)
        git remote add engine <エンジンのリポジトリの URL>
    - project/Engine/CHANGELOG.md に "## v1.1.0" の見出しを書いてコミットしておく
#>
param(
    [Parameter(Mandatory = $true)]
    [string]$Version,

    [string]$Remote = "engine"
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

function Stop-Release([string]$message)
{
    Write-Host $message -ForegroundColor Red
    Exit-Script 1
}

function Invoke-Git
{
    $output = & git @args
    if ($LASTEXITCODE -ne 0)
    {
        Stop-Release "git $($args -join ' ') が失敗した"
    }
    return $output
}

if ($Version -notmatch '^v\d+\.\d+\.\d+$')
{
    Stop-Release "版は v1.2.3 の形で指定する: $Version"
}

# git が出す絶対パスは日本語のフォルダ名が文字化けするので、相対パスだけを使う
Set-Location $PSScriptRoot
$enginePrefix = "$(Invoke-Git rev-parse --show-prefix)Engine"
$pathToRoot = Invoke-Git rev-parse --show-cdup
if ($pathToRoot)
{
    Set-Location $pathToRoot
}

# Windows PowerShell では git の標準エラーをリダイレクトすると例外になるので、一覧から探す
if ((Invoke-Git remote) -notcontains $Remote)
{
    Stop-Release "remote `"$Remote`" がない。git remote add $Remote <エンジンのリポジトリの URL> で登録する"
}

# split はコミット済みの履歴だけを使うので、コミットしていない変更は出ない
$uncommitted = Invoke-Git status --porcelain -- $enginePrefix
if ($uncommitted)
{
    Write-Host ($uncommitted -join "`n")
    Stop-Release "$enginePrefix にコミットしていない変更がある。コミットしてからやり直す"
}

$changelog = Get-Content "$enginePrefix/CHANGELOG.md" -Encoding UTF8
if (-not ($changelog -match "^## $([regex]::Escape($Version))(\s|$)"))
{
    Stop-Release "$enginePrefix/CHANGELOG.md に `"## $Version`" の見出しがない。変更点を書いてコミットしてからやり直す"
}

$existingTag = Invoke-Git ls-remote --tags $Remote "refs/tags/$Version"
if ($existingTag)
{
    Stop-Release "$Version はすでに出している。新しい版の番号を指定する"
}

# 今の Engine の中身をそのまま 1 つのコミットにして、前の版の続きとして出す。
# 2 年生は --squash で取り込むので、細かい履歴は配らない (履歴はこのリポジトリにある)
$engineTree = Invoke-Git rev-parse "HEAD:$enginePrefix"
$sourceCommit = Invoke-Git rev-parse --short HEAD
$parentArguments = @()

if (Invoke-Git ls-remote --heads $Remote main)
{
    # タグはこのリポジトリのタグと混ざらないように取ってこない
    Invoke-Git fetch --quiet --no-tags $Remote main
    $previousRelease = Invoke-Git rev-parse FETCH_HEAD
    if ((Invoke-Git rev-parse "${previousRelease}^{tree}") -eq $engineTree)
    {
        Stop-Release "前の版から $enginePrefix の中身が変わっていない"
    }
    $parentArguments = @("-p", $previousRelease)
}

$releaseCommit = Invoke-Git commit-tree $engineTree @parentArguments -m $Version -m "元のリポジトリのコミット: $sourceCommit"

Invoke-Git push $Remote "${releaseCommit}:refs/heads/main" | Out-Null
Invoke-Git push $Remote "${releaseCommit}:refs/tags/$Version" | Out-Null

Write-Host "$Version を出した ($releaseCommit)" -ForegroundColor Green
Write-Host "2 年生には、版の番号と CHANGELOG の内容を伝える"
Set-Location $originalLocation
