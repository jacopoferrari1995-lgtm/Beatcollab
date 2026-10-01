#!/usr/bin/env python3
"""Costruisce piano/index.html (file unico, senza dipendenze) dai sorgenti in piano/src/."""
import os
here=os.path.dirname(os.path.abspath(__file__));src=os.path.join(here,'src')
read=lambda n:open(os.path.join(src,n),encoding='utf-8').read()
html=read('index.tpl.html')
for tag,f in [('CSS','style.css'),('ENGINE','engine.js'),('SYNTH','synth.js'),('APP','app.js'),('EX','ex.js')]:
    body=read(f)
    assert '</script' not in body.lower() and '</style' not in body.lower(), f
    html=html.replace('/*@@%s@@*/'%tag,body)
open(os.path.join(here,'index.html'),'w',encoding='utf-8').write(html)
print('index.html', len(html)//1024, 'KB')
