#include <algorithm>
#include <iostream>
#include <map>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include <windows.h>

namespace tasks {

class task4 {
public:
    static void run(int argc, char **argv);
};

void task4::run(int argc, char **argv) {
    int n = 5;
    int K = 0;

    if (argc >= 3) {
        try {
            n = std::stoi(argv[1]);
            K = std::stoi(argv[2]);
        } catch (...) {
            std::cerr << "Аргументы должны быть целыми числами" << std::endl;
            std::cerr << "Использование: " << (argc > 0 ? argv[0] : "task4") << " n K" << std::endl;
            return;
        }
    } else if (argc >= 2) {
        try {
            K = std::stoi(argv[1]);
        } catch (...) {
            std::cerr << "K должно быть целым числом" << std::endl;
            return;
        }
    } else {
        std::cout << "Введите n K через пробел (например: 5 100): ";
        if (!(std::cin >> n >> K)) {
            std::cerr << "Ошибка ввода" << std::endl;
            return;
        }
    }

    if (n < 1) {
        std::cerr << "n должно быть >= 1" << std::endl;
        return;
    }

    if (K < 1) {
        std::cerr << "K должно быть >= 1" << std::endl;
        return;
    }

    std::mt19937 rng(std::random_device{}());

    std::map<int, int> counts;
    for (int t = 1; t <= n; ++t) {
        counts[t] = 0;
    }

    std::vector<int> keys(static_cast<size_t>(n));
    std::iota(keys.begin(), keys.end(), 1);

    std::cout << "n = " << n << ", K = " << K << std::endl;
    std::cout << std::endl;

    for (int exp = 1; exp <= K; ++exp) {
        std::vector<int> perm = keys;
        std::shuffle(perm.begin(), perm.end(), rng);

        int trial = -1;
        for (int t = 0; t < n; ++t) {
            if (perm[static_cast<size_t>(t)] == 1) {
                trial = t + 1;
                break;
            }
        }

        counts[trial]++;

        std::cout << "Эксперимент " << exp << ": ключ найден на испытании " << trial << std::endl;
    }

    std::cout << std::endl;
    std::cout << "Эмпирические вероятности исходов:" << std::endl;

    double analytic = 1.0 / n;

    for (int t = 1; t <= n; ++t) {
        double emp = static_cast<double>(counts[t]) / K;
        std::cout << "P(испытание = " << t << "): эмпирическая = " << emp
                  << ", аналитическая = " << analytic
                  << ", count = " << counts[t] << std::endl;
    }
}

}

int main(int argc, char **argv) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    tasks::task4::run(argc, argv);
    return 0;
}