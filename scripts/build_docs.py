import os
import glob
import markdown

DOCS_DIR = 'Docs'
HTML_DIR = 'Docs_html'

html_template = """<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>{title}</title>
    <style>
        body {{
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
            line-height: 1.6;
            color: #333;
            max-width: 800px;
            margin: 0 auto;
            padding: 2rem;
        }}
        pre {{
            background: #f4f4f4;
            padding: 1rem;
            border-radius: 5px;
            overflow-x: auto;
        }}
        code {{
            background: #f4f4f4;
            padding: 0.2rem 0.4rem;
            border-radius: 3px;
        }}
        h1, h2, h3 {{ border-bottom: 1px solid #eaecef; padding-bottom: 0.3em; }}
        a {{ color: #0366d6; text-decoration: none; }}
        a:hover {{ text-decoration: underline; }}
        table {{ border-collapse: collapse; width: 100%; }}
        th, td {{ border: 1px solid #dfe2e5; padding: 6px 13px; }}
        th {{ background-color: #f6f8fa; }}
    </style>
</head>
<body>
{content}
</body>
</html>
"""

def convert_md_to_html():
    if not os.path.exists(HTML_DIR):
        os.makedirs(HTML_DIR)

    md_files = glob.glob(f'{DOCS_DIR}/**/*.md', recursive=True)
    
    for md_file in md_files:
        # Read markdown
        with open(md_file, 'r', encoding='utf-8') as f:
            text = f.read()
            
        # Convert to HTML
        html_content = markdown.markdown(text, extensions=['fenced_code', 'tables'])
        
        # Output paths
        rel_path = os.path.relpath(md_file, DOCS_DIR)
        html_rel_path = os.path.splitext(rel_path)[0] + '.html'
        out_file = os.path.join(HTML_DIR, html_rel_path)
        
        # Ensure output dir exists
        os.makedirs(os.path.dirname(out_file), exist_ok=True)
        
        # Write HTML
        title = os.path.basename(md_file).replace('.md', '')
        final_html = html_template.format(title=title, content=html_content)
        
        with open(out_file, 'w', encoding='utf-8') as f:
            f.write(final_html)
            
        print(f"Converted {md_file} -> {out_file}")

if __name__ == '__main__':
    convert_md_to_html()
