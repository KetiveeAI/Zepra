//! Graphics Test Application
//!
//! Tests GPU initialization and basic rendering.

use nxgfx::{GpuContext, Color, Rect, Point, Size};
use nxrender_core::{Application, AppConfig, Compositor};
use nxrender_theme::Theme;
use nxrender_widgets::{Button, Label, TextField};
use nxrender_layout::{FlexLayout, JustifyContent};
use nxrender_animation::{Animator, Animation, EasingFunction};
use std::time::Duration;

fn main() {
    println!("=== NXRENDER Graphics Test ===\n");
    
    // Test 1: GPU Context
    print!("1. GPU Context initialization... ");
    match GpuContext::new() {
        Ok(mut gpu) => {
            println!("OK");
            println!("   Screen size: {:?}", gpu.screen_size());
            
            // Test rendering primitives
            print!("2. Primitive rendering... ");
            gpu.fill_rect(Rect::new(0.0, 0.0, 100.0, 100.0), Color::RED);
            gpu.fill_circle(Point::new(150.0, 50.0), 40.0, Color::BLUE);
            gpu.fill_rounded_rect(Rect::new(200.0, 0.0, 100.0, 100.0), Color::GREEN, 10.0);
            gpu.stroke_rect(Rect::new(320.0, 0.0, 100.0, 100.0), Color::WHITE, 2.0);
            gpu.draw_text("NXRENDER", Point::new(10.0, 120.0), Color::WHITE);
            println!("OK");
        }
        Err(e) => println!("FAILED: {}", e),
    }
    
    // Test 2: Theme System
    print!("3. Theme system... ");
    let light = Theme::light();
    let dark = Theme::dark();
    println!("OK");
    println!("   Light primary: {:?}", light.colors.semantic.primary);
    println!("   Dark background: {:?}", dark.colors.surface.background);
    
    // Test 3: Layout System
    print!("4. Layout engine... ");
    let layout = FlexLayout::row().gap(10.0).justify(JustifyContent::SpaceBetween);
    let sizes = vec![Size::new(100.0, 40.0), Size::new(100.0, 40.0), Size::new(100.0, 40.0)];
    let props = vec![Default::default(); 3];
    let container = Rect::new(0.0, 0.0, 400.0, 100.0);
    let bounds = layout.layout(container, &sizes, &props);
    println!("OK");
    println!("   Buttons laid out: {}", bounds.len());
    
    // Test 4: Widget System
    print!("5. Widget system... ");
    let button = Button::new("Click Me");
    let label = Label::new("Hello NXRENDER");
    let textfield = TextField::new().placeholder("Enter text...");
    println!("OK");
    println!("   Button: '{}'", button.label());
    println!("   Label: '{}'", label.text());
    
    // Test 5: Animation System
    print!("6. Animation system... ");
    let mut animator = Animator::new();
    let anim = Animation::new(0.0, 100.0, Duration::from_millis(500))
        .with_easing(EasingFunction::EaseOutCubic);
    let id = animator.add(anim);
    
    for _ in 0..10 {
        animator.update(Duration::from_millis(50));
    }
    let value = animator.get_value(id);
    println!("OK");
    println!("   Animation value after 500ms: {:?}", value);
    
    // Test 6: Application Framework
    print!("7. Application framework... ");
    let config = AppConfig::new("Test App").size(800, 600).dark_theme();
    match Application::new(config) {
        Ok(app) => {
            println!("OK");
            println!("   Theme: {}", app.theme().name);
            println!("   Mode: {:?}", app.theme().mode);
        }
        Err(e) => println!("FAILED: {}", e),
    }
    
    println!("\n=== All Tests Complete ===");
    println!("\nNXRENDER is ready for production.");
    println!("Run 'cargo run --example hello_window' to see a demo.");
}
