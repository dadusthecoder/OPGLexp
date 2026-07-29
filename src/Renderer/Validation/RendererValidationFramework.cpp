#include "RendererValidationFramework.h"
#include "../../Vendor/glad.h"
#include "../../Vendor/stb_image.h"
#include <fstream>
#include <cmath>
#include <vector>
#include <iostream>

#ifdef ATLAS_VALIDATION

namespace lgt {

    ValidationConfig RendererValidationFramework::s_Config;

    void RendererValidationFramework::Init() {
        // Initialization logic for the framework
    }

    void RendererValidationFramework::Update() {
        // Framework update logic
    }

    void RendererValidationFramework::Shutdown() {
        // Framework shutdown logic
    }

    ValidationResult RendererValidationFramework::RunRegressionTest(int stage, const std::string& expectedImagePath, uint32_t finalTextureID, uint32_t width, uint32_t height) {
        ValidationResult result;
        result.Stage = "Stage " + std::to_string(stage);
        
        // Read current frame from GPU
        std::vector<unsigned char> currentPixels(width * height * 4);
        glBindTexture(GL_TEXTURE_2D, finalTextureID);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, currentPixels.data());

        // Load expected image
        int expW, expH, expChannels;
        unsigned char* expectedPixels = stbi_load(expectedImagePath.c_str(), &expW, &expH, &expChannels, 4);

        if (!expectedPixels) {
            result.Passed = false;
            result.Errors.push_back("Failed to load expected image: " + expectedImagePath);
            return result;
        }

        if (expW != width || expH != height) {
            result.Passed = false;
            result.Errors.push_back("Image dimension mismatch. Expected: " + std::to_string(expW) + "x" + std::to_string(expH) + ", Got: " + std::to_string(width) + "x" + std::to_string(height));
            stbi_image_free(expectedPixels);
            return result;
        }

        double sumSquaredError = 0.0;
        float maxError = 0.0f;
        uint32_t pixelsAboveEpsilon = 0;
        
        float epsilon = s_Config.Epsilon * 255.0f; // Scale epsilon to 0-255 range

        for (size_t i = 0; i < currentPixels.size(); i++) {
            // Ignore alpha channel for RMSE
            if ((i + 1) % 4 == 0) continue;

            float diff = std::abs((float)currentPixels[i] - (float)expectedPixels[i]);
            
            if (diff > maxError) maxError = diff;
            if (diff > epsilon) pixelsAboveEpsilon++;
            
            sumSquaredError += diff * diff;
        }

        stbi_image_free(expectedPixels);

        // Divide by number of color channels (width * height * 3)
        double mse = sumSquaredError / (width * height * 3.0);
        float rmse = (float)std::sqrt(mse);
        
        // Normalize RMSE and MaxError back to 0.0 - 1.0 range
        rmse /= 255.0f;
        maxError /= 255.0f;

        result.Regression.RMSE = rmse;
        result.Regression.MaxError = maxError;
        result.Regression.PixelsAboveEpsilon = pixelsAboveEpsilon;

        // Pass Criteria (e.g. RMSE < 0.5%)
        if (rmse < 0.005f) {
            result.Passed = true;
            result.Summary = "Regression passed with RMSE: " + std::to_string(rmse);
        } else {
            result.Passed = false;
            result.Summary = "Regression failed. RMSE: " + std::to_string(rmse) + ", Max Error: " + std::to_string(maxError) + ", Pixels > Epsilon: " + std::to_string(pixelsAboveEpsilon);
            result.Errors.push_back(result.Summary);
        }

        return result;
    }

    void RendererValidationFramework::GenerateReport(const std::vector<ValidationResult>& results, const std::string& outputPath) {
        std::ofstream file(outputPath);
        if (!file.is_open()) return;

        file << "# Radiance Cascades Validation Report\n\n";
        
        file << "## Configuration\n";
        file << "- **Deterministic Mode**: " << (s_Config.Deterministic ? "True" : "False") << "\n";
        file << "- **Random Seed**: " << s_Config.RandomSeed << "\n";
        file << "- **Freeze Camera**: " << (s_Config.FreezeCamera ? "True" : "False") << "\n";
        file << "- **Freeze Animations**: " << (s_Config.FreezeAnimations ? "True" : "False") << "\n";
        file << "- **Freeze Time**: " << (s_Config.FreezeTime ? "True" : "False") << "\n\n";

        file << "## Results\n\n";
        file << "| Stage | Status | GPU Time (ms) | RMSE | Max Error | Summary |\n";
        file << "|-------|--------|---------------|------|-----------|---------|\n";

        for (const auto& res : results) {
            std::string status = res.Passed ? "✅ PASS" : "❌ FAIL";
            file << "| " << res.Stage << " | " << status << " | " << res.GPUTimeMS << " | " 
                 << res.Regression.RMSE << " | " << res.Regression.MaxError << " | " << res.Summary << " |\n";
        }
        
        file.close();
    }
} // namespace lgt
#endif
