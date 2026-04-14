#include "DemoWindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Graffodil Demo");

    DemoWindow window;
    window.resize(1200, 800);
    window.show();

    return app.exec();
}
