#include "mainwindow.hpp"
#include "telemetry_analyzer.hpp"
#include "csv_parser.hpp"
#include "downsampler.hpp"
#include "math_channel_evaluator.hpp"
#include "session_manager.hpp"
#include "telemetry_plot_widget.hpp"
#include "telemetry_record.hpp"

#include <QAction>
#include <QColor>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QVBoxLayout>

#include <future>
#include <memory>
#include <string>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    criarMenu();
    criarInterface();
    setWindowTitle("Analisador de Telemetria");
    resize(1100, 700);
}

void MainWindow::criarMenu() {
    m_actionAbrirCsv = new QAction(tr("Abrir CSV"), this);
    connect(m_actionAbrirCsv, &QAction::triggered, this, &MainWindow::abrirCsv);

    QMenu* arquivoMenu = menuBar()->addMenu(tr("Arquivo"));
    arquivoMenu->addAction(m_actionAbrirCsv);
}

void MainWindow::criarInterface() {
    auto* central = new QWidget(this);
    auto* splitter = new QSplitter(Qt::Horizontal, central);

    auto* leftWidget = new QWidget(splitter);
    auto* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(12, 12, 12, 12);
    leftLayout->setSpacing(10);

    auto* formLayout = new QFormLayout();
    m_registrosLabel = new QLabel(tr("-"), this);
    m_velocidadeMediaLabel = new QLabel(tr("-"), this);
    m_rpmMaximoLabel = new QLabel(tr("-"), this);
    m_aceleracaoMediaLabel = new QLabel(tr("-"), this);
    m_lateralGMaximoLabel = new QLabel(tr("-"), this);
    m_overspeedLabel = new QLabel(tr("-"), this);
    m_highRpmLabel = new QLabel(tr("-"), this);
    m_hardBrakingLabel = new QLabel(tr("-"), this);
    m_curveLabel = new QLabel(tr("-"), this);

    formLayout->addRow(tr("Registros:"), m_registrosLabel);
    formLayout->addRow(tr("Velocidade media:"), m_velocidadeMediaLabel);
    formLayout->addRow(tr("RPM maximo:"), m_rpmMaximoLabel);
    formLayout->addRow(tr("Aceleracao media:"), m_aceleracaoMediaLabel);
    formLayout->addRow(tr("Lateral G maximo:"), m_lateralGMaximoLabel);
    formLayout->addRow(tr("Eventos overspeed:"), m_overspeedLabel);
    formLayout->addRow(tr("Eventos high RPM:"), m_highRpmLabel);
    formLayout->addRow(tr("Frenagens bruscas:"), m_hardBrakingLabel);
    formLayout->addRow(tr("Eventos de curva:"), m_curveLabel);

    leftLayout->addLayout(formLayout);

    m_mathChannelInput = new QLineEdit(this);
    m_mathChannelInput->setPlaceholderText(tr("Digite formula do Math Channel"));
    connect(m_mathChannelInput, &QLineEdit::editingFinished, this, &MainWindow::aplicarMathChannel);

    leftLayout->addWidget(new QLabel(tr("Math Channel:"), this));
    leftLayout->addWidget(m_mathChannelInput);

    auto* button = new QPushButton(tr("Aplicar canal"), this);
    connect(button, &QPushButton::clicked, this, &MainWindow::aplicarMathChannel);
    leftLayout->addWidget(button);
    leftLayout->addStretch();

    auto* rightWidget = new QFrame(splitter);
    rightWidget->setFrameShape(QFrame::StyledPanel);
    auto* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(12, 12, 12, 12);
    rightLayout->setSpacing(10);

    m_placeholderLabel = new QLabel(tr("Gráficos e visualização de dados serão exibidos aqui."), this);
    m_placeholderLabel->setAlignment(Qt::AlignCenter);
    m_placeholderLabel->setWordWrap(true);
    rightLayout->addWidget(m_placeholderLabel);

    m_speedPlot = new TelemetryPlotWidget(tr("Velocidade (km/h)"), this);
    m_rpmPlot = new TelemetryPlotWidget(tr("RPM"), this);
    m_lateralPlot = new TelemetryPlotWidget(tr("Força G Lateral"), this);
    m_mathChannelPlot = new TelemetryPlotWidget(tr("Math Channel"), this);
    m_mathChannelPlot->setVisible(false);

    rightLayout->addWidget(m_speedPlot);
    rightLayout->addWidget(m_rpmPlot);
    rightLayout->addWidget(m_lateralPlot);
    rightLayout->addWidget(m_mathChannelPlot);
    rightLayout->addStretch();

    splitter->addWidget(leftWidget);
    splitter->addWidget(rightWidget);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);

    auto* mainLayout = new QVBoxLayout(central);
    mainLayout->addWidget(splitter);
    central->setLayout(mainLayout);
    setCentralWidget(central);
}

