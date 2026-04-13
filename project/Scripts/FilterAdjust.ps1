# ============================
# DirectXGame.vcxproj.filters Generator
# ============================
$targetFile = "DirectXGame.vcxproj"

# 1. Check Location
$currentDir = Get-Location
Write-Host "Current Directory: $($currentDir.Path)"

if (-not (Test-Path $targetFile)) {
    Write-Error "Error: $targetFile not found in $($currentDir.Path)"
    return
}

$proj = (Resolve-Path $targetFile).Path
$filters = "$proj.filters"
$root = Split-Path $proj

try {
    # 2. Get Files
    $files = Get-ChildItem $root -Recurse -File |
             Where-Object { $_.Extension -match '\.(cpp|c|h|hpp)$' }

    if ($null -eq $files) {
        Write-Warning "No source files found."
        return
    }

    # 3. Collect Filters (Case-insensitive)
    $filterSet = New-Object System.Collections.Generic.HashSet[string]([System.StringComparer]::OrdinalIgnoreCase)

    foreach ($f in $files) {
        $rel = $f.FullName.Substring($root.Length + 1) -replace '/', '\'
        $dir = Split-Path $rel

        if ($dir -and $dir -ne ".") {
            $parts = $dir.Split('\')
            for ($i = 1; $i -le $parts.Length; $i++) {
                $filterSet.Add(($parts[0..($i - 1)] -join '\')) | Out-Null
            }
        }
    }

    # 4. Build XML
    $xml = New-Object System.Collections.Generic.List[string]
    $xml.Add('<Project ToolsVersion="4.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">')

    # --- Filters ---
    $xml.Add('  <ItemGroup>')
    foreach ($filter in $filterSet | Sort-Object) {
        $guid = [guid]::NewGuid().ToString()
        $xml.Add("    <Filter Include=""$filter"">")
        $xml.Add("      <UniqueIdentifier>{$guid}</UniqueIdentifier>")
        $xml.Add("    </Filter>")
    }
    $xml.Add('  </ItemGroup>')

    # --- Files ---
    $xml.Add('  <ItemGroup>')
    foreach ($f in $files) {
        $rel = $f.FullName.Substring($root.Length + 1) -replace '/', '\'
        $dir = Split-Path $rel
        $tag = if ($f.Extension -match 'h|hpp') { "ClInclude" } else { "ClCompile" }
        
        if ($dir -and $dir -ne ".") {
            $xml.Add("    <$tag Include=""$rel""><Filter>$dir</Filter></$tag>")
        } else {
            $xml.Add("    <$tag Include=""$rel"" />")
        }
    }
    $xml.Add('  </ItemGroup>')
    $xml.Add('</Project>')

    # 5. Save File (UTF-8 with BOM for Visual Studio)
    Write-Host "Writing to: $filters"
    [System.IO.File]::WriteAllLines($filters, $xml, [System.Text.Encoding]::UTF8)
    
    Write-Host "Success!" -ForegroundColor Green
}
catch {
    Write-Error $_.Exception.Message
}