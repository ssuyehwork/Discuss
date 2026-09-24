# Implementation Plan - FramelessWindowHelper-9.md

## Overview
本实施方案旨在解决 FramelessWindowHelper 重构中的顶层无边框窗口原生 WM_NCHITTEST 拖拽与缩放逻辑，确保完全符合系统 Win32 原生交互规范。

## Modified Files List
- `QuarkMeta Architecture/Implementation Plan/FramelessWindowHelper-9.md`

## Detailed Line-by-Line Changes
无物理源码修改，仅创建此实施方案。

## Build & Verification Steps
1. 检查方案规范完整性；
2. 确保符合 AGENTS.md 1.0 与 3.1 节物理隔离与只读规则。

## SSOT API Reuse & Anti-Redundancy Self-Check
- 方案完全物理隔离于 `QuarkMeta Architecture/Implementation Plan/FramelessWindowHelper-9.md`。

## Header API Signature Verification
- `FramelessWindowHelper::apply(QWidget*, QWidget*)`
- `FramelessWindowHelper::handleNativeEvent(void*, qintptr*)`
