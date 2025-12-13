//! System Hardware Detection
//!
//! Detects GPU, display, and audio devices.

use std::process::Command;

#[derive(Debug, Clone)]
pub struct GpuInfo {
    pub name: String,
    pub vendor: String,
    pub driver: String,
    pub vram_mb: u32,
}

#[derive(Debug, Clone)]
pub struct DisplayInfo {
    pub name: String,
    pub resolution: (u32, u32),
    pub refresh_rate: u32,
    pub primary: bool,
    pub connected: bool,
}

#[derive(Debug, Clone)]
pub struct AudioInfo {
    pub name: String,
    pub device_type: String,
    pub driver: String,
}

#[derive(Debug, Clone)]
pub struct SystemInfo {
    pub gpus: Vec<GpuInfo>,
    pub displays: Vec<DisplayInfo>,
    pub audio_devices: Vec<AudioInfo>,
}

impl SystemInfo {
    pub fn detect() -> Self {
        Self {
            gpus: detect_gpus(),
            displays: detect_displays(),
            audio_devices: detect_audio(),
        }
    }
    
    pub fn print_report(&self) {
        println!("=== System Hardware Detection ===\n");
        
        println!("GPU:");
        for (i, gpu) in self.gpus.iter().enumerate() {
            println!("  [{}] {}", i, gpu.name);
            println!("      Vendor: {}", gpu.vendor);
            println!("      Driver: {}", gpu.driver);
            if gpu.vram_mb > 0 {
                println!("      VRAM: {} MB", gpu.vram_mb);
            }
        }
        
        println!("\nDisplays:");
        for (i, display) in self.displays.iter().enumerate() {
            let primary = if display.primary { " (Primary)" } else { "" };
            println!("  [{}] {}{}", i, display.name, primary);
            println!("      Resolution: {}x{} @ {}Hz", 
                display.resolution.0, display.resolution.1, display.refresh_rate);
        }
        
        println!("\nAudio:");
        for (i, audio) in self.audio_devices.iter().enumerate() {
            println!("  [{}] {} ({})", i, audio.name, audio.device_type);
        }
    }
}

fn detect_gpus() -> Vec<GpuInfo> {
    let mut gpus = Vec::new();
    
    // Try lspci for GPU info
    if let Ok(output) = Command::new("lspci").arg("-v").output() {
        let text = String::from_utf8_lossy(&output.stdout);
        for line in text.lines() {
            if line.contains("VGA") || line.contains("3D controller") {
                let name = line.split(':').last().unwrap_or("Unknown GPU").trim().to_string();
                let vendor = if line.contains("NVIDIA") {
                    "NVIDIA"
                } else if line.contains("AMD") || line.contains("ATI") {
                    "AMD"
                } else if line.contains("Intel") {
                    "Intel"
                } else {
                    "Unknown"
                }.to_string();
                
                gpus.push(GpuInfo {
                    name,
                    vendor,
                    driver: String::new(),
                    vram_mb: 0,
                });
            }
        }
    }
    
    // Try glxinfo for driver info
    if let Ok(output) = Command::new("glxinfo").arg("-B").output() {
        let text = String::from_utf8_lossy(&output.stdout);
        for line in text.lines() {
            if line.contains("OpenGL renderer") {
                if let Some(gpu) = gpus.first_mut() {
                    gpu.name = line.split(':').last().unwrap_or(&gpu.name).trim().to_string();
                }
            }
            if line.contains("OpenGL version") {
                if let Some(gpu) = gpus.first_mut() {
                    gpu.driver = line.split(':').last().unwrap_or("").trim().to_string();
                }
            }
        }
    }
    
    if gpus.is_empty() {
        gpus.push(GpuInfo {
            name: "Unknown GPU".to_string(),
            vendor: "Unknown".to_string(),
            driver: "Unknown".to_string(),
            vram_mb: 0,
        });
    }
    
    gpus
}

