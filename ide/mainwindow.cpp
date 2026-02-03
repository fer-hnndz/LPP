#include <QDebug>
#include <iostream>
#include <QFileDialog>
#include <QFileSystemModel>
#include <QMessageBox>
#include <QProcess>

#include "mainwindow.h"
#include "lpp_highlighter.h"
#include "lpp_conf.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(const LppConf& lpp_conf, QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      lpp_conf(lpp_conf),
      lppi_ctrl(lpp_conf)
{
    ui->setupUi(this);
    ui->edtSourceCode->setTabStopDistance(ui->edtSourceCode->fontMetrics().horizontalAdvance(' ')*4);
    highlighter = new LPPHighlighter(ui->edtSourceCode->document());
    last_dir = QDir::homePath();

    connect(&lppi_ctrl, &LppInterpCtrl::finished, this, &MainWindow::onProgramFinished);
    //connect(&lppi, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this, &MainWindow::onProgramFinished);
    connect(ui->edtSourceCode, &QPlainTextEdit::modificationChanged, this, &MainWindow::onModificationChanged);

    QFont fixed_font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    fixed_font.setStyleHint(QFont::TypeWriter);
    fixed_font.setPointSize(14);

    ui->edtSourceCode->setFont(fixed_font);

    // Leave only 1 tab open
    ui->editorTabs->removeTab(1);

    ui->editorTabs->setTabText(0, "Nuevo Programa");
}

MainWindow::~MainWindow()
{
    delete ui;
}

/**
 * Adds a new tab to the editor and saves its state in the openEditors map.
 * @brief MainWindow::addNewEditor
 * @param file
 */
void MainWindow::addNewEditor(QFile &file, QString &filePath)
{
    int realIndex = ui->editorTabs->count();

    QWidget *tabPage = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(tabPage);
    layout->setContentsMargins(0, 0, 0, 0);

    // Mover el editor al nuevo tab
    layout->addWidget(ui->edtSourceCode);

    LppEditorFile newEditor(
        false,
        QString::fromUtf8(file.readAll()),
        filePath,
        realIndex
        );

    QFileInfo info(filePath);
    ui->editorTabs->addTab(tabPage, info.fileName());

    openEditors[realIndex] = newEditor;

    // Activar el tab recién creado
    ui->editorTabs->setCurrentIndex(realIndex);
}


void MainWindow::updateEditorCode(QString& openedFile, std::string& contents)
{
    last_dir = QFileInfo(openedFile).dir().absolutePath();
    ui->edtSourceCode->setPlainText(QString::fromStdString(contents));

}

void MainWindow::updateExplorerTreeView(QString filePath)
{
    QFileSystemModel *model = new QFileSystemModel;
    QDir dir(filePath);

    dir.cdUp();
    QString explorerPath = dir.absolutePath();
    model->setRootPath(explorerPath);

    model->setNameFilters({ "*.lpp", "*.lppprj" });
    model->setNameFilterDisables(false);

    ui->tvExplorer->setModel(model);
    ui->tvExplorer->setRootIndex(model->index(explorerPath));

    // Run here since models need to be initialized for these functions to work.
    ui->tvExplorer->hideColumn(1); // Size
    ui->tvExplorer->hideColumn(2); // Type
    ui->tvExplorer->hideColumn(3); // Date
    ui->tvExplorer->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->tvExplorer->setHeaderHidden(true);
    ui->tvExplorer->setIndentation(16);
    ui->tvExplorer->setAnimated(true);
}

bool MainWindow::saveProgramIfModified()
{
    if (ui->edtSourceCode->document()->isModified()) {
        int res = QMessageBox::question(this, "Confirmacion", "El programa actual no ha sido guardado.<br/>"
                                                              "\u00bfDesea guardarlo antes de continuar?");

        if (res == QMessageBox::Yes) {
            on_actionGuardar_triggered();

            if (ui->edtSourceCode->document()->isModified()) {
                return false;
            }
        }
    }
    return true;
}

