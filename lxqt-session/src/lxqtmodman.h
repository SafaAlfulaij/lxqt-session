/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LxQt - a lightweight, Qt based, desktop toolset
 * http://razor-qt.org, http://lxde.org/
 *
 * Copyright: 2010-2011 LxQt team
 * Authors:
 *   Petr Vanek <petr@scribus.info>
 *
 * This program or library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#ifndef LXQTMODMAN_H
#define LXQTMODMAN_H

#include <QProcess>
#include <QList>
#include <QMap>
#include <QTimer>
#include <XdgDesktopFile>
#include <QEventLoop>
#include <time.h>

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QAbstractNativeEventFilter>
#endif

class LxQtModule;
namespace LxQt {
class Settings;
}
class QFileSystemWatcher;

typedef QMap<QString,LxQtModule*> ModulesMap;
typedef QMapIterator<QString,LxQtModule*> ModulesMapIterator;
typedef QList<time_t> ModuleCrashReport;
typedef QMap<QProcess*, ModuleCrashReport> ModulesCrashReport;

/*! \brief LxQtModuleManager manages the processes of the session
and which modules of lxqt are about to load.

LxQtModuleManager handles the session management (logout/restart/shutdown)
as well.

Also it watches the current theme to react if it was removed or modified.

Processes in LxQtModuleManager are started as follows:
 - run lxqt-confupdate
 - start the window manager and wait until it's active
 - start all normal autostart items (including LxQt modules)
 - if there are any applications that need a system tray, wait until a system tray
   implementation becomes active, and then start those

Potential process recovering is done in \see restartModules()
*/

class LxQtModuleManager : public QObject
#ifndef Q_MOC_RUN // Qt4 moc has some problem handling multiple inheritence with conditional compilation, disable it
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
	, public QAbstractNativeEventFilter // we need to filter some native events in Qt5
#endif
#endif // Q_MOC_RUN
{
    Q_OBJECT

public:
    //! \brief Construct LxQtModuleManager
    LxQtModuleManager(const QString& windowManager, QObject* parent = 0);
    virtual ~LxQtModuleManager();

    //! \brief Start a module given its file name (e.g. "lxqt-panel.desktop")
    void startProcess(const QString& name);

    //! \brief Stop a running module
    void stopProcess(const QString& name);

    //! \brief List the running modules, identified by their file names
    QStringList listModules() const;

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    // Qt5 uses native event filter
    virtual bool nativeEventFilter(const QByteArray & eventType, void * message, long * result);
#else
    // X11 event is no longer supported in Qt5
    bool x11EventFilter(XEvent* event);
#endif
    void x11PropertyNotify(unsigned long atom); // called in X11 only
    void x11ClientMessage(void* _event); // called in X11 only

    //! \brief Read configuration and start processes
    void startup(LxQt::Settings& s);

public slots:
    /*! \brief Exit LxQt session.
    It tries to terminate processes from procMap and autostartList
    gracefully (to kill it if it is not possible). Then the session
    exits - it returns to the kdm/gdm in most cases.
    */
    void logout();

signals:
    void moduleStateChanged(QString moduleName, bool state);

private:
    //! \brief Start Window Manager
    void startWm(LxQt::Settings *settings);
    void wmStarted();

    void startAutostartApps();

    //! \brief Show Window Manager select dialog
    QString showWmSelectDialog();

    //! \brief Start a process described in a desktop file
    void startProcess(const XdgDesktopFile &file);

    //! \brief Start the lxqt-confupdate.
    void startConfUpdate();

    //! \brief Window manager command
    QString mWindowManager;

    //! \brief map file names to module processes
    ModulesMap mNameMap;

    //! \brief the window manager
    QProcess* mWmProcess;

    /*! \brief Keep creashes for given process to raise a message in the
        case of repeating crashes
     */
    ModulesCrashReport mCrashReport;

    //! \brief file system watcher to react on theme modifications
    QFileSystemWatcher *mThemeWatcher;
    QString mCurrentThemePath;
    
    bool mWmStarted;
    bool mTrayStarted;
    QEventLoop mWaitLoop;

private slots:

    /*! \brief this slot is called by the QProcesses if they end.
    \warning The slot *has* to be called as a slot only due sender() cast.

    There are 2 types of handling in this slot.

    If the process has ended correctly (no crash or kill) nothing happens
    except when the process is *not* specified with doespower in the
    configuration. If there is no "doespower" (window manager mainly)
    entire session performs logout. (Handling of the window manager 3rd
    party "logout")

    If the process crashed and is set as "doespower" it's tried to
    be restarted automatically.
    */
    void restartModules(int exitCode, QProcess::ExitStatus exitStatus);

    /*! Clear m_crashReport after some amount of time
     */
    void resetCrashReport();

    void themeFolderChanged(const QString&);

    void themeChanged();
};


/*! \brief platform independent way how to set up an environment variable.
It sets env variable for all lxqt-session childs.
\param env a raw string variable name (PATH, TERM, ...)
\param value a QByteArray with the value. Variable will use this new value only
             - no appending/prepending is performed.
See lxqt_setenv_prepend.
*/
void lxqt_setenv(const char *env, const QByteArray &value);
/*! \brief Set up a environment variable with original value with new value prepending.
\param env a raw string with variable name
\param value a QByteArray value to be pre-pend to original content of the variable
\param separator an optional string with separator character. Eg. ":" for PATH.
See lxqt_setenv.
*/
void lxqt_setenv_prepend(const char *env, const QByteArray &value, const QByteArray &separator=":");

class LxQtModule : public QProcess
{
    Q_OBJECT
public:
    LxQtModule(const XdgDesktopFile& file, QObject *parent = 0);
    void start();
    void terminate();
    bool isTerminating();

    const XdgDesktopFile file;
    const QString fileName;

signals:
    void moduleStateChanged(QString name, bool state);

private slots:
    void updateState(QProcess::ProcessState newState);

private:
    bool mIsTerminating;
};

#endif
