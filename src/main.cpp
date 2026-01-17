// 必须先包含 arv.h，然后再包含 Qt 头文件，避免宏冲突
#include <arv.h>
#include <iostream>
#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // 打印Aravis库版本信息
    std::cout << "=== Aravis相机控制系统 ===" << std::endl;
    std::cout << "Aravis版本: "
              << arv_get_major_version() << "."
              << arv_get_minor_version() << "."
              << arv_get_micro_version() << std::endl;
    std::cout << "Qt版本: " << qVersion() << std::endl;
    std::cout << "==============================" << std::endl;

    // 创建并显示主窗口
    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}