//! Gradient Rendering
//!
//! Provides linear and radial gradient fills.

use crate::primitives::{Color, Rect, Point};

/// Gradient color stop
#[derive(Debug, Clone, Copy)]
pub struct ColorStop {
    /// Position along the gradient (0.0 to 1.0)
    pub position: f32,
    /// Color at this position
    pub color: Color,
}

impl ColorStop {
    pub fn new(position: f32, color: Color) -> Self {
        Self {
            position: position.clamp(0.0, 1.0),
            color,
        }
    }
}

/// Linear gradient direction
#[derive(Debug, Clone, Copy)]
pub enum GradientDirection {
    /// Left to right
    Horizontal,
    /// Top to bottom
    Vertical,
    /// Custom angle in degrees (0 = right, 90 = down)
    Angle(f32),
    /// From point A to point B
    Points { start: Point, end: Point },
}

/// Linear gradient definition
#[derive(Debug, Clone)]
pub struct LinearGradient {
    pub direction: GradientDirection,
    pub stops: Vec<ColorStop>,
}

impl LinearGradient {
    /// Create a horizontal gradient
    pub fn horizontal(start_color: Color, end_color: Color) -> Self {
        Self {
            direction: GradientDirection::Horizontal,
            stops: vec![
                ColorStop::new(0.0, start_color),
                ColorStop::new(1.0, end_color),
            ],
        }
    }
    
    /// Create a vertical gradient
    pub fn vertical(start_color: Color, end_color: Color) -> Self {
        Self {
            direction: GradientDirection::Vertical,
            stops: vec![
                ColorStop::new(0.0, start_color),
                ColorStop::new(1.0, end_color),
            ],
        }
    }
    
    /// Create a gradient with custom stops
    pub fn with_stops(direction: GradientDirection, stops: Vec<ColorStop>) -> Self {
        Self { direction, stops }
    }
    
    /// Sample the gradient at a position (0.0 to 1.0)
    pub fn sample(&self, t: f32) -> Color {
        let t = t.clamp(0.0, 1.0);
        
        if self.stops.is_empty() {
            return Color::BLACK;
        }
        
        if self.stops.len() == 1 {
            return self.stops[0].color;
        }
        
        // Find the two stops we're between
        let mut prev = &self.stops[0];
        for stop in &self.stops[1..] {
            if t <= stop.position {
                // Interpolate between prev and stop
                let range = stop.position - prev.position;
                if range > 0.0 {
                    let local_t = (t - prev.position) / range;
                    return Color::lerp(prev.color, stop.color, local_t);
                } else {
                    return stop.color;
                }
            }
            prev = stop;
        }
        
        // Beyond last stop
        self.stops.last().unwrap().color
    }
    
