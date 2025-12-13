//! Hello Window Example
//!
//! Minimal NXRender application.

use nxrender_core::{Application, AppConfig};
use nxrender_theme::Theme;
use std::time::Duration;

fn main() -> Result<(), String> {
    let config = AppConfig::new("Hello NXRender")
        .size(800, 600);
    
    let mut app = Application::new(config)?;
    
    println!("NXRender Hello Window");
    println!("Theme: {}", app.theme().name);
    println!("Mode: {:?}", app.theme().mode);
    
    // Simple demo - just render a few frames
    for i in 0..60 {
        let dt = app.update();
        app.render();
        
        if i % 10 == 0 {
            println!("Frame {} - dt: {:?}", app.frame_count(), dt);
        }
    }
    
    println!("Done: {} frames rendered", app.frame_count());
    
    Ok(())
}
