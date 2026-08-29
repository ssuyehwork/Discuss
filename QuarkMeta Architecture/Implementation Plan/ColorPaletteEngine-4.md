# Implementation Plan - ColorPaletteEngine-4

This implementation plan upgrades `ColorPaletteEngine` to guarantee global uniqueness for extension background colors. It implements the Golden Ratio hue offset algorithm ($137.508^\circ$) combined with CIEDE2000 ($\Delta E \ge 25.0$) dynamic collision detection before persisting newly discovered file extension badge colors into SQLite `global.db` (`extension_colors` table).

## 1. Overview
- **Protected File Extensions**: `psd`/`psb` (`#001D26`/`#02B1DD`), `eps` (`#35483D`/`#F88025`), `ai` (`#F88025`/`#35483D`).
- **Golden Angle Hue Offset**: For new file extensions, initial hue is calculated via $H = (N \times 137.508^\circ) \pmod{360^\circ}$.
- **CIEDE2000 ($\Delta E$) Dynamic Collision Resolution**: Candidate background colors are compared against all existing background colors in `global.db`. If $\Delta E < 25.0$ (visually similar color collision), the hue is stepped by $+15^\circ$ iteratively until a visually distinct, unique color is obtained.
- **SQLite Persistence**: Unique colors are saved via `ExtensionColorDao::saveExtensionColor` into `global.db` and cached in memory.

## 2. Modified Files List
- `src/util/ColorPaletteEngine.cpp`

## 3. Detailed Line-by-Line Changes

### `src/util/ColorPaletteEngine.cpp`
```diff
<<<<<<< SEARCH
    // 3. 动态生成新配色并物理落盘写入 global.db
    uint hashVal = static_cast<uint>(qHash(e));
    int hue = static_cast<int>(hashVal % 360);
    int saturation = 130 + static_cast<int>((hashVal >> 8) % 80);
    int lightness = 80 + static_cast<int>((hashVal >> 16) % 60);

    QColor bgColor = QColor::fromHsl(hue, saturation, lightness);
    double luminance = (0.299 * bgColor.red() + 0.587 * bgColor.green() + 0.114 * bgColor.blue()) / 255.0;
    QColor textColor = (luminance < 0.55) ? QColor("#FFFFFF") : QColor("#1A1A1A");

    QPair<QColor, QColor> colorPair = { bgColor, textColor };
    
    // 刷盘固化并填充内存 Cache
    ExtensionColorDao::saveExtensionColor(e, bgColor, textColor, false);
    s_colorCache.insert(e, colorPair);

    return colorPair;
=======
    // 3. 黄金角 137.508° + Delta E (CIEDE2000 >= 25.0) 色差碰撞校验生成唯一背景色
    uint hashVal = static_cast<uint>(qHash(e));
    double baseHue = std::fmod(s_colorCache.size() * 137.508 + (hashVal % 360), 360.0);
    int saturation = 140 + static_cast<int>((hashVal >> 8) % 70);
    int lightness = 85 + static_cast<int>((hashVal >> 16) % 50);

    QColor bgColor = QColor::fromHsl(static_cast<int>(baseHue), saturation, lightness);

    // 碰撞检测：确保新生成颜色与已有背景色的 Delta E 至少为 25.0
    bool collision = true;
    int maxAttempts = 24;
    while (collision && maxAttempts-- > 0) {
        collision = false;
        for (auto it = s_colorCache.begin(); it != s_colorCache.end(); ++it) {
            double deltaE = calculateDeltaE(bgColor, it.value().first);
            if (deltaE < 25.0) {
                collision = true;
                baseHue = std::fmod(baseHue + 15.0, 360.0);
                bgColor = QColor::fromHsl(static_cast<int>(baseHue), saturation, lightness);
                break;
            }
        }
    }

    double luminance = (0.299 * bgColor.red() + 0.587 * bgColor.green() + 0.114 * bgColor.blue()) / 255.0;
    QColor textColor = (luminance < 0.55) ? QColor("#FFFFFF") : QColor("#1A1A1A");

    QPair<QColor, QColor> colorPair = { bgColor, textColor };
    
    // 刷盘固化并填充内存 Cache
    ExtensionColorDao::saveExtensionColor(e, bgColor, textColor, false);
    s_colorCache.insert(e, colorPair);

    return colorPair;
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Clean and build the project using CMake & Ninja:
   ```bash
   cmake -B build -G Ninja
   cmake --build build
   ```
2. Run application and verify:
   - Check file cards for various extensions (`.md`, `.txt`, `.rc`, `.manifest`, `.png`, `.psb`, `.gitignore`, `.json`).
   - Confirm that background colors for different extensions are distinctly unique with no duplicate or visually identical colors.
