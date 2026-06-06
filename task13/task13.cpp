#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <random>
#include <set>
#include <vector>

#include <windows.h>

namespace tasks {

class task13 {
public:
    static void run(int argc, char **argv);
};

struct Edge {
    int u;
    int v;
    int w;
};

struct DSU {
    std::vector<int> parent;
    std::vector<int> rank;

    DSU(int n) : parent(n), rank(n, 0) {
        std::iota(parent.begin(), parent.end(), 0);
    }

    int find(int v) {
        if (parent[v] == v) {
            return v;
        }

        parent[v] = find(parent[v]);

        return parent[v];
    }

    bool unite(int a, int b) {
        a = find(a);
        b = find(b);

        if (a == b) {
            return false;
        }

        if (rank[a] < rank[b]) {
            std::swap(a, b);
        }

        parent[b] = a;

        if (rank[a] == rank[b]) {
            ++rank[a];
        }

        return true;
    }
};

void task13::run(int argc, char **argv) {
    int n;
    int experiments;

    std::cout << "Введите количество вершин n: " << std::endl;
    std::cin >> n;

    if (n < 2) {
        std::cout << "n должно быть >= 2" << std::endl;
        return;
    }

    std::cout << "Введите количество испытаний: " << std::endl;
    std::cin >> experiments;

    if (experiments < 1) {
        std::cout << "Количество испытаний должно быть >= 1" << std::endl;
        return;
    }

    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_int_distribution<> weightDist(1, 10);
    std::uniform_real_distribution<> edgeDist(0.0, 1.0);

    std::vector<double> A;
    std::vector<double> B;
    std::vector<double> D;
    std::vector<double> E;
    std::vector<double> F;
    std::vector<double> G;
    std::vector<double> H;

    for (int experiment = 0; experiment < experiments; ++experiment) {
        std::vector<std::vector<int>> matrix(n, std::vector<int>(n, 0));
        std::vector<Edge> edges;

        for (int i = 0; i < n - 1; ++i) {
            int w = weightDist(gen);

            matrix[i][i + 1] = w;
            matrix[i + 1][i] = w;

            edges.push_back({i, i + 1, w});
        }

        for (int i = 0; i < n; ++i) {
            for (int j = i + 1; j < n; ++j) {
                if (matrix[i][j] != 0) {
                    continue;
                }

                if (edgeDist(gen) < 0.4) {
                    int w = weightDist(gen);

                    matrix[i][j] = w;
                    matrix[j][i] = w;

                    edges.push_back({i, j, w});
                }
            }
        }

        std::vector<Edge> sortedEdges = edges;

        std::sort(
            sortedEdges.begin(),
            sortedEdges.end(),
            [](const Edge &a, const Edge &b) {
                return a.w < b.w;
            }
        );

        DSU mstDsu(n);

        int mstWeight = 0;

        for (const auto &edge : sortedEdges) {
            if (mstDsu.unite(edge.u, edge.v)) {
                mstWeight += edge.w;
            }
        }

        A.push_back(mstWeight);

        int longestCycleEdges = 0;
        int maxWeightCycleEdges = 0;
        int maxCycleWeight = 0;

        for (size_t e1 = 0; e1 < edges.size(); ++e1) {
            for (size_t e2 = e1 + 1; e2 < edges.size(); ++e2) {
                for (size_t e3 = e2 + 1; e3 < edges.size(); ++e3) {
                    std::set<int> vertices;

                    vertices.insert(edges[e1].u);
                    vertices.insert(edges[e1].v);
                    vertices.insert(edges[e2].u);
                    vertices.insert(edges[e2].v);
                    vertices.insert(edges[e3].u);
                    vertices.insert(edges[e3].v);

                    if (vertices.size() != 3) {
                        continue;
                    }

                    int cycleWeight =
                        edges[e1].w +
                        edges[e2].w +
                        edges[e3].w;

                    longestCycleEdges = std::max(longestCycleEdges, 3);

                    if (cycleWeight > maxCycleWeight) {
                        maxCycleWeight = cycleWeight;
                        maxWeightCycleEdges = 3;
                    }
                }
            }
        }

        B.push_back(longestCycleEdges);
        D.push_back(maxWeightCycleEdges);

        int isolatedVertices = 0;

        for (int i = 0; i < n; ++i) {
            bool isolated = true;

            for (int j = 0; j < n; ++j) {
                if (matrix[i][j] != 0) {
                    isolated = false;
                    break;
                }
            }

            if (isolated) {
                ++isolatedVertices;
            }
        }

        E.push_back(isolatedVertices);

        double treesCount = std::pow(n, n - 2);

        F.push_back(treesCount);

        DSU componentsDsu(n);

        for (const auto &edge : edges) {
            componentsDsu.unite(edge.u, edge.v);
        }

        std::set<int> components;

        for (int i = 0; i < n; ++i) {
            components.insert(componentsDsu.find(i));
        }

        int componentsCount = static_cast<int>(components.size());

        G.push_back(componentsCount);

        std::map<int, std::vector<int>> groups;

        for (int i = 0; i < n; ++i) {
            groups[componentsDsu.find(i)].push_back(i);
        }

        int completeComponents = 0;

        for (const auto &group : groups) {
            const std::vector<int> &vertices = group.second;

            bool complete = true;

            for (size_t i = 0; i < vertices.size() && complete; ++i) {
                for (size_t j = i + 1; j < vertices.size(); ++j) {
                    if (matrix[vertices[i]][vertices[j]] == 0) {
                        complete = false;
                        break;
                    }
                }
            }

            if (complete) {
                ++completeComponents;
            }
        }

        H.push_back(completeComponents);
    }

    auto printStats = [](const std::string &name, const std::vector<double> &values) {
        double mean = 0.0;

        for (double value : values) {
            mean += value;
        }

        mean /= values.size();

        double variance = 0.0;

        for (double value : values) {
            variance += (value - mean) * (value - mean);
        }

        variance /= values.size();

        std::cout << name << std::endl;
        std::cout << "Математическое ожидание: " << mean << std::endl;
        std::cout << "Дисперсия: " << variance << std::endl;
        std::cout << std::endl;
    };

    std::cout << std::fixed << std::setprecision(6);

    printStats("A - сумма весов минимального остовного дерева", A);
    printStats("B - длина самого большого цикла", B);
    printStats("D - количество рёбер самого тяжёлого цикла", D);
    printStats("E - количество изолированных вершин", E);
    printStats("F - количество деревьев", F);
    printStats("G - количество компонент связности", G);
    printStats("H - количество компонент связности, являющихся полными подграфами", H);
}

}

int main(int argc, char **argv) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    tasks::task13::run(argc, argv);

    return 0;
}