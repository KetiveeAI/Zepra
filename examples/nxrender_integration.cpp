// NXRENDER Browser Integration Example
// Shows how to use NXRENDER from C++ browser code

#include <iostream>
#include <cstring>
#include "nxrender.h"

int main() {
    std::cout << "=== NXRENDER Browser Integration ===" << std::endl;
    std::cout << "Version: " << nx_version() << std::endl;
    
    // Detect system hardware
    std::cout << "\n--- System Detection ---" << std::endl;
    NxSystemInfo* sysinfo = nx_detect_system();
    if (sysinfo) {
        std::cout << "GPU: " << sysinfo->gpu_name << std::endl;
        std::cout << "GPU Vendor: " << sysinfo->gpu_vendor << std::endl;
        std::cout << "GPU Driver: " << sysinfo->gpu_driver << std::endl;
        std::cout << "Display: " << sysinfo->display_name 
                  << " (" << sysinfo->display_width << "x" << sysinfo->display_height 
                  << " @ " << sysinfo->display_refresh << "Hz)" << std::endl;
        nx_free_system_info(sysinfo);
    }
    
    // Create GPU context
    std::cout << "\n--- GPU Context ---" << std::endl;
    NxGpuContext* gpu = nx_gpu_create();
    if (gpu) {
        std::cout << "GPU Context created successfully" << std::endl;
        
        // Draw some primitives
        NxColor red = nx_color_rgb(255, 0, 0);
        NxColor blue = nx_color_rgb(0, 0, 255);
        NxColor white = nx_color_rgb(255, 255, 255);
        
        nx_gpu_fill_rect(gpu, nx_rect(10, 10, 100, 50), red);
        nx_gpu_fill_rounded_rect(gpu, nx_rect(120, 10, 100, 50), blue, 8.0);
        nx_gpu_fill_circle(gpu, 280, 35, 25, white);
        nx_gpu_draw_text(gpu, "NXRENDER", 10, 80, white);
        
        std::cout << "Drawing commands issued" << std::endl;
        
        nx_gpu_destroy(gpu);
        std::cout << "GPU Context destroyed" << std::endl;
    } else {
        std::cout << "Failed to create GPU Context" << std::endl;
    }
    
    // Create theme
    std::cout << "\n--- Theme System ---" << std::endl;
    NxTheme* theme = nx_theme_dark();
    if (theme) {
        NxColor primary = nx_theme_get_primary_color(theme);
        NxColor bg = nx_theme_get_background_color(theme);
        
        std::cout << "Primary color: RGB(" 
                  << (int)primary.r << ", " << (int)primary.g << ", " << (int)primary.b << ")" << std::endl;
        std::cout << "Background color: RGB(" 
                  << (int)bg.r << ", " << (int)bg.g << ", " << (int)bg.b << ")" << std::endl;
        
        nx_theme_destroy(theme);
    }
    
    std::cout << "\n=== Integration Complete ===" << std::endl;
    return 0;
}
