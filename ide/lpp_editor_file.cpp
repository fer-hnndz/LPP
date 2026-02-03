#include "lpp_editor_file.h"
#include <QFile>
#include <QTextStream>

LppEditorFile::LppEditorFile() :
    isModified(false),
    contents(QString::fromStdString("")),
    filePath(QString::fromStdString("")),
    tabIndex(-1)
{}
LppEditorFile::LppEditorFile(bool _isModified, QString _contents, QString _filePath, int _tabIndex) :
    isModified(_isModified),
    contents(_contents),
    filePath(_filePath),
    tabIndex(_tabIndex) {

}

void LppEditorFile::saveFile() {
    QFile file(filePath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        throw std::runtime_error(std::string("No se ha podido guardar el archivo en ") + filePath.toStdString());
    }

    QTextStream out(&file);
    out << contents;
    file.close();
}
