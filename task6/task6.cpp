#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsEllipseItem>
#include <QtWidgets/QGraphicsLineItem>
#include <QtWidgets/QGraphicsTextItem>

#include <windows.h>

#include <vector>
#include <random>
#include <cmath>
#include <sstream>

namespace tasks {

class task6 {
public:
    static void run(int argc, char** argv);
};

struct Node {
    int id;
    int level;
    bool leaf;
    double stopProbability;
    std::vector<int> children;
    double x;
    double y;
};

class MainWindow : public QMainWindow {
public:
    MainWindow() {
        QWidget* central = new QWidget;
        setCentralWidget(central);

        QVBoxLayout* rootLayout = new QVBoxLayout(central);

        QHBoxLayout* controls = new QHBoxLayout;

        controls->addWidget(new QLabel("M"));

        mBox = new QSpinBox;
        mBox->setRange(2, 6);
        mBox->setValue(3);
        controls->addWidget(mBox);

        controls->addWidget(new QLabel("Глубина"));

        depthBox = new QSpinBox;
        depthBox->setRange(1, 6);
        depthBox->setValue(3);
        controls->addWidget(depthBox);

        controls->addWidget(new QLabel("p"));

        pBox = new QDoubleSpinBox;
        pBox->setRange(0.0, 1.0);
        pBox->setSingleStep(0.05);
        pBox->setValue(0.2);
        controls->addWidget(pBox);

        controls->addWidget(new QLabel("Эксперименты"));

        experimentsBox = new QSpinBox;
        experimentsBox->setRange(100, 1000000);
        experimentsBox->setValue(10000);
        controls->addWidget(experimentsBox);

        controls->addWidget(new QLabel("Лист"));

        targetLeafBox = new QSpinBox;
        targetLeafBox->setRange(0, 100000);
        controls->addWidget(targetLeafBox);

        QPushButton* runButton = new QPushButton("Запуск");
        controls->addWidget(runButton);

        rootLayout->addLayout(controls);

        scene = new QGraphicsScene(this);

        view = new QGraphicsView(scene);
        rootLayout->addWidget(view, 1);

        report = new QTextEdit;
        report->setReadOnly(true);
        report->setMinimumHeight(220);
        rootLayout->addWidget(report);

        resize(1400, 900);

        connect(runButton, &QPushButton::clicked, this, [this]() {
            simulate();
        });

        simulate();
    }

private:
    QSpinBox* mBox;
    QSpinBox* depthBox;
    QDoubleSpinBox* pBox;
    QSpinBox* experimentsBox;
    QSpinBox* targetLeafBox;

    QTextEdit* report;
    QGraphicsView* view;
    QGraphicsScene* scene;

    std::vector<Node> nodes;
    std::vector<int> lastPath;

    void buildTree(int m, int depth) {
        nodes.clear();

        int nextId = 0;

        Node root;
        root.id = nextId++;
        root.level = 0;
        root.leaf = depth == 0;
        root.stopProbability = pBox->value();
        root.x = 0;
        root.y = 0;

        nodes.push_back(root);

        std::vector<int> current;
        current.push_back(0);

        for (int level = 1; level <= depth; ++level) {
            std::vector<int> next;

            for (int parentId : current) {
                for (int i = 0; i < m; ++i) {
                    Node node;
                    node.id = nextId++;
                    node.level = level;
                    node.leaf = level == depth;
                    node.stopProbability = pBox->value();

                    nodes.push_back(node);

                    nodes[parentId].children.push_back(node.id);

                    next.push_back(node.id);
                }
            }

            current = next;
        }

        std::vector<std::vector<int>> levels(depth + 1);

        for (auto& node : nodes) {
            levels[node.level].push_back(node.id);
        }

        for (int level = 0; level <= depth; ++level) {
            int count = static_cast<int>(levels[level].size());

            for (int i = 0; i < count; ++i) {
                int id = levels[level][i];

                nodes[id].x = (i + 1) * 120.0;
                nodes[id].y = (level + 1) * 120.0;
            }
        }
    }

