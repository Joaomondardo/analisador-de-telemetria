#pragma once

#include <QColor>
#include <QMainWindow>
#include <memory>
#include <string>
#include <vector>

namespace Telemetry {
class TelemetryAnalyzer;
struct TelemetryRecord;
class CsvParser;
class Downsampler;
struct DataPoint;
}

QT_BEGIN_NAMESPACE
class QAction;
class QLabel;
class QLineEdit;
class QPushButton;
class QSplitter;
class QWidget;
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void abrirCsv();
    void aplicarMathChannel();

private:
    void criarInterface();
    void criarMenu();
    void carregarDadosEPlotar(const std::string& csvPath);
    void atualizarResumoUI(size_t registros,
                           double velocidadeMedia,
                           int32_t rpmMaximo,
                           double aceleracaoMedia,
                           double lateralGMaximo,
                           int eventosOverspeed,
                           int eventosHighRpm,
                           int eventosHardBraking,
                           int eventosCurve);
    void mostrarErro(const QString& mensagem);

    QAction* m_actionAbrirCsv{nullptr};
    QLabel* m_registrosLabel{nullptr};
    QLabel* m_velocidadeMediaLabel{nullptr};
    QLabel* m_rpmMaximoLabel{nullptr};
    QLabel* m_aceleracaoMediaLabel{nullptr};
    QLabel* m_lateralGMaximoLabel{nullptr};
    QLabel* m_overspeedLabel{nullptr};
    QLabel* m_highRpmLabel{nullptr};
    QLabel* m_hardBrakingLabel{nullptr};
    QLabel* m_curveLabel{nullptr};
    QLineEdit* m_mathChannelInput{nullptr};
    QLabel* m_placeholderLabel{nullptr};
    class TelemetryPlotWidget* m_speedPlot{nullptr};
    class TelemetryPlotWidget* m_rpmPlot{nullptr};
    class TelemetryPlotWidget* m_lateralPlot{nullptr};
    class TelemetryPlotWidget* m_mathChannelPlot{nullptr};
    std::shared_ptr<std::vector<Telemetry::TelemetryRecord>> m_loadedData;
};
