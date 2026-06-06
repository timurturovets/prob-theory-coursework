#include <iostream>
#include <random>
#include <string>

#include <windows.h>

namespace tasks {

class task9 {
public:
    static void run(int argc, char **argv);
};

void task9::run(int argc, char **argv) {
    int n = 10;
    int k = 3;
    long long experiments = 1000000;

    if (argc >= 3) {
        try {
            n = std::stoi(argv[1]);
            k = std::stoi(argv[2]);
            if (argc >= 4) {
                experiments = std::stoll(argv[3]);
            }
        } catch (...) {
            std::cerr << "Аргументы должны быть целыми числами" << std::endl;
            return;
        }
    } else {
        std::cout << "Введите n k [число экспериментов] через пробел (например: 10 3 1000000): ";
        if (!(std::cin >> n >> k)) {
            std::cerr << "Ошибка ввода" << std::endl;
            return;
        }
        std::string rest;
        std::getline(std::cin, rest);
        if (!rest.empty()) {
            try {
                experiments = std::stoll(rest);
            } catch (...) {}
        }
    }

    if (n < 1) {
        std::cerr << "n должно быть >= 1" << std::endl;
        return;
    }
    if (k < 0 || k > n) {
        std::cerr << "k должно быть в диапазоне [0, n]" << std::endl;
        return;
    }
    if (experiments < 1) {
        std::cerr << "Число экспериментов должно быть >= 1" << std::endl;
        return;
    }

    std::mt19937_64 rng(std::random_device{}());
    std::uniform_int_distribution<int> coinDist(0, n - 1);
    std::uniform_real_distribution<double> flipDist(0.0, 1.0);

    long long conditionMet = 0;
    long long fourthHeads = 0;

    for (long long i = 0; i < experiments; ++i) {
        int coinIdx = coinDist(rng);
        bool doubleSided = (coinIdx < k);

        bool first3 = true;
        for (int toss = 0; toss < 3; ++toss) {
            bool heads = doubleSided ? true : (flipDist(rng) < 0.5);
            if (!heads) {
                first3 = false;
                break;
            }
        }

        if (!first3) {
            continue;
        }

        ++conditionMet;

        bool fourth = doubleSided ? true : (flipDist(rng) < 0.5);
        if (fourth) {
            ++fourthHeads;
        }
    }

    double pSimulated = (conditionMet > 0) ? (static_cast<double>(fourthHeads) / conditionMet) : 0.0;

    double pDouble = static_cast<double>(k) / n;
    double pNormal = static_cast<double>(n - k) / n;
    double pFirst3 = pDouble * 1.0 + pNormal * 0.125;
    double pAll4   = pDouble * 1.0 + pNormal * 0.0625;
    double pAnalytic = (pFirst3 > 0.0) ? (pAll4 / pFirst3) : 0.0;

    std::cout << "n = " << n << ", k = " << k << ", экспериментов = " << experiments << std::endl;
    std::cout << std::endl;
    std::cout << "Выполнено условие (первые 3 орла): " << conditionMet << std::endl;
    std::cout << "Из них 4-й бросок тоже орёл: " << fourthHeads << std::endl;
    std::cout << std::endl;
    std::cout << "P(4-й орёл | первые 3 орла) моделирование = " << pSimulated << std::endl;
    std::cout << "P(4-й орёл | первые 3 орла) аналитика     = " << pAnalytic << std::endl;
}

}

int main(int argc, char **argv) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    tasks::task9::run(argc, argv);
    return 0;
}