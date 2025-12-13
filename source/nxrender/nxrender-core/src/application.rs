//! Application Framework

use nxgfx::GpuContext;
use crate::Compositor;
use nxrender_input::{Event, MouseState, KeyboardState};
use nxrender_animation::Animator;
use nxrender_theme::Theme;
use std::time::{Duration, Instant};

pub struct AppConfig {
    pub title: String,
    pub width: u32,
    pub height: u32,
    pub theme: Theme,
    pub vsync: bool,
    pub target_fps: u32,
}

impl Default for AppConfig {
    fn default() -> Self {
        Self {
            title: String::from("NXRender Application"),
            width: 1280,
            height: 720,
            theme: Theme::light(),
            vsync: true,
            target_fps: 60,
        }
    }
}

impl AppConfig {
    pub fn new(title: impl Into<String>) -> Self {
        Self { title: title.into(), ..Default::default() }
    }
    
    pub fn size(mut self, width: u32, height: u32) -> Self {
        self.width = width;
        self.height = height;
        self
    }
    
    pub fn dark_theme(mut self) -> Self {
        self.theme = Theme::dark();
        self
    }
}

pub struct Application {
    config: AppConfig,
    gpu: GpuContext,
    compositor: Compositor,
    mouse: MouseState,
    keyboard: KeyboardState,
    animator: Animator,
    running: bool,
    last_frame: Instant,
    frame_count: u64,
}

impl Application {
    pub fn new(config: AppConfig) -> Result<Self, String> {
        let gpu = GpuContext::new().map_err(|e| e.to_string())?;
        let gpu2 = GpuContext::new().map_err(|e| e.to_string())?;
        let compositor = Compositor::new(gpu2);
        
        Ok(Self {
            config,
            gpu,
            compositor,
            mouse: MouseState::new(),
            keyboard: KeyboardState::new(),
            animator: Animator::new(),
            running: false,
            last_frame: Instant::now(),
            frame_count: 0,
        })
    }
    
    pub fn theme(&self) -> &Theme { &self.config.theme }
    pub fn set_theme(&mut self, theme: Theme) { self.config.theme = theme; }
    
    pub fn gpu(&mut self) -> &mut GpuContext { &mut self.gpu }
    pub fn compositor(&mut self) -> &mut Compositor { &mut self.compositor }
    pub fn animator(&mut self) -> &mut Animator { &mut self.animator }
    
    pub fn mouse(&self) -> &MouseState { &self.mouse }
    pub fn keyboard(&self) -> &KeyboardState { &self.keyboard }
    
    pub fn is_running(&self) -> bool { self.running }
    pub fn frame_count(&self) -> u64 { self.frame_count }
    
    pub fn handle_event(&mut self, event: &Event) {
        self.mouse.process_event(event);
        self.keyboard.process_event(event);
    }
    
    pub fn update(&mut self) -> Duration {
        let now = Instant::now();
        let dt = now.duration_since(self.last_frame);
        self.last_frame = now;
        self.animator.update(dt);
        dt
    }
    
    pub fn render(&mut self) {
        self.compositor.composite();
        self.gpu.present();
        self.frame_count += 1;
    }
    
    pub fn quit(&mut self) {
        self.running = false;
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_app_config() {
        let config = AppConfig::new("Test App").size(800, 600).dark_theme();
        assert_eq!(config.title, "Test App");
        assert_eq!(config.width, 800);
        assert!(config.theme.is_dark());
    }
    
    #[test]
    fn test_app_creation() {
        let config = AppConfig::default();
        let app = Application::new(config);
        assert!(app.is_ok());
    }
}
