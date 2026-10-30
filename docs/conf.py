# FromSoheWare Engine ドキュメント - Sphinx 設定ファイル
#
# ビルド方法は docs/buildingDocs.md を参照してください。
# このファイルはビルドのたびに Doxygen を実行し、C++ ヘッダーから
# API リファレンス用の XML を生成してから Sphinx に渡します。

import os
import shutil
import subprocess
import sys

DOCS_DIR = os.path.dirname(os.path.abspath(__file__))
DOXYGEN_XML_DIR = os.path.join(DOCS_DIR, "_build", "doxygen", "xml")

# -- プロジェクト情報 -------------------------------------------------------

project = "FromSoheWare Engine"
author = "FromSoheWare"
copyright = "2026, FromSoheWare"
release = "1.0"

language = "ja"

# -- Doxygen の実行 -----------------------------------------------------------
# 環境変数 SKIP_DOXYGEN=1 を付けると、既存の XML を使って Doxygen をスキップします
# (ガイドの文章だけを直しているときにビルドを速くしたい場合など)。


def run_doxygen():
    if os.environ.get("SKIP_DOXYGEN") == "1" and os.path.isdir(DOXYGEN_XML_DIR):
        return
    doxygen = os.environ.get("DOXYGEN", "doxygen")
    if shutil.which(doxygen) is None:
        sys.stderr.write(
            "\n[conf.py] Doxygen が見つかりません。インストールして PATH を通すか、\n"
            "環境変数 DOXYGEN に doxygen.exe のフルパスを指定してください。\n"
            "(API リファレンスなしでビルドを続けます)\n\n"
        )
        return
    os.makedirs(os.path.dirname(DOXYGEN_XML_DIR), exist_ok=True)
    subprocess.run([doxygen, "Doxyfile"], cwd=DOCS_DIR, check=True)


run_doxygen()

# -- 拡張機能 ---------------------------------------------------------------

extensions = [
    "myst_parser",            # Markdown (.md) で書けるようにする
    "breathe",                # Doxygen の XML を Sphinx に取り込む
    "sphinxcontrib.mermaid",  # Mermaid 図 (```mermaid) を描画する
    "sphinx_copybutton",      # コードブロックにコピーボタンを付ける
]

source_suffix = {
    ".rst": "restructuredtext",
    ".md": "markdown",
}

root_doc = "index"

exclude_patterns = [
    "_build",
    "Thumbs.db",
    ".DS_Store",
    "README.md",
]

# -- MyST (Markdown) ---------------------------------------------------------

myst_enable_extensions = [
    "colon_fence",  # ::: で囲むディレクティブ
    "deflist",
    "attrs_inline",
]
# 見出しにアンカーを付け、ページ内リンク (#1-xxx) を GitHub と同じように使えるようにする
myst_heading_anchors = 4
# ```mermaid のコードブロックを図として描画する (GitHub 上でもそのまま図になる)
myst_fence_as_directive = ["mermaid"]

# -- Breathe (Doxygen 連携) ---------------------------------------------------

breathe_projects = {"FromSoheWare": DOXYGEN_XML_DIR}
breathe_default_project = "FromSoheWare"
breathe_default_members = ("members", "undoc-members")
breathe_show_include = True
breathe_show_define_initializer = True
breathe_show_enumvalue_initializer = True

primary_domain = "cpp"
highlight_language = "cpp"

# C++ ドメインがパースできない MSVC 固有の記述を無視させる
cpp_id_attributes = ["STDMETHODCALLTYPE", "WINAPI", "__declspec"]
cpp_paren_attributes = ["alignas", "__declspec"]

# Doxygen 由来の警告のうち、ビルドに影響しないものを抑制
suppress_warnings = [
    "duplicate_declaration.cpp",
    "myst.xref_missing",
]

# -- HTML 出力 ---------------------------------------------------------------

html_theme = "furo"
html_title = "FromSoheWare Engine ドキュメント"
html_static_path = []
html_search_language = "ja"

html_theme_options = {
    "navigation_with_keys": True,
}

# コードブロックのプロンプト記号をコピー対象から外す
copybutton_prompt_text = r">>> |\$ |> |PS> "
copybutton_prompt_is_regexp = True
