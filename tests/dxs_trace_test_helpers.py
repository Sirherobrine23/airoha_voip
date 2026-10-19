"""Helpers for compiling actual driver function bodies in small host models."""
from pathlib import Path
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[1]

def function(source, signature):
    start = source.index(signature)
    opening = source.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

def run_c(source):
    with tempfile.TemporaryDirectory() as tmp:
        src = Path(tmp)/'test.c'
        out = Path(tmp)/'test'
        src.write_text(source)
        subprocess.run(['cc','-std=gnu11','-Wall','-Wextra','-Werror',
                        '-Wno-unused-parameter',str(src),'-o',str(out)],check=True)
        subprocess.run([str(out)],check=True)
