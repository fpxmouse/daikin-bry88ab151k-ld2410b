# BRY88AB151K + HLK-LD2410B 固件

本项目在 `louliangsheng/daikin-air-sensor` 的 BRY88AB151K ESPHome 配置基础上，使用空闲的刷机 UART0 焊盘读取 LD2410B。提供 PM2005 和 PM2105 两个 OTA 镜像。

## 引脚结论

原配置已占用 GPIO16/17（CM1106）、GPIO22/23（PM2.5 I²C）、GPIO19/21（温湿度/TVOC I²C）、GPIO4/25/32/33/34（指示灯与复位）。上游 YAML 没有使用刷机口 GPIO1/GPIO3，因此可在启动后复用为 LD2410B 的硬件 UART：

| 大金主板 | LD2410B | 说明 |
|---|---|---|
| RXD / GPIO3 | 2 UART_TX | 串口交叉连接 |
| TXD / GPIO1 | 3 UART_RX | 串口交叉连接 |
| GND | 4 GND | 必须共地 |
| 已确认的 5V 点 | 5 VCC | 5V，供电能力大于 200mA |
| 不接 | 1 OUT | 本固件通过 UART 读取 |

详见 [`docs/wiring.svg`](docs/wiring.svg)。RTX 焊盘本身只确认 RXD/TXD/GND，不要猜测 5V。断电后用万用表确认板上 5V 点；也可以使用独立稳压 5V 电源，但必须共地。

## OTA 升级

1. PM2005 设备直接下载 [`firmware/bin/bry88ab151k-pm2005-ld2410b-ota.bin`](firmware/bin/bry88ab151k-pm2005-ld2410b-ota.bin)。
2. PM2105 设备直接下载 [`firmware/bin/bry88ab151k-pm2105-ld2410b-ota.bin`](firmware/bin/bry88ab151k-pm2105-ld2410b-ota.bin)。
3. 在 ESPHome Dashboard 或设备网页中选择对应的 `.bin` 进行 OTA。`dist/` 中另有 ZIP 压缩包可选。
4. 升级后按上游流程重新配网：连接设备发出的 `DAIKIN Fallback Hotspot`，在弹出的配网页面填写 Wi-Fi。热点未设置密码，建议接通后立即完成配网。
5. 重新加入 Home Assistant 后，检查 `Radar Presence`、移动/静止目标和距离实体。

GPIO1/GPIO3 同时是有线刷机 UART。固件用 `logger.baud_rate: 0` 关闭串口日志，避免与 LD2410B 冲突；API 日志仍可用。以后若需通过 USB-TTL 有线刷机，建议先断开 LD2410B 的 TX/RX。

## 本地构建

构建固定使用 ESPHome 2024.12.4，因为上游 PM/CO₂ 代码使用该版本仍支持的 `custom` 平台：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Python "C:\path\to\python.exe"
```

脚本先运行两份配置校验，再编译并复制 OTA 镜像到 `dist/`，最后输出 SHA-256。

## 资料来源

- [上游 BRY88AB151K 固件](https://github.com/louliangsheng/daikin-air-sensor)
- [海凌科 LD2410B 资料页](https://h.hlktech.com/Mobile/download/fdetail/204.html)
- [海凌科 LD2410B 串口协议 V1.08](https://h.hlktech.com/download/HLK-LD2410B-24G/1/LD2410B%20%E4%B8%B2%E5%8F%A3%E9%80%9A%E4%BF%A1%E5%8D%8F%E8%AE%AE%20V1.08.pdf)
- [ESPHome LD2410 组件](https://esphome.io/components/sensor/ld2410/)
