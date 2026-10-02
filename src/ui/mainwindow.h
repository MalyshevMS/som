#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "core/config/settings.h"
#include "core/launcher/versionmanager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void loadVersions();

private:
    Ui::MainWindow *ui;
    Settings m_settings;
    VersionManager m_versionManager;
};

#endif // MAINWINDOW_H
