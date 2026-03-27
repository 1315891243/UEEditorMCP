# UEEditorMCP 变更日志

## [未发布] UE5.2 兼容性适配

> 分支：`copilot/optimize-ai-plugin-for-ue5-2`
> 日期：2026-03-27

### 背景

本插件原以 UE5.7 为目标开发，使用了 `ModelViewViewModelBlueprint` 模块中的
Blueprint 级别 MVVM API（`UMVVMBlueprintView`、`UMVVMWidgetBlueprintExtension_View`
等）。该模块在 UE5.3 才作为独立模块正式提供，因此在 UE5.2 上直接编译会报找不到
头文件的错误。本次变更通过编译期版本宏将相关代码隔离，使插件在 UE5.2 与 UE5.3+
上均可正常编译。

---

### 变更文件

#### `Source/UEEditorMCP/UEEditorMCP.Build.cs`

- 将 `ModelViewViewModelBlueprint`（MVVM 编辑器端绑定 API）和
  `FieldNotification`（`INotifyFieldValueChanged` 接口）两个模块依赖移入
  `Target.Version.MinorEngineVersion >= 3` 版本判断块。
- `ModelViewViewModel`（MVVM 运行时类型）保持无条件引用，因为该模块自
  UE5.2 起即以实验性功能存在。

#### `Source/UEEditorMCP/Public/Actions/UMGActions.h`

- 使用 `#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3` 包裹
  以下 5 个 MVVM Action 类声明，UE5.2 下这些类不参与编译：
  - `FMVVMAddViewModelAction`
  - `FMVVMAddBindingAction`
  - `FMVVMGetBindingsAction`
  - `FMVVMRemoveBindingAction`
  - `FMVVMRemoveViewModelAction`

#### `Source/UEEditorMCP/Private/Actions/UMGActions.cpp`

- 用同一版本宏包裹 MVVM 相关 `#include` 块（共 8 个头文件）。
- 用同一版本宏包裹以上 5 个 MVVM Action 的全部实现代码（约 650 行）。

#### `Source/UEEditorMCP/Private/MCPBridge.cpp`

- `UE5.3+`：照常注册真实的 MVVM handler。
- `UE5.2`：为 5 个 `mvvm_*` 命令注册轻量 stub handler，调用时返回：
  ```
  Action 'mvvm_xxx' requires UE5.3 or later
  (ModelViewViewModelBlueprint module). Current engine version is 5.2.
  ```
  这样 MCP 客户端能收到明确的版本不支持提示，而不是未知命令错误。

#### `UEEditorMCP.uplugin`

- 将 `ModelViewViewModel` 插件依赖标记为 `"Optional": true`，
  避免在 UE5.2 中该插件未启用时导致插件加载失败。

---

### 版本兼容性说明

| 引擎版本 | 编译结果 | MVVM 功能 | 其他功能 |
|----------|----------|-----------|----------|
| UE 5.2   | ✅ 正常 | ❌ 不可用（返回错误提示） | ✅ 完整 |
| UE 5.3   | ✅ 正常 | ✅ 完整 | ✅ 完整 |
| UE 5.4+  | ✅ 正常 | ✅ 完整 | ✅ 完整 |
| UE 5.7   | ✅ 正常 | ✅ 完整 | ✅ 完整 |

---

### 未受影响的功能

以下功能在所有版本上均完整可用，本次变更未触及：

- Blueprint 蓝图编辑（创建、节点、变量、函数等）
- UMG 控件编辑（除 MVVM 绑定外的全部控件操作）
- 材质编辑（创建、节点连接、编译等）
- 场景与 Actor 操作
- 项目与资产管理
- 图表操作（图节点布局、迁移等）
- EnhancedInput 节点
- Diff 对比功能
