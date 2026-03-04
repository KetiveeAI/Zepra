import os
import re

base_dir = "/home/swana/dev/search/zeprabrowser/docs/NXRENDER"
files = ["architecture.html", "dev.html", "todo.html"]

def create_template(title_text, subtitle_text, active_link_id):
    nav_links = {
        "index": "NXRender Home",
        "architecture": "Architecture API",
        "dev": "Developer Guide",
        "todo": "TODOs",
    }
    
    nav_items_html = ""
    for k, v in nav_links.items():
        href = k + ".html"
        
        if k == "index":
            icon = '<svg class="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M3 12l2-2m0 0l7-7 7 7M5 10v10a1 1 0 001 1h3m10-11l2 2m-2-2v10a1 1 0 01-1 1h-3m-6 0a1 1 0 001-1v-4a1 1 0 011-1h2a1 1 0 011 1v4a1 1 0 001 1m-6 0h6"/></svg>'
        elif k == "architecture":
            icon = '<svg class="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M19 21V5a2 2 0 00-2-2H7a2 2 0 00-2 2v16m14 0h2m-2 0h-5m-9 0H3m2 0h5M9 7h1m-1 4h1m4-4h1m-1 4h1m-5 10v-5a1 1 0 011-1h2a1 1 0 011 1v5m-4 0h4"/></svg>'
        elif k == "dev":
            icon = '<svg class="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M10 20l4-16m4 4l4 4-4 4M6 16l-4-4 4-4"/></svg>'
        else:
            icon = '<svg class="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M9 5H7a2 2 0 00-2 2v12a2 2 0 002 2h10a2 2 0 002-2V7a2 2 0 00-2-2h-2M9 5a2 2 0 002 2h2a2 2 0 002-2M9 5a2 2 0 012-2h2a2 2 0 012 2"/></svg>'
            
        if k == active_link_id:
            cls = "nav-link active flex items-center gap-3 px-3 py-2 text-sm text-white rounded-lg border-l-2 border-accent"
        else:
            cls = "nav-link flex items-center gap-3 px-3 py-2 text-sm text-gray-400 hover:text-white rounded-lg hover:bg-white/5 transition-all border-l-2 border-transparent"
        
        nav_items_html += f'                    <a href="{href}" class="{cls}">\n                        {icon}\n                        {v}</a>\n'

    template = f"""<!DOCTYPE html>
<html lang="en" class="scroll-smooth">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>{title_text}</title>
    <script src="https://cdn.tailwindcss.com?plugins=typography"></script>
    <script src="https://cdnjs.cloudflare.com/ajax/libs/gsap/3.12.5/gsap.min.js"></script>
    <script src="https://cdnjs.cloudflare.com/ajax/libs/gsap/3.12.5/ScrollTrigger.min.js"></script>
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700;800&family=JetBrains+Mono:wght@400;500&display=swap" rel="stylesheet">
    <script>
        tailwind.config = {{
            theme: {{
                extend: {{
                    colors: {{
                        dark: {{ 900: '#0a0a0f', 800: '#12121a', 700: '#1a1a24', 600: '#24242f' }},
                        accent: {{ DEFAULT: '#f43f5e', light: '#fb7185' }},
                    }},
                    fontFamily: {{ sans: ['Inter', 'sans-serif'], mono: ['JetBrains Mono', 'monospace'] }},
                    typography: (theme) => ({{
                        DEFAULT: {{
                            css: {{
                                color: theme('colors.gray.300'),
                                a: {{ color: theme('colors.accent.DEFAULT'), '&:hover': {{ color: theme('colors.accent.light') }} }},
                                h1: {{ color: theme('colors.white') }}, h2: {{ color: theme('colors.white') }},
                                h3: {{ color: theme('colors.white') }}, h4: {{ color: theme('colors.white') }},
                                code: {{ color: theme('colors.pink.400') }},
                                pre: {{ backgroundColor: theme('colors.dark.800'), color: theme('colors.gray.200') }},
                                blockquote: {{ color: theme('colors.gray.400'), borderLeftColor: theme('colors.accent.DEFAULT') }}
                            }}
                        }}
                    }})
                }}
            }}
        }}
    </script>
    <style>
        ::-webkit-scrollbar {{ width: 8px; }}
        ::-webkit-scrollbar-track {{ background: #12121a; }}
        ::-webkit-scrollbar-thumb {{ background: #3f3f5a; border-radius: 4px; }}
        .gradient-text {{ background: linear-gradient(135deg, #f43f5e 0%, #a855f7 50%, #3b82f6 100%); -webkit-background-clip: text; background-clip: text; color: transparent; }}
        .hero-gradient {{ background: radial-gradient(ellipse 80% 50% at 50% -20%, rgba(244, 63, 94, 0.15), transparent); }}
        .glass {{ background: rgba(26, 26, 36, 0.8); backdrop-filter: blur(20px); border: 1px solid rgba(244, 63, 94, 0.1); }}
        .glow {{ box-shadow: 0 0 80px rgba(244, 63, 94, 0.2); }}
        .nav-link.active {{ background: linear-gradient(90deg, rgba(244, 63, 94, 0.2) 0%, transparent 100%); border-left-color: #f43f5e; }}
        .float {{ animation: float 6s ease-in-out infinite; }}
        @keyframes float {{ 0%, 100% {{ transform: translateY(0); }} 50% {{ transform: translateY(-20px); }} }}
    </style>
</head>
<body class="bg-dark-900 text-gray-100 font-sans antialiased">
    <div class="fixed inset-0 pointer-events-none hero-gradient"></div>
    <div class="fixed inset-0 pointer-events-none overflow-hidden">
        <div class="absolute top-1/4 left-1/4 w-[800px] h-[800px] bg-accent/5 rounded-full blur-[200px] float"></div>
        <div class="absolute bottom-0 right-1/4 w-[600px] h-[600px] bg-purple-500/5 rounded-full blur-[150px]"></div>
    </div>

    <aside class="fixed left-0 top-0 w-72 h-screen bg-dark-800/90 backdrop-blur-xl border-r border-white/5 z-50 overflow-y-auto">
        <div class="p-6 border-b border-white/5">
            <div class="flex items-center gap-3">
                <div class="w-10 h-10 rounded-xl bg-gradient-to-br from-accent to-purple-500 flex items-center justify-center glow">
                    <svg class="w-6 h-6 text-white" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M7 21a4 4 0 01-4-4V5a2 2 0 012-2h4a2 2 0 012 2v12a4 4 0 01-4 4zm0 0h12a2 2 0 002-2v-4a2 2 0 00-2-2h-2.343M11 7.343l1.657-1.657a2 2 0 012.828 0l2.829 2.829a2 2 0 010 2.828l-8.486 8.485M7 17h.01"/></svg>
                </div>
                <div>
                    <h1 class="font-bold text-lg">NXRender</h1>
                    <span class="text-xs text-gray-500">v2.0.0 Documentation</span>
                </div>
            </div>
        </div>
        <nav class="p-4">
            <div class="mb-6">
                <span class="text-[10px] font-semibold uppercase tracking-wider text-gray-500 px-3">Navigation</span>
                <div class="mt-2 space-y-1">
                    <a href="../index.html" class="nav-link flex items-center gap-3 px-3 py-2 text-sm text-gray-400 hover:text-white rounded-lg hover:bg-white/5 transition-all border-l-2 border-transparent">
                        <svg class="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M10 19l-7-7m0 0l7-7m-7 7h18"/></svg>
                        Hub Home</a>
{nav_items_html}
                </div>
            </div>
        </nav>
    </aside>

    <main class="ml-72 min-h-screen relative z-10">
        <div class="max-w-4xl mx-auto px-8 py-20">
            <!-- Header -->
            <div class="mb-12 border-b border-white/10 pb-8 animate-header">
                <h1 class="text-4xl font-bold text-white tracking-tight">{title_text}</h1>
                <p class="text-gray-400 mt-2">{subtitle_text}</p>
            </div>

            <!-- Content -->
            <div class="prose prose-invert prose-rose max-w-none animate-section">
                __CONTENT_PLACEHOLDER__
            </div>

            <footer class="pt-12 border-t border-white/5 text-center mt-12 pb-8">
                <div class="text-sm text-gray-500">NXRender Framework • ZepraBrowser • © 2026 KetiveeAI</div>
            </footer>
        </div>
    </main>

    <script>
        gsap.registerPlugin(ScrollTrigger);
        gsap.from('.animate-header', {{ opacity: 0, y: 30, duration: 1, ease: 'power3.out' }});
        gsap.from('.animate-section', {{ opacity: 0, y: 30, duration: 1, ease: 'power3.out', delay: 0.2 }});
    </script>
</body>
</html>"""
    return template

