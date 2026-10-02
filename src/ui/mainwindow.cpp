#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow) {
    ui->setupUi(this);
    setWindowTitle("SOM Launcher");

    // Загрузка сохранённых настроек
    ui->spinMinRam->setValue(m_settings.minRamMb());
    ui->spinMaxRam->setValue(m_settings.maxRamMb());
    ui->spinWidth->setValue(m_settings.windowWidth());
    ui->spinHeight->setValue(m_settings.windowHeight());
    ui->editJavaPath->setText(m_settings.javaPath());
    ui->editGameDir->setText(m_settings.gameDirectory());

    connect(ui->btnBrowseJava, &QPushButton::clicked, this, [this]() {
        QString path = QFileDialog::getOpenFileName(this, "Выберите файл Java");
        if (!path.isEmpty()) ui->editJavaPath->setText(path);
    });

    connect(ui->btnBrowseGameDir, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Выберите папку игры");
        if (!dir.isEmpty()) ui->editGameDir->setText(dir);
    });

    connect(ui->btnSaveSettings, &QPushButton::clicked, this, [this]() {
        m_settings.setMinRamMb(ui->spinMinRam->value());
        m_settings.setMaxRamMb(ui->spinMaxRam->value());
        m_settings.setWindowSize(ui->spinWidth->value(), ui->spinHeight->value());
        m_settings.setJavaPath(ui->editJavaPath->text());
        m_settings.setGameDirectory(ui->editGameDir->text());
        m_settings.save();
        QMessageBox::information(this, "Настройки", "Настройки сохранены!");
    });

    connect(&m_versionManager, &VersionManager::manifestLoaded, this, [this](const QList<VersionInfo> &versions) {
        ui->comboVersions->clear();
        for (const auto &ver : versions) {
            if (ver.type == "release") {
                ui->comboVersions->addItem(ver.id, ver.url);
            } else {
                ui->comboVersions->addItem(QString("%1 (%2)").arg(ver.id, ver.type), ver.url);
            }
        }
        ui->lblStatus->setText("Список версий обновлен");
        ui->btnRefreshVersions->setEnabled(true);
    });

    connect(&m_versionManager, &VersionManager::errorOccurred, this, [this](const QString &err) {
        ui->lblStatus->setText("Ошибка: " + err);
        ui->btnRefreshVersions->setEnabled(true);
    });

    connect(ui->btnRefreshVersions, &QPushButton::clicked, this, &MainWindow::loadVersions);

    loadVersions();
}

void MainWindow::loadVersions() {
    ui->lblStatus->setText("Загрузка списка версий...");
    ui->btnRefreshVersions->setEnabled(false);
    m_versionManager.fetchVersionManifest();
}

MainWindow::~MainWindow() {
    delete ui;
}
