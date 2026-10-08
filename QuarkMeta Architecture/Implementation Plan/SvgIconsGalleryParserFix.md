# SvgIconsGalleryParserFix.md Implementation Plan

## 1. Overview
Fix regex parsing failure in `svg_icons_gallery_v5.html` when parsing `SvgIcons.h`. Replace the fragile regular expression (`/\{"([^"]+)",\s*R"svg\(([\s\S]*?)\)svg"\}/g`) with a flexible, universal regex that tolerates formatting variations, space variations around `{`, `"`, `,`, `}`, and alternative raw string tags like `R"(` / `R"xml(`. Strip XML headers and comments during SVG string parsing, and update the button label.

## 2. Modified Files List
- `svg_icons_gallery_v5.html`

## 3. Detailed Line-by-Line Changes

```path
svg_icons_gallery_v5.html
```

<<<<<<< SEARCH
  <div class="input-area" id="parserUI">
    <textarea id="rawPaste" placeholder="直接粘贴 SvgIcons.h 的全部内容..."></textarea>
    <button class="parse-btn" onclick="initAndParse()">立即解析并显示 426 个图标</button>
  </div>
=======
  <div class="input-area" id="parserUI">
    <textarea id="rawPaste" placeholder="直接粘贴 SvgIcons.h 的全部内容..."></textarea>
    <button class="parse-btn" onclick="initAndParse()">立即解析并显示所有图标</button>
  </div>
>>>>>>> REPLACE

<<<<<<< SEARCH
// 2. 解析逻辑：从你粘贴的 C++ 代码中提取数据
function initAndParse() {
    const raw = document.getElementById('rawPaste').value;
    const regex = /\{"([^"]+)",\s*R"svg\(([\s\S]*?)\)svg"\}/g;
    let match;
    allIcons = [];
    while ((match = regex.exec(raw)) !== null) {
        allIcons.push({ name: match[1], svg: match[2] });
    }

    if (allIcons.length === 0) {
        alert("未能识别 C++ 代码，请检查粘贴的内容是否包含 {\"name\", R\"svg(...)svg\"}");
        return;
    }
=======
// 2. 解析逻辑：从你粘贴的 C++ 代码中提取数据
function initAndParse() {
    const raw = document.getElementById('rawPaste').value;
    // 超强容错正则表达式：兼容各种空格/换行、各种 Raw String 标识符（如 R"svg(、R"(、R"xml( 等）及普通字符串
    const regex = /\{\s*"([^"]+)"\s*,\s*(?:R"([a-zA-Z0-9_]*)\(([\s\S]*?)\)\2"|"((?:[^"\\]|\\.)*)")\s*\}/g;
    let match;
    allIcons = [];
    while ((match = regex.exec(raw)) !== null) {
        const name = match[1];
        let svg = match[3] !== undefined ? match[3] : match[4];
        // 清理可能夹带的 XML 声明与 HTML 注释，确保渲染绝对纯净
        svg = svg.replace(/<\?xml[\s\S]*?\?>/gi, '').replace(/<!--[\s\S]*?-->/g, '').trim();
        allIcons.push({ name: name, svg: svg });
    }

    if (allIcons.length === 0) {
        alert("未能识别 C++ 代码，请检查粘贴的内容是否包含 {\"name\", R\"svg(...)svg\"} 或 {\"name\", R\"(...)\"} 等声明。");
        return;
    }
>>>>>>> REPLACE

## 4. Build & Verification Steps
1. Open `svg_icons_gallery_v5.html` in any web browser.
2. Copy and paste the complete content of `src/ui/SvgIcons.h` into the text area.
3. Click "立即解析并显示所有图标".
4. Confirm that all icons (760+) are parsed without errors and displayed cleanly in the gallery.

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- Enhances parser regex robustness without modifying C++ source code.
- Eliminates hardcoded icon count "426".

## 6. Header API Signature Verification
N/A (HTML/JS file).

## 7. Header Inclusion Chain & Type Completeness Check
N/A (HTML/JS file).