    /// Generate vertices for rendering a gradient-filled rectangle
    /// Returns vertices with colors for each corner
    pub fn generate_rect_vertices(&self, rect: Rect) -> Vec<(Point, Color)> {
        match self.direction {
            GradientDirection::Horizontal => {
                let left_color = self.sample(0.0);
                let right_color = self.sample(1.0);
                vec![
                    (Point::new(rect.x, rect.y), left_color),
                    (Point::new(rect.x + rect.width, rect.y), right_color),
                    (Point::new(rect.x + rect.width, rect.y + rect.height), right_color),
                    (Point::new(rect.x, rect.y + rect.height), left_color),
                ]
            }
            GradientDirection::Vertical => {
                let top_color = self.sample(0.0);
                let bottom_color = self.sample(1.0);
                vec![
                    (Point::new(rect.x, rect.y), top_color),
                    (Point::new(rect.x + rect.width, rect.y), top_color),
                    (Point::new(rect.x + rect.width, rect.y + rect.height), bottom_color),
                    (Point::new(rect.x, rect.y + rect.height), bottom_color),
                ]
            }
            GradientDirection::Angle(degrees) => {
                // Calculate gradient direction vector
                let radians = degrees.to_radians();
                let dx = radians.cos();
                let dy = radians.sin();
                
                // Project corners onto gradient axis
                let corners = [
                    Point::new(rect.x, rect.y),
                    Point::new(rect.x + rect.width, rect.y),
                    Point::new(rect.x + rect.width, rect.y + rect.height),
                    Point::new(rect.x, rect.y + rect.height),
                ];
                
                let center = rect.center();
                let projections: Vec<f32> = corners.iter()
                    .map(|p| (p.x - center.x) * dx + (p.y - center.y) * dy)
                    .collect();
                
                let min_proj = projections.iter().cloned().fold(f32::INFINITY, f32::min);
                let max_proj = projections.iter().cloned().fold(f32::NEG_INFINITY, f32::max);
                let range = max_proj - min_proj;
                
                corners.iter().zip(projections.iter())
                    .map(|(p, &proj)| {
                        let t = if range > 0.0 { (proj - min_proj) / range } else { 0.5 };
                        (*p, self.sample(t))
                    })
                    .collect()
            }
            GradientDirection::Points { start, end } => {
                let dx = end.x - start.x;
                let dy = end.y - start.y;
                let length_sq = dx * dx + dy * dy;
                
                if length_sq == 0.0 {
                    let color = self.sample(0.5);
                    return vec![
                        (Point::new(rect.x, rect.y), color),
                        (Point::new(rect.x + rect.width, rect.y), color),
                        (Point::new(rect.x + rect.width, rect.y + rect.height), color),
                        (Point::new(rect.x, rect.y + rect.height), color),
                    ];
                }
                
                let corners = [
                    Point::new(rect.x, rect.y),
                    Point::new(rect.x + rect.width, rect.y),
                    Point::new(rect.x + rect.width, rect.y + rect.height),
                    Point::new(rect.x, rect.y + rect.height),
                ];
                
                corners.iter()
                    .map(|p| {
                        let px = p.x - start.x;
                        let py = p.y - start.y;
                        let t = (px * dx + py * dy) / length_sq;
                        (*p, self.sample(t))
                    })
                    .collect()
            }
        }
    }
}

/// Radial gradient definition
#[derive(Debug, Clone)]
pub struct RadialGradient {
    pub center: Point,
    pub radius: f32,
    pub stops: Vec<ColorStop>,
}

impl RadialGradient {
    /// Create a new radial gradient
    pub fn new(center: Point, radius: f32, inner: Color, outer: Color) -> Self {
        Self {
            center,
            radius,
            stops: vec![
                ColorStop::new(0.0, inner),
                ColorStop::new(1.0, outer),
            ],
        }
    }
    
    /// Sample the gradient at a distance from center
    pub fn sample(&self, distance: f32) -> Color {
        let t = (distance / self.radius).clamp(0.0, 1.0);
        
        if self.stops.is_empty() {
            return Color::BLACK;
        }
        
        if self.stops.len() == 1 {
            return self.stops[0].color;
        }
        
        let mut prev = &self.stops[0];
        for stop in &self.stops[1..] {
            if t <= stop.position {
                let range = stop.position - prev.position;
                if range > 0.0 {
                    let local_t = (t - prev.position) / range;
                    return Color::lerp(prev.color, stop.color, local_t);
                } else {
                    return stop.color;
                }
            }
            prev = stop;
        }
        
        self.stops.last().unwrap().color
    }
    
    /// Sample at a specific point
    pub fn sample_at(&self, point: Point) -> Color {
        let distance = self.center.distance_to(point);
        self.sample(distance)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_linear_gradient_sample() {
        let gradient = LinearGradient::horizontal(Color::BLACK, Color::WHITE);
        
        let start = gradient.sample(0.0);
        assert_eq!(start, Color::BLACK);
        
        let end = gradient.sample(1.0);
        assert_eq!(end, Color::WHITE);
        
        let mid = gradient.sample(0.5);
        assert_eq!(mid.r, 127);
    }
    
    #[test]
    fn test_radial_gradient_sample() {
        let gradient = RadialGradient::new(
            Point::new(0.0, 0.0),
            100.0,
            Color::RED,
            Color::BLUE,
        );
        
        let center = gradient.sample(0.0);
        assert_eq!(center, Color::RED);
        
        let edge = gradient.sample(100.0);
        assert_eq!(edge, Color::BLUE);
    }
}
