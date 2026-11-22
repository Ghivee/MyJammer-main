$pkg = 'C:\Users\Ahmad Ghifari\AppData\Local\Arduino15\packages\esp8266\hardware\esp8266\3.1.2'
Write-Host '=== cari file FreeRTOS/task/portmacro ==='
Get-ChildItem $pkg -Recurse -File -ErrorAction SilentlyContinue |
  Where-Object { $_.Name -match 'FreeRTOS|task\.h$|portmacro' } |
  Select-Object -First 20 -ExpandProperty FullName
Write-Host '=== isi osapi.h (include) ==='
Select-String -Path (Join-Path $pkg 'tools\sdk\include\osapi.h') -Pattern '#include' |
  Select-Object -First 20 | ForEach-Object { $_.Line }
Write-Host '=== Esp.h: bagian freertos/Task ==='
Select-String -Path (Join-Path $pkg 'cores\esp8266\Esp.h') -Pattern 'Task|freertos|FreeRTOS' |
  Select-Object -First 30 | ForEach-Object { "$($_.LineNumber): $($_.Line.Trim())" }
