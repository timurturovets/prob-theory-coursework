#include <cmath>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>

#include <windows.h>

namespace tasks {

    class task10 {
    public:
        static void run(int argc, char **argv);
    };

    static const double LAMBDA = 1.94;
    static const int K_MAX = 200;

    double poisson_pmf(int k, double lam) {
        double log_p = -lam + k * std::log(lam);
        for (int i = 1; i <= k; ++i) {
            log_p -= std::log(static_cast<double>(i));
        }

        return std::exp(log_p);
    }

    struct AnalyticResult {
        double p_b;
        double p_ab;
        double p_cond;
        double p_unconditional;
    };

    AnalyticResult compute_analytic(int m) {
        double p_b = 0.0;
        double p_ab = 0.0;
        double p_a_num = 0.0;
        double p_kplus_sum = 0.0;

        for (int k = 1; k <= K_MAX; ++k) {
            double pk = poisson_pmf(k, LAMBDA);
            double prob_all_boys = std::pow(0.5, k);

            p_kplus_sum += pk;
            p_b += pk * prob_all_boys;

            if (k >= m) {
                p_ab += pk * prob_all_boys;
                p_a_num += pk;
            }
        }

        AnalyticResult res;
        res.p_b = p_b;
        res.p_ab = p_ab;
        res.p_cond = (p_b > 0.0) ? (p_ab / p_b) : 0.0;
        res.p_unconditional = (p_kplus_sum > 0.0) ? (p_a_num / p_kplus_sum) : 0.0;

        return res;
    }

    struct SimResult {
        long long total_families;
        long long families_with_boys_only;
        long long families_boys_only_ge_m;
        long long families_ge_m;
        double p_cond_empiric;
        double p_unconditional_empiric;
    };

    SimResult simulate(int m, long long n_sim, unsigned int seed) {
        std::mt19937_64 rng(seed);

        std::vector<double> pk_table(K_MAX + 1);
        double sum_k_plus = 0.0;
        for (int k = 1; k <= K_MAX; ++k) {
            pk_table[k] = poisson_pmf(k, LAMBDA);
            sum_k_plus += pk_table[k];
        }

        std::vector<double> cdf(K_MAX + 2, 0.0);
        for (int k = 1; k <= K_MAX; ++k) {
            cdf[k] = cdf[k - 1] + pk_table[k] / sum_k_plus;
        }
        cdf[K_MAX + 1] = 1.0;

        std::uniform_real_distribution<double> uni(0.0, 1.0);

        SimResult res{};
        res.total_families = n_sim;

        for (long long i = 0; i < n_sim; ++i) {
            double u = uni(rng);
            int k = 1;
            while (k <= K_MAX && cdf[k] < u) {
                ++k;
            }

            bool all_boys = true;
            for (int c = 0; c < k; ++c) {
                if (uni(rng) >= 0.5) {
                    all_boys = false;
                    break;
                }
            }

            if (all_boys) {
                ++res.families_with_boys_only;
                if (k >= m) {
                    ++res.families_boys_only_ge_m;
                }
            }

            if (k >= m) {
                ++res.families_ge_m;
            }
        }

        res.p_cond_empiric = (res.families_with_boys_only > 0)
            ? (static_cast<double>(res.families_boys_only_ge_m) / res.families_with_boys_only)
            : 0.0;

        res.p_unconditional_empiric = (n_sim > 0)
            ? (static_cast<double>(res.families_ge_m) / n_sim)
            : 0.0;

        return res;
    }

    void task10::run(int argc, char **argv) {
        int m = 2;
        long long n_sim = 1000000;

        if (argc >= 2) {
            try {
                m = std::stoi(argv[1]);
            } catch (...) {
                std::cerr << "Аргументы должны быть целыми числами" << std::endl;
                return;
            }
        } else {
            std::cout << "Введите m (минимальное число детей, >= 1): ";
            if (!(std::cin >> m)) {
                std::cerr << "Ошибка ввода" << std::endl;
                return;
            }
        }

        if (argc >= 3) {
            try {
                n_sim = std::stoll(argv[2]);
            } catch (...) {
                std::cerr << "Аргументы должны быть целыми числами" << std::endl;
                return;
            }
        }

        if (m < 1) {
            std::cerr << "m должно быть >= 1" << std::endl;
            return;
        }

        if (n_sim <= 0) {
            std::cerr << "Число симуляций должно быть > 0" << std::endl;
            return;
        }

        AnalyticResult an = compute_analytic(m);
        SimResult sim = simulate(m, n_sim, 42u);

        std::cout << std::fixed << std::setprecision(6);

        std::cout << "Параметры: lambda = " << LAMBDA << ", m = " << m << ", симуляций = " << n_sim << std::endl;
        std::cout << std::endl;

        std::cout << "Аналитический расчёт" << std::endl;
        std::cout << "P(B) = все дети мальчики: " << an.p_b << std::endl;
        std::cout << "P(A∩B) = >= m детей И все мальчики: " << an.p_ab << std::endl;
        std::cout << "P(A|B) = P(>= m детей | все мальчики): " << an.p_cond << std::endl;
        std::cout << "P(A) = P(>= m детей) безусловная: " << an.p_unconditional << std::endl;
        std::cout << "Разница |P(A|B) - P(A)|: " << std::abs(an.p_cond - an.p_unconditional) << std::endl;
        std::cout << std::endl;

        std::cout << "Эмпирический расчёт (Монте-Карло)" << std::endl;
        std::cout << "Семей с только мальчиками: " << sim.families_with_boys_only << std::endl;
        std::cout << "Из них с >= m детьми: " << sim.families_boys_only_ge_m << std::endl;
        std::cout << "P(A|B) эмпирическая: " << sim.p_cond_empiric << std::endl;
        std::cout << "P(A) эмпирическая: " << sim.p_unconditional_empiric << std::endl;
        std::cout << "Разница |P(A|B) - P(A)| эмпирическая: " << std::abs(sim.p_cond_empiric - sim.p_unconditional_empiric) << std::endl;
        std::cout << std::endl;

        std::cout << "Сравнение аналитики и симуляции" << std::endl;
        std::cout << "P(A|B): аналитика = " << an.p_cond << ", эмпирика = " << sim.p_cond_empiric << ", погрешность = " << std::abs(an.p_cond - sim.p_cond_empiric) << std::endl;
        std::cout << "P(A):   аналитика = " << an.p_unconditional << ", эмпирика = " << sim.p_unconditional_empiric << ", погрешность = " << std::abs(an.p_unconditional - sim.p_unconditional_empiric) << std::endl;
    }
}

int main(int argc, char **argv) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    tasks::task10::run(argc, argv);

    return 0;
}