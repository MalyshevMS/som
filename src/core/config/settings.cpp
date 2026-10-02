#include "settings.h"
#include <QSettings>
#include <QStandardPaths>

Settings::Settings() {
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/.minecraft";
    m_gameDir = defaultPath;
    load();
}

int Settings::minRamMb() const { return m_minRam; }
void Settings::setMinRamMb(int mb) { m_minRam = mb; }

int Settings::maxRamMb() const { return m_maxRam; }
void Settings::setMaxRamMb(int mb) { m_maxRam = mb; }

int Settings::windowWidth() const { return m_windowWidth; }
int Settings::windowHeight() const { return m_windowHeight; }
void Settings::setWindowSize(int width, int height) {
    m_windowWidth = width;
    m_windowHeight = height;
}

QString Settings::javaPath() const { return m_javaPath; }
void Settings::setJavaPath(const QString &path) { m_javaPath = path; }

QString Settings::gameDirectory() const { return m_gameDir; }
void Settings::setGameDirectory(const QString &path) { m_gameDir = path; }

void Settings::save() {
    QSettings s;
    s.setValue("game/minRam", m_minRam);
    s.setValue("game/maxRam", m_maxRam);
    s.setValue("game/width", m_windowWidth);
    s.setValue("game/height", m_windowHeight);
    s.setValue("game/javaPath", m_javaPath);
    s.setValue("game/gameDir", m_gameDir);
}

void Settings::load() {
    QSettings s;
    m_minRam = s.value("game/minRam", 1024).toInt();
    m_maxRam = s.value("game/maxRam", 4096).toInt();
    m_windowWidth = s.value("game/width", 854).toInt();
    m_windowHeight = s.value("game/height", 480).toInt();
    m_javaPath = s.value("game/javaPath", "java").toString();
    m_gameDir = s.value("game/gameDir", m_gameDir).toString();
}
