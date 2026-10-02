#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow) {
    ui->setupUi(this);
    setWindowTitle("SOM Launcher");

    ui->spinMinRam->setValue(m_settings.minRamMb());
    ui->spinMaxRam->setValue(m_settings.maxRamMb());
    ui->spinWidth->setValue(m_settings.windowWidth());
    ui->spinHeight->setValue(m_settings.windowHeight());
    ui->editJavaPath->setText(m_settings.javaPath());
    ui->editGameDir->setText(m_settings.gameDirectory());

    connect(ui->btnBrowseJava, &QPushButton::clicked, this, [this]() {
        QString path = QFileDialog::getOpenFileName(this, "Выберите исполняемый файл Java");
        if (!path.isEmpty()) {
            ui->editJavaPath->setText(path);
        }
    });

    connect(ui->btnBrowseGameDir, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Выберите папку игры");
        if (!dir.isEmpty()) {
            ui->editGameDir->setText(dir);
        }
    });

    connect(ui->btnSaveSettings, &QPushButton::clicked, this, [this]() {
        m_settings.setMinRamMb(ui->spinMinRam->value());
        m_settings.setMaxRamMb(ui->spinMaxRam->value());
        m_settings.setWindowSize(ui->spinWidth->value(), ui->spinHeight->value());
        m_settings.setJavaPath(ui->editJavaPath->text());
        m_settings.setGameDirectory(ui->editGameDir->text());
        m_settings.save();

        QMessageBox::information(this, "Настройки", "Настройки успешно сохранены!");
    });
}

MainWindow::~MainWindow() {
    delete ui;
}
