#ifndef SETTINGS_H
#define SETTINGS_H

#include <QString>
#include <QDir>

class Settings {
public:
    Settings();

    int minRamMb() const;
    void setMinRamMb(int mb);

    int maxRamMb() const;
    void setMaxRamMb(int mb);

    int windowWidth() const;
    int windowHeight() const;
    void setWindowSize(int width, int height);

    QString javaPath() const;
    void setJavaPath(const QString &path);

    QString gameDirectory() const;
    void setGameDirectory(const QString &path);

    void save();
    void load();

private:
    int m_minRam = 1024;
    int m_maxRam = 4096;
    int m_windowWidth = 854;
    int m_windowHeight = 480;
    QString m_javaPath;
    QString m_gameDir;
};

#endif // SETTINGS_H
