# KetiveeSearch Engine Integration

## Overview

The Zepra Browser now includes an integrated search engine that combines:
- **C++ Browser Engine**: Handles UI, rendering, and user interactions
- **Node.js Search Backend**: Provides search functionality, indexing, and suggestions

## Architecture

```
┌─────────────────┐    HTTP/JSON    ┌──────────────────┐
│   Zepra Browser │ ◄─────────────► │ KetiveeSearch    │
│   (C++/SDL2)    │                 │ Backend (Node.js)│
└─────────────────┘                 └──────────────────┘
         │                                    │
         │                                    │
    ┌────▼────┐                         ┌────▼────┐
    │  UI     │                         │ Search  │
    │  Rendering │                      │ Index   │
    │  Events  │                         │ Scoring │
    └─────────┘                         └─────────┘
```

## Features

### Search Engine Features
- **Web Search**: Full-text search across indexed content
- **Search Suggestions**: Real-time query suggestions
- **Relevance Scoring**: Intelligent result ranking
- **Content Indexing**: Add new content to search index
- **Local Search**: Fallback when backend is unavailable

### Browser Integration
- **Address Bar Search**: Type queries directly in address bar
- **Search Shortcuts**: Ctrl+L to focus search
- **Search History**: Track and suggest previous searches
- **Bookmarks**: Save and search through bookmarks

## Setup Instructions

### 1. Install Node.js Dependencies
```bash
cd ketiveeserchengin/backend
npm install
```

### 2. Start the Search Backend
```bash
cd ketiveeserchengin/backend
npm start
```
The backend will run on `http://localhost:6329`

### 3. Build and Run the Browser
```bash
cd build
cmake --build . --config Debug
bin/zepra.exe
```

### 4. Quick Start (Windows)
Use the provided batch script:
```bash
start_browser.bat
```

## API Endpoints

### Search API
- **POST** `/api/search` - Perform web search
- **POST** `/api/search/suggestions` - Get search suggestions
- **POST** `/api/search/index` - Index new content
- **GET** `/api/search/health` - Health check

### Config API
- **GET** `/api/config` - Get search engine configuration
- **PUT** `/api/config` - Update configuration

## Usage Examples

### Basic Search
```cpp
// In your C++ code
zepra::KetiveeSearchEngine searchEngine;
zepra::SearchQuery query("JavaScript tutorial");
auto results = searchEngine.search(query);

for (const auto& result : results) {
    std::cout << "Title: " << result.title << std::endl;
    std::cout << "URL: " << result.url << std::endl;
    std::cout << "Description: " << result.description << std::endl;
}
```

### Search Suggestions
```cpp
auto suggestions = searchEngine.getSuggestions("react");
for (const auto& suggestion : suggestions) {
    std::cout << "Suggestion: " << suggestion << std::endl;
}
```

### Indexing Content
```bash
curl -X POST http://localhost:3001/api/search/index \
  -H "Content-Type: application/json" \
  -d '{
    "url": "https://example.com/article",
    "title": "Example Article",
    "content": "Article content here...",
    "type": "web"
  }'
```

## Demo Data

The search engine comes with demo data including:
- JavaScript tutorials
- React guides
- Node.js backend tutorials
- CSS Flexbox guides
- Python data science content

## Configuration

### Backend Configuration
Edit `ketiveeserchengin/backend/server.js`:
```javascript
const PORT = process.env.PORT || 3001;
const FRONTEND_URL = process.env.FRONTEND_URL || 'http://localhost:3000';
```

### Browser Configuration
Edit `include/common/constants.h`:
```cpp
constexpr const char* KETIVEE_SEARCH_URL = "http://localhost:6329";
constexpr int SEARCH_DELAY_MS = 300;
constexpr int MAX_SEARCH_RESULTS = 20;
```

## Development

### Adding New Search Features
1. **Backend**: Add new methods to `searchService.js`
2. **API**: Create new routes in `routes/search.js`
3. **Browser**: Add corresponding methods to `KetiveeSearchEngine`

### Extending Search Index
```javascript
// In searchService.js
async addCustomData(data) {
    const id = Date.now().toString();
    this.searchIndex.set(id, { ...data, id });
    return { id, indexed: true };
}
```

### Custom Relevance Scoring
```javascript
// In searchService.js
calculateCustomRelevanceScore(item, searchTerms) {
    let score = 0;
    // Add your custom scoring logic here
    return score;
}
```

## Troubleshooting

### Backend Not Starting
- Check if Node.js is installed: `node --version`
- Install dependencies: `npm install`
- Check port availability: `netstat -an | findstr :3001`

### Browser Can't Connect
- Verify backend is running: `curl http://localhost:3001/health`
- Check firewall settings
- Ensure CORS is configured correctly

### Search Not Working
- Check browser console for errors
- Verify network connectivity
- Test API directly: `curl -X POST http://localhost:3001/api/search -H "Content-Type: application/json" -d '{"query":"test"}'`

## Performance

### Optimization Tips
- Use connection pooling for HTTP requests
- Implement caching for frequent searches
- Add pagination for large result sets
- Use async/await for non-blocking operations

### Monitoring
- Check search response times
- Monitor memory usage
- Track search query patterns
- Monitor backend health

## Security

### Best Practices
- Validate all input data
- Use HTTPS in production
- Implement rate limiting
- Sanitize search queries
- Add authentication for admin features

## Future Enhancements

### Planned Features
- [ ] Voice search integration
- [ ] Image search capabilities
- [ ] Advanced filtering options
- [ ] Search analytics dashboard
- [ ] Multi-language support
- [ ] Search result caching
- [ ] Personalized search results

### Contributing
1. Fork the repository
2. Create a feature branch
3. Implement your changes
4. Add tests
5. Submit a pull request

## License

This search engine integration is part of the Zepra Browser project and follows the same licensing terms. 