//! Display Configuration
//!
//! 8K, HDR10+, and screen management.

use crate::Size;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Resolution {
    HD,       // 1280x720
    FullHD,   // 1920x1080
    QHD,      // 2560x1440
    UHD4K,    // 3840x2160
    UHD5K,    // 5120x2880
    UHD8K,    // 7680x4320
    Custom(u32, u32),
}

impl Resolution {
    pub fn size(&self) -> Size {
        match self {
            Resolution::HD => Size::new(1280.0, 720.0),
            Resolution::FullHD => Size::new(1920.0, 1080.0),
            Resolution::QHD => Size::new(2560.0, 1440.0),
            Resolution::UHD4K => Size::new(3840.0, 2160.0),
            Resolution::UHD5K => Size::new(5120.0, 2880.0),
            Resolution::UHD8K => Size::new(7680.0, 4320.0),
            Resolution::Custom(w, h) => Size::new(*w as f32, *h as f32),
        }
    }
    
    pub fn pixels(&self) -> u64 {
        let s = self.size();
        (s.width * s.height) as u64
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum RefreshRate {
    Hz24,
    Hz30,
    Hz50,
    Hz60,
    Hz120,
    Hz144,
    Hz240,
    Custom(u32),
}

impl RefreshRate {
    pub fn hz(&self) -> u32 {
        match self {
            RefreshRate::Hz24 => 24,
            RefreshRate::Hz30 => 30,
            RefreshRate::Hz50 => 50,
            RefreshRate::Hz60 => 60,
            RefreshRate::Hz120 => 120,
            RefreshRate::Hz144 => 144,
            RefreshRate::Hz240 => 240,
            RefreshRate::Custom(hz) => *hz,
        }
    }
    
    pub fn frame_time_ms(&self) -> f32 {
        1000.0 / self.hz() as f32
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum HDRMode {
    SDR,
    HDR10,
    HDR10Plus,
    DolbyVision,
    HLG,
}

impl HDRMode {
    pub fn max_luminance(&self) -> u32 {
        match self {
            HDRMode::SDR => 100,
            HDRMode::HDR10 => 1000,
            HDRMode::HDR10Plus => 4000,
            HDRMode::DolbyVision => 10000,
            HDRMode::HLG => 1000,
        }
    }
    
    pub fn bit_depth(&self) -> u8 {
        match self {
            HDRMode::SDR => 8,
            _ => 10,
        }
    }
    
    pub fn supports_dynamic_metadata(&self) -> bool {
        matches!(self, HDRMode::HDR10Plus | HDRMode::DolbyVision)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ColorSpace {
    SRGB,
    DisplayP3,
    Rec2020,
    AdobeRGB,
}

#[derive(Debug, Clone)]
pub struct DisplayConfig {
    pub resolution: Resolution,
    pub refresh_rate: RefreshRate,
    pub hdr_mode: HDRMode,
    pub color_space: ColorSpace,
    pub vsync: bool,
    pub max_frame_latency: u32,
    pub scaling_factor: f32,
}

impl Default for DisplayConfig {
    fn default() -> Self {
        Self {
            resolution: Resolution::FullHD,
            refresh_rate: RefreshRate::Hz60,
            hdr_mode: HDRMode::SDR,
            color_space: ColorSpace::SRGB,
            vsync: true,
            max_frame_latency: 2,
            scaling_factor: 1.0,
        }
    }
}

impl DisplayConfig {
    pub fn uhd_8k_hdr() -> Self {
        Self {
            resolution: Resolution::UHD8K,
            refresh_rate: RefreshRate::Hz60,
            hdr_mode: HDRMode::HDR10Plus,
            color_space: ColorSpace::Rec2020,
            vsync: true,
            max_frame_latency: 1,
            scaling_factor: 1.0,
        }
    }
    
    pub fn uhd_4k_120hz() -> Self {
        Self {
            resolution: Resolution::UHD4K,
            refresh_rate: RefreshRate::Hz120,
            hdr_mode: HDRMode::HDR10Plus,
            color_space: ColorSpace::Rec2020,
            vsync: true,
            max_frame_latency: 1,
            scaling_factor: 1.0,
        }
    }
    
    pub fn bandwidth_gbps(&self) -> f32 {
        let pixels = self.resolution.pixels() as f32;
        let fps = self.refresh_rate.hz() as f32;
        let bits = self.hdr_mode.bit_depth() as f32 * 3.0; // RGB
        (pixels * fps * bits) / 1_000_000_000.0
    }
}

#[derive(Debug)]
pub struct ScreenInfo {
    pub id: u32,
    pub name: String,
    pub primary: bool,
    pub config: DisplayConfig,
    pub physical_size_mm: (u32, u32),
    pub position: (i32, i32),
}

pub struct ScreenManager {
    screens: Vec<ScreenInfo>,
    active_screen: u32,
}

impl Default for ScreenManager {
    fn default() -> Self {
        Self::new()
    }
}

impl ScreenManager {
    pub fn new() -> Self {
        Self {
            screens: vec![ScreenInfo {
                id: 0,
                name: String::from("Primary"),
                primary: true,
                config: DisplayConfig::default(),
                physical_size_mm: (600, 340),
                position: (0, 0),
            }],
            active_screen: 0,
        }
    }
    
    pub fn screens(&self) -> &[ScreenInfo] {
        &self.screens
    }
    
    pub fn active(&self) -> Option<&ScreenInfo> {
        self.screens.iter().find(|s| s.id == self.active_screen)
    }
    
    pub fn switch_to(&mut self, screen_id: u32) -> bool {
        if self.screens.iter().any(|s| s.id == screen_id) {
            self.active_screen = screen_id;
            true
        } else {
            false
        }
    }
    
    pub fn add_screen(&mut self, name: String, config: DisplayConfig) -> u32 {
        let id = self.screens.len() as u32;
        self.screens.push(ScreenInfo {
            id,
            name,
            primary: false,
            config,
            physical_size_mm: (600, 340),
            position: (0, 0),
        });
        id
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_8k_config() {
        let config = DisplayConfig::uhd_8k_hdr();
        assert_eq!(config.resolution, Resolution::UHD8K);
        assert_eq!(config.hdr_mode, HDRMode::HDR10Plus);
    }
    
    #[test]
    fn test_hdr_luminance() {
        assert_eq!(HDRMode::HDR10Plus.max_luminance(), 4000);
        assert!(HDRMode::HDR10Plus.supports_dynamic_metadata());
    }
    
    #[test]
    fn test_bandwidth() {
        let config = DisplayConfig::uhd_8k_hdr();
        let bw = config.bandwidth_gbps();
        assert!(bw > 50.0); // 8K60 HDR needs ~60 Gbps
    }
}
