# 测试日志系统脚本
# 用于验证Release模式下日志文件是否正常生成

Write-Host "=====================================" -ForegroundColor Cyan
Write-Host "日志系统测试脚本" -ForegroundColor Cyan
Write-Host "=====================================" -ForegroundColor Cyan
Write-Host ""

# 1. 检查可执行文件
$exePath = "D:\DispCtrl\DispCtrl\build\bin\Release\DispCtrl.exe"
if (Test-Path $exePath) {
    Write-Host "[✓] 找到可执行文件: $exePath" -ForegroundColor Green
} else {
    Write-Host "[✗] 未找到可执行文件: $exePath" -ForegroundColor Red
    Write-Host "请先编译Release版本: cmake --build build --config Release" -ForegroundColor Yellow
    exit 1
}

# 2. 查找所有可能的日志文件位置
Write-Host ""
Write-Host "查找现有日志文件..." -ForegroundColor Yellow
$logLocations = @(
    "D:\DispCtrl\DispCtrl\build\bin\Release\disp_ctrl_log.txt",
    "D:\DispCtrl\DispCtrl\disp_ctrl_log.txt",
    "$env:TEMP\disp_ctrl_log.txt"
)

foreach ($logPath in $logLocations) {
    if (Test-Path $logPath) {
        Write-Host "[✓] 找到日志文件: $logPath" -ForegroundColor Green
        $fileInfo = Get-Item $logPath
        Write-Host "    大小: $($fileInfo.Length) bytes" -ForegroundColor Gray
        Write-Host "    修改时间: $($fileInfo.LastWriteTime)" -ForegroundColor Gray
    }
}

# 3. 运行程序（5秒后自动关闭）
Write-Host ""
Write-Host "即将运行程序（5秒后自动显示日志）..." -ForegroundColor Yellow
Write-Host "按 Ctrl+C 提前停止" -ForegroundColor Gray
Write-Host ""

# 启动程序（非阻塞）
$process = Start-Process -FilePath $exePath -PassThru -WorkingDirectory "D:\DispCtrl\DispCtrl\build\bin\Release"

# 等待5秒
Start-Sleep -Seconds 5

# 停止程序
if (!$process.HasExited) {
    Write-Host "正在停止程序..." -ForegroundColor Yellow
    $process.Kill()
    $process.WaitForExit(2000)
}

# 4. 检查日志文件是否生成/更新
Write-Host ""
Write-Host "=====================================" -ForegroundColor Cyan
Write-Host "日志文件检查结果" -ForegroundColor Cyan
Write-Host "=====================================" -ForegroundColor Cyan

$foundLog = $false
foreach ($logPath in $logLocations) {
    if (Test-Path $logPath) {
        $fileInfo = Get-Item $logPath
        # 检查文件是否在最近10秒内修改
        if ($fileInfo.LastWriteTime -gt (Get-Date).AddSeconds(-10)) {
            Write-Host ""
            Write-Host "[✓✓✓ 成功] 日志文件已更新: $logPath" -ForegroundColor Green
            Write-Host "    大小: $($fileInfo.Length) bytes" -ForegroundColor Gray
            Write-Host "    修改时间: $($fileInfo.LastWriteTime)" -ForegroundColor Gray

            # 显示最后20行
            Write-Host ""
            Write-Host "最后20行日志内容:" -ForegroundColor Cyan
            Write-Host "-------------------------------------" -ForegroundColor Gray
            Get-Content $logPath -Tail 20
            Write-Host "-------------------------------------" -ForegroundColor Gray

            $foundLog = $true
            break
        }
    }
}

if (-not $foundLog) {
    Write-Host ""
    Write-Host "[✗✗✗ 失败] 未找到新生成的日志文件" -ForegroundColor Red
    Write-Host ""
    Write-Host "可能的原因:" -ForegroundColor Yellow
    Write-Host "1. 程序运行目录没有写权限" -ForegroundColor Gray
    Write-Host "2. 日志初始化失败" -ForegroundColor Gray
    Write-Host "3. qInstallMessageHandler 未正确安装" -ForegroundColor Gray
    Write-Host ""
    Write-Host "建议:" -ForegroundColor Yellow
    Write-Host "1. 以管理员身份运行程序" -ForegroundColor Gray
    Write-Host "2. 检查 main.cpp 中的 qInstallMessageHandler(enhancedLog)" -ForegroundColor Gray
    Write-Host "3. 手动运行程序并观察控制台输出的日志路径" -ForegroundColor Gray
}

Write-Host ""
Write-Host "=====================================" -ForegroundColor Cyan
Write-Host "测试完成" -ForegroundColor Cyan
Write-Host "=====================================" -ForegroundColor Cyan