    int chooseChild(const Node& node, std::mt19937& gen) {
        int count = static_cast<int>(node.children.size());

        std::vector<double> weights;

        double sum = 0.0;

        for (int i = 0; i < count; ++i) {
            double w = i + 1;
            weights.push_back(w);
            sum += w;
        }

        std::uniform_real_distribution<double> dist(0.0, sum);

        double r = dist(gen);

        double acc = 0.0;

        for (int i = 0; i < count; ++i) {
            acc += weights[i];

            if (r <= acc) {
                return node.children[i];
            }
        }

        return node.children.back();
    }

    void drawTree() {
        scene->clear();

        for (auto& node : nodes) {
            for (int childId : node.children) {
                scene->addLine(
                    node.x,
                    node.y,
                    nodes[childId].x,
                    nodes[childId].y
                );
            }
        }

        for (size_t i = 1; i < lastPath.size(); ++i) {
            Node& a = nodes[lastPath[i - 1]];
            Node& b = nodes[lastPath[i]];

            QPen pen(Qt::red);
            pen.setWidth(4);

            scene->addLine(a.x, a.y, b.x, b.y, pen);
        }

        for (auto& node : nodes) {
            QColor color = node.leaf ? Qt::green : Qt::cyan;

            if (!lastPath.empty()) {
                for (int id : lastPath) {
                    if (id == node.id) {
                        color = Qt::red;
                    }
                }
            }

            scene->addEllipse(
                node.x - 15,
                node.y - 15,
                30,
                30,
                QPen(Qt::black),
                QBrush(color)
            );

            auto* text = scene->addText(QString::number(node.id));
            text->setPos(node.x - 10, node.y - 35);
        }
    }

    void simulate() {
        int m = mBox->value();
        int depth = depthBox->value();
        int experiments = experimentsBox->value();

        buildTree(m, depth);

        std::vector<int> leaves;

        for (auto& node : nodes) {
            if (node.leaf) {
                leaves.push_back(node.id);
            }
        }

        if (!leaves.empty()) {
            targetLeafBox->setMaximum(leaves.back());
        }

        int targetLeaf = targetLeafBox->value();

        std::random_device rd;
        std::mt19937 gen(rd());

        long long targetHits = 0;
        long long lHits = 0;

        int l = depth + 1;

        lastPath.clear();

        for (int experiment = 0; experiment < experiments; ++experiment) {
            std::vector<int> path;

            int current = 0;

            path.push_back(current);

            while (true) {
                Node& node = nodes[current];

                if (node.leaf) {
                    break;
                }

                std::uniform_real_distribution<double> stopDist(0.0, 1.0);

                if (stopDist(gen) < node.stopProbability) {
                    break;
                }

                current = chooseChild(node, gen);

                path.push_back(current);
            }

            if (experiment == 0) {
                lastPath = path;
            }

            if (!path.empty()) {
                int lastNode = path.back();

                if (lastNode == targetLeaf) {
                    ++targetHits;
                }
            }

            if (static_cast<int>(path.size()) == l) {
                ++lHits;
            }
        }

        drawTree();

        double pLeaf = static_cast<double>(targetHits) / experiments;
        double pLength = static_cast<double>(lHits) / experiments;

        std::stringstream ss;

        ss << "M = " << m << std::endl;
        ss << "Глубина = " << depth << std::endl;
        ss << "p = " << pBox->value() << std::endl;
        ss << "Экспериментов = " << experiments << std::endl;
        ss << std::endl;
        ss << "Листовой узел = " << targetLeaf << std::endl;
        ss << "Вероятность попадания в лист = " << pLeaf << std::endl;
        ss << std::endl;
        ss << "l = " << l << std::endl;
        ss << "Вероятность пройти l вершин = " << pLength << std::endl;
        ss << std::endl;
        ss << "Последний визуализированный путь:" << std::endl;

        for (size_t i = 0; i < lastPath.size(); ++i) {
            ss << lastPath[i];

            if (i + 1 < lastPath.size()) {
                ss << " -> ";
            }
        }

        report->setPlainText(QString::fromStdString(ss.str()));
    }
};

void task6::run(int argc, char** argv) {
    QApplication app(argc, argv);

    MainWindow window;
    window.show();

    QApplication::exec();
}

}

int main(int argc, char** argv) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    tasks::task6::run(argc, argv);

    return 0;
}