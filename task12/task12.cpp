#include <cmath>
#include <iomanip>
#include <iostream>
#include <random>

#include <windows.h>

namespace tasks {

class task12 {
public:
    static void run();
};

void task12::run() {
    double d;
    double L;
    int experiments;

    std::cout << "Введите расстояние между прямыми d: ";
    std::cin >> d;

    std::cout << "Введите длину иглы L: ";
    std::cin >> L;

    std::cout << "Введите количество бросков: ";
    std::cin >> experiments;

    if (d <= 0) {
        std::cout << "d должно быть больше 0" << std::endl;
        return;
    }

    if (L <= 0) {
        std::cout << "L должно быть больше 0" << std::endl;
        return;
    }

    if (L > d) {
        std::cout << "L должно быть не больше d" << std::endl;
        return;
    }

    if (experiments <= 0) {
        std::cout << "Количество бросков должно быть больше 0" << std::endl;
        return;
    }

    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_real_distribution<double> xDist(0.0, d / 2.0);
    std::uniform_real_distribution<double> phiDist(0.0, M_PI / 2.0);

    int hits = 0;

    for (int i = 0; i < experiments; i++) {
        double x = xDist(gen);
        double phi = phiDist(gen);

        if (x <= (L / 2.0) * std::sin(phi)) {
            hits++;
        }
    }

    double experimentalProbability =
        static_cast<double>(hits) / experiments;

    double exactProbability =
        (2.0 * L) / (M_PI * d);

    std::cout << std::fixed << std::setprecision(6);

    std::cout << std::endl;
    std::cout << "Количество пересечений: " << hits << std::endl;
    std::cout << "Количество бросков: " << experiments << std::endl;
    std::cout << "Экспериментальная вероятность: "
              << experimentalProbability << std::endl;
    std::cout << "Точная вероятность: "
              << exactProbability << std::endl;
    std::cout << "Абсолютная погрешность: "
              << std::abs(experimentalProbability - exactProbability)
              << std::endl;
}

}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    tasks::task12::run();

    return 0;
}