/* =================================================
 * This file is part of the TTK qmmp plugin project
 * Copyright (C) 2015 - 2020 Greedysky Studio

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; If not, see <http://www.gnu.org/licenses/>.
 ================================================= */

#ifndef MOUNTAINWIDGET_H
#define MOUNTAINWIDGET_H

#include <QGLWidget>
#if QT_VERSION >= 0x050400
#ifdef Q_OS_UNIX
  #include <QOpenGLWidget>
  #define QT_OPENGL_WIDGET
#endif
#endif

#define NUM_BANDS 32
#define DB_RANGE 40
#define BAR_SPACING (3.2f / NUM_BANDS)
#define BAR_WIDTH   (0.8f * BAR_SPACING)

/*!
 * @author Greedysky <greedysky@163.com>
 */
#ifdef QT_OPENGL_WIDGET
class MountainWidget : public QOpenGLWidget
#else
class MountainWidget : public QGLWidget
#endif
{
    Q_OBJECT
public:
    explicit MountainWidget(QWidget *parent = nullptr);
    virtual ~MountainWidget();

    void addBuffer(float *left);

protected:
    virtual void initializeGL() override;
    virtual void resizeGL(int width, int height) override;
    virtual void paintGL() override;

    void makeLogGraph(const float * freq, float * graph);
    void drawRectangle(float x1, float y1, float z1, float x2, float y2, float z2, float r, float g, float b);
    void drawBar(float x, float z, float h, float r, float g, float b);
    void drawBars();

private:
    float m_logScale[NUM_BANDS + 1];
    float m_colors[NUM_BANDS][NUM_BANDS][3];

    int m_pos;
    float m_angle, m_angleSpeed;
    float m_bars[NUM_BANDS][NUM_BANDS];

};

#endif
