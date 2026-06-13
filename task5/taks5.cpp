#include <fstream>
#include <iostream>
#include <map>
#include <random>
#include <string>
#include <vector>
#include <cmath>

#include <windows.h>

namespace tasks {

class task5 {
public:
    static void run(int argc, char **argv);
};

struct Config {
    double p = 0.5;
    double q = 0.5;
    int max_n = 32;
    double s_plus = 1.0;
    double s_minus = 1.0;
    long long trials = 1000000;
};

std::string trim(const std::string &str) {
    size_t a = str.find_first_not_of(" \t\r\n\"");
    size_t b = str.find_last_not_of(" \t\r\n\"");

    if (a == std::string::npos) {
        return "";
    }

    return str.substr(a, b - a + 1);
}

Config parse_json(const std::string &path) {
    Config cfg;
    std::ifstream f(path);

    if (!f.is_open()) {
        return cfg;
    }

    std::string line;
    while (std::getline(f, line)) {
        size_t colon = line.find(':');
        if (colon == std::string::npos) {
            continue;
        }

        std::string key = trim(line.substr(0, colon));
        std::string val = trim(line.substr(colon + 1));

        if (!val.empty() && val.back() == ',') {
            val.pop_back();
            val = trim(val);
        }

        try {
            if (key == "p") {
                cfg.p = std::stod(val);
            } else if (key == "q") {
                cfg.q = std::stod(val);
            } else if (key == "max_n") {
                cfg.max_n = std::stoi(val);
            } else if (key == "s_plus") {
                cfg.s_plus = std::stod(val);
            } else if (key == "s_minus") {
                cfg.s_minus = std::stod(val);
            } else if (key == "trials") {
                cfg.trials = std::stoll(val);
            }
        } catch (...) {}
    }

    return cfg;
}

void task5::run(int argc, char **argv) {
    std::string config_path = "config.json";

    if (argc >= 2) {
        config_path = argv[1];
    }

    Config cfg = parse_json(config_path);

    if (cfg.p <= 0.0 || cfg.q <= 0.0) {
        std::cerr << "p и q должны быть > 0" << std::endl;
        return;
    }

    double pq_sum = cfg.p + cfg.q;

    if (pq_sum <= 0.0) {
        std::cerr << "p + q должно быть > 0" << std::endl;
        return;
    }

    cfg.p /= pq_sum;
    cfg.q = 1.0 - cfg.p;

    if (cfg.max_n < 2 || cfg.max_n > 32) {
        std::cerr << "max_n должно быть в диапазоне [2, 32]" << std::endl;
        return;
    }

    if (cfg.s_plus <= 0.0 || cfg.s_minus <= 0.0) {
        std::cerr << "s_plus и s_minus должны быть > 0" << std::endl;
        return;
    }

    if (cfg.trials <= 0) {
        std::cerr << "trials должно быть > 0" << std::endl;
        return;
    }

    std::mt19937_64 rng(std::random_device{}());
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    std::map<int, long long> return_counts;
    long long no_return = 0;

    for (long long t = 0; t < cfg.trials; ++t) {
        double pos = 0.0;
        int returned_at = -1;

        for (int step = 1; step <= cfg.max_n; ++step) {
            if (dist(rng) < cfg.p) {
                pos += cfg.s_plus;
            } else {
                pos -= cfg.s_minus;
            }

            if (std::abs(pos) < 1e-9) {
                returned_at = step;
                break;
            }
        }

        if (returned_at == -1) {
            ++no_return;
        } else {
            ++return_counts[returned_at];
        }
    }

    std::cout << "Параметры: "
              << "p=" << cfg.p
              << " q=" << cfg.q
              << " s_plus=" << cfg.s_plus
              << " s_minus=" << cfg.s_minus
              << " max_n=" << cfg.max_n
              << " trials=" << cfg.trials
              << std::endl;

    std::cout << std::endl;
    std::cout << "N    P(первое возвращение на N-м шаге)" << std::endl;

    for (int n = 1; n <= cfg.max_n; ++n) {
        double prob = 0.0;

        auto it = return_counts.find(n);
        if (it != return_counts.end()) {
            prob = static_cast<double>(it->second) / cfg.trials;
        }

        std::cout << n << "    " << prob << std::endl;
    }

    std::cout << std::endl;

    long long total_returned = 0;

    for (auto &kv : return_counts) {
        total_returned += kv.second;
    }

    double p_any_return =
        static_cast<double>(total_returned) / cfg.trials;

    double p_no_return =
        static_cast<double>(no_return) / cfg.trials;

    std::cout << "P(возврат хотя бы раз за "
              << cfg.max_n
              << " шагов) = "
              << p_any_return
              << std::endl;

    std::cout << "P(нет возврата за "
              << cfg.max_n
              << " шагов) = "
              << p_no_return
              << std::endl;
}

}

int main(int argc, char **argv) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    tasks::task5::run(argc, argv);

    return 0;
}
