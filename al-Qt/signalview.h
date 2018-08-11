#ifndef SIGNALVIEW_H
#define SIGNALVIEW_H

#include <QWidget>
#include <vector>

class SignalView : public QWidget
{
    Q_OBJECT

    const quint32 dim = 2500;
    std::vector<float> top = std::vector<float>(dim);
    std::vector<float> bottom = std::vector<float>(dim);

    quint32 m_nch = 0;
    qint32 m_dur = 0;

public:
    explicit SignalView(QWidget *parent = nullptr);
    void setData(std::vector<float> &src, quint32 nch, qint32 dur);

protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;

signals:

public slots:
};

#endif // SIGNALVIEW_H
