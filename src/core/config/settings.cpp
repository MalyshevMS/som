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

bool Settings::showSnapshots() const { return m_showSnapshots; }
void Settings::setShowSnapshots(bool show) { m_showSnapshots = show; }

bool Settings::showBetas() const { return m_showBetas; }
void Settings::setShowBetas(bool show) { m_showBetas = show; }

bool Settings::showAlphas() const { return m_showAlphas; }
void Settings::setShowAlphas(bool show) { m_showAlphas = show; }

QString Settings::selectedVersion() const { return m_selectedVersion; }
void Settings::setSelectedVersion(const QString &ver) { m_selectedVersion = ver; }

void Settings::save() {
    QSettings s;
    s.setValue("game/minRam", m_minRam);
    s.setValue("game/maxRam", m_maxRam);
    s.setValue("game/width", m_windowWidth);
    s.setValue("game/height", m_windowHeight);
    s.setValue("game/javaPath", m_javaPath);
    s.setValue("game/gameDir", m_gameDir);
    s.setValue("game/showSnapshots", m_showSnapshots);
    s.setValue("game/showBetas", m_showBetas);
    s.setValue("game/showAlphas", m_showAlphas);
    s.setValue("game/selectedVersion", m_selectedVersion);
}

void Settings::load() {
    QSettings s;
    m_minRam = s.value("game/minRam", 1024).toInt();
    m_maxRam = s.value("game/maxRam", 4096).toInt();
    m_windowWidth = s.value("game/width", 854).toInt();
    m_windowHeight = s.value("game/height", 480).toInt();
    m_javaPath = s.value("game/javaPath", "java").toString();
    m_gameDir = s.value("game/gameDir", m_gameDir).toString();
    m_showSnapshots = s.value("game/showSnapshots", false).toBool();
    m_showBetas = s.value("game/showBetas", false).toBool();
    m_showAlphas = s.value("game/showAlphas", false).toBool();
    m_selectedVersion = s.value("game/selectedVersion", "").toString();
}
