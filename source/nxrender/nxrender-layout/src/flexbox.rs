//! Flexbox Layout
//!
//! CSS Flexbox-inspired layout algorithm for arranging children
//! in rows or columns with flexible sizing and alignment.

use nxgfx::{Rect, Size};

/// Flex direction
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum FlexDirection {
    #[default]
    Row,
    RowReverse,
    Column,
    ColumnReverse,
}

impl FlexDirection {
    /// Check if this is a row-based direction
    pub fn is_row(&self) -> bool {
        matches!(self, FlexDirection::Row | FlexDirection::RowReverse)
    }
    
    /// Check if this is reversed
    pub fn is_reversed(&self) -> bool {
        matches!(self, FlexDirection::RowReverse | FlexDirection::ColumnReverse)
    }
}

/// Justify content (main axis alignment)
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum JustifyContent {
    #[default]
    Start,
    Center,
    End,
    SpaceBetween,
    SpaceAround,
    SpaceEvenly,
}

/// Align items (cross axis alignment)
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum AlignItems {
    Start,
    #[default]
    Center,
    End,
    Stretch,
    Baseline,
}

/// Align self (individual child cross axis alignment)
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum AlignSelf {
    #[default]
    Auto,
    Start,
    Center,
    End,
    Stretch,
}

/// Flex wrap behavior
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum FlexWrap {
    #[default]
    NoWrap,
    Wrap,
    WrapReverse,
}

/// Properties for a flex child
#[derive(Debug, Clone, Copy, Default)]
pub struct FlexChild {
    /// Flex grow factor
    pub grow: f32,
    /// Flex shrink factor
    pub shrink: f32,
    /// Flex basis (initial size before growing/shrinking)
    pub basis: Option<f32>,
    /// Individual alignment override
    pub align_self: AlignSelf,
    /// Minimum size
    pub min_size: Option<Size>,
    /// Maximum size
    pub max_size: Option<Size>,
}

impl FlexChild {
    /// Create a fixed-size child (no grow/shrink)
    pub fn fixed() -> Self {
        Self {
            grow: 0.0,
            shrink: 0.0,
            ..Default::default()
        }
    }
    
    /// Create a flexible child that grows
    pub fn flexible(grow: f32) -> Self {
        Self {
            grow,
            shrink: 1.0,
            ..Default::default()
        }
    }
    
    /// Create a child that expands to fill available space
    pub fn expanded() -> Self {
        Self::flexible(1.0)
    }
    
    /// Set the basis
    pub fn with_basis(mut self, basis: f32) -> Self {
        self.basis = Some(basis);
        self
    }
}

/// Flexbox layout configuration
#[derive(Debug, Clone, Default)]
pub struct FlexLayout {
    /// Direction of the main axis
    pub direction: FlexDirection,
    /// How to distribute space along main axis
    pub justify: JustifyContent,
    /// How to align items along cross axis
    pub align: AlignItems,
    /// Gap between items
    pub gap: f32,
    /// Wrap behavior
    pub wrap: FlexWrap,
    /// Padding inside the container
    pub padding: f32,
}

impl FlexLayout {
    /// Create a row layout
    pub fn row() -> Self {
        Self {
            direction: FlexDirection::Row,
            ..Default::default()
        }
    }
    
    /// Create a column layout
    pub fn column() -> Self {
        Self {
            direction: FlexDirection::Column,
            ..Default::default()
        }
    }
    
    /// Set the gap between items
    pub fn gap(mut self, gap: f32) -> Self {
        self.gap = gap;
        self
    }
    
    /// Set justify content
    pub fn justify(mut self, justify: JustifyContent) -> Self {
        self.justify = justify;
        self
    }
    
    /// Set align items
    pub fn align(mut self, align: AlignItems) -> Self {
        self.align = align;
        self
    }
    
    /// Set padding
    pub fn padding(mut self, padding: f32) -> Self {
        self.padding = padding;
        self
    }
    