for filename in files:
    filepath = os.path.join(base_dir, filename)
    with open(filepath, "r") as f:
        html = f.read()
    
    # regex extract the prose div
    match = re.search(r'<div class="prose.*?(?=>)>(.*?)</div>\s*</main>', html, re.DOTALL)
    if not match:
        print(f"Skipping {filename}: no prose div found")
        continue

    content_html = match.group(1).strip()
    
    if filename == "architecture.html":
        title = "Architecture & API"
        sub = "Deep dive into NXRender internals and subsystems"
        fid = "architecture"
    elif filename == "dev.html":
        title = "Developer Guide"
        sub = "How to build, extend, and debug the rendering engine"
        fid = "dev"
    else:
        title = "Implementation TODOs"
        sub = "Current development status and roadmap"
        fid = "todo"
        
    template = create_template(title, sub, fid)
    final_html = template.replace("__CONTENT_PLACEHOLDER__", content_html)
    
    with open(filepath, "w") as f:
        f.write(final_html)
    print(f"Updated {filename}")

# Update NXRENDER/index.html to have the exact same navigation links (without re-wrapping the page)
index_path = os.path.join(base_dir, "index.html")
with open(index_path, "r") as f:
    idx_content = f.read()

replacement_nav = """<div class="mt-2 space-y-1">
                    <a href="../index.html" class="nav-link flex items-center gap-3 px-3 py-2 text-sm text-gray-400 hover:text-white rounded-lg hover:bg-white/5 transition-all border-l-2 border-transparent">
                        <svg class="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M10 19l-7-7m0 0l7-7m-7 7h18"/></svg>
                        Hub Home</a>
                    <a href="index.html" class="nav-link active flex items-center gap-3 px-3 py-2 text-sm text-white rounded-lg border-l-2 border-accent">
                        <svg class="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M3 12l2-2m0 0l7-7 7 7M5 10v10a1 1 0 001 1h3m10-11l2 2m-2-2v10a1 1 0 01-1 1h-3m-6 0a1 1 0 001-1v-4a1 1 0 011-1h2a1 1 0 011 1v4a1 1 0 001 1m-6 0h6"/></svg>
                        NXRender Home</a>
                    <a href="architecture.html" class="nav-link flex items-center gap-3 px-3 py-2 text-sm text-gray-400 hover:text-white rounded-lg hover:bg-white/5 transition-all border-l-2 border-transparent">
                        <svg class="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M19 21V5a2 2 0 00-2-2H7a2 2 0 00-2 2v16m14 0h2m-2 0h-5m-9 0H3m2 0h5M9 7h1m-1 4h1m4-4h1m-1 4h1m-5 10v-5a1 1 0 011-1h2a1 1 0 011 1v5m-4 0h4"/></svg>
                        Architecture API</a>
                    <a href="dev.html" class="nav-link flex items-center gap-3 px-3 py-2 text-sm text-gray-400 hover:text-white rounded-lg hover:bg-white/5 transition-all border-l-2 border-transparent">
                        <svg class="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M10 20l4-16m4 4l4 4-4 4M6 16l-4-4 4-4"/></svg>
                        Developer Guide</a>
                    <a href="todo.html" class="nav-link flex items-center gap-3 px-3 py-2 text-sm text-gray-400 hover:text-white rounded-lg hover:bg-white/5 transition-all border-l-2 border-transparent">
                        <svg class="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M9 5H7a2 2 0 00-2 2v12a2 2 0 002 2h10a2 2 0 002-2V7a2 2 0 00-2-2h-2M9 5a2 2 0 002 2h2a2 2 0 002-2M9 5a2 2 0 012-2h2a2 2 0 012 2"/></svg>
                        TODOs</a>
                </div>"""
idx_content = re.sub(r'<div class="mt-2 space-y-1">.*?</div>', replacement_nav, idx_content, count=1, flags=re.DOTALL)
with open(index_path, "w") as f:
    f.write(idx_content)
print("Updated index.html nav!")

