#pragma once

#include <QColor>
#include <QRectF>
#include <QString>
#include <QWidget>
#include <vector>

namespace Telemetry {
struct DataPoint;
}

class TelemetryPlotWidget : public QWidget {
    Q_OBJECT

public:
    explicit TelemetryPlotWidget(const QString& title = QString(), QWidget* parent = nullptr);

    void setSeries(const std::vector<std::vector<Telemetry::DataPoint>>& series);
    void clearSeries();
    void setTitle(const QString& title);
    void resetView();
    void replot();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    QRectF plotArea() const;
    QPointF mapPoint(const Telemetry::DataPoint& source) const;
    void updateViewRange();
    void ensureValidView();

    std::vector<std::vector<Telemetry::DataPoint>> m_series;
    QString m_title;
    QRectF m_viewRect;
    QPoint m_lastMousePos;
    bool m_panning{false};
    std::vector<QColor> m_colors;
};
