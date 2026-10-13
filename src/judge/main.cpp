#include "config.h"
#include "database.h"
#include "judge_worker.h"
#include "logging.h"
#include "oj/models.h"
#include "sample_runner.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <string>
#include <thread>
#include <vector>

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

    Database database;
    if (!database.connect(config.database)) {
        log_error("Judge worker failed to connect to MySQL");
        return 1;
    }

    std::signal(SIGINT, handle_shutdown_signal);
    std::signal(SIGTERM, handle_shutdown_signal);

    JudgeWorkerPool pool(static_cast<std::size_t>(config.judge.worker_count));
    pool.start();
    log_info("Judge worker pool started with " +
             std::to_string(pool.worker_count()) + " workers");

    while (g_running.load()) {
        JudgeTask task;
        if (!database.claim_pending_submission(task)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            continue;
        }

        const JudgeConfig judge_config = config.judge;
        bool submitted = pool.submit([&database, judge_config, task]() {
            std::vector<TestCase> test_cases;
            if (!database.list_all_test_cases(task.problem_id, test_cases)) {
                std::string error;
                database.finish_submission(task.submission_id, "RE", error);
                return;
            }

            SampleRunner runner(judge_config);
            SampleRunResult result = runner.run(task.code, test_cases);

            std::string error;
            if (!database.finish_submission(task.submission_id, result.verdict,
                                            error)) {
                log_error("Failed to update submission " +
                          std::to_string(task.submission_id) + ": " + error);
            }
        });

        if (!submitted) {
            std::string error;
            database.finish_submission(task.submission_id, "RE", error);
            break;
        }
    }

    pool.stop();
    log_info("Judge worker pool stopped");
    return 0;
}