void MainWindow::reportErrorMessage(LppInterpResult lires)
{
    switch (lires) {
        case LppInterpResult::TerminalNotFound: {
            QMessageBox::critical(this, "Error", "El comando para ejecutar la terminal no existe.<br/>"
                                                 "Por favor especifique el comando en el archivo lpp.ini");
            break;
        }
        case LppInterpResult::InterpNotFound: {
            QMessageBox::critical(this, "Error", "No se encontr\u00f3 el ejecutable del int\u00e9rprete de LPP.<br/>"
                                                 "Por favor aseg\u00farese de haber copiado el int\u00e9rprete al directorio del IDE.");
            break;
        }
        case LppInterpResult::CannotStartInterp: {
            QMessageBox::critical(this, "Error", "No se pudo iniciar el interprete de LPP");
            break;
        }
        default:
            break;
    }
}

void MainWindow::on_actionSalir_triggered()
{
    if (!saveProgramIfModified()) {
        return;
    }

    qApp->quit();
}

void MainWindow::on_actionNuevoPrg_triggered()
{
    if (!saveProgramIfModified()) {
        return;
    }
    ui->statusbar->clearMessage();
    prg_filepath.clear();
    ui->edtSourceCode->document()->clear();
}

void MainWindow::on_actionAbrir_triggered()
{
    if (!saveProgramIfModified()) {
        return;
    }

    // Clear all tabs

    for (int i = 1; i < ui->editorTabs->count(); i++) {
        ui->editorTabs->removeTab(i);
    }

    openEditors.clear();

    QString filepath = QFileDialog::getOpenFileName(
        this,
        "Seleccione un programa/proyecto",
        last_dir,
        "Programas de LPP (*.lpp);;Todos los archivos (*)");

    if (!filepath.isEmpty()) {
        QFile file(filepath);

        // Only update Explorer view if a new project is selected.
        // Else, just add the new file to the editor but don't change the files/folders displayed in the tree view.
        if (ui->tvExplorer->model() == nullptr && !filepath.endsWith(".llpprj")) {

            QFileInfo info(filepath);

            ui->editorTabs->setTabText(0, info.fileName());
            updateExplorerTreeView(filepath);
        }

        addNewEditor(file, filepath);

        if (file.open(QFile::ReadOnly | QFile::Text)) {
            prg_filepath = filepath;
            std::string fileContents = file.readAll().toStdString();

            updateEditorCode(prg_filepath, fileContents);
            file.close();

            ui->edtSourceCode->document()->setModified(false);

        } else {
            QMessageBox::critical(this, "Error", "No se pudo abrir el archivo de programa");
        }
    }
}

void MainWindow::on_actionGuardar_triggered()
{
    if (!ui->edtSourceCode->document()->isModified()) {
        return;
    }

    if (prg_filepath.isEmpty()) {
        prg_filepath = QFileDialog::getSaveFileName(
            this,
            "Seleccione un archivo para guardar el programa ...",
            last_dir,
            "Programas de LPP (*.lpp);;Todos los archivos (*)");
    }

    if (!prg_filepath.isEmpty()) {
        QFile file(prg_filepath);

        if (file.open(QFile::WriteOnly | QFile::Truncate | QFile::Text)) {
            QString fileContents = ui->edtSourceCode->toPlainText();
            file.write(fileContents.toUtf8());

            ui->edtSourceCode->document()->setModified(false);
            openEditors[ui->editorTabs->currentIndex()].setContents(fileContents);
            openEditors[ui->editorTabs->currentIndex()].setIsModified(false);

        } else {
            QMessageBox::critical(this, "Error", "No se pudo guardar el programa");
        }
    }
}

void MainWindow::onProgramFinished(int action)
{
    ui->actionEjecutarPrg->setEnabled(true);
    ui->actionCompilarPrg->setEnabled(true);
    ui->actionDetenerPrg->setEnabled(false);

    if (action == 0) {
        if (!lppi_ctrl.lastError().isEmpty()) {
            QRegularExpression lre("^Linea[ ]+([0-9]+):");
            QString last_err = lppi_ctrl.lastError();
            QRegularExpressionMatch match = lre.match(last_err);
            if (match.hasMatch()) {
                int line_number = match.captured(1).toInt();
                ui->edtSourceCode->gotoLine(line_number);
            }
            QMessageBox::critical(this, "Error", lppi_ctrl.lastError());
        } else {
            QMessageBox::information(this, "Info", "El programa compila con \u00e9xito");
        }
    } else if (action == 1) {
        if (!lppi_ctrl.lastError().isEmpty()) {
             QMessageBox::critical(this, "Error", lppi_ctrl.lastError());
        }
    }
}