void MainWindow::abrirCsv() {
    const QString path = QFileDialog::getOpenFileName(this, tr("Abrir CSV"), QString(), tr("Arquivos CSV (*.csv);;Todos os arquivos (*.*)"));
    if (path.isEmpty()) {
        return;
    }

    carregarDadosEPlotar(path.toStdString());
}

void MainWindow::aplicarMathChannel() {
    const QString formula = m_mathChannelInput->text().trimmed();

    if (formula.isEmpty()) {
        m_placeholderLabel->setText(tr("Digite uma fórmula e clique em Aplicar canal para atualizar o Math Channel."));
        m_mathChannelPlot->clearSeries();
        m_mathChannelPlot->setVisible(false);
        return;
    }

    m_placeholderLabel->setText(tr("Aplicando Math Channel: %1").arg(formula));
    m_mathChannelPlot->setVisible(true);

    if (!m_loadedData) {
        m_placeholderLabel->setText(tr("Carregue um arquivo CSV antes de aplicar o Math Channel."));
        return;
    }

    auto dataSnapshot = m_loadedData;
    auto* savedThis = this;
    const std::string formulaStd = formula.toStdString();
    auto worker = std::async(std::launch::async, [savedThis, dataSnapshot, formulaStd]() {
        try {
            const auto outputs = Telemetry::processarCanalCustomizado(*dataSnapshot, formulaStd);
            if (outputs.empty()) {
                throw std::runtime_error("Falha ao processar o Math Channel.");
            }

            std::vector<Telemetry::DataPoint> formulaSeries;
            formulaSeries.reserve(outputs.size());
            for (size_t idx = 0; idx < outputs.size(); ++idx) {
                const double xValue = static_cast<double>((*dataSnapshot)[idx].timestamp);
                formulaSeries.push_back({xValue, outputs[idx]});
            }

            const auto sampledFormula = Telemetry::Downsampler::lttb(formulaSeries, 2000);
            QMetaObject::invokeMethod(savedThis, [savedThis, sampledFormula]() {
                savedThis->m_mathChannelPlot->setSeries({sampledFormula});
                savedThis->m_mathChannelPlot->setVisible(true);
                savedThis->m_mathChannelPlot->replot();
                savedThis->m_placeholderLabel->setText(tr("Math Channel renderizado com sucesso."));
            }, Qt::QueuedConnection);
        } catch (const std::exception& ex) {
            QMetaObject::invokeMethod(savedThis, [savedThis, message = QString::fromUtf8(ex.what())]() {
                savedThis->mostrarErro(message);
            }, Qt::QueuedConnection);
        }
    });
    (void)worker;
}

