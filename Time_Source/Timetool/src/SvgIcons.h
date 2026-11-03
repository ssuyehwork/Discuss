#ifndef SVGICONS_H
#define SVGICONS_H

#include <QString>
#include <QMap>

namespace SvgIcons {
    inline QString getSvg(const QString& key) {
        static const QMap<QString, QString> icons = {
            // [新增] 用户定制应用主图标 (白底+青色时钟表盘)
            {"app_logo", R"svg(<svg version="1.1" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" viewBox="0 0 1000 1000">
<style type="text/css">
	.st0{fill:#FFFFFF;}
	.st1{fill:#00BFA5;}
</style>
<g>
	<circle class="st0" cx="500" cy="500" r="297.2"/>
	<path class="st1" d="M500,3.2C225.6,3.2,3.2,225.6,3.2,500S225.6,996.8,500,996.8S996.8,774.4,996.8,500S774.4,3.2,500,3.2z M500,896.2c-218.8,0-396.2-177.4-396.2-396.2S281.2,103.8,500,103.8S896.2,281.2,896.2,500S718.8,896.2,500,896.2z"/>
	<path class="st1" d="M665,695.6L665,695.6c-16.8,14.5-42.2,12.5-56.7-4.3l-150.2-180l-3.7-4.3c-1.7-2-2.7-4.6-2.7-7.2V318.7 c0-27.4,22.2-49.7,49.7-49.7h0c27.4,0,49.7,22.2,49.7,49.7v173.5c0,2.7,0.9,5.2,2.7,7.2L669.3,639 C683.7,655.8,681.8,681.2,665,695.6z"/>
</g>
</svg>)svg"},

            // [托盘菜单] 显示界面图标
            {"window", R"svg(<svg fill="currentColor" width="800px" height="800px" viewBox="0 0 32 32" version="1.1" xmlns="http://www.w3.org/2000/svg"><path d="M28 4h-24c-1.105 0-2 0.896-2 2v20c0 1.104 0.895 2 2 2h24c1.104 0 2-0.896 2-2v-20c0-1.104-0.896-2-2-2zM11.562 5.5c0.552 0 0.999 0.448 0.999 1s-0.447 1-0.999 1c-0.553 0-1-0.448-1-1s0.448-1 1-1zM8.5 5.5c0.552 0 1 0.448 1 1s-0.448 1-1 1c-0.553 0-1-0.448-1-1s0.447-1 1-1zM5.499 5.5c0.553 0 1 0.448 1 1s-0.447 1-1 1c-0.552 0-0.999-0.448-0.999-1s0.447-1 0.999-1zM28 26h-24v-16.979h24v16.979zM28 7.021h-14v-1h14v1z"></path></svg>)svg"},

            // [托盘菜单] 退出程序图标
            {"power", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M18.36 6.64a9 9 0 1 1-12.73 0"></path><line x1="12" y1="2" x2="12" y2="12"></line></svg>)svg"},

            // 备用线框图标
            {"clock", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="10"></circle><polyline points="12 6 12 12 16 14"></polyline></svg>)svg"},
            {"close", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="3.5" stroke-linecap="round" stroke-linejoin="round"><line x1="18" y1="6" x2="6" y2="18"></line><line x1="6" y1="6" x2="18" y2="18"></line></svg>)svg"}
        };
        return icons.value(key);
    }
}

#endif // SVGICONS_H