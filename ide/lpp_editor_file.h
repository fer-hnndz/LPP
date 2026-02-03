#ifndef LPP_EDITOR_FILE_H
#define LPP_EDITOR_FILE_H

#include <QString>

class LppEditorFile {
private:
    bool isModified;
    QString contents;
    QString filePath;
    int tabIndex;

public:
    LppEditorFile();
    LppEditorFile(bool _isModified, QString _contents, QString _filePath, int _tabIndex);
    void saveFile();

    bool getIsModified() const;
    void setIsModified(bool newIsModified);
    QString getContents() const;
    void setContents(const QString &newContents);
    QString getFilePath() const;
    void setFilePath(const QString &newFilePath);
    int getTabIndex() const;
    void setTabIndex(int newTabIndex);
};

inline QString LppEditorFile::getContents() const
{
    return contents;
}

inline void LppEditorFile::setContents(const QString &newContents)
{
    contents = newContents;
}

inline QString LppEditorFile::getFilePath() const
{
    return filePath;
}

inline void LppEditorFile::setFilePath(const QString &newFilePath)
{
    filePath = newFilePath;
}

inline int LppEditorFile::getTabIndex() const
{
    return tabIndex;
}

inline void LppEditorFile::setTabIndex(int newTabIndex)
{
    tabIndex = newTabIndex;
}

inline bool LppEditorFile::getIsModified() const
{
    return isModified;
}

inline void LppEditorFile::setIsModified(bool newIsModified)
{
    isModified = newIsModified;
}

#endif // LPP_EDITOR_FILE_H
