#include "telemetry_plot_widget.hpp"
#include "telemetry_record.hpp"
#include "downsampler.hpp"

#include <algorithm>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPen>

TelemetryPlotWidget::TelemetryPlotWidget(const QString& title, QWidget* parent)
    : QWidget(parent)
    , m_title(title)
    , m_colors({QColor(0, 122, 204), QColor(204, 0, 0), QColor(0, 153, 76), QColor(128, 0, 128), QColor(255, 153, 0)}) {
    setMinimumHeight(180);
    resetView();
}

void TelemetryPlotWidget::setSeries(const std::vector<std::vector<Telemetry::DataPoint>>& series) {
    m_series = series;
    updateViewRange();
    update();
}

void TelemetryPlotWidget::clearSeries() {
    m_series.clear();
    resetView();
    update();
}

void TelemetryPlotWidget::setTitle(const QString& title) {
    m_title = title;
    update();
}

void TelemetryPlotWidget::resetView() {
    m_viewRect = QRectF(0.0, 0.0, 1.0, 1.0);
    updateViewRange();
}

void TelemetryPlotWidget::replot() {
    update();
}

void TelemetryPlotWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(rect(), Qt::white);

    const QRectF plot = plotArea();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(245, 245, 245));
    painter.drawRect(plot);

    painter.setPen(QPen(Qt::lightGray, 1));
    for (int i = 0; i <= 4; ++i) {
        const qreal y = plot.top() + i * plot.height() / 4.0;
        painter.drawLine(plot.left(), y, plot.right(), y);
    }

    painter.setPen(QPen(Qt::lightGray, 1));
    for (int i = 0; i <= 4; ++i) {
        const qreal x = plot.left() + i * plot.width() / 4.0;
        painter.drawLine(x, plot.top(), x, plot.bottom());
    }

    painter.setPen(QPen(Qt::black, 1));
    painter.drawText(QRectF(rect().left() + 10, rect().top() + 4, rect().width() - 20, 18), Qt::AlignLeft | Qt::AlignVCenter, m_title);

    if (m_series.empty()) {
        return;
    }

    for (size_t seriesIndex = 0; seriesIndex < m_series.size(); ++seriesIndex) {
        const auto& points = m_series[seriesIndex];
        if (points.size() < 2) {
            continue;
        }

        QPainterPath path;
        path.moveTo(mapPoint(points[0]));
        for (size_t idx = 1; idx < points.size(); ++idx) {
            path.lineTo(mapPoint(points[idx]));
        }

        const QColor lineColor = m_colors[seriesIndex % m_colors.size()];
        painter.setPen(QPen(lineColor, 2));
        painter.drawPath(path);
    }
}

void TelemetryPlotWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_lastMousePos = event->pos();
        m_panning = true;
        setCursor(Qt::ClosedHandCursor);
    }
    QWidget::mousePressEvent(event);
}

void TelemetryPlotWidget::mouseMoveEvent(QMouseEvent* event) {
    if (m_panning) {
        const QPoint delta = event->pos() - m_lastMousePos;
        const QRectF plot = plotArea();
        if (!plot.isEmpty()) {
            const qreal dx = -delta.x() * m_viewRect.width() / plot.width();
            const qreal dy = delta.y() * m_viewRect.height() / plot.height();
            m_viewRect.translate(dx, dy);
            ensureValidView();
            update();
            m_lastMousePos = event->pos();
        }
    }
    QWidget::mouseMoveEvent(event);
}

void TelemetryPlotWidget::wheelEvent(QWheelEvent* event) {
    const QPointF pos = event->position();
    const QRectF plot = plotArea();
    if (!plot.contains(pos)) {
        QWidget::wheelEvent(event);
        return;
    }

    const qreal zoomFactor = event->angleDelta().y() > 0 ? 0.9 : 1.1;
    const QPointF viewCenter = m_viewRect.center();
    const QRectF newView(viewCenter.x() - m_viewRect.width() * zoomFactor / 2.0,
                         viewCenter.y() - m_viewRect.height() * zoomFactor / 2.0,
                         m_viewRect.width() * zoomFactor,
                         m_viewRect.height() * zoomFactor);

    m_viewRect = newView;
    ensureValidView();
    update();
    event->accept();
}

QRectF TelemetryPlotWidget::plotArea() const {
    return rect().adjusted(10, 30, -10, -10);
}

QPointF TelemetryPlotWidget::mapPoint(const Telemetry::DataPoint& source) const {
    const QRectF plot = plotArea();
    if (m_viewRect.width() <= 0.0 || m_viewRect.height() <= 0.0) {
        return plot.topLeft();
    }

    const qreal x = plot.left() + (source.x - m_viewRect.left()) / m_viewRect.width() * plot.width();
    const qreal y = plot.bottom() - (source.y - m_viewRect.top()) / m_viewRect.height() * plot.height();
    return QPointF(x, y);
}

void TelemetryPlotWidget::updateViewRange() {
    if (m_series.empty()) {
        m_viewRect = QRectF(0, 0, 1, 1);
        return;
    }

    bool firstPoint = true;
    double minX = 0.0;
    double maxX = 0.0;
    double minY = 0.0;
    double maxY = 0.0;

    for (const auto& series : m_series) {
        for (const auto& point : series) {
            if (firstPoint) {
                minX = maxX = point.x;
                minY = maxY = point.y;
                firstPoint = false;
            } else {
                minX = std::min(minX, point.x);
                maxX = std::max(maxX, point.x);
                minY = std::min(minY, point.y);
                maxY = std::max(maxY, point.y);
            }
        }
    }

    if (minX == maxX) {
        maxX = minX + 1.0;
    }
    if (minY == maxY) {
        maxY = minY + 1.0;
    }

    const double paddingX = (maxX - minX) * 0.05;
    const double paddingY = (maxY - minY) * 0.1;
    m_viewRect = QRectF(minX - paddingX, minY - paddingY, (maxX - minX) + paddingX * 2.0, (maxY - minY) + paddingY * 2.0);
}

void TelemetryPlotWidget::ensureValidView() {
    if (m_viewRect.width() <= 0.0) {
        m_viewRect.setWidth(1.0);
    }
    if (m_viewRect.height() <= 0.0) {
        m_viewRect.setHeight(1.0);
    }
}
