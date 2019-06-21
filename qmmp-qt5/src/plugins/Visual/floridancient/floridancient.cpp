#include <QPainter>
#include "fft.h"
#include "inlines.h"
#include "floridancient.h"

#include <QPropertyAnimation>

#define POINT_SIZE          8
#define ANIMATION_SIZE      100
#define ANIMATION_DURATION  10000

AncientLabel::AncientLabel(QWidget *parent)
    : QWidget(parent)
{
    m_color = QColor(255, 255, 255);
    m_opacity = 1;
    m_pos = QPoint(0, 0);
    m_size = POINT_SIZE;

    m_posAnimation = new QPropertyAnimation(this, QByteArray(), this);
    m_posAnimation->setDuration(ANIMATION_DURATION);
    m_posAnimation->setEasingCurve(QEasingCurve::InOutSine);

    connect(m_posAnimation, SIGNAL(finished()), SLOT(finished()));
    connect(m_posAnimation, SIGNAL(valueChanged(QVariant)), SLOT(posValueChanged(QVariant)));
}

AncientLabel::~AncientLabel()
{
    delete m_posAnimation;
}

void AncientLabel::start()
{
    QTimer::singleShot(qrand() % ANIMATION_DURATION, this, [this](){
        const QPoint &center = rect().center();
        int pos_x = qrand() % (DISTANCE * 2), pos_y = qrand() % (DISTANCE * 2);
        if(pos_x % 2 == 0)
        {
            pos_x = -pos_x;
        }
        if(pos_y % 2 == 0)
        {
            pos_y = -pos_y;
        }

        m_posAnimation->setStartValue(center);
        m_posAnimation->setEndValue(center + QPoint(DISTANCE / 2 + pos_x, DISTANCE / 2 + pos_y));
        m_posAnimation->start();
    });
}

void AncientLabel::setColor(const QColor &color)
{
    m_color = color;
}

void AncientLabel::finished()
{
    start();
}

void AncientLabel::posValueChanged(const QVariant &value)
{
    m_pos = value.toPoint();

    const QPoint &startPoint = m_posAnimation->startValue().toPoint();
    const QPoint &endPoint = m_posAnimation->endValue().toPoint();
    const int totalLength = sqrt(pow(startPoint.x() - endPoint.x(), 2) + pow(startPoint.y() - endPoint.y(), 2));
    const int currentLength = sqrt(pow(startPoint.x() - m_pos.x(), 2) + pow(startPoint.y() - m_pos.y(), 2));
    const float delta = (totalLength - currentLength) * 1.0 / totalLength;

    m_size = POINT_SIZE * delta;
    m_opacity = delta;

    update();
}

void AncientLabel::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    QPainter painter(this);
    painter.setOpacity(m_opacity);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(m_color);
    painter.drawEllipse(m_pos, m_size, m_size);
}



FloridAncient::FloridAncient (QWidget *parent) : Florid (parent)
{
    m_intern_vis_data = nullptr;
    m_x_scale = nullptr;
    m_running = false;
    m_rows = 0;
    m_cols = 0;

    setWindowTitle(tr("Florid Ancient Widget"));
    m_timer = new QTimer(this);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(timeout()));

    m_analyzer_falloff = 1.2;
    m_timer->setInterval(QMMP_VISUAL_INTERVAL);
    m_cell_size = QSize(6, 2);

    clear();

    for(int i=0; i<ANIMATION_SIZE; ++i)
    {
        AncientLabel *label = new AncientLabel(this);
        label->setGeometry(0, 0, width(), height());
        m_labels << label;
    }
}

FloridAncient::~FloridAncient()
{
    if(m_intern_vis_data)
    {
        delete[] m_intern_vis_data;
    }
    if(m_x_scale)
    {
        delete[] m_x_scale;
    }
}

void FloridAncient::start()
{
    Florid::start();
    m_running = true;
    if(isVisible())
    {
        m_timer->start();
    }

    for(int i=0; i<m_labels.size(); ++i)
    {
        AncientLabel *label = m_labels[i];
        label->setColor(m_averageColor);
        label->start();
    }
}

void FloridAncient::stop()
{
    Florid::stop();
    m_running = false;
    m_timer->stop();
    clear();
}

void FloridAncient::clear()
{
    m_rows = 0;
    m_cols = 0;
    update();
}

void FloridAncient::timeout()
{
    if(takeData(m_left_buffer, m_right_buffer))
    {
        Florid::start();
        process();
        update();
    }
    else
    {
        Florid::stop();
    }
}

void FloridAncient::hideEvent(QHideEvent *)
{
    m_timer->stop();
}

void FloridAncient::showEvent(QShowEvent *)
{
    if(m_running)
    {
        m_timer->start();
    }
}

void FloridAncient::paintEvent(QPaintEvent *e)
{
    Florid::paintEvent(e);
    QPainter painter(this);
    draw(&painter);
    paintRoundLabel();
}

void FloridAncient::process()
{
    static fft_state *state = nullptr;
    if(!state)
    {
        state = fft_init();
    }

    const int rows = (height() - 2) / m_cell_size.height();
    const int cols = (width() - 2) / m_cell_size.width();

    if(m_rows != rows || m_cols != cols)
    {
        m_rows = rows;
        m_cols = cols;
        if(m_intern_vis_data)
        {
            delete[] m_intern_vis_data;
        }
        if(m_x_scale)
        {
            delete[] m_x_scale;
        }
        m_intern_vis_data = new double[m_cols];
        m_x_scale = new int[m_cols + 1];

        for(int i = 0; i < m_cols; ++i)
        {
            m_intern_vis_data[i] = 0;
        }
        for(int i = 0; i < m_cols + 1; ++i)
        {
            m_x_scale[i] = pow(pow(255.0, 1.0 / m_cols), i);
        }
    }

    short dest[256];
    short y;
    int k, magnitude;

    calc_freq (dest, m_left_buffer);

    const double y_scale = (double) 1.25 * m_rows / log(256);

    for(int i = 0; i < m_cols; i++)
    {
        y = 0;
        magnitude = 0;

        if(m_x_scale[i] == m_x_scale[i + 1])
        {
            y = dest[i];
        }
        for(k = m_x_scale[i]; k < m_x_scale[i + 1]; k++)
        {
            y = qMax(dest[k], y);
        }

        y >>= 7; //256

        if(y)
        {
            magnitude = int(log (y) * y_scale);
            magnitude = qBound(0, magnitude, m_rows);
        }

        m_intern_vis_data[i] -= m_analyzer_falloff * m_rows / 15;
        m_intern_vis_data[i] = magnitude > m_intern_vis_data[i] ? magnitude : m_intern_vis_data[i];
    }
}

void FloridAncient::draw(QPainter *p)
{
    if(m_cols == 0)
    {
        return;
    }

    p->save();
    p->setRenderHints(QPainter::Antialiasing);
    p->setPen(QPen(m_averageColor, 3));
    p->translate(rect().center());

    qreal startAngle = 0;
    for(int i = 0; i < m_cols; ++i)
    {
        p->save();
        p->rotate(startAngle);
        const int value = m_intern_vis_data[int(i * 0.8)];
        p->drawLine(0, DISTANCE + 5 + value * 0.2, 0.5, DISTANCE + 5 + value * 0.3);

        p->restore();
        startAngle += 360.0 / m_cols;
    }
    p->restore();
}
