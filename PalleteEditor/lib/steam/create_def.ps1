# Скрипт для создания .def файла из вывода dumpbin /exports
# Сохраните как create_def.ps1 и запустите: powershell -ExecutionPolicy Bypass -File create_def.ps1

$inputFile = "exports.txt"
$outputFile = "steam_api.def"

# Проверяем существование входного файла
if (-not (Test-Path $inputFile)) {
    Write-Host "Error: file $inputFile not found!" -ForegroundColor Red
    exit 1
}

Write-Host "Readind file $inputFile..." -ForegroundColor Yellow

# Читаем все строки файла
$exports = Get-Content $inputFile
$defContent = @()
$defContent += "LIBRARY steam_api.dll"
$defContent += "EXPORTS"

$functionCount = 0
$skipMode = $false

foreach ($line in $exports) {
    # Пропускаем пустые строки
    if ([string]::IsNullOrWhiteSpace($line)) {
        continue
    }
    
    # Находим начало таблицы функций (после строки с заголовками)
    if ($line -match "^\s*ordinal\s+hint\s+RVA\s+name") {
        $skipMode = $true
        continue
    }
    
    # Если мы еще не достигли таблицы функций, пропускаем
    if (-not $skipMode) {
        continue
    }
    
    # Парсим строки с функциями
    # Формат строки: "ordinal hint RVA      name"
    if ($line -match '^\s+(\d+)\s+([0-9A-Fa-f]+)\s+([0-9A-Fa-f]+)\s+(\S+)$') {
        $ordinal = $matches[1]
        $function = $matches[4]
        $defContent += "    $function @$ordinal"
        $functionCount++
    }
    # Альтернативный вариант парсинга (для некоторых форматов dumpbin)
    elseif ($line -match '^\s*\d+\s+\d+\s+[0-9A-F]+\s+(\S+)') {
        # Если не удалось извлечь ordinal, используем счетчик
        $function = $matches[1]
        $defContent += "    $function"
        $functionCount++
    }
}

# Записываем результат
$defContent | Out-File -Encoding ASCII $outputFile

Write-Host "Ready!" -ForegroundColor Green
Write-Host "Find func: $functionCount" -ForegroundColor Green
