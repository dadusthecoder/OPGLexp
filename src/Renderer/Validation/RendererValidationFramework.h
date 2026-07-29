#pragma once

#define ATLAS_VALIDATION

#ifdef ATLAS_VALIDATION

#include <string>
#include <vector>
#include <cstdint>

namespace lgt {

    struct ValidationConfig {
        bool EnableValidation = true;
        bool StopOnError = false;
        int  TargetStage = 0; // -1 to 14
        
        // Deterministic Validation
        bool Deterministic = true;
        uint32_t RandomSeed = 12345;
        bool FreezeCamera = true;
        bool FreezeAnimations = true;
        bool FreezeTime = true;
        float Epsilon = 1e-4f;
    };

    struct RegressionMetrics {
        float RMSE = 0.0f;
        float MaxError = 0.0f;
        uint32_t PixelsAboveEpsilon = 0;
    };

    struct ValidationResult {
        bool Passed = false;
        std::string Stage;
        double GPUTimeMS = 0.0;
        std::string Summary;
        std::string ScreenshotPath;
        std::vector<std::string> Errors;
        RegressionMetrics Regression;
    };

    class ValidationStage {
    public:
        virtual ~ValidationStage() = default;
        virtual ValidationResult Execute(const ValidationConfig& config) = 0;
    };

    class RendererValidationFramework {
    public:
        static void Init();
        static void Update();
        static void Shutdown();

        static ValidationResult RunRegressionTest(int stage, const std::string& expectedImagePath, uint32_t finalTextureID, uint32_t width, uint32_t height);
        static void GenerateReport(const std::vector<ValidationResult>& results, const std::string& outputPath);

        static ValidationConfig& GetConfig() { return s_Config; }

    private:
        static ValidationConfig s_Config;
    };

}

#endif