void MainWindow::carregarDadosEPlotar(const std::string& csvPath) {
    m_placeholderLabel->setText(tr("Carregando dados... isso pode levar alguns instantes para arquivos grandes."));

    auto* savedThis = this;
    auto worker = std::async(std::launch::async, [savedThis, csvPath]() {
        try {
            Telemetry::CsvParser parser;
            auto parsedData = parser.parse(csvPath);
            if (parsedData.empty()) {
                throw std::runtime_error("Nenhum registro encontrado no arquivo CSV.");
            }

            auto loadedData = std::make_shared<std::vector<Telemetry::TelemetryRecord>>(std::move(parsedData));
            Telemetry::TelemetryAnalyzer analyzer(*loadedData);
            const size_t registros = loadedData->size();
            const double velocidadeMedia = analyzer.getAverageSpeed();
            const int32_t rpmMaximo = analyzer.getMaxRPM().rpm;
            const double aceleracaoMedia = analyzer.getAverageAcceleration();
            const double lateralGMaximo = analyzer.getMaxLateralG();
            const int eventosOverspeed = analyzer.countOverspeedEvents(100.0);
            const int eventosHighRpm = analyzer.countHighRpmEvents(3000);
            const int eventosHardBraking = analyzer.countHardBrakingEvents(-4.0);
            const int eventosCurve = analyzer.countCurveEvents(0.5);

            std::vector<Telemetry::DataPoint> speedSeries;
            std::vector<Telemetry::DataPoint> rpmSeries;
            std::vector<Telemetry::DataPoint> lateralSeries;
            speedSeries.reserve(loadedData->size());
            rpmSeries.reserve(loadedData->size());
            lateralSeries.reserve(loadedData->size());

            for (const auto& record : *loadedData) {
                const double xValue = static_cast<double>(record.timestamp);
                speedSeries.push_back({xValue, record.speed});
                rpmSeries.push_back({xValue, static_cast<double>(record.rpm)});
                lateralSeries.push_back({xValue, record.lateral_g});
            }

            const auto sampledSpeed = Telemetry::Downsampler::lttb(speedSeries, 2000);
            const auto sampledRpm = Telemetry::Downsampler::lttb(rpmSeries, 2000);
            const auto sampledLateral = Telemetry::Downsampler::lttb(lateralSeries, 2000);

            QMetaObject::invokeMethod(savedThis, [savedThis,
                                                 loadedData,
                                                 registros,
                                                 velocidadeMedia,
                                                 rpmMaximo,
                                                 aceleracaoMedia,
                                                 lateralGMaximo,
                                                 eventosOverspeed,
                                                 eventosHighRpm,
                                                 eventosHardBraking,
                                                 eventosCurve,
                                                 sampledSpeed,
                                                 sampledRpm,
                                                 sampledLateral]() {
                savedThis->m_loadedData = loadedData;
                savedThis->atualizarResumoUI(registros,
                                            velocidadeMedia,
                                            rpmMaximo,
                                            aceleracaoMedia,
                                            lateralGMaximo,
                                            eventosOverspeed,
                                            eventosHighRpm,
                                            eventosHardBraking,
                                            eventosCurve);
                savedThis->m_speedPlot->setSeries({sampledSpeed});
                savedThis->m_rpmPlot->setSeries({sampledRpm});
                savedThis->m_lateralPlot->setSeries({sampledLateral});
                savedThis->m_speedPlot->replot();
                savedThis->m_rpmPlot->replot();
                savedThis->m_lateralPlot->replot();
                savedThis->m_mathChannelPlot->clearSeries();
                savedThis->m_mathChannelPlot->setVisible(false);
                savedThis->m_placeholderLabel->setText(QString::fromLatin1(
                    "Dados carregados com sucesso. "
                    "Downsampling aplicado a 2000 pontos para speed, rpm e lateral_g."));
            }, Qt::QueuedConnection);
        } catch (const std::exception& ex) {
            QMetaObject::invokeMethod(savedThis, [savedThis, message = QString::fromUtf8(ex.what())]() {
                savedThis->mostrarErro(message);
            }, Qt::QueuedConnection);
        }
    });

    (void)worker; // manter o futuro vivo até que o lambda seja executado.
}

void MainWindow::atualizarResumoUI(size_t registros,
                                   double velocidadeMedia,
                                   int32_t rpmMaximo,
                                   double aceleracaoMedia,
                                   double lateralGMaximo,
                                   int eventosOverspeed,
                                   int eventosHighRpm,
                                   int eventosHardBraking,
                                   int eventosCurve) {
    m_registrosLabel->setText(QString::number(registros));
    m_velocidadeMediaLabel->setText(QString::number(velocidadeMedia, 'f', 2));
    m_rpmMaximoLabel->setText(QString::number(rpmMaximo));
    m_aceleracaoMediaLabel->setText(QString::number(aceleracaoMedia, 'f', 2));
    m_lateralGMaximoLabel->setText(QString::number(lateralGMaximo, 'f', 2));
    m_overspeedLabel->setText(QString::number(eventosOverspeed));
    m_highRpmLabel->setText(QString::number(eventosHighRpm));
    m_hardBrakingLabel->setText(QString::number(eventosHardBraking));
    m_curveLabel->setText(QString::number(eventosCurve));
}

void MainWindow::mostrarErro(const QString& mensagem) {
    QMessageBox::critical(this, tr("Erro ao carregar CSV"), mensagem);
    m_placeholderLabel->setText(tr("Erro ao carregar os dados. Verifique o arquivo e tente novamente."));
}
