#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <map>
#include <queue>
#include <random>
#include <string>
#include <vector>

#include <windows.h>

namespace tasks {

    class task3 {
    public:
        static void run(int argc, char **argv);
    };

    struct AssemblyJob {
        long long finish_time;
        int result_quality;
        bool operator>(const AssemblyJob &o) const { return finish_time > o.finish_time; }
    };

    struct RecycleJob {
        long long finish_time;
        int product_quality;
        std::vector<int> amounts;
        bool operator>(const RecycleJob &o) const { return finish_time > o.finish_time; }
    };

    int roll_quality(int base_quality, int modules, std::mt19937 &rng) {
        if (modules == 0) return base_quality;

        std::uniform_real_distribution<double> dist(0.0, 1.0);

        double r = dist(rng);
        double cumulative = 0.0;

        int max_k = std::min(modules, 5 - base_quality);
        for (int k = 1; k <= max_k; ++k) {
            double p_k = 52.0 * std::pow(10.0, -2.0 - k) * modules;
            cumulative += p_k;

            if (r < cumulative) {
                return base_quality + k;
            }
        }

        return base_quality;
    }

    void task3::run(int argc, char **argv) {
        int AM = 0, RM = 0;
        long long t_asm = 0, t_rec = 0;
        int qm_asm = 0, qm_rec = 0;

        std::vector<int> speeds;
        std::vector<int> recipe;

        if (argc < 2) {
            std::cout << "Введите AM RM t_сб t_пр qm_сб qm_пр:" << std::endl;
            if (!(std::cin >> AM >> RM >> t_asm >> t_rec >> qm_asm >> qm_rec)) {
                std::cerr << "Ошибка ввода" << std::endl;
                return;
            }

            int n;
            std::cout << "Введите n (количество компонентов):" << std::endl;
            if (!(std::cin >> n) || n <= 0) {
                std::cerr << "n должно быть > 0" << std::endl;

                return;
            }

            speeds.resize(n);
            recipe.resize(n);

            std::cout << "Введите скорости s1..s" << n << ":" << std::endl;
            for (int i = 0; i < n; ++i) {
                if (!(std::cin >> speeds[i])) {
                    std::cerr << "Ошибка ввода скоростей" << std::endl;

                    return;
                }
            }

            std::cout << "Введите количества компонентов c1..c" << n << ":" << std::endl;
            for (int i = 0; i < n; ++i) {
                if (!(std::cin >> recipe[i])) {
                    std::cerr << "Ошибка ввода рецепта" << std::endl;

                    return;
                }
            }
        } else {
            int idx = 1;
            try {
                AM = std::stoi(argv[idx++]);
                RM = std::stoi(argv[idx++]);
                t_asm = std::stoll(argv[idx++]);
                t_rec = std::stoll(argv[idx++]);
                qm_asm = std::stoi(argv[idx++]);
                qm_rec = std::stoi(argv[idx++]);

                int n = std::stoi(argv[idx++]);
                if (n <= 0) {
                    std::cerr << "n должно быть > 0" << std::endl;
                    return;
                }

                speeds.resize(n);
                recipe.resize(n);

                for (int i = 0; i < n; ++i) {
                    speeds[i] = std::stoi(argv[idx++]);
                }

                for (int i = 0; i < n; ++i) {
                    recipe[i] = std::stoi(argv[idx++]);
                }
            } catch (...) {
                std::cerr << "Ошибка парсинга аргументов" << std::endl;
                return;
            }
        }

        if (AM <= 0) { std::cerr << "AM должно быть > 0" << std::endl; return; }
        if (RM <= 0) { std::cerr << "RM должно быть > 0" << std::endl; return; }
        if (t_asm <= 0) { std::cerr << "t_сб должно быть > 0" << std::endl; return; }
        if (t_rec <= 0) { std::cerr << "t_пр должно быть > 0" << std::endl; return; }
        if (qm_asm < 0 || qm_asm > 4) { std::cerr << "qm_сб должно быть в [0,4]" << std::endl; return; }
        if (qm_rec < 0 || qm_rec > 4) { std::cerr << "qm_пр должно быть в [0,4]" << std::endl; return; }

        int n = static_cast<int>(speeds.size());
        for (int i = 0; i < n; ++i) {
            if (speeds[i] <= 0) { std::cerr << "Скорость компонента должна быть > 0" << std::endl; return; }
            if (recipe[i] <= 0) { std::cerr << "Количество в рецепте должно быть > 0" << std::endl; return; }
        }

        double slowdown_asm = 1.0 + 0.1 * qm_asm;
        double slowdown_rec = 1.0 + 0.1 * qm_rec;
        long long eff_t_asm = static_cast<long long>(std::ceil(t_asm * slowdown_asm));
        long long eff_t_rec = static_cast<long long>(std::ceil(t_rec * slowdown_rec));

        std::mt19937 rng(42);

        std::vector<std::map<int, long long>> comp_stock(n);
        for (int i = 0; i < n; ++i) {
            comp_stock[i][1] = 0;
        }

        std::map<int, long long> product_stock;

        std::vector<long long> times_q3, times_q4, times_q5;

        std::vector asm_free(AM, 0LL);
        std::vector rec_free(RM, 0LL);

        std::priority_queue<AssemblyJob, std::vector<AssemblyJob>, std::greater<AssemblyJob>> asm_done;
        std::priority_queue<RecycleJob, std::vector<RecycleJob>, std::greater<RecycleJob>> rec_done;

        long long legendary_count = 0;
        long long time = 0;

        auto can_assemble = [&]() -> bool {
            for (int i = 0; i < n; ++i) {
                long long total = 0;
                for (auto &kv : comp_stock[i]) total += kv.second;

                if (total < recipe[i]) return false;
            }
            return true;
        };

        auto can_recycle = [&]() -> bool {
            for (auto &kv : product_stock) {
                if (kv.second > 0) return true;
            }
            return false;
        };

        auto consume_components = [&]() -> int {
            int base_q = 5;
            for (int i = 0; i < n; ++i) {
                long long needed = recipe[i];

                for (auto it = comp_stock[i].begin(); it != comp_stock[i].end() && needed > 0; ++it) {
                    long long use = std::min(it->second, needed);

                    it->second -= use;
                    needed -= use;

                    base_q = std::min(base_q, it->first);
                }
            }
            return base_q;
        };

        auto add_components_from_recycle = [&](int product_quality, const std::vector<int> &amounts_vec) {
            for (int i = 0; i < n; ++i) {
                int comp_q = roll_quality(product_quality, qm_rec, rng);

                comp_q = std::min(comp_q, 5);
                comp_stock[i][comp_q] += amounts_vec[i];
            }
        };

        auto pick_product_for_recycle = [&]() -> int {
            for (auto it = product_stock.rbegin(); it != product_stock.rend(); ++it) {
                if (it->second > 0) return it->first;
            }
            return -1;
        };

        auto consume_product = [&](int q) {
            product_stock[q]--;
            if (product_stock[q] == 0) product_stock.erase(q);
        };

        while (legendary_count < 25) {
            for (int i = 0; i < n; ++i) {
                comp_stock[i][1] += static_cast<long long>(speeds[i]);
            }

            while (!asm_done.empty() && asm_done.top().finish_time <= time) {
                AssemblyJob job = asm_done.top();
                asm_done.pop();

                int q = job.result_quality;
                product_stock[q]++;

                if (q == 3) times_q3.push_back(job.finish_time);
                if (q == 4) times_q4.push_back(job.finish_time);
                if (q == 5) {
                    times_q5.push_back(job.finish_time);
                    legendary_count++;
                }
            }

            while (!rec_done.empty() && rec_done.top().finish_time <= time) {
                RecycleJob job = rec_done.top();
                rec_done.pop();
                add_components_from_recycle(job.product_quality, job.amounts);
            }

            for (int a = 0; a < AM; ++a) {
                if (asm_free[a] <= time && can_assemble()) {
                    int base_q = consume_components();
                    int result_q = roll_quality(base_q, qm_asm, rng);
                    result_q = std::min(result_q, 5);

                    AssemblyJob job;
                    job.finish_time = time + eff_t_asm;
                    job.result_quality = result_q;

                    asm_done.push(job);
                    asm_free[a] = time + eff_t_asm;
                }
            }

            for (int r = 0; r < RM; ++r) {
                if (rec_free[r] <= time && can_recycle()) {
                    int pq = pick_product_for_recycle();
                    if (pq < 0) break;

                    consume_product(pq);

                    RecycleJob job;
                    job.finish_time = time + eff_t_rec;
                    job.product_quality = pq;

                    for (int i = 0; i < n; ++i) {
                        int raw = static_cast<int>(std::ceil(recipe[i] * 0.25));
                        job.amounts.push_back(raw);
                    }

                    rec_done.push(job);
                    rec_free[r] = time + eff_t_rec;
                }
            }

            if (legendary_count >= 25) break;

            time++;
        }

        std::cout << "Уровень 3 (" << times_q3.size() << " шт.):" << std::endl;
        for (size_t i = 0; i < times_q3.size(); ++i) {
            if (i > 0) std::cout << " ";
            std::cout << "t=" << times_q3[i];
        }
        if (!times_q3.empty()) std::cout << std::endl;

        std::cout << "Уровень 4 (" << times_q4.size() << " шт.):" << std::endl;
        for (size_t i = 0; i < times_q4.size(); ++i) {
            if (i > 0) std::cout << " ";
            std::cout << "t=" << times_q4[i];
        }
        if (!times_q4.empty()) std::cout << std::endl;

        std::cout << "Уровень 5 (" << times_q5.size() << " шт.):" << std::endl;
        for (size_t i = 0; i < times_q5.size(); ++i) {
            if (i > 0) std::cout << " ";
            std::cout << "t=" << times_q5[i];
        }
        if (!times_q5.empty()) std::cout << std::endl;

        std::cout << "Моделирование завершено на t=" << time << std::endl;
    }
}

int main(int argc, char **argv) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    tasks::task3::run(argc, argv);
    return 0;
}