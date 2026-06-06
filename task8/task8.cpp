#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QPushButton>
#include <QGroupBox>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QFont>
#include <QSizePolicy>
#include <QTextEdit>

#include <vector>
#include <random>
#include <cmath>
#include <algorithm>

#include <windows.h>

namespace tasks {

struct SimResult {
    int B;
    double p_cliff;
    double p_cafe;
    double p_analytic_cliff;
    long long n_cliff;
    long long n_cafe;
    long long n_total;
};

static std::vector<SimResult> g_results;

static double analytic_cliff(int B, double p, double q) {
    if (std::abs(p - q) < 1e-12) {
        return 1.0;
    }
    double r = q / p;
    if (r > 1.0 + 1e-12) {
        return 1.0;
    }
    return std::pow(r, B);
}

static std::vector<SimResult> simulate(double p, int maxB, int simCount, int maxSteps) {
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    std::vector<SimResult> results;

    for (int B = 1; B <= maxB; ++B) {
        long long cliff = 0, cafe = 0, total = 0;

        for (int s = 0; s < simCount; ++s) {
            int pos = B;
            bool ended = false;

            for (int step = 0; step < maxSteps; ++step) {
                if (dist(rng) < p) {
                    pos++;
                } else {
                    pos--;
                }

                if (pos == 0) {
                    cliff++;
                    ended = true;
                    break;
                }
                if (pos == B) {
                    cafe++;
                    ended = true;
                    break;
                }
            }

            if (ended) total++;
        }

        SimResult r;
        r.B = B;
        r.n_cliff = cliff;
        r.n_cafe = cafe;
        r.n_total = total;
        r.p_cliff = total > 0 ? static_cast<double>(cliff) / total : 0.0;
        r.p_cafe = total > 0 ? static_cast<double>(cafe) / total : 0.0;
        r.p_analytic_cliff = analytic_cliff(B, p, 1.0 - p);

        results.push_back(r);
    }

    return results;
}


class ChartWidget : public QWidget {
public:
    struct Series {
        std::vector<double> xs;
        std::vector<double> ys;
        QColor color;
        QString label;
        bool dashed = false;
    };

    QString title;
    QString xLabel;
    QString yLabel;
    std::vector<Series> series;
    bool showPoints = true;

