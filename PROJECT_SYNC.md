# PROJECT_SYNC

## 基本信息

- 项目名称：ble_sensor_terminal
- 项目编号：project003

## 全局同步状态

- 已读取全局规则版本：v2.0
- 最近读取时间：2026-06-30 14:07:22
- 最近全项目刷新检查时间：2026-06-30 14:07:22
- 当前是否落后于全局版本：否

## 待沉淀经验

- [ ] 基于 BlueZ D-Bus 的 BLE Peripheral/GATT Server 如果后续要迁入平台层，优先先抽出 socket 风格传输接口，再挂接具体 BLE 实现。
- [ ] 协议层坚持只依赖 `transport_open/send/recv/close` 这类抽象边界，可以降低从 demo 到平台模组的迁移成本。

## 已处理经验

- [x] 已回写全局：
- [x] 已判定不回写：
