#include "mainwindow.h"

#include <QApplication>
#include <QFile>
#include <QFont>
#include <QTimer>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("GridGuard"));
    QApplication::setOrganizationName(QStringLiteral("ProgrammingPractice2026"));
    QApplication::setFont(QFont(QStringLiteral("Microsoft YaHei UI"), 10));

    QFile styleFile(QStringLiteral(":/styles/app.qss"));
    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        app.setStyleSheet(QString::fromUtf8(styleFile.readAll()));
    }

    MainWindow window;
    window.show();

    if (app.arguments().contains(QStringLiteral("--smoke-test"))) {
        window.startSmokeScenario();
        QTimer::singleShot(2300, &app, &QApplication::quit);
    }
    return app.exec();
}
