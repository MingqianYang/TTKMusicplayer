#ifndef MUSICFILEINFORMATIONWIDGET_H
#define MUSICFILEINFORMATIONWIDGET_H

/* =================================================
 * This file is part of the TTK Music Player project
 * Copyright (c) 2014 - 2016 Greedysky Studio
 * All rights reserved!
 * Redistribution and use of the source code or any derivative
 * works are strictly forbiden.
   =================================================*/

#include <QDialog>
#include <QLineEdit>
#include <QMouseEvent>
#include "musicabstractmovedialog.h"

namespace Ui {
class MusicFileInformationWidget;
}

/*! @brief The class of the modify lineEdit.
 * @author Greedysky <greedysky@163.com>
 */
class MUSIC_TOOLSET_EXPORT MusicModifyLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit MusicModifyLineEdit(QWidget *parent = 0);
    /*!
     * Object contsructor.
     */
    virtual ~MusicModifyLineEdit();

    bool getTextEdited() const { return m_isTextEdited;}
    /*!
     * Get text edit state.
     */

private Q_SLOTS:
    void isTextEdited();
    /*!
     * Current text edit state changed.
     */

protected:
    virtual void leaveEvent(QEvent *event) override;
    virtual void mouseDoubleClickEvent(QMouseEvent *event) override;
    /*!
     * Override the widget event.
     */

    bool m_isTextEdited;

};


/*! @brief The class of the file information widget.
 * @author Greedysky <greedysky@163.com>
 */
class MUSIC_TOOLSET_EXPORT MusicFileInformationWidget : public MusicAbstractMoveDialog
{
    Q_OBJECT
public:
    explicit MusicFileInformationWidget(QWidget *parent = 0);
    /*!
     * Object contsructor.
     */
    virtual ~MusicFileInformationWidget();

    void setFileInformation(const QString &name);
    /*!
     * Set music file song path name.
     */

public Q_SLOTS:
    void musicOpenFileDir();
    /*!
     * Open the music at local path.
     */
    virtual int exec();
    /*!
     * Override exec function.
     */

protected:
    void saveModifyData();
    /*!
     * Save modify data to local file.
     */

    Ui::MusicFileInformationWidget *ui;
    QString m_path;

};

#endif // MUSICFILEINFORMATIONWIDGET_H
