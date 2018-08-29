#include "signalview.h"
#include <QPainter>

SignalView::SignalView(QWidget *parent) : QWidget(parent)
{

}



void SignalView::setData(std::vector<short> &src, quint32 nch, qint32 dur)
{
    std::vector<float> ff;
    ff.assign(src.begin(), src.end());
    m_range = 32768.0;
    setData(ff, nch, dur);
}

void SignalView::setData(std::vector<float> &src, quint32 nch, qint32 dur)
{
    m_nch = nch;
    if (!m_nch) return;
    top.resize(dim * m_nch);
    bottom.resize(dim * m_nch);
    quint32 sz = src.size() / m_nch;
    float scale = (float)dim / sz;
    m_dur = dur > 0 ? dur * scale : -1;
    for (int ch = 0; ch < m_nch; ch++)
    {
        int idx = -1;
        float mn = 1000, mx = -1000;
        for (int i = 0; i < sz; ++i)
        {
            int x = i * scale;
            if (x != idx)
            {
                if (idx >= 0)
                {
                    int idx2 = idx + dim * ch;
                    top[idx2] = mx;
                    bottom[idx2] = mn;
                }
                idx = x;
                mn = 1000;
                mx = -1000;
            }
            float v = src[i * m_nch + ch];
            if (v < mn) mn = v;
            if (v > mx) mx = v;
        }
    }
}

void SignalView::paintEvent(QPaintEvent *event)
{
    if (!m_nch) return;

    QRect rrect(0, 0, width() - 1, height() / m_nch - 1);
    QPainter painter(this);
    painter.setPen(Qt::black);
    float scx = (float)width() / dim;
    float scy = (float)height() / m_nch / 2 / m_range;
    for (quint32 ch = 0; ch < m_nch; ch++)
    {
        painter.drawRect(rrect);
        int idx = dim * ch;
        int mid = height() / m_nch * (ch + 0.5);
        for (int i = 0; i < dim; i++, idx++)
        {
            int x = scx * i;
            float y1 = top[idx] * scy + mid;
            float y2 = bottom[idx] * scy + mid;
            painter.drawLine(x, y1, x, y2);
        }
        rrect.translate(0, rrect.height() + 1);
    }

    if (m_dur > 0)
    {
        int mark = scx * m_dur;
        painter.drawLine(mark, 0, mark, height());
    }
}
