#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <set>
#include <string>

#include <windows.h>

namespace tasks {

class task7 {
public:
    static void run(int argc, char **argv);
};

static double simulate_R(int k, long long N, int trials, std::mt19937_64 &rng) {
    std::uniform_int_distribution<long long> dist(0, N - 1);
    int hits = 0;

    for (int t = 0; t < trials; ++t) {
        std::set<long long> X, Y;
        while ((int)X.size() < k) X.insert(dist(rng));
        while ((int)Y.size() < k) Y.insert(dist(rng));

        bool intersects = false;
        for (long long x : X) {
            if (Y.count(x)) {
                intersects = true;
                break;
            }
        }
        if (intersects) ++hits;
    }

    return static_cast<double>(hits) / trials;
}

void task7::run(int argc, char **argv) {
    int trials = 5000;
    long long N_vernam = 65536LL;

    int k_input = -1;
    long long N_input = -1;

    if (argc >= 3) {
        try {
            k_input = std::stoi(argv[1]);
            N_input = std::stoll(argv[2]);
            if (argc >= 4) trials = std::stoi(argv[3]);
        } catch (...) {
            std::cerr << "Аргументы: k N [trials]" << std::endl;
            return;
        }
    } else {
        std::cout << "Введите k и N (например: 300 65536): ";
        if (!(std::cin >> k_input >> N_input)) {
            std::cerr << "Ошибка ввода" << std::endl;
            return;
        }
    }

    if (k_input < 1) {
        std::cerr << "k должно быть >= 1" << std::endl;
        return;
    }
    if (N_input < 2) {
        std::cerr << "N должно быть >= 2" << std::endl;
        return;
    }
    if (k_input >= N_input) {
        std::cerr << "k должно быть < N" << std::endl;
        return;
    }
    if (trials < 1) {
        std::cerr << "trials должно быть >= 1" << std::endl;
        return;
    }

    std::mt19937_64 rng(std::random_device{}());

    double R_sim = simulate_R(k_input, N_input, trials, rng);
    double R_bound = 1.0 - std::exp(-(static_cast<double>(k_input) * k_input) / static_cast<double>(N_input));

    std::cout << "k = " << k_input << ", N = " << N_input << std::endl;
    std::cout << "R(k,N) симуляция = " << R_sim << std::endl;
    std::cout << "R(k,N) нижняя оценка 1-exp(-k^2/N) = " << R_bound << std::endl;

    double k_threshold_analytic = std::sqrt(static_cast<double>(N_input) * std::log(2.0));
    std::cout << "Аналитический порог R > 1/2: k > sqrt(N*ln2) = " << k_threshold_analytic << std::endl;
    std::cout << "Минимальное целое k для R > 1/2 (оценка): " << static_cast<long long>(std::ceil(k_threshold_analytic)) << std::endl;

    std::cout << std::endl;
    std::cout << "Атака на шифр Вернама (N = 2^16 = 65536, m = 16 бит)" << std::endl;

    double k_vernam = std::sqrt(static_cast<double>(N_vernam) * std::log(2.0));
    long long k_vernam_int = static_cast<long long>(std::ceil(k_vernam));

    std::cout << "Для вероятности коллизии > 1/2 нужно k > " << k_vernam << std::endl;
    std::cout << "Минимальное k = " << k_vernam_int << std::endl;

    double R_vernam_sim = simulate_R(static_cast<int>(k_vernam_int), N_vernam, trials, rng);
    double R_vernam_bound = 1.0 - std::exp(-(static_cast<double>(k_vernam_int * k_vernam_int)) / static_cast<double>(N_vernam));

    std::cout << "R(" << k_vernam_int << ", 65536) симуляция = " << R_vernam_sim << std::endl;
    std::cout << "R(" << k_vernam_int << ", 65536) нижняя оценка = " << R_vernam_bound << std::endl;
}

}

int main(int argc, char **argv) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    tasks::task7::run(argc, argv);

    return 0;
}