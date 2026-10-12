#include "config.h"
#include "judge_worker.h"
#include "logging.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <thread>

namespace {

std::atomic<bool> g_running{true};

void handle_shutdown_signal(int) {
    g_running = false;
}

}  // namespace

int main() {
    ConfigManager& config_manager = ConfigManager::instance();
    if (!config_manager.load("./config.yaml")) {
        log_error("Failed to load ./config.yaml");
        return 1;
    }

    const ServerConfig config = config_manager.config();
    g_log_level = parse_log_level(config.log_level);

    std::signal(SIGINT, handle_shutdown_signal);
    std::signal(SIGTERM, handle_shutdown_signal);

    JudgeWorkerPool pool(static_cast<std::size_t>(config.judge.worker_count));
    pool.start();
    log_info("Judge worker pool started with " +
             std::to_string(pool.worker_count()) + " workers");

    while (g_running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    pool.stop();
    log_info("Judge worker pool stopped");
    return 0;
}
