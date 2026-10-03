#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "core/launcher/gamelauncher.h"
#include <QFileDialog>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_versionManager(m_settings)
    , ui(new Ui::MainWindow) {
    ui->setupUi(this);
    setWindowTitle("SOM Launcher");

    ui->spinMinRam->setValue(m_settings.minRamMb());
    ui->spinMaxRam->setValue(m_settings.maxRamMb());
    ui->spinWidth->setValue(m_settings.windowWidth());
    ui->spinHeight->setValue(m_settings.windowHeight());
    ui->editJavaPath->setText(m_settings.javaPath());
    ui->editGameDir->setText(m_settings.gameDirectory());

    ui->chkShowSnapshots->setChecked(m_settings.showSnapshots());
    ui->chkShowBetas->setChecked(m_settings.showBetas());
    ui->chkShowAlphas->setChecked(m_settings.showAlphas());
    ui->chkShowFabric->setChecked(m_settings.showFabric());
    ui->chkShowForge->setChecked(m_settings.showForge());
    ui->chkShowNeoForge->setChecked(m_settings.showNeoForge());

    auto onFilterToggled = [this]() {
        m_settings.setShowSnapshots(ui->chkShowSnapshots->isChecked());
        m_settings.setShowBetas(ui->chkShowBetas->isChecked());
        m_settings.setShowAlphas(ui->chkShowAlphas->isChecked());
        m_settings.setShowFabric(ui->chkShowFabric->isChecked());
        m_settings.setShowForge(ui->chkShowForge->isChecked());
        m_settings.setShowNeoForge(ui->chkShowNeoForge->isChecked());
        m_settings.save();

        loadVersions();
    };

    connect(ui->chkShowSnapshots, &QCheckBox::toggled, this, onFilterToggled);
    connect(ui->chkShowBetas, &QCheckBox::toggled, this, onFilterToggled);
    connect(ui->chkShowAlphas, &QCheckBox::toggled, this, onFilterToggled);
    connect(ui->chkShowFabric, &QCheckBox::toggled, this, onFilterToggled);
    connect(ui->chkShowForge, &QCheckBox::toggled, this, onFilterToggled);
    connect(ui->chkShowNeoForge, &QCheckBox::toggled, this, onFilterToggled);

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

    connect(&m_versionManager, &VersionManager::versionsLoaded, this, [this](const QList<VersionInfo> &versions) {
        QString previousSelection = ui->comboVersions->currentData().toString();
        if (previousSelection.isEmpty()) {
            previousSelection = m_settings.selectedVersion();
        }

        ui->comboVersions->clear();
        for (const auto &ver : versions) {
            bool show = false;

            if (ver.type == "release") {
                show = true;
            } else if (ver.type == "snapshot" && ui->chkShowSnapshots->isChecked()) {
                show = true;
            } else if (ver.type == "old_beta" && ui->chkShowBetas->isChecked()) {
                show = true;
            } else if (ver.type == "old_alpha" && ui->chkShowAlphas->isChecked()) {
                show = true;
            } else if (ver.type == "fabric" && ui->chkShowFabric->isChecked()) {
                show = true;
            } else if (ver.type == "forge" && ui->chkShowForge->isChecked()) {
                show = true;
            } else if (ver.type == "neoforge" && ui->chkShowNeoForge->isChecked()) {
                show = true;
            }

            if (show) {
                QString label = (ver.type == "release") ? ver.id : QString("%1 (%2)").arg(ver.id, ver.type);
                ui->comboVersions->addItem(label, ver.id);
                ui->comboVersions->setItemData(ui->comboVersions->count() - 1, ver.url, Qt::UserRole + 1);
            }
        }

        int index = ui->comboVersions->findData(previousSelection);
        if (index != -1) {
            ui->comboVersions->setCurrentIndex(index);
        }

        ui->lblStatus->setText("Список версий обновлен");
        ui->btnRefreshVersions->setEnabled(true);
    });

    connect(&m_versionManager, &VersionManager::errorOccurred, this, [this](const QString &err) {
        ui->lblStatus->setText("Ошибка: " + err);
        ui->btnRefreshVersions->setEnabled(true);
    });

    connect(ui->btnRefreshVersions, &QPushButton::clicked, this, &MainWindow::loadVersions);

    connect(ui->comboVersions, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (index >= 0) {
            m_settings.setSelectedVersion(ui->comboVersions->currentData().toString());
            m_settings.save();
        }
    });

    connect(ui->btnLaunch, &QPushButton::clicked, this, [this]() {
        int index = ui->comboVersions->currentIndex();
        if (index < 0) return;

        QString versionId = ui->comboVersions->currentData().toString();
        QString versionUrl = ui->comboVersions->itemData(index, Qt::UserRole + 1).toString();
        QString username = ui->editUsername->text();

        auto *launcher = new GameLauncher(m_settings, this);

        connect(launcher, &GameLauncher::statusChanged, this, [this](const QString &st) {
            ui->lblStatus->setText(st);
        });

        connect(launcher, &GameLauncher::progressChanged, this, [this](int curr, int total) {
            ui->progressBar->setVisible(true);
            if (total > 0) {
                ui->progressBar->setMinimum(0);
                ui->progressBar->setMaximum(total);
                ui->progressBar->setValue(curr);
            } else {
                ui->progressBar->setMinimum(0);
                ui->progressBar->setMaximum(0);
            }
        });

        connect(launcher, &GameLauncher::gameStarted, this, [this]() {
            ui->lblStatus->setText("Игра запущена!");
            ui->progressBar->setVisible(false);
            ui->btnLaunch->setEnabled(true);
        });

        connect(launcher, &GameLauncher::gameExited, this, [this](int code) {
            ui->lblStatus->setText(QString("Игра завершена (код %1)").arg(code));
            ui->btnLaunch->setEnabled(true);
        });

        connect(launcher, &GameLauncher::errorOccurred, this, [this](const QString &err) {
            ui->lblStatus->setText("Ошибка: " + err);
            ui->progressBar->setVisible(false);
            ui->btnLaunch->setEnabled(true);
        });

        ui->btnLaunch->setEnabled(false);
        launcher->launch(versionId, versionUrl, username);
    });

    loadVersions();
}

void MainWindow::loadVersions() {
    ui->lblStatus->setText("Загрузка списка версий...");
    ui->btnRefreshVersions->setEnabled(false);
    m_versionManager.fetchVersions();
}

MainWindow::~MainWindow() {
    delete ui;
}
