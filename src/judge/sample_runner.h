#pragma once

#include "config.h"
#include "oj/models.h"

#include <string>
#include <vector>

struct SampleRunResult {
    bool passed = false;
    std::string verdict;
    std::string message;
};

class SampleRunner {
public:
    explicit SampleRunner(const JudgeConfig& config);

    SampleRunResult run(const std::string& code,
                        const std::vector<TestCase>& samples) const;

private:
    JudgeConfig config_;
};