fn detect_displays() -> Vec<DisplayInfo> {
    let mut displays = Vec::new();
    
    // Use xrandr for X11
    if let Ok(output) = Command::new("xrandr").arg("--current").output() {
        let text = String::from_utf8_lossy(&output.stdout);
        let mut current_display: Option<String> = None;
        
        for line in text.lines() {
            if line.contains(" connected") {
                let parts: Vec<&str> = line.split_whitespace().collect();
                if let Some(name) = parts.first() {
                    current_display = Some(name.to_string());
                    let primary = line.contains("primary");
                    
                    // Parse resolution from connected line
                    let mut resolution = (0u32, 0u32);
                    let mut refresh = 60u32;
                    
                    for part in &parts {
                        if part.contains('x') && part.contains('+') {
                            // e.g., "1920x1080+0+0"
                            let res_part = part.split('+').next().unwrap_or("");
                            let dims: Vec<&str> = res_part.split('x').collect();
                            if dims.len() == 2 {
                                resolution.0 = dims[0].parse().unwrap_or(0);
                                resolution.1 = dims[1].parse().unwrap_or(0);
                            }
                        }
                    }
                    
                    displays.push(DisplayInfo {
                        name: name.to_string(),
                        resolution,
                        refresh_rate: refresh,
                        primary,
                        connected: true,
                    });
                }
            } else if let Some(ref _display_name) = current_display {
                // Look for active resolution line with *
                if line.contains('*') {
                    let parts: Vec<&str> = line.split_whitespace().collect();
                    if let Some(res) = parts.first() {
                        let dims: Vec<&str> = res.split('x').collect();
                        if dims.len() == 2 {
                            if let Some(last) = displays.last_mut() {
                                last.resolution.0 = dims[0].parse().unwrap_or(last.resolution.0);
                                last.resolution.1 = dims[1].parse().unwrap_or(last.resolution.1);
                            }
                        }
                    }
                    // Get refresh rate
                    for part in &parts {
                        if part.contains('*') || part.contains('+') {
                            let rate_str = part.replace('*', "").replace('+', "");
                            if let Ok(rate) = rate_str.parse::<f32>() {
                                if let Some(last) = displays.last_mut() {
                                    last.refresh_rate = rate as u32;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    if displays.is_empty() {
        displays.push(DisplayInfo {
            name: "Unknown Display".to_string(),
            resolution: (1920, 1080),
            refresh_rate: 60,
            primary: true,
            connected: true,
        });
    }
    
    displays
}

fn detect_audio() -> Vec<AudioInfo> {
    let mut devices = Vec::new();
    
    // Use pactl for PulseAudio/PipeWire
    if let Ok(output) = Command::new("pactl").args(["list", "sinks", "short"]).output() {
        let text = String::from_utf8_lossy(&output.stdout);
        for line in text.lines() {
            let parts: Vec<&str> = line.split('\t').collect();
            if parts.len() >= 2 {
                devices.push(AudioInfo {
                    name: parts[1].to_string(),
                    device_type: "Output".to_string(),
                    driver: "PulseAudio/PipeWire".to_string(),
                });
            }
        }
    }
    
    // Also try aplay for ALSA
    if let Ok(output) = Command::new("aplay").arg("-l").output() {
        let text = String::from_utf8_lossy(&output.stdout);
        for line in text.lines() {
            if line.starts_with("card") {
                let name = line.split(':').nth(1).unwrap_or("Unknown").trim();
                let name = name.split('[').next().unwrap_or(name).trim();
                if !devices.iter().any(|d| d.name.contains(name)) {
                    devices.push(AudioInfo {
                        name: name.to_string(),
                        device_type: "ALSA".to_string(),
                        driver: "ALSA".to_string(),
                    });
                }
            }
        }
    }
    
    if devices.is_empty() {
        devices.push(AudioInfo {
            name: "Unknown Audio".to_string(),
            device_type: "Unknown".to_string(),
            driver: "Unknown".to_string(),
        });
    }
    
    devices
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_system_detection() {
        let info = SystemInfo::detect();
        assert!(!info.gpus.is_empty());
        assert!(!info.displays.is_empty());
    }
}
