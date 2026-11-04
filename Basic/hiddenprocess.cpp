#include "hiddenprocess.h"

#include <QProcess>

#ifdef Q_OS_WIN
#include <QtCore/qt_windows.h>
#endif

void hideConsoleForChildProcess(QProcess& process)
{
#ifdef Q_OS_WIN
    // Keep QProcess stdout/stderr pipes for the asynchronous ping result.
    process.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments* args) {
        args->flags |= CREATE_NO_WINDOW;
    });
#else
    Q_UNUSED(process);
#endif
}
