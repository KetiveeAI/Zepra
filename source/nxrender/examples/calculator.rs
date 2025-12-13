//! Calculator Example
//!
//! Simple calculator demonstrating widget layout.

use nxrender_widgets::{Button, ButtonStyle, Label, LabelStyle, TextAlign};
use nxrender_layout::{GridLayout, GridTrack, GridPlacement};
use nxgfx::{Rect, Size, Color};

fn main() {
    println!("Calculator Example");
    
    // Create calculator layout
    let layout = GridLayout::new()
        .columns(vec![
            GridTrack::Fraction(1.0),
            GridTrack::Fraction(1.0),
            GridTrack::Fraction(1.0),
            GridTrack::Fraction(1.0),
        ])
        .gap(4.0);
    
    // Button labels for calculator
    let buttons = [
        "7", "8", "9", "/",
        "4", "5", "6", "*",
        "1", "2", "3", "-",
        "0", ".", "=", "+",
    ];
    
    // Create placements
    let placements: Vec<_> = (0..16).map(|i| {
        GridPlacement::at(i % 4, i / 4)
    }).collect();
    
    let sizes: Vec<_> = (0..16).map(|_| Size::new(60.0, 60.0)).collect();
    
    let container = Rect::new(0.0, 0.0, 260.0, 260.0);
    let bounds = layout.layout(container, &placements, &sizes);
    
    println!("Calculator grid:");
    for (i, (label, rect)) in buttons.iter().zip(bounds.iter()).enumerate() {
        println!("  Button '{}': {:?}", label, rect);
    }
    
    println!("Layout complete");
}