void MainWindow::onModificationChanged(bool change)
{
    if (change) {
        statusBar()->showMessage("**" + prg_filepath);
    } else {
        statusBar()->showMessage(prg_filepath);
    }
}

void MainWindow::on_actionDetenerPrg_triggered()
{
    lppi_ctrl.killProcess();
}

void MainWindow::on_actionEjecutarPrg_triggered()
{
    if (ui->edtSourceCode->document()->isModified() || prg_filepath.isEmpty()) {
        QMessageBox::critical(this, "Error", "Debe guardar el programa actual antes de ejecutarlo");
        return;
    }

    ui->actionEjecutarPrg->setEnabled(false);
    ui->actionCompilarPrg->setEnabled(false);
    ui->actionDetenerPrg->setEnabled(true);

    LppInterpResult lires = lppi_ctrl.execProgram(prg_filepath);
    if (lires == LppInterpResult::Success) {
        return;
    }

    ui->actionEjecutarPrg->setEnabled(true);
    ui->actionCompilarPrg->setEnabled(true);
    ui->actionDetenerPrg->setEnabled(false);

    reportErrorMessage(lires);
}

void MainWindow::on_actionCompilarPrg_triggered()
{
    if (ui->edtSourceCode->document()->isModified() || prg_filepath.isEmpty()) {
        QMessageBox::critical(this, "Error", "Debe guardar el programa actual antes de ejecutarlo");
        return;
    }

    ui->actionEjecutarPrg->setEnabled(false);
    ui->actionCompilarPrg->setEnabled(false);
    ui->actionDetenerPrg->setEnabled(true);

    LppInterpResult lires = lppi_ctrl.compileProgram(prg_filepath);
    if (lires == LppInterpResult::Success) {
        return;
    }

    ui->actionEjecutarPrg->setEnabled(true);
    ui->actionCompilarPrg->setEnabled(true);
    ui->actionDetenerPrg->setEnabled(false);

    reportErrorMessage(lires);
}


/**
 * Triggers when an item on the explorer's tree view was double clicked
 * @brief MainWindow::on_tvExplorer_doubleClicked
 * @param index
 */
void MainWindow::on_tvExplorer_doubleClicked(const QModelIndex &index)
{
    auto *model = qobject_cast<QFileSystemModel*>(ui->tvExplorer->model());
    if (!model) {
        return;
    }

    QString filePath = model->filePath(index);

    qDebug() << "Double clicked element in tree view.\n";
    // Ignorar directorios
    if (model->isDir(index) || (!filePath.endsWith(".lpp") && !filePath.endsWith(".lppprj"))) {
        qDebug() << "No apto para abrir :c\n";
        return;
    }

    QFile file(filePath);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::critical(this, "Error",
                              "No se pudo abrir el archivo");
        return;
    }

    addNewEditor(file, filePath);
    file.close();
}


/**
 * Moves the editor widget (ui->edtSourceCode) to the new tab, saves old file state in its corresponsing object and updates source code with the new tab's state
 * @brief MainWindow::on_editorTabs_currentChanged
 * @param index
 */
void MainWindow::on_editorTabs_currentChanged(int index)
{
    QWidget *tab = ui->editorTabs->widget(index);

    auto *tabLayout = tab->layout();
    tabLayout->addWidget(ui->edtSourceCode);

    std::string fileContents = openEditors[index].getContents().toStdString();
    QString filePath = openEditors[index].getFilePath();
    updateEditorCode(filePath, fileContents);
}


/**
 * Save current editor state before changing tab
 * @brief MainWindow::on_editorTabs_tabBarClicked
 * @param index
 */
void MainWindow::on_editorTabs_tabBarClicked(int _)
{
    int currentIndex = ui->editorTabs->currentIndex();
    openEditors[currentIndex].setContents(ui->edtSourceCode->toPlainText());
    openEditors[currentIndex].setIsModified(ui->edtSourceCode->document()->isModified());
}

