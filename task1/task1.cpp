#include <algorithm>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

#include <windows.h>

namespace tasks {
    class task1 {
    public:
        static void run(int argc, char **argv);
    };
    using Perm = std::vector<int>;

    struct Result {
        std::vector<Perm> fav_ai;
        std::vector<Perm> fav_aj;
        std::vector<Perm> fav_union;
        std::vector<Perm> fav_intersect;

        double p_ai = 0.0;
        double p_aj = 0.0;
        double p_union = 0.0;
        double p_intersect = 0.0;

        double p_ai_analytic = 0.0;
        double p_aj_analytic = 0.0;
        double p_intersect_analytic = 0.0;
        double p_union_analytic = 0.0;

        long long total = 0;
    };

    std::string perm_to_string(const Perm &p) {
        std::string s = "[";
        for (size_t k = 0; k < p.size(); ++k) {
            s += std::to_string(p[k]);
            if (k + 1 < p.size()) {
                s += " ";
            }
        }
        s += "]";
        return s;
    }

    bool event_a(const Perm &p, int i) {
        return p[static_cast<size_t>(i - 1)] == i;
    }

    Result compute(int n, int i, int j) {
        Result res;
        Perm p(static_cast<size_t>(n));
        std::iota(p.begin(), p.end(), 1);

        do {
            ++res.total;

            bool ai = event_a(p, i);
            bool aj = event_a(p, j);
            bool uni = ai || aj;
            bool ins = ai && aj;

            if (ai) {
                res.fav_ai.push_back(p);
            }
            if (aj) {
                res.fav_aj.push_back(p);
            }
            if (uni) {
                res.fav_union.push_back(p);
            }
            if (ins) {
                res.fav_intersect.push_back(p);
            }
        } while (std::next_permutation(p.begin(), p.end()));

        res.p_ai = static_cast<double>(res.fav_ai.size()) / res.total;
        res.p_aj = static_cast<double>(res.fav_aj.size()) / res.total;
        res.p_union = static_cast<double>(res.fav_union.size()) / res.total;
        res.p_intersect = static_cast<double>(res.fav_intersect.size()) / res.total;

        res.p_ai_analytic = 1.0 / n;
        res.p_aj_analytic = 1.0 / n;
        res.p_intersect_analytic = 1.0 / (static_cast<double>(n) * (n - 1));
        res.p_union_analytic = res.p_ai_analytic + res.p_aj_analytic - res.p_intersect_analytic;

        return res;
    }

    std::string make_report(const Result &res, int n, int i, int j) {
        auto format_list = [&](const std::vector<Perm> &perms, const std::string &name) {
            std::string text = name + " (" + std::to_string(perms.size()) + "):\n";
            const size_t limit = std::min<size_t>(perms.size(), 30);
            for (size_t index = 0; index < limit; ++index) {
                text += "  " + perm_to_string(perms[index]) + "\n";
            }
            if (perms.size() > limit) {
                text += "  ... показано " + std::to_string(limit) + " из " + std::to_string(perms.size()) + "\n";
            }
            text += "\n";
            return text;
        };

        std::string text;
        text += "N = " + std::to_string(n) + "    i = " + std::to_string(i) + "    j = " + std::to_string(j) + "\n";
        text += "Всего перестановок: " + std::to_string(res.total) + "\n\n";
        text += format_list(res.fav_ai, "Ai");
        text += format_list(res.fav_aj, "Aj");
        text += format_list(res.fav_union, "Ai + Aj");
        text += format_list(res.fav_intersect, "Ai * Aj");
        text += "Вероятности\n";
        text += "p(Ai)      счёт = " + std::to_string(res.p_ai) + "    аналитика = " + std::to_string(res.p_ai_analytic) + "\n";
        text += "p(Aj)      счёт = " + std::to_string(res.p_aj) + "    аналитика = " + std::to_string(res.p_aj_analytic) + "\n";
        text += "p(Ai*Aj)   счёт = " + std::to_string(res.p_intersect) + "    аналитика = " + std::to_string(res.p_intersect_analytic) + "\n";
        text += "p(Ai+Aj)   счёт = " + std::to_string(res.p_union) + "    аналитика = " + std::to_string(res.p_union_analytic) + "\n\n";
        text += "Проверка теоремы сложения\n";
        text += "p(Ai+Aj) = " + std::to_string(res.p_union) + "\n";
        text += "p(Ai)+p(Aj)-p(Ai*Aj) = " + std::to_string(res.p_ai + res.p_aj - res.p_intersect) + "\n";
        return text;
    }

    void task1::run(int argc, char **argv) {
        int n = 4;
        int i = 1;
        int j = 2;

        if (argc >= 4) {
            try {
                n = std::stoi(argv[1]);
                i = std::stoi(argv[2]);
                j = std::stoi(argv[3]);
            } catch (...) {
                std::cerr << "Ошибка: аргументы должны быть целыми числами\n";
                std::cerr << "Использование: " << (argc > 0 ? argv[0] : "task1") << " N i j\n";
                return;
            }
        } else {
            std::cout << "Введите N i j через пробел (например: 4 1 2): ";
            if (!(std::cin >> n >> i >> j)) {
                std::cerr << "Ошибка ввода\n";
                return;
            }
        }

        if (n < 2) {
            std::cerr << "Ошибка: N должно быть не меньше 2\n";
            return;
        }
        if (i < 1 || i > n || j < 1 || j > n) {
            std::cerr << "Ошибка: i и j должны быть в диапазоне [1, N]\n";
            return;
        }
        if (i == j) {
            std::cerr << "Ошибка: i и j должны быть различными\n";
            return;
        }

        Result res = compute(n, i, j);
        std::cout << make_report(res, n, i, j);
    }
}

int main(int argc, char **argv) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    tasks::task1::run(argc, argv);
    return 0;
}
