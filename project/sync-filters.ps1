$vcxprojPath = "DirectXGame.vcxproj"
$filtersPath = "DirectXGame.vcxproj.filters"

# 検索対象の拡張子
$extensions = "*.cpp", "*.h", "*.hpp", "*.inl", "*.hlsl", "*.hlsli", "*.json", "*.png", "*.dds", "*.jpg", "*.txt"

# ビルド成果物・一時フォルダのみを除外 (externals 等のソースフォルダは検索対象に含める)
$excludePatterns = '\\(x64|x86|Debug|Release|Bin|obj|\.vs)\\'

$files = Get-ChildItem -Recurse -Include $extensions | Where-Object { $_.FullName -notmatch $excludePatterns }

$filters = @()
$compileItems = @()
$includeItems = @()
$fxItems = @()
$noneItems = @()

foreach ($file in $files) {
    $relativePath = (Resolve-Path -Relative $file.FullName) -replace '^\.\\', ''
    $dirName = [System.IO.Path]::GetDirectoryName($relativePath)

    if ($dirName) {
        $filters += $dirName
        $parent = $dirName
        while ($parent -match '\\') {
            $parent = [System.IO.Path]::GetDirectoryName($parent)
            if ($parent) { $filters += $parent }
        }
    }

    $ext = $file.Extension.ToLower()

    if ($ext -eq ".cpp") {
        if ($dirName) {
            $compileItems += '    <ClCompile Include="' + $relativePath + '"><Filter>' + $dirName + '</Filter></ClCompile>'
        } else {
            $compileItems += '    <ClCompile Include="' + $relativePath + '" />'
        }
    } elseif ($ext -in @(".h", ".hpp", ".inl")) {
        if ($dirName) {
            $includeItems += '    <ClInclude Include="' + $relativePath + '"><Filter>' + $dirName + '</Filter></ClInclude>'
        } else {
            $includeItems += '    <ClInclude Include="' + $relativePath + '" />'
        }
    } elseif ($ext -in @(".hlsl", ".hlsli")) {
        if ($dirName) {
            $fxItems += '    <FxCompile Include="' + $relativePath + '"><Filter>' + $dirName + '</Filter></FxCompile>'
        } else {
            $fxItems += '    <FxCompile Include="' + $relativePath + '" />'
        }
    } else {
        if ($dirName) {
            $noneItems += '    <None Include="' + $relativePath + '"><Filter>' + $dirName + '</Filter></None>'
        } else {
            $noneItems += '    <None Include="' + $relativePath + '" />'
        }
    }
}

$uniqueFilters = $filters | Select-Object -Unique

$xmlLines = @()
$xmlLines += '<?xml version="1.0" encoding="utf-8"?>'
$xmlLines += '<Project ToolsVersion="4.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">'
$xmlLines += '  <ItemGroup>'

foreach ($f in $uniqueFilters) {
    $guid = '{' + [Guid]::NewGuid().ToString() + '}'
    $xmlLines += '    <Filter Include="' + $f + '"><UniqueIdentifier>' + $guid + '</UniqueIdentifier></Filter>'
}

$xmlLines += '  </ItemGroup>'

if ($compileItems.Count -gt 0) {
    $xmlLines += '  <ItemGroup>'
    $xmlLines += $compileItems
    $xmlLines += '  </ItemGroup>'
}
if ($includeItems.Count -gt 0) {
    $xmlLines += '  <ItemGroup>'
    $xmlLines += $includeItems
    $xmlLines += '  </ItemGroup>'
}
if ($fxItems.Count -gt 0) {
    $xmlLines += '  <ItemGroup>'
    $xmlLines += $fxItems
    $xmlLines += '  </ItemGroup>'
}
if ($noneItems.Count -gt 0) {
    $xmlLines += '  <ItemGroup>'
    $xmlLines += $noneItems
    $xmlLines += '  </ItemGroup>'
}

$xmlLines += '</Project>'

Set-Content -Path $filtersPath -Value $xmlLines -Encoding UTF8

Write-Host "Filter sync complete! Reload project in Visual Studio." -ForegroundColor Green