#include <QApplication>
#include "RhythmFlow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    RhythmFlow w;
    w.show();
    return app.exec();
}