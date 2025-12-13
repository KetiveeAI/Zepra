#include "core/zepra_core.h"
#include <iostream>
#include <memory>

int main(int argc, char* argv[]) {
    std::cout << "🦓 Zepra Core Browser v1.0.0 - Unified System" << std::endl;
    std::cout << "================================================" << std::endl;
    
    // Get the unified Zepra Core instance
    ZepraCore::ZepraCore& zepra = ZepraCore::ZepraCore::getInstance();
    
    // Set up callbacks
    zepra.setOnAuthStateChanged([](ZepraCore::AuthState state, const ZepraCore::UserInfo& user) {
        std::cout << "🔐 Authentication state changed: ";
        switch (state) {
            case ZepraCore::AuthState::UNAUTHENTICATED:
                std::cout << "UNAUTHENTICATED" << std::endl;
                break;
            case ZepraCore::AuthState::AUTHENTICATING:
                std::cout << "AUTHENTICATING" << std::endl;
                break;
            case ZepraCore::AuthState::AUTHENTICATED:
                std::cout << "AUTHENTICATED - Welcome " << user.firstName << " " << user.lastName << std::endl;
                break;
            case ZepraCore::AuthState::EXPIRED:
                std::cout << "SESSION EXPIRED" << std::endl;
                break;
            case ZepraCore::AuthState::LOCKED:
                std::cout << "ACCOUNT LOCKED" << std::endl;
                break;
            case ZepraCore::AuthState::ERROR:
                std::cout << "AUTHENTICATION ERROR" << std::endl;
                break;
        }
    });
    
    zepra.setOnDownloadCompleted([](const ZepraCore::DownloadInfo& download) {
        std::cout << "✅ Download completed: " << download.filename << std::endl;
        std::cout << "   Size: " << ZepraCore::Utils::formatFileSize(download.totalSize) << std::endl;
        std::cout << "   Path: " << download.localPath << std::endl;
    });
    
    zepra.setOnDownloadFailed([](const ZepraCore::DownloadInfo& download) {
        std::cout << "❌ Download failed: " << download.filename << std::endl;
        if (!download.errorMessage.empty()) {
            std::cout << "   Error: " << download.errorMessage << std::endl;
        }
    });
    
    // Initialize the unified system
    if (!zepra.initialize("config/zepra_config.json")) {
        std::cerr << "❌ Failed to initialize Zepra Core Browser" << std::endl;
        return -1;
    }
    
    std::cout << "✅ Zepra Core Browser initialized successfully!" << std::endl;
    std::cout << "🚀 Starting browser..." << std::endl;
    std::cout << "" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  F12 - Toggle Developer Tools" << std::endl;
    std::cout << "  F5  - Refresh page" << std::endl;
    std::cout << "  Ctrl+Shift+D - Show Download Manager" << std::endl;
    std::cout << "  Ctrl+Shift+J - Open Console" << std::endl;
    std::cout << "" << std::endl;
    
    // Start the main loop
    zepra.run();
    
    std::cout << "🦓 Zepra Core Browser shutdown complete" << std::endl;
    return 0;
} 