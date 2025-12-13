//! Rectangle and Point types

/// 2D Point
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Point {
    pub x: f32,
    pub y: f32,
}

impl Point {
    pub const ZERO: Point = Point { x: 0.0, y: 0.0 };
    
    pub fn new(x: f32, y: f32) -> Self {
        Self { x, y }
    }
    
    pub fn distance_to(&self, other: Point) -> f32 {
        let dx = other.x - self.x;
        let dy = other.y - self.y;
        (dx * dx + dy * dy).sqrt()
    }
    
    pub fn midpoint(&self, other: Point) -> Point {
        Point::new(
            (self.x + other.x) / 2.0,
            (self.y + other.y) / 2.0,
        )
    }
}

/// 2D Size
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Size {
    pub width: f32,
    pub height: f32,
}

impl Size {
    pub const ZERO: Size = Size { width: 0.0, height: 0.0 };
    
    pub fn new(width: f32, height: f32) -> Self {
        Self { width, height }
    }
    
    pub fn area(&self) -> f32 {
        self.width * self.height
    }
    
    pub fn is_empty(&self) -> bool {
        self.width <= 0.0 || self.height <= 0.0
    }
}

/// Rectangle with position and size
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Rect {
    pub x: f32,
    pub y: f32,
    pub width: f32,
    pub height: f32,
}

impl Rect {
    pub const ZERO: Rect = Rect {
        x: 0.0,
        y: 0.0,
        width: 0.0,
        height: 0.0,
    };
    
    pub fn new(x: f32, y: f32, width: f32, height: f32) -> Self {
        Self { x, y, width, height }
    }
    
    pub fn from_pos_size(pos: Point, size: Size) -> Self {
        Self {
            x: pos.x,
            y: pos.y,
            width: size.width,
            height: size.height,
        }
    }
    
    pub fn from_size(size: Size) -> Self {
        Self {
            x: 0.0,
            y: 0.0,
            width: size.width,
            height: size.height,
        }
    }
    
    pub fn origin(&self) -> Point {
        Point::new(self.x, self.y)
    }
    
    pub fn size(&self) -> Size {
        Size::new(self.width, self.height)
    }
    
    pub fn center(&self) -> Point {
        Point::new(
            self.x + self.width / 2.0,
            self.y + self.height / 2.0,
        )
    }
    
    pub fn left(&self) -> f32 { self.x }
    pub fn right(&self) -> f32 { self.x + self.width }
    pub fn top(&self) -> f32 { self.y }
    pub fn bottom(&self) -> f32 { self.y + self.height }
    
    pub fn contains(&self, pos: Point) -> bool {
        pos.x >= self.x
            && pos.x < self.x + self.width
            && pos.y >= self.y
            && pos.y < self.y + self.height
    }
    
    pub fn intersects(&self, other: &Rect) -> bool {
        self.x < other.right()
            && self.right() > other.x
            && self.y < other.bottom()
            && self.bottom() > other.y
    }
    
    pub fn inflate(&self, amount: f32) -> Rect {
        Rect::new(
            self.x - amount,
            self.y - amount,
            self.width + amount * 2.0,
            self.height + amount * 2.0,
        )
    }
    
    pub fn inset(&self, amount: f32) -> Rect {
        self.inflate(-amount)
    }
    
    /// Get the intersection of two rectangles
    pub fn intersection(&self, other: &Rect) -> Option<Rect> {
        if !self.intersects(other) {
            return None;
        }
        
        let x = self.x.max(other.x);
        let y = self.y.max(other.y);
        let right = self.right().min(other.right());
        let bottom = self.bottom().min(other.bottom());
        
        Some(Rect::new(x, y, right - x, bottom - y))
    }
    
    /// Get the union (bounding box) of two rectangles
    pub fn union(&self, other: &Rect) -> Rect {
        let x = self.x.min(other.x);
        let y = self.y.min(other.y);
        let right = self.right().max(other.right());
        let bottom = self.bottom().max(other.bottom());
        
        Rect::new(x, y, right - x, bottom - y)
    }
    
    /// Check if two rectangles touch (share an edge)
    pub fn touches(&self, other: &Rect) -> bool {
        // Check if they share an edge (adjacent but not overlapping)
        let h_adjacent = (self.right() - other.x).abs() < 0.001 
            || (other.right() - self.x).abs() < 0.001;
        let v_adjacent = (self.bottom() - other.y).abs() < 0.001 
            || (other.bottom() - self.y).abs() < 0.001;
        
        let h_overlap = self.x < other.right() && self.right() > other.x;
        let v_overlap = self.y < other.bottom() && self.bottom() > other.y;
        
        (h_adjacent && v_overlap) || (v_adjacent && h_overlap)
    }
    
    /// Check if rectangle is empty (zero size)
    pub fn is_empty(&self) -> bool {
        self.width <= 0.0 || self.height <= 0.0
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_rect_contains() {
        let rect = Rect::new(10.0, 10.0, 100.0, 50.0);
        assert!(rect.contains(Point::new(50.0, 30.0)));
        assert!(!rect.contains(Point::new(5.0, 30.0)));
    }
    
    #[test]
    fn test_rect_intersects() {
        let a = Rect::new(0.0, 0.0, 100.0, 100.0);
        let b = Rect::new(50.0, 50.0, 100.0, 100.0);
        let c = Rect::new(200.0, 200.0, 50.0, 50.0);
        
        assert!(a.intersects(&b));
        assert!(!a.intersects(&c));
    }
}
