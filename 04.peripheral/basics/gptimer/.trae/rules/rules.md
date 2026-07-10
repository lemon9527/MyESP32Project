---
alwaysApply: true
---

# ESP-IDF v5.x 项目规则

## UART API
- `uart_write_bytes()` 只接受 **3 个参数**：`(uart_port_t, const void*, size_t)`，**不要**添加第 4 个超时参数。
- `uart_read_bytes()` 才需要 **4 个参数**：`(uart_port_t, void*, uint32_t, TickType_t)`。

## 通用规则
- 本项目基于 ESP-IDF v5.3.5，所有 API 以 ESP-IDF v5.x 为准，不要参考 v4.x 的旧版 API。
- 使用 `ESP_ERROR_CHECK()` 检查所有 ESP-IDF 函数调用的返回值。
- UART 初始化顺序：`uart_param_config()` → `uart_set_pin()` → `uart_driver_install()`，缺一不可。