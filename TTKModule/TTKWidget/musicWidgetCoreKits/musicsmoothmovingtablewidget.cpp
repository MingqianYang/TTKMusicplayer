#include "musicsmoothmovingtablewidget.h"
#include "musicwidgetheaders.h"

#include <QTimer>
#include <QWheelEvent>
#include <QPropertyAnimation>

MusicSmoothMovingTableWidget::MusicSmoothMovingTableWidget(QWidget *parent)
    : MusicAbstractTableWidget(parent)
{
    m_deltaValue = 0;
    m_previousValue = 0;
    m_isFirstInit = true;
    m_slowAnimation = nullptr;
    m_scrollBar = nullptr;
    m_animationTimer = new QTimer(this);
    m_animationTimer->setInterval(100*MT_MS);

    verticalScrollBar()->setStyleSheet(MusicUIObject::MScrollBarStyle03);

    connect(m_animationTimer, SIGNAL(timeout()), SLOT(timeToAnimation()));
}

MusicSmoothMovingTableWidget::~MusicSmoothMovingTableWidget()
{
    m_animationTimer->stop();
    delete m_animationTimer;
    delete m_slowAnimation;
}

void MusicSmoothMovingTableWidget::setMovedScrollBar(QScrollBar *bar)
{
    m_scrollBar = bar;
    delete m_slowAnimation;
    m_slowAnimation = new QPropertyAnimation(m_scrollBar, "value", this);
    m_slowAnimation->setDuration(MT_S2MS);
    connect(m_scrollBar, SIGNAL(valueChanged(int)), SLOT(valueChanged(int)));
}

void MusicSmoothMovingTableWidget::timeToAnimation()
{
    m_isFirstInit = true;
    m_animationTimer->stop();

    float delta = (rowCount() > 0) ? (height()*1.0/rowCount()) : 0;
    m_deltaValue = (m_deltaValue/480.0)*(m_deltaValue < 0 ? m_deltaValue + 120 : -m_deltaValue + 120);

    m_slowAnimation->setStartValue(m_previousValue);
    m_slowAnimation->setEndValue(m_scrollBar->value() + m_deltaValue*delta/30);
    m_slowAnimation->start();
}

void MusicSmoothMovingTableWidget::valueChanged(int value)
{
    m_previousValue = value;
}

void MusicSmoothMovingTableWidget::wheelEvent(QWheelEvent *event)
{
    MusicAbstractTableWidget::wheelEvent(event);

    if(!m_slowAnimation)
    {
        return;
    }

    m_animationTimer->stop();
    m_slowAnimation->stop();
    if(m_isFirstInit)
    {
        m_deltaValue = 0;
        m_previousValue = m_scrollBar->value();
        m_isFirstInit = false;
    }
    m_deltaValue += event->delta();
    m_animationTimer->start();
}
