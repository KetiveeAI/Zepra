//! Grid Layout
//!
//! CSS Grid-inspired layout for two-dimensional arrangements.

use nxgfx::{Rect, Size};

/// Grid track sizing
#[derive(Debug, Clone, Copy, PartialEq)]
pub enum GridTrack {
    /// Fixed pixel size
    Fixed(f32),
    /// Fractional unit (like CSS fr)
    Fraction(f32),
    /// Size to content
    Auto,
    /// Minimum size with flexible maximum
    MinContent,
    /// Maximum size with flexible minimum
    MaxContent,
}

impl Default for GridTrack {
    fn default() -> Self {
        GridTrack::Auto
    }
}

impl GridTrack {
    /// Create fixed tracks
    pub fn fixed(size: f32) -> Self {
        GridTrack::Fixed(size)
    }
    
    /// Create fraction track
    pub fn fr(value: f32) -> Self {
        GridTrack::Fraction(value)
    }
}

/// Grid item placement
#[derive(Debug, Clone, Copy, Default)]
pub struct GridPlacement {
    /// Column start (0-indexed)
    pub column: usize,
    /// Row start (0-indexed)    
    pub row: usize,
    /// Number of columns to span
    pub column_span: usize,
    /// Number of rows to span
    pub row_span: usize,
}

impl GridPlacement {
    /// Place at a specific cell
    pub fn at(column: usize, row: usize) -> Self {
        Self {
            column,
            row,
            column_span: 1,
            row_span: 1,
        }
    }
    
    /// Span multiple columns
    pub fn span_columns(mut self, span: usize) -> Self {
        self.column_span = span.max(1);
        self
    }
    
    /// Span multiple rows
    pub fn span_rows(mut self, span: usize) -> Self {
        self.row_span = span.max(1);
        self
    }
}

/// Grid layout configuration
#[derive(Debug, Clone, Default)]
pub struct GridLayout {
    /// Column track definitions
    pub columns: Vec<GridTrack>,
    /// Row track definitions
    pub rows: Vec<GridTrack>,
    /// Gap between columns
    pub column_gap: f32,
    /// Gap between rows
    pub row_gap: f32,
    /// Padding inside the grid
    pub padding: f32,
}

impl GridLayout {
    /// Create a new grid layout
    pub fn new() -> Self {
        Self::default()
    }
    
    /// Create a grid with equal columns
    pub fn columns_equal(count: usize) -> Self {
        Self {
            columns: vec![GridTrack::Fraction(1.0); count],
            ..Default::default()
        }
    }
    
    /// Create a grid with specified column tracks
    pub fn columns(mut self, columns: Vec<GridTrack>) -> Self {
        self.columns = columns;
        self
    }
    
    /// Create a grid with specified row tracks
    pub fn rows(mut self, rows: Vec<GridTrack>) -> Self {
        self.rows = rows;
        self
    }
    
    /// Set both column and row gap
    pub fn gap(mut self, gap: f32) -> Self {
        self.column_gap = gap;
        self.row_gap = gap;
        self
    }
    
    /// Set padding
    pub fn padding(mut self, padding: f32) -> Self {
        self.padding = padding;
        self
    }
    
    /// Calculate track sizes for a given available space
    fn calculate_track_sizes(&self, tracks: &[GridTrack], available: f32, gap: f32, content_sizes: &[f32]) -> Vec<f32> {
        if tracks.is_empty() {
            return Vec::new();
        }
        
        let total_gaps = (tracks.len() - 1) as f32 * gap;
        let available_for_tracks = available - total_gaps;
        
        // First pass: calculate fixed and auto sizes
        let mut sizes: Vec<f32> = Vec::with_capacity(tracks.len());
        let mut total_fixed = 0.0;
        let mut total_fr = 0.0;
        
        for (i, track) in tracks.iter().enumerate() {
            match track {
                GridTrack::Fixed(size) => {
                    sizes.push(*size);
                    total_fixed += size;
                }
                GridTrack::Auto | GridTrack::MinContent | GridTrack::MaxContent => {
                    let content_size = content_sizes.get(i).copied().unwrap_or(50.0);
                    sizes.push(content_size);
                    total_fixed += content_size;
                }
                GridTrack::Fraction(fr) => {
                    sizes.push(0.0); // Placeholder
                    total_fr += fr;
                }
            }
        }
        
        // Second pass: distribute remaining space to fraction tracks
        let remaining = (available_for_tracks - total_fixed).max(0.0);
        
        if total_fr > 0.0 {
            for (i, track) in tracks.iter().enumerate() {
                if let GridTrack::Fraction(fr) = track {
                    sizes[i] = remaining * fr / total_fr;
                }
            }
        }
        
        sizes
    }
    