    ChartWidget(QWidget *parent = nullptr) : QWidget(parent) {
        setMinimumSize(400, 300);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        const int marginL = 60, marginR = 20, marginT = 40, marginB = 50;
        int W = width() - marginL - marginR;
        int H = height() - marginT - marginB;

        p.fillRect(rect(), QColor(30, 30, 35));
        p.fillRect(marginL, marginT, W, H, QColor(22, 22, 28));

        if (series.empty()) return;

        double xMin = 1e18, xMax = -1e18, yMin = 0.0, yMax = -1e18;
        for (auto &s : series) {
            for (double v : s.xs) { xMin = std::min(xMin, v); xMax = std::max(xMax, v); }
            for (double v : s.ys) { yMax = std::max(yMax, v); }
        }
        if (xMin >= xMax) xMax = xMin + 1;
        if (yMax <= yMin) yMax = 1.0;
        yMax = std::min(yMax * 1.05, 1.0);

        auto toScreen = [&](double x, double y) -> QPointF {
            double sx = marginL + (x - xMin) / (xMax - xMin) * W;
            double sy = marginT + H - (y - yMin) / (yMax - yMin) * H;
            return {sx, sy};
        };

        p.setPen(QPen(QColor(60, 60, 70), 1));
        int nGridY = 5;
        for (int i = 0; i <= nGridY; ++i) {
            double gy = yMin + i * (yMax - yMin) / nGridY;
            QPointF pt = toScreen(xMin, gy);
            p.drawLine(QPointF(marginL, pt.y()), QPointF(marginL + W, pt.y()));
        }

        p.setPen(QPen(QColor(80, 80, 90), 1));
        int nTickX = std::min((int)(xMax - xMin + 1), 15);
        for (int i = 0; i <= nTickX; ++i) {
            double gx = xMin + i * (xMax - xMin) / nTickX;
            QPointF pt = toScreen(gx, yMin);
            p.drawLine(QPointF(pt.x(), marginT), QPointF(pt.x(), marginT + H));
        }

        p.setPen(QPen(QColor(120, 120, 130), 1));
        QFont axFont("Consolas", 8);
        p.setFont(axFont);

        for (int i = 0; i <= nGridY; ++i) {
            double gy = yMin + i * (yMax - yMin) / nGridY;
            QPointF pt = toScreen(xMin, gy);
            QString lbl = QString::number(gy, 'f', 2);
            p.drawText(QRectF(0, pt.y() - 10, marginL - 4, 20), Qt::AlignRight | Qt::AlignVCenter, lbl);
        }

        for (int i = 0; i <= nTickX; ++i) {
            double gx = xMin + i * (xMax - xMin) / nTickX;
            QPointF pt = toScreen(gx, yMin);
            p.drawText(QRectF(pt.x() - 20, marginT + H + 4, 40, 16), Qt::AlignHCenter, QString::number((int)std::round(gx)));
        }

        p.setPen(QPen(QColor(160, 160, 170), 1));
        p.drawRect(marginL, marginT, W, H);

        for (auto &s : series) {
            if (s.xs.empty()) continue;
            QPen pen(s.color, 2);
            if (s.dashed) pen.setStyle(Qt::DashLine);
            p.setPen(pen);

            QVector<QPointF> pts;
            for (size_t i = 0; i < s.xs.size(); ++i) {
                pts.append(toScreen(s.xs[i], s.ys[i]));
            }
            p.drawPolyline(pts);

            if (showPoints) {
                p.setBrush(s.color);
                p.setPen(Qt::NoPen);
                for (auto &pt : pts) {
                    p.drawEllipse(pt, 3, 3);
                }
            }
        }

        QFont titleFont("Consolas", 10, QFont::Bold);
        p.setFont(titleFont);
        p.setPen(QColor(220, 220, 230));
        p.drawText(QRectF(marginL, 4, W, marginT - 4), Qt::AlignHCenter | Qt::AlignVCenter, title);

        QFont axLabelFont("Consolas", 9);
        p.setFont(axLabelFont);
        p.setPen(QColor(180, 180, 190));
        p.drawText(QRectF(marginL, marginT + H + 20, W, 20), Qt::AlignHCenter, xLabel);

        p.save();
        p.translate(12, marginT + H / 2);
        p.rotate(-90);
        p.drawText(QRectF(-H / 2, -10, H, 20), Qt::AlignHCenter, yLabel);
        p.restore();

        if (series.size() > 1) {
            int lx = marginL + 10, ly = marginT + 10;
            for (auto &s : series) {
                QPen lp(s.color, 2);
                if (s.dashed) lp.setStyle(Qt::DashLine);
                p.setPen(lp);
                p.drawLine(lx, ly + 7, lx + 20, ly + 7);
                p.setPen(QColor(200, 200, 210));
                p.setFont(QFont("Consolas", 8));
                p.drawText(lx + 24, ly + 12, s.label);
                ly += 18;
            }
        }
    }
};


class MainWindow : public QMainWindow {
    QDoubleSpinBox *spinP;
    QSpinBox *spinMaxB;
    QSpinBox *spinSims;
    QSpinBox *spinSteps;
    QPushButton *btnRun;
    ChartWidget *chartCliff;
    ChartWidget *chartCafe;
    QTextEdit *logEdit;

public:
    MainWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Задача 8");
        setMinimumSize(1000, 680);

        QWidget *central = new QWidget(this);
        setCentralWidget(central);

        QVBoxLayout *mainLayout = new QVBoxLayout(central);
        mainLayout->setContentsMargins(8, 8, 8, 8);
        mainLayout->setSpacing(6);

        QGroupBox *paramBox = new QGroupBox("Параметры");
        QHBoxLayout *paramLayout = new QHBoxLayout(paramBox);

        auto makeLabel = [](const QString &text) {
            QLabel *l = new QLabel(text);
            l->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            return l;
        };

        spinP = new QDoubleSpinBox();
        spinP->setRange(0.01, 0.99);
        spinP->setSingleStep(0.05);
        spinP->setDecimals(2);
        spinP->setValue(0.4);

        spinMaxB = new QSpinBox();
        spinMaxB->setRange(1, 100);
        spinMaxB->setValue(20);

        spinSims = new QSpinBox();
        spinSims->setRange(100, 100000);
        spinSims->setSingleStep(1000);
        spinSims->setValue(10000);

        spinSteps = new QSpinBox();
        spinSteps->setRange(100, 1000000);
        spinSteps->setSingleStep(10000);
        spinSteps->setValue(50000);

        btnRun = new QPushButton("Симулировать");
        btnRun->setFixedHeight(32);

        paramLayout->addWidget(makeLabel("p (шаг вперёд):"));
        paramLayout->addWidget(spinP);
        paramLayout->addSpacing(10);
        paramLayout->addWidget(makeLabel("Макс B:"));
        paramLayout->addWidget(spinMaxB);
        paramLayout->addSpacing(10);
        paramLayout->addWidget(makeLabel("Симуляций:"));
        paramLayout->addWidget(spinSims);
        paramLayout->addSpacing(10);
        paramLayout->addWidget(makeLabel("Макс шагов:"));
        paramLayout->addWidget(spinSteps);
        paramLayout->addSpacing(16);
        paramLayout->addWidget(btnRun);
        paramLayout->addStretch();

        mainLayout->addWidget(paramBox);

        QHBoxLayout *chartsLayout = new QHBoxLayout();
        chartsLayout->setSpacing(8);

