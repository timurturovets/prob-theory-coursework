#include <iostream>
#include <random>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <windows.h>

namespace tasks {

class task2 {
public:
    static void run(int argc, char **argv);
};

struct Result {
    long long count_before_k = 0;
    long long count_even = 0;
    long long total = 0;
    double emp_a = 0.0;
    double emp_b = 0.0;
    double an_a = 0.0;
    double an_b = 0.0;
};

Result compute(int k) {
    const int N = 10000000;
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> coin(0, 1);

    Result res;
    res.total = N;

    for (int i = 0; i < N; i++) {
        int prev = coin(rng);
        int tosses = 1;
        while (true) {
            int cur = coin(rng);
            tosses++;
            if (cur == prev) break;
            prev = cur;
        }
        if (tosses < k) res.count_before_k++;
        if (tosses % 2 == 0) res.count_even++;
    }

    res.emp_a = static_cast<double>(res.count_before_k) / N;
    res.emp_b = static_cast<double>(res.count_even) / N;
    res.an_a = (k >= 3) ? (1.0 - std::pow(0.5, k - 2)) : 0.0;
    res.an_b = 2.0 / 3.0;

    return res;
}

void task2::run(int argc, char **argv) {
    int k = 0;

    if (argc >= 2) {
        try {
            k = std::stoi(argv[1]);
        } catch (...) {
            std::cerr << "Ошибка: аргумент должен быть целым числом" << std::endl;
            return;
        }
    } else {
        std::cout << "Введите k: ";
        if (!(std::cin >> k)) {
            std::cerr << "Ошибка ввода" << std::endl;
            return;
        }
    }

    if (k < 2) {
        std::cerr << "Ошибка: k должно быть не меньше 2" << std::endl;
        return;
    }

    Result res = compute(k);

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "Бросание монеты до двух одинаковых подряд" << std::endl;
    std::cout << "k = " << k << std::endl;
    std::cout << "Число симуляций: " << res.total << std::endl;
    std::cout << std::endl;
    std::cout << "а) P(опыт закончится до " << k << "-го броска):" << std::endl;
    std::cout << "Аналитическая: " << res.an_a << std::endl;
    std::cout << "Эмпирическая: " << res.emp_a << std::endl;
    std::cout << "Отклонение: " << std::abs(res.an_a - res.emp_a) << std::endl;
    std::cout << std::endl;
    std::cout << "б) P(чётное число бросаний):" << std::endl;
    std::cout << "Аналитическая: " << res.an_b << std::endl;
    std::cout << "Эмпирическая: " << res.emp_b << std::endl;
    std::cout << "Отклонение: " << std::abs(res.an_b - res.emp_b) << std::endl;
}

}

int main(int argc, char **argv) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    tasks::task2::run(argc, argv);
    return 0;
}