    /// Layout items in the grid
    pub fn layout(&self, container: Rect, placements: &[GridPlacement], content_sizes: &[Size]) -> Vec<Rect> {
        if placements.is_empty() {
            return Vec::new();
        }
        
        // Determine grid dimensions
        let num_cols = self.columns.len().max(1);
        let num_rows = self.rows.len().max(
            placements.iter().map(|p| p.row + p.row_span).max().unwrap_or(1)
        );
        
        // Available space
        let available_width = container.width - self.padding * 2.0;
        let available_height = container.height - self.padding * 2.0;
        
        // Calculate track sizes
        let column_content_sizes: Vec<f32> = (0..num_cols)
            .map(|col| {
                placements.iter()
                    .zip(content_sizes.iter())
                    .filter(|(p, _)| p.column == col && p.column_span == 1)
                    .map(|(_, s)| s.width)
                    .max_by(|a, b| a.partial_cmp(b).unwrap())
                    .unwrap_or(50.0)
            })
            .collect();
        
        let row_content_sizes: Vec<f32> = (0..num_rows)
            .map(|row| {
                placements.iter()
                    .zip(content_sizes.iter())
                    .filter(|(p, _)| p.row == row && p.row_span == 1)
                    .map(|(_, s)| s.height)
                    .max_by(|a, b| a.partial_cmp(b).unwrap())
                    .unwrap_or(30.0)
            })
            .collect();
        
        let col_sizes = self.calculate_track_sizes(&self.columns, available_width, self.column_gap, &column_content_sizes);
        
        // If no explicit rows, create auto rows
        let row_tracks = if self.rows.is_empty() {
            vec![GridTrack::Auto; num_rows]
        } else {
            self.rows.clone()
        };
        let row_sizes = self.calculate_track_sizes(&row_tracks, available_height, self.row_gap, &row_content_sizes);
        
        // Calculate cumulative positions
        let col_positions: Vec<f32> = {
            let mut positions = vec![container.x + self.padding];
            for (i, &size) in col_sizes.iter().enumerate() {
                if i < col_sizes.len() - 1 {
                    positions.push(positions[i] + size + self.column_gap);
                }
            }
            positions
        };
        
        let row_positions: Vec<f32> = {
            let mut positions = vec![container.y + self.padding];
            for (i, &size) in row_sizes.iter().enumerate() {
                if i < row_sizes.len() - 1 {
                    positions.push(positions[i] + size + self.row_gap);
                }
            }
            positions
        };
        
        // Generate bounds for each placement
        placements.iter().map(|placement| {
            let col = placement.column.min(col_positions.len() - 1);
            let row = placement.row.min(row_positions.len() - 1);
            
            let x = col_positions[col];
            let y = row_positions[row];
            
            // Calculate width including spanned columns
            let width: f32 = (col..col + placement.column_span)
                .filter_map(|c| col_sizes.get(c))
                .sum::<f32>()
                + (placement.column_span.saturating_sub(1)) as f32 * self.column_gap;
            
            // Calculate height including spanned rows
            let height: f32 = (row..row + placement.row_span)
                .filter_map(|r| row_sizes.get(r))
                .sum::<f32>()
                + (placement.row_span.saturating_sub(1)) as f32 * self.row_gap;
            
            Rect::new(x, y, width, height)
        }).collect()
    }
}

/// Helper to create common grid patterns
pub struct GridBuilder;

impl GridBuilder {
    /// Create a simple 2-column layout
    pub fn two_columns() -> GridLayout {
        GridLayout::columns_equal(2)
    }
    
    /// Create a 3-column layout
    pub fn three_columns() -> GridLayout {
        GridLayout::columns_equal(3)
    }
    
    /// Create a sidebar + content layout
    pub fn sidebar_content(sidebar_width: f32) -> GridLayout {
        GridLayout::new().columns(vec![
            GridTrack::Fixed(sidebar_width),
            GridTrack::Fraction(1.0),
        ])
    }
    
    /// Create a header + content + footer layout
    pub fn header_content_footer(header_height: f32, footer_height: f32) -> GridLayout {
        GridLayout::new().rows(vec![
            GridTrack::Fixed(header_height),
            GridTrack::Fraction(1.0),
            GridTrack::Fixed(footer_height),
        ]).columns(vec![GridTrack::Fraction(1.0)])
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_grid_equal_columns() {
        let layout = GridLayout::columns_equal(3).gap(10.0);
        let container = Rect::new(0.0, 0.0, 320.0, 100.0);
        
        let placements = vec![
            GridPlacement::at(0, 0),
            GridPlacement::at(1, 0),
            GridPlacement::at(2, 0),
        ];
        let sizes = vec![
            Size::new(50.0, 30.0),
            Size::new(50.0, 30.0),
            Size::new(50.0, 30.0),
        ];
        
        let bounds = layout.layout(container, &placements, &sizes);
        
        assert_eq!(bounds.len(), 3);
        // 320 - 20 (gaps) = 300, divided by 3 = 100 each
        assert!((bounds[0].width - 100.0).abs() < 0.1);
    }
    
    #[test]
    fn test_grid_span() {
        let layout = GridLayout::columns_equal(2).gap(10.0);
        let container = Rect::new(0.0, 0.0, 210.0, 100.0);
        
        let placements = vec![
            GridPlacement::at(0, 0).span_columns(2),
        ];
        let sizes = vec![Size::new(100.0, 30.0)];
        
        let bounds = layout.layout(container, &placements, &sizes);
        
        // Should span full width: 100 + 100 + 10 gap = 210
        assert!((bounds[0].width - 210.0).abs() < 0.1);
    }
    
    #[test]
    fn test_grid_fixed_and_fraction() {
        let layout = GridLayout::new()
            .columns(vec![GridTrack::Fixed(100.0), GridTrack::Fraction(1.0)])
            .gap(10.0);
        
        let container = Rect::new(0.0, 0.0, 310.0, 100.0);
        
        let placements = vec![
            GridPlacement::at(0, 0),
            GridPlacement::at(1, 0),
        ];
        let sizes = vec![
            Size::new(100.0, 30.0),
            Size::new(100.0, 30.0),
        ];
        
        let bounds = layout.layout(container, &placements, &sizes);
        
        assert_eq!(bounds[0].width, 100.0);
        // 310 - 100 - 10 gap = 200
        assert!((bounds[1].width - 200.0).abs() < 0.1);
    }
}
