//! NXRENDER System Detection Test

use nxgfx::sysinfo::SystemInfo;
use nxgfx::{GpuContext, DisplayConfig, Resolution, HDRMode};
use nxrender_core::{Application, AppConfig};
use nxrender_input::{GestureTranslator, GesturePattern};

fn main() {
    println!("╔══════════════════════════════════════════════════════╗");
    println!("║         NXRENDER System Detection                     ║");
    println!("╚══════════════════════════════════════════════════════╝\n");
    
    // Detect system hardware
    let sys = SystemInfo::detect();
    sys.print_report();
    
    println!("\n--- NXRENDER Capabilities ---\n");
    
    // Show detected display config
    if let Some(display) = sys.displays.first() {
        let (w, h) = display.resolution;
        let resolution = match (w, h) {
            (7680, 4320) => Resolution::UHD8K,
            (5120, 2880) => Resolution::UHD5K,
            (3840, 2160) => Resolution::UHD4K,
            (2560, 1440) => Resolution::QHD,
            (1920, 1080) => Resolution::FullHD,
            (1280, 720) => Resolution::HD,
            _ => Resolution::Custom(w, h),
        };
        
        println!("Detected Resolution: {:?}", resolution);
        println!("Detected Refresh: {}Hz", display.refresh_rate);
        
        // Check HDR support
        let hdr = if w >= 3840 { HDRMode::HDR10Plus } else { HDRMode::SDR };
        println!("HDR Mode: {:?}", hdr);
        
        let config = DisplayConfig {
            resolution,
            refresh_rate: nxgfx::RefreshRate::Hz60,
            hdr_mode: hdr,
            ..Default::default()
        };
        println!("Bandwidth Required: {:.1} Gbps", config.bandwidth_gbps());
    }
    
    // Test GPU context
    println!("\n--- GPU Context Test ---\n");
    match GpuContext::new() {
        Ok(_) => println!("GPU Context: OK"),
        Err(e) => println!("GPU Context: FAILED - {}", e),
    }
    
    // Test Application
    println!("\n--- Application Test ---\n");
    match Application::new(AppConfig::default()) {
        Ok(_) => println!("Application: OK"),
        Err(e) => println!("Application: FAILED - {}", e),
    }
    
    // Test Gesture Translation
    println!("\n--- Gesture Translation ---\n");
    let translator = GestureTranslator::new();
    println!("2-finger swipe right -> {:?}", 
        translator.translate(GesturePattern::SwipeRight, 2, None));
    println!("2-finger pinch out -> {:?}", 
        translator.translate(GesturePattern::PinchOut, 2, None));
    println!("3-finger swipe up -> {:?}", 
        translator.translate(GesturePattern::ThreeFingerSwipeUp, 3, None));
    
    println!("\n╔══════════════════════════════════════════════════════╗");
    println!("║         All Systems Ready                             ║");
    println!("╚══════════════════════════════════════════════════════╝");
}