        chartCliff = new ChartWidget();
        chartCliff->title = "P(падение в обрыв)";
        chartCliff->xLabel = "B (начальная позиция)";
        chartCliff->yLabel = "Вероятность";

        chartCafe = new ChartWidget();
        chartCafe->title = "P(возврат в кафе)";
        chartCafe->xLabel = "B (начальная позиция)";
        chartCafe->yLabel = "Вероятность";

        chartsLayout->addWidget(chartCliff);
        chartsLayout->addWidget(chartCafe);

        mainLayout->addLayout(chartsLayout, 3);

        logEdit = new QTextEdit();
        logEdit->setReadOnly(true);
        logEdit->setMaximumHeight(140);
        logEdit->setFont(QFont("Consolas", 8));
        mainLayout->addWidget(logEdit, 1);

        connect(btnRun, &QPushButton::clicked, this, &MainWindow::onRun);

        QString style = R"(
            QMainWindow, QWidget { background: #1a1a22; color: #d0d0dc; }
            QGroupBox { border: 1px solid #3a3a4a; border-radius: 4px; margin-top: 6px; padding-top: 6px; color: #a0a0b0; }
            QGroupBox::title { subcontrol-origin: margin; left: 8px; }
            QDoubleSpinBox, QSpinBox { background: #22222e; border: 1px solid #3a3a4a; color: #d0d0dc; padding: 2px 4px; border-radius: 3px; }
            QLabel { color: #a0a0b0; }
            QPushButton { background: #2e5faa; color: #e0e0f0; border: none; border-radius: 4px; padding: 4px 16px; font-weight: bold; }
            QPushButton:hover { background: #3a72c8; }
            QPushButton:pressed { background: #1e4080; }
            QTextEdit { background: #12121a; border: 1px solid #3a3a4a; color: #90d090; border-radius: 3px; }
        )";
        setStyleSheet(style);
    }

private slots:
    void onRun() {
        double p = spinP->value();
        double q = 1.0 - p;
        int maxB = spinMaxB->value();
        int sims = spinSims->value();
        int steps = spinSteps->value();

        btnRun->setEnabled(false);
        btnRun->setText("Считаем...");
        QApplication::processEvents();

        g_results = simulate(p, maxB, sims, steps);

        std::vector<double> xs, ysCliff, ysCafe, ysAnalytic;
        for (auto &r : g_results) {
            xs.push_back(r.B);
            ysCliff.push_back(r.p_cliff);
            ysCafe.push_back(r.p_cafe);
            ysAnalytic.push_back(r.p_analytic_cliff);
        }

        ChartWidget::Series sCliff;
        sCliff.xs = xs; sCliff.ys = ysCliff;
        sCliff.color = QColor(255, 90, 90);
        sCliff.label = "Симуляция";

        ChartWidget::Series sCliffAn;
        sCliffAn.xs = xs; sCliffAn.ys = ysAnalytic;
        sCliffAn.color = QColor(255, 200, 60);
        sCliffAn.label = "Аналитика";
        sCliffAn.dashed = true;

        chartCliff->series = {sCliff, sCliffAn};
        chartCliff->title = QString("P(падение в обрыв)  p=%1  q=%2").arg(p, 0, 'f', 2).arg(q, 0, 'f', 2);
        chartCliff->update();

        ChartWidget::Series sCafe;
        sCafe.xs = xs; sCafe.ys = ysCafe;
        sCafe.color = QColor(80, 200, 140);
        sCafe.label = "Симуляция";

        chartCafe->series = {sCafe};
        chartCafe->title = QString("P(возврат в кафе)  p=%1  q=%2").arg(p, 0, 'f', 2).arg(q, 0, 'f', 2);
        chartCafe->update();

        logEdit->clear();
        logEdit->append(QString("p = %1   q = %2   симуляций = %3   макс. шагов = %4")
            .arg(p, 0, 'f', 2).arg(q, 0, 'f', 2).arg(sims).arg(steps));
        logEdit->append(QString("B | P(обрыв) симул. | P(обрыв) аналит. | P(кафе) симул. | учтено"));
        for (auto &r : g_results) {
            logEdit->append(QString("%1 | %2 | %3 | %4 | %5/%6")
                .arg(r.B, 3)
                .arg(r.p_cliff, 7, 'f', 4)
                .arg(r.p_analytic_cliff, 7, 'f', 4)
                .arg(r.p_cafe, 7, 'f', 4)
                .arg(r.n_cliff + r.n_cafe)
                .arg(sims));
        }

        btnRun->setEnabled(true);
        btnRun->setText("Симулировать");
    }
};


class task8 {
public:
    static void run(int argc, char **argv);
};

void task8::run(int argc, char **argv) {
    QApplication app(argc, argv);

    QFont appFont("Consolas", 9);
    app.setFont(appFont);

    MainWindow w;
    w.show();

    app.exec();
}

}

int main(int argc, char **argv) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    tasks::task8::run(argc, argv);

    return 0;
}