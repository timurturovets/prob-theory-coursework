#include <QApplication>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QSplitter>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace tasks {

class DRV {
public:
    struct Entry {
        double value;
        double prob;
    };

    DRV() {}

    explicit DRV(const std::vector<Entry> &entries) {
        validate(entries);
        m_entries = entries;
        std::sort(m_entries.begin(), m_entries.end(), [](const Entry &a, const Entry &b) {
            return a.value < b.value;
        });
    }

    const std::vector<Entry> &entries() const {
        return m_entries;
    }

    DRV operator*(double c) const {
        std::vector<std::pair<double, double>> acc;
        for (const auto &e : m_entries) {
            double v = e.value * c;
            bool found = false;
            for (auto &kv : acc) {
                if (std::abs(kv.first - v) < 1e-12) {
                    kv.second += e.prob;
                    found = true;
                    break;
                }
            }
            if (!found) {
                acc.push_back(std::make_pair(v, e.prob));
            }
        }
        std::sort(acc.begin(), acc.end(), [](const std::pair<double,double> &a, const std::pair<double,double> &b) {
            return a.first < b.first;
        });
        std::vector<Entry> result;
        for (const auto &kv : acc) {
            Entry en;
            en.value = kv.first;
            en.prob = kv.second;
            result.push_back(en);
        }
        return DRV(result);
    }

    DRV operator+(const DRV &other) const {
        return convolve(other, [](double a, double b) { return a + b; });
    }

    DRV operator*(const DRV &other) const {
        return convolve(other, [](double a, double b) { return a * b; });
    }

    double mean() const {
        double m = 0.0;
        for (const auto &e : m_entries) {
            m += e.value * e.prob;
        }
        return m;
    }

    double variance() const {
        double m = mean();
        double v = 0.0;
        for (const auto &e : m_entries) {
            double d = e.value - m;
            v += d * d * e.prob;
        }
        return v;
    }

    double skewness() const {
        double m = mean();
        double s = std::sqrt(variance());
        if (s < 1e-15) {
            return 0.0;
        }
        double mu3 = 0.0;
        for (const auto &e : m_entries) {
            double d = e.value - m;
            mu3 += d * d * d * e.prob;
        }
        return mu3 / (s * s * s);
    }

    double kurtosis() const {
        double m = mean();
        double v = variance();
        if (v < 1e-15) {
            return 0.0;
        }
        double mu4 = 0.0;
        for (const auto &e : m_entries) {
            double d = e.value - m;
            mu4 += d * d * d * d * e.prob;
        }
        return mu4 / (v * v) - 3.0;
    }

    void serialize(std::ostream &out) const {
        out << m_entries.size() << "\n";
        for (const auto &e : m_entries) {
            out << e.value << " " << e.prob << "\n";
        }
    }

    static DRV deserialize(std::istream &in) {
        size_t n = 0;
        if (!(in >> n) || n == 0) {
            throw std::invalid_argument("Неверный формат файла");
        }
        std::vector<Entry> entries;
        for (size_t i = 0; i < n; ++i) {
            double v, p;
            if (!(in >> v >> p)) {
                throw std::invalid_argument("Недостаточно данных в файле");
            }
            Entry en;
            en.value = v;
            en.prob = p;
            entries.push_back(en);
        }
        return DRV(entries);
    }

    std::vector<std::pair<double, double>> cdf() const {
        std::vector<std::pair<double, double>> result;
        double cum = 0.0;
        for (const auto &e : m_entries) {
            cum += e.prob;
            result.push_back(std::make_pair(e.value, cum));
        }
        return result;
    }

private:
    std::vector<Entry> m_entries;

    static void validate(const std::vector<Entry> &entries) {
        if (entries.empty()) {
            throw std::invalid_argument("Таблица не может быть пустой");
        }
        for (const auto &e : entries) {
            if (e.prob < 0.0) {
                throw std::invalid_argument("Вероятность не может быть отрицательной");
            }
        }
        for (size_t i = 0; i < entries.size(); ++i) {
            for (size_t j = i + 1; j < entries.size(); ++j) {
                if (std::abs(entries[i].value - entries[j].value) < 1e-12) {
                    throw std::invalid_argument("Значения случайной величины должны быть попарно различны");
                }
            }
        }
        double sum = 0.0;
        for (const auto &e : entries) {
            sum += e.prob;
        }
        if (std::abs(sum - 1.0) > 1e-9) {
            throw std::invalid_argument(
                std::string("Сумма вероятностей должна равняться 1, получено: ") + std::to_string(sum)
            );
        }
    }

    template<typename F>
    DRV convolve(const DRV &other, F op) const {
        std::vector<std::pair<double, double>> acc;
        for (const auto &a : m_entries) {
            for (const auto &b : other.m_entries) {
                double v = op(a.value, b.value);
                double p = a.prob * b.prob;
                bool found = false;
                for (auto &kv : acc) {
                    if (std::abs(kv.first - v) < 1e-12) {
                        kv.second += p;
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    acc.push_back(std::make_pair(v, p));
                }
            }
        }
        std::sort(acc.begin(), acc.end(), [](const std::pair<double,double> &a, const std::pair<double,double> &b) {
            return a.first < b.first;
        });
        std::vector<Entry> result;
        for (const auto &kv : acc) {
            Entry en;
            en.value = kv.first;
            en.prob = kv.second;
            result.push_back(en);
        }
        return DRV(result);
    }
};


class PlotWidget : public QWidget {
public:
    enum Mode { Polyline, CDF };

    PlotWidget(Mode mode, QWidget *parent = nullptr) : QWidget(parent), m_mode(mode) {
        setMinimumSize(400, 260);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }

    void setDRV(const DRV &drv) {
        m_drv = drv;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.fillRect(rect(), QColor(30, 30, 40));

        const int mL = 58, mR = 20, mT = 18, mB = 38;
        QRect plot(mL, mT, width() - mL - mR, height() - mT - mB);

        if (m_drv.entries().empty()) {
            p.setPen(Qt::gray);
            p.drawText(rect(), Qt::AlignCenter, "Нет данных");
            return;
        }

        p.setPen(QPen(QColor(70, 70, 90)));
        p.drawRect(plot);

        const auto &ents = m_drv.entries();
        double xMin = ents.front().value;
        double xMax = ents.back().value;
        double xRange = xMax - xMin;
        if (std::abs(xRange) < 1e-15) {
            xMin -= 1.0;
            xMax += 1.0;
            xRange = 2.0;
        }

        double yMin = 0.0;
        double yMax = 1.0;
        if (m_mode == Polyline) {
            yMax = 0.0;
            for (const auto &e : ents) {
                if (e.prob > yMax) yMax = e.prob;
            }
            yMax = yMax < 1e-15 ? 1.0 : yMax * 1.2;
        }

        auto toScreen = [&](double x, double y) -> QPointF {
            double px = plot.left() + (x - xMin) / xRange * plot.width();
            double py = plot.bottom() - (y - yMin) / (yMax - yMin) * plot.height();
            return QPointF(px, py);
        };

        p.setPen(QPen(QColor(70, 70, 90), 1, Qt::DashLine));
        for (int i = 0; i <= 5; ++i) {
            double yv = yMin + i * (yMax - yMin) / 5.0;
            double py = plot.bottom() - static_cast<double>(i) / 5.0 * plot.height();
            p.drawLine(QPointF(plot.left(), py), QPointF(plot.right(), py));
            p.setPen(Qt::lightGray);
            p.drawText(QRectF(0, py - 10, mL - 4, 20), Qt::AlignRight | Qt::AlignVCenter,
                       QString::number(yv, 'f', 3));
            p.setPen(QPen(QColor(70, 70, 90), 1, Qt::DashLine));
        }

        p.setPen(Qt::white);
        for (const auto &e : ents) {
            QPointF pt = toScreen(e.value, 0.0);
            p.drawLine(QPointF(pt.x(), plot.bottom()), QPointF(pt.x(), plot.bottom() + 4));
            p.drawText(QRectF(pt.x() - 28, plot.bottom() + 5, 56, 18), Qt::AlignCenter,
                       QString::number(e.value, 'f', 2));
        }

        if (m_mode == Polyline) {
            p.setPen(QPen(QColor(80, 180, 255), 2));
            QVector<QPointF> pts;
            for (const auto &e : ents) {
                pts.append(toScreen(e.value, e.prob));
            }
            p.drawPolyline(pts.data(), pts.size());

            for (const auto &e : ents) {
                QPointF top = toScreen(e.value, e.prob);
                QPointF bot = toScreen(e.value, 0.0);
                p.setPen(QPen(QColor(80, 180, 255, 80), 1, Qt::DotLine));
                p.drawLine(top, bot);
                p.setPen(Qt::NoPen);
                p.setBrush(QColor(80, 180, 255));
                p.drawEllipse(top, 5.0, 5.0);
            }
        } else {
            auto cdfData = m_drv.cdf();
            double extra = xRange * 0.15;
            double xL0 = xMin - extra;
            double xR0 = xMax + extra;

            p.setPen(QPen(QColor(255, 150, 60), 2));
            p.drawLine(toScreen(xL0, 0.0), toScreen(cdfData.front().first, 0.0));

            for (size_t i = 0; i < cdfData.size(); ++i) {
                double xR = (i + 1 < cdfData.size()) ? cdfData[i + 1].first : xR0;
                p.setPen(QPen(QColor(255, 150, 60), 2));
                p.drawLine(toScreen(cdfData[i].first, cdfData[i].second),
                           toScreen(xR, cdfData[i].second));

                p.setPen(Qt::NoPen);
                p.setBrush(QColor(255, 150, 60));
                p.drawEllipse(toScreen(cdfData[i].first, cdfData[i].second), 4.5, 4.5);

                if (i > 0) {
                    p.setBrush(QColor(30, 30, 40));
                    p.setPen(QPen(QColor(255, 150, 60), 2));
                    p.drawEllipse(toScreen(cdfData[i].first, cdfData[i - 1].second), 4.5, 4.5);
                }
            }
        }
    }

private:
    DRV m_drv;
    Mode m_mode;
};


static bool tryParseDRV(QTableWidget *table, DRV &out, QString &err) {
    std::vector<DRV::Entry> entries;
    for (int i = 0; i < table->rowCount(); ++i) {
        QTableWidgetItem *iv = table->item(i, 0);
        QTableWidgetItem *ip = table->item(i, 1);
        if (!iv || !ip || iv->text().trimmed().isEmpty() || ip->text().trimmed().isEmpty()) {
            continue;
        }
        bool ok1, ok2;
        double v = iv->text().toDouble(&ok1);
        double prob = ip->text().toDouble(&ok2);
        if (!ok1 || !ok2) {
            err = QString("Строка %1: неверный формат числа").arg(i + 1);
            return false;
        }
        DRV::Entry en;
        en.value = v;
        en.prob = prob;
        entries.push_back(en);
    }
    try {
        out = DRV(entries);
    } catch (const std::exception &e) {
        err = QString::fromStdString(e.what());
        return false;
    }
    return true;
}

static void fillTable(QTableWidget *table, const DRV &drv) {
    table->blockSignals(true);
    table->setRowCount(0);
    for (const auto &e : drv.entries()) {
        int row = table->rowCount();
        table->insertRow(row);
        table->setItem(row, 0, new QTableWidgetItem(QString::number(e.value, 'f', 6)));
        table->setItem(row, 1, new QTableWidgetItem(QString::number(e.prob, 'f', 6)));
    }
    table->blockSignals(false);
}

static QTableWidget *makeTable(QWidget *parent) {
    auto *t = new QTableWidget(0, 2, parent);
    t->setHorizontalHeaderLabels({"x", "p"});
    t->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    t->setMaximumHeight(160);
    return t;
}

static DRV defaultA() {
    std::vector<DRV::Entry> v;
    DRV::Entry e;
    e.value = -1.0; e.prob = 0.2; v.push_back(e);
    e.value =  0.0; e.prob = 0.3; v.push_back(e);
    e.value =  1.0; e.prob = 0.4; v.push_back(e);
    e.value =  2.0; e.prob = 0.1; v.push_back(e);
    return DRV(v);
}

static DRV defaultB() {
    std::vector<DRV::Entry> v;
    DRV::Entry e;
    e.value = 0.0; e.prob = 0.5; v.push_back(e);
    e.value = 1.0; e.prob = 0.3; v.push_back(e);
    e.value = 3.0; e.prob = 0.2; v.push_back(e);
    return DRV(v);
}

static void updateStatsLabel(QTableWidget *table, QLabel *label) {
    DRV drv;
    QString err;
    if (!tryParseDRV(table, drv, err)) {
        label->setText("Ошибка: " + err);
        return;
    }
    label->setText(
        QString("M(X)=%1  D(X)=%2  As=%3  Ex=%4")
            .arg(drv.mean(), 0, 'f', 5)
            .arg(drv.variance(), 0, 'f', 5)
            .arg(drv.skewness(), 0, 'f', 5)
            .arg(drv.kurtosis(), 0, 'f', 5)
    );
}

static QString makeLawText(const DRV &drv) {
    QString text;
    text += QString("%1  %2\n").arg("x", 16).arg("p", 16);
    text += QString(34, '-') + "\n";
    for (const auto &e : drv.entries()) {
        text += QString("%1  %2\n")
            .arg(QString::number(e.value, 'f', 6), 16)
            .arg(QString::number(e.prob, 'f', 6), 16);
    }
    text += QString(34, '-') + "\n";
    text += QString("M(X)    = %1\n").arg(drv.mean(), 0, 'f', 8);
    text += QString("D(X)    = %1\n").arg(drv.variance(), 0, 'f', 8);
    text += QString("As      = %1\n").arg(drv.skewness(), 0, 'f', 8);
    text += QString("Ex      = %1\n").arg(drv.kurtosis(), 0, 'f', 8);
    return text;
}

static QString styleSheet() {
    return
        "QWidget{background:#1e1e28;color:#e0e0e0;font-size:12px;}"
        "QTableWidget{background:#252535;gridline-color:#444;}"
        "QHeaderView::section{background:#333348;padding:4px;}"
        "QPushButton{background:#3a3a5c;border:1px solid #555;padding:5px 10px;border-radius:4px;}"
        "QPushButton:hover{background:#4a4a7c;}"
        "QTabWidget::pane{border:1px solid #444;}"
        "QTabBar::tab{background:#2a2a3c;padding:6px 14px;}"
        "QTabBar::tab:selected{background:#3a3a5c;}"
        "QLineEdit,QDoubleSpinBox{background:#252535;border:1px solid #555;padding:3px;}"
        "QTextEdit{background:#252535;border:1px solid #444;font-family:Courier New;}"
        "QLabel{color:#e0e0e0;}";
}

class task2 {
public:
    static void run(int argc, char **argv) {
        QApplication app(argc, argv);

        QWidget win;
        win.setWindowTitle("Дискретная случайная величина");
        win.resize(1100, 800);
        win.setStyleSheet(styleSheet());

        auto *root = new QVBoxLayout(&win);
        auto *tabs = new QTabWidget(&win);
        root->addWidget(tabs);

        QTableWidget *tblA = nullptr;
        QTableWidget *tblB = nullptr;
        QTableWidget *tblR = nullptr;
        QLabel *lblStatsA = nullptr;
        QLabel *lblStatsB = nullptr;
        QLabel *lblStatsR = nullptr;
        QDoubleSpinBox *scalarSpin = nullptr;

        PlotWidget *plotPoly = nullptr;
        PlotWidget *plotCDF = nullptr;
        QTextEdit *lawText = nullptr;
        QTableWidget *tblVis = nullptr;

        QLineEdit *savePath = nullptr;
        QLineEdit *loadPath = nullptr;
        QTableWidget *tblSer = nullptr;

        {
            auto *tab = new QWidget();
            auto *vbox = new QVBoxLayout(tab);

            auto *splitter = new QSplitter(Qt::Horizontal, tab);

            auto *wA = new QWidget(splitter);
            auto *vA = new QVBoxLayout(wA);
            vA->addWidget(new QLabel("Величина A", wA));
            tblA = makeTable(wA);
            vA->addWidget(tblA);
            auto *rowA = new QHBoxLayout();
            auto *btnAddA = new QPushButton("+ строка", wA);
            auto *btnDelA = new QPushButton("- строка", wA);
            rowA->addWidget(btnAddA); rowA->addWidget(btnDelA);
            vA->addLayout(rowA);
            lblStatsA = new QLabel(wA);
            lblStatsA->setStyleSheet("color:#aaffaa;");
            lblStatsA->setWordWrap(true);
            vA->addWidget(lblStatsA);
            vA->addStretch();

            auto *wB = new QWidget(splitter);
            auto *vB = new QVBoxLayout(wB);
            vB->addWidget(new QLabel("Величина B", wB));
            tblB = makeTable(wB);
            vB->addWidget(tblB);
            auto *rowB = new QHBoxLayout();
            auto *btnAddB = new QPushButton("+ строка", wB);
            auto *btnDelB = new QPushButton("- строка", wB);
            rowB->addWidget(btnAddB); rowB->addWidget(btnDelB);
            vB->addLayout(rowB);
            lblStatsB = new QLabel(wB);
            lblStatsB->setStyleSheet("color:#aaffaa;");
            lblStatsB->setWordWrap(true);
            vB->addWidget(lblStatsB);
            vB->addStretch();

            auto *wR = new QWidget(splitter);
            auto *vR = new QVBoxLayout(wR);
            vR->addWidget(new QLabel("Результат", wR));
            tblR = makeTable(wR);
            vR->addWidget(tblR);
            lblStatsR = new QLabel(wR);
            lblStatsR->setStyleSheet("color:#aaffaa;");
            lblStatsR->setWordWrap(true);
            vR->addWidget(lblStatsR);
            vR->addStretch();

            splitter->setStretchFactor(0, 1);
            splitter->setStretchFactor(1, 1);
            splitter->setStretchFactor(2, 1);
            vbox->addWidget(splitter, 1);

            auto *opRow = new QHBoxLayout();
            auto *btnAdd = new QPushButton("A + B", tab);
            auto *btnMul = new QPushButton("A * B", tab);
            scalarSpin = new QDoubleSpinBox(tab);
            scalarSpin->setRange(-1e6, 1e6);
            scalarSpin->setValue(2.0);
            scalarSpin->setDecimals(4);
            scalarSpin->setPrefix("c = ");
            auto *btnScalarA = new QPushButton("A * c", tab);
            auto *btnScalarB = new QPushButton("B * c", tab);
            opRow->addWidget(btnAdd);
            opRow->addWidget(btnMul);
            opRow->addWidget(scalarSpin);
            opRow->addWidget(btnScalarA);
            opRow->addWidget(btnScalarB);
            opRow->addStretch();
            vbox->addLayout(opRow);

            tabs->addTab(tab, "Операции и характеристики");

            fillTable(tblA, defaultA());
            fillTable(tblB, defaultB());
            updateStatsLabel(tblA, lblStatsA);
            updateStatsLabel(tblB, lblStatsB);

            auto refreshA = [=]() { updateStatsLabel(tblA, lblStatsA); };
            auto refreshB = [=]() { updateStatsLabel(tblB, lblStatsB); };

            QObject::connect(tblA, &QTableWidget::itemChanged, refreshA);
            QObject::connect(tblB, &QTableWidget::itemChanged, refreshB);

            QObject::connect(btnAddA, &QPushButton::clicked, [=]() { tblA->insertRow(tblA->rowCount()); });
            QObject::connect(btnDelA, &QPushButton::clicked, [=]() {
                int r = tblA->currentRow();
                if (r >= 0) { tblA->removeRow(r); updateStatsLabel(tblA, lblStatsA); }
            });
            QObject::connect(btnAddB, &QPushButton::clicked, [=]() { tblB->insertRow(tblB->rowCount()); });
            QObject::connect(btnDelB, &QPushButton::clicked, [=]() {
                int r = tblB->currentRow();
                if (r >= 0) { tblB->removeRow(r); updateStatsLabel(tblB, lblStatsB); }
            });

            auto doOp = [&](auto fn) {
                DRV a, b;
                QString err;
                if (!tryParseDRV(tblA, a, err) || !tryParseDRV(tblB, b, err)) {
                    QMessageBox::warning(&win, "Ошибка", err);
                    return;
                }
                try {
                    DRV r = fn(a, b);
                    fillTable(tblR, r);
                    updateStatsLabel(tblR, lblStatsR);
                } catch (const std::exception &e) {
                    QMessageBox::warning(&win, "Ошибка", QString::fromStdString(e.what()));
                }
            };

            QObject::connect(btnAdd, &QPushButton::clicked, [&]() {
                doOp([](const DRV &a, const DRV &b) { return a + b; });
            });
            QObject::connect(btnMul, &QPushButton::clicked, [&]() {
                doOp([](const DRV &a, const DRV &b) { return a * b; });
            });
            QObject::connect(btnScalarA, &QPushButton::clicked, [&]() {
                DRV a; QString err;
                if (!tryParseDRV(tblA, a, err)) { QMessageBox::warning(&win, "Ошибка", err); return; }
                try {
                    fillTable(tblR, a * scalarSpin->value());
                    updateStatsLabel(tblR, lblStatsR);
                } catch (const std::exception &e) {
                    QMessageBox::warning(&win, "Ошибка", QString::fromStdString(e.what()));
                }
            });
            QObject::connect(btnScalarB, &QPushButton::clicked, [&]() {
                DRV b; QString err;
                if (!tryParseDRV(tblB, b, err)) { QMessageBox::warning(&win, "Ошибка", err); return; }
                try {
                    fillTable(tblR, b * scalarSpin->value());
                    updateStatsLabel(tblR, lblStatsR);
                } catch (const std::exception &e) {
                    QMessageBox::warning(&win, "Ошибка", QString::fromStdString(e.what()));
                }
            });
        }

        {
            auto *tab = new QWidget();
            auto *vbox = new QVBoxLayout(tab);

            auto *srcRow = new QHBoxLayout();
            srcRow->addWidget(new QLabel("Источник:"));
            auto *btnVisA = new QPushButton("Использовать A", tab);
            auto *btnVisB = new QPushButton("Использовать B", tab);
            auto *btnVisR = new QPushButton("Использовать Результат", tab);
            srcRow->addWidget(btnVisA);
            srcRow->addWidget(btnVisB);
            srcRow->addWidget(btnVisR);
            srcRow->addStretch();
            vbox->addLayout(srcRow);

            tblVis = makeTable(tab);
            tblVis->setMaximumHeight(140);
            vbox->addWidget(tblVis);

            auto *plotTabs = new QTabWidget(tab);

            auto *tabPoly = new QWidget();
            auto *vPoly = new QVBoxLayout(tabPoly);
            plotPoly = new PlotWidget(PlotWidget::Polyline, tabPoly);
            vPoly->addWidget(plotPoly);
            plotTabs->addTab(tabPoly, "Полилайн p(x)");

            auto *tabCDF = new QWidget();
            auto *vCDF = new QVBoxLayout(tabCDF);
            plotCDF = new PlotWidget(PlotWidget::CDF, tabCDF);
            vCDF->addWidget(plotCDF);
            plotTabs->addTab(tabCDF, "Функция распределения F(x)");

            auto *tabLaw = new QWidget();
            auto *vLaw = new QVBoxLayout(tabLaw);
            lawText = new QTextEdit(tabLaw);
            lawText->setReadOnly(true);
            vLaw->addWidget(lawText);
            plotTabs->addTab(tabLaw, "Закон распределения");

            vbox->addWidget(plotTabs, 1);

            auto *btnRefresh = new QPushButton("Обновить", tab);
            vbox->addWidget(btnRefresh);

            tabs->addTab(tab, "Визуализация");

            auto refreshVis = [&]() {
                DRV drv; QString err;
                if (!tryParseDRV(tblVis, drv, err)) return;
                plotPoly->setDRV(drv);
                plotCDF->setDRV(drv);
                lawText->setPlainText(makeLawText(drv));
            };

            fillTable(tblVis, defaultA());
            refreshVis();

            QObject::connect(tblVis, &QTableWidget::itemChanged, refreshVis);
            QObject::connect(btnRefresh, &QPushButton::clicked, refreshVis);

            QObject::connect(btnVisA, &QPushButton::clicked, [&]() {
                DRV a; QString err;
                if (!tryParseDRV(tblA, a, err)) { QMessageBox::warning(&win, "Ошибка", err); return; }
                fillTable(tblVis, a);
                refreshVis();
            });
            QObject::connect(btnVisB, &QPushButton::clicked, [&]() {
                DRV b; QString err;
                if (!tryParseDRV(tblB, b, err)) { QMessageBox::warning(&win, "Ошибка", err); return; }
                fillTable(tblVis, b);
                refreshVis();
            });
            QObject::connect(btnVisR, &QPushButton::clicked, [&]() {
                DRV r; QString err;
                if (!tryParseDRV(tblR, r, err)) { QMessageBox::warning(&win, "Ошибка", err); return; }
                fillTable(tblVis, r);
                refreshVis();
            });
        }

        {
            auto *tab = new QWidget();
            auto *vbox = new QVBoxLayout(tab);

            vbox->addWidget(new QLabel("Величина для сериализации", tab));
            tblSer = makeTable(tab);
            tblSer->setMaximumHeight(9999);
            vbox->addWidget(tblSer, 1);

            auto *rowBtns = new QHBoxLayout();
            auto *btnAddSer = new QPushButton("+ строка", tab);
            auto *btnDelSer = new QPushButton("- строка", tab);
            rowBtns->addWidget(btnAddSer);
            rowBtns->addWidget(btnDelSer);
            rowBtns->addStretch();
            vbox->addLayout(rowBtns);

            auto *saveRow = new QHBoxLayout();
            savePath = new QLineEdit(tab);
            savePath->setPlaceholderText("Путь для сохранения...");
            auto *btnBrowseSave = new QPushButton("...", tab);
            btnBrowseSave->setMaximumWidth(30);
            auto *btnSave = new QPushButton("Сохранить", tab);
            saveRow->addWidget(new QLabel("Сохранить:"));
            saveRow->addWidget(savePath, 1);
            saveRow->addWidget(btnBrowseSave);
            saveRow->addWidget(btnSave);
            vbox->addLayout(saveRow);

            auto *loadRow = new QHBoxLayout();
            loadPath = new QLineEdit(tab);
            loadPath->setPlaceholderText("Путь для загрузки...");
            auto *btnBrowseLoad = new QPushButton("...", tab);
            btnBrowseLoad->setMaximumWidth(30);
            auto *btnLoad = new QPushButton("Загрузить", tab);
            loadRow->addWidget(new QLabel("Загрузить:"));
            loadRow->addWidget(loadPath, 1);
            loadRow->addWidget(btnBrowseLoad);
            loadRow->addWidget(btnLoad);
            vbox->addLayout(loadRow);

            tabs->addTab(tab, "Сериализация");

            fillTable(tblSer, defaultA());

            QObject::connect(btnAddSer, &QPushButton::clicked, [=]() { tblSer->insertRow(tblSer->rowCount()); });
            QObject::connect(btnDelSer, &QPushButton::clicked, [=]() {
                int r = tblSer->currentRow();
                if (r >= 0) tblSer->removeRow(r);
            });

            QObject::connect(btnBrowseSave, &QPushButton::clicked, [&]() {
                QString p = QFileDialog::getSaveFileName(&win, "Сохранить", "", "*.txt;;*");
                if (!p.isEmpty()) savePath->setText(p);
            });
            QObject::connect(btnBrowseLoad, &QPushButton::clicked, [&]() {
                QString p = QFileDialog::getOpenFileName(&win, "Загрузить", "", "*.txt;;*");
                if (!p.isEmpty()) loadPath->setText(p);
            });

            QObject::connect(btnSave, &QPushButton::clicked, [&]() {
                DRV drv; QString err;
                if (!tryParseDRV(tblSer, drv, err)) { QMessageBox::warning(&win, "Ошибка", err); return; }
                QString path = savePath->text().trimmed();
                if (path.isEmpty()) { QMessageBox::warning(&win, "Ошибка", "Укажите путь для сохранения"); return; }
                std::ofstream out(path.toStdString());
                if (!out.is_open()) { QMessageBox::warning(&win, "Ошибка", "Не удалось открыть файл для записи"); return; }
                drv.serialize(out);
                QMessageBox::information(&win, "Готово", "Файл сохранён");
            });

            QObject::connect(btnLoad, &QPushButton::clicked, [&]() {
                QString path = loadPath->text().trimmed();
                if (path.isEmpty()) { QMessageBox::warning(&win, "Ошибка", "Укажите путь для загрузки"); return; }
                std::ifstream in(path.toStdString());
                if (!in.is_open()) { QMessageBox::warning(&win, "Ошибка", "Не удалось открыть файл"); return; }
                try {
                    fillTable(tblSer, DRV::deserialize(in));
                    QMessageBox::information(&win, "Готово", "Файл загружен");
                } catch (const std::exception &e) {
                    QMessageBox::warning(&win, "Ошибка", QString::fromStdString(e.what()));
                }
            });
        }

        win.show();
        app.exec();
    }
};

}

int main(int argc, char **argv) {
    tasks::task2::run(argc, argv);
    return 0;
}