    /// Enable wrapping
    pub fn wrap(mut self) -> Self {
        self.wrap = FlexWrap::Wrap;
        self
    }
    
    /// Calculate layout for children and return their bounds
    pub fn layout(&self, container: Rect, child_sizes: &[Size], flex_props: &[FlexChild]) -> Vec<Rect> {
        if child_sizes.is_empty() {
            return Vec::new();
        }
        
        let is_row = self.direction.is_row();
        let is_reversed = self.direction.is_reversed();
        
        // Available space after padding
        let available = Size::new(
            container.width - self.padding * 2.0,
            container.height - self.padding * 2.0,
        );
        
        // Main and cross axis sizes
        let main_size = if is_row { available.width } else { available.height };
        let cross_size = if is_row { available.height } else { available.width };
        
        // Calculate total child size on main axis and total flex grow
        let total_gaps = (child_sizes.len() - 1) as f32 * self.gap;
        
        let mut total_basis = 0.0;
        let mut total_grow = 0.0;
        let mut total_shrink = 0.0;
        
        for (i, size) in child_sizes.iter().enumerate() {
            let flex = flex_props.get(i).copied().unwrap_or_default();
            let child_main = if is_row { size.width } else { size.height };
            let basis = flex.basis.unwrap_or(child_main);
            
            total_basis += basis;
            total_grow += flex.grow;
            total_shrink += flex.shrink;
        }
        
        // Calculate free space
        let free_space = main_size - total_basis - total_gaps;
        
        // Calculate final sizes
        let mut final_main_sizes: Vec<f32> = Vec::with_capacity(child_sizes.len());
        
        for (i, size) in child_sizes.iter().enumerate() {
            let flex = flex_props.get(i).copied().unwrap_or_default();
            let child_main = if is_row { size.width } else { size.height };
            let basis = flex.basis.unwrap_or(child_main);
            
            let final_size = if free_space > 0.0 && total_grow > 0.0 {
                // Growing
                basis + (free_space * flex.grow / total_grow)
            } else if free_space < 0.0 && total_shrink > 0.0 {
                // Shrinking
                (basis + (free_space * flex.shrink / total_shrink)).max(0.0)
            } else {
                basis
            };
            
            final_main_sizes.push(final_size);
        }
        
        // Calculate starting position based on justify
        let total_final_size: f32 = final_main_sizes.iter().sum();
        let remaining = main_size - total_final_size - total_gaps;
        
        let (mut main_offset, item_spacing) = match self.justify {
            JustifyContent::Start => (0.0, self.gap),
            JustifyContent::End => (remaining, self.gap),
            JustifyContent::Center => (remaining / 2.0, self.gap),
            JustifyContent::SpaceBetween => {
                if child_sizes.len() > 1 {
                    (0.0, self.gap + remaining / (child_sizes.len() - 1) as f32)
                } else {
                    (0.0, self.gap)
                }
            }
            JustifyContent::SpaceAround => {
                let space = remaining / child_sizes.len() as f32;
                (space / 2.0, self.gap + space)
            }
            JustifyContent::SpaceEvenly => {
                let space = remaining / (child_sizes.len() + 1) as f32;
                (space, self.gap + space)
            }
        };
        
        main_offset += self.padding;
        
        // Generate bounds
        let mut bounds: Vec<Rect> = Vec::with_capacity(child_sizes.len());
        
        for (i, size) in child_sizes.iter().enumerate() {
            let flex = flex_props.get(i).copied().unwrap_or_default();
            let final_main = final_main_sizes[i];
            let child_cross = if is_row { size.height } else { size.width };
            
            // Cross axis alignment
            let align = match flex.align_self {
                AlignSelf::Auto => self.align,
                AlignSelf::Start => AlignItems::Start,
                AlignSelf::Center => AlignItems::Center,
                AlignSelf::End => AlignItems::End,
                AlignSelf::Stretch => AlignItems::Stretch,
            };
            
            let (cross_offset, final_cross) = match align {
                AlignItems::Start => (self.padding, child_cross),
                AlignItems::Center => (self.padding + (cross_size - child_cross) / 2.0, child_cross),
                AlignItems::End => (self.padding + cross_size - child_cross, child_cross),
                AlignItems::Stretch => (self.padding, cross_size),
                AlignItems::Baseline => (self.padding, child_cross), // Simplified
            };
            
            // Create bounds based on direction
            let (x, y, w, h) = if is_row {
                (container.x + main_offset, container.y + cross_offset, final_main, final_cross)
            } else {
                (container.x + cross_offset, container.y + main_offset, final_cross, final_main)
            };
            
            bounds.push(Rect::new(x, y, w, h));
            main_offset += final_main + item_spacing;
        }
        
        // Reverse if needed
        if is_reversed {
            bounds.reverse();
        }
        
        bounds
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_flex_row_basic() {
        let layout = FlexLayout::row().gap(10.0);
        let container = Rect::new(0.0, 0.0, 300.0, 100.0);
        let sizes = vec![
            Size::new(50.0, 30.0),
            Size::new(50.0, 30.0),
            Size::new(50.0, 30.0),
        ];
        let props = vec![FlexChild::fixed(); 3];
        
        let bounds = layout.layout(container, &sizes, &props);
        
        assert_eq!(bounds.len(), 3);
        assert_eq!(bounds[0].x, 0.0);
        assert_eq!(bounds[1].x, 60.0); // 50 + 10 gap
        assert_eq!(bounds[2].x, 120.0); // 110 + 10 gap
    }
    
    #[test]
    fn test_flex_column() {
        let layout = FlexLayout::column().gap(5.0);
        let container = Rect::new(0.0, 0.0, 100.0, 200.0);
        let sizes = vec![
            Size::new(80.0, 40.0),
            Size::new(80.0, 40.0),
        ];
        let props = vec![FlexChild::fixed(); 2];
        
        let bounds = layout.layout(container, &sizes, &props);
        
        assert_eq!(bounds.len(), 2);
        assert_eq!(bounds[0].y, 0.0);
        assert_eq!(bounds[1].y, 45.0); // 40 + 5 gap
    }
    
    #[test]
    fn test_flex_grow() {
        let layout = FlexLayout::row();
        let container = Rect::new(0.0, 0.0, 300.0, 100.0);
        let sizes = vec![
            Size::new(50.0, 30.0),
            Size::new(50.0, 30.0),
        ];
        let props = vec![
            FlexChild::flexible(1.0),
            FlexChild::flexible(2.0),
        ];
        
        let bounds = layout.layout(container, &sizes, &props);
        
        // Total basis is 100, free space is 200
        // Child 0 gets 200 * 1/3 = 66.67
        // Child 1 gets 200 * 2/3 = 133.33
        assert!((bounds[0].width - 116.67).abs() < 1.0);
        assert!((bounds[1].width - 183.33).abs() < 1.0);
    }
    
    #[test]
    fn test_justify_center() {
        let layout = FlexLayout::row().justify(JustifyContent::Center);
        let container = Rect::new(0.0, 0.0, 200.0, 100.0);
        let sizes = vec![Size::new(50.0, 30.0)];
        let props = vec![FlexChild::fixed()];
        
        let bounds = layout.layout(container, &sizes, &props);
        
        // Centered: (200 - 50) / 2 = 75
        assert!((bounds[0].x - 75.0).abs() < 0.1);
    }
    
    #[test]
    fn test_align_stretch() {
        let layout = FlexLayout::row().align(AlignItems::Stretch);
        let container = Rect::new(0.0, 0.0, 200.0, 100.0);
        let sizes = vec![Size::new(50.0, 30.0)];
        let props = vec![FlexChild::fixed()];
        
        let bounds = layout.layout(container, &sizes, &props);
        
        // Stretched to full height
        assert_eq!(bounds[0].height, 100.0);
    }
}
