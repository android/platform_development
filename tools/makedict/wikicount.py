#!/usr/bin/python

'''
	wikicount -- build word counts from wikipedia dumps
	Copyright (C) 2011 The Android Open Source Project

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

		http://www.apache.org/licenses/LICENSE-2.0

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
'''

# usage: ./wikicount.py < wikidump.xml > counts.json
# output format is {'forWord':[numArticles,numOccurrences]}

# TODO: this could be faster in c++
# took several hours to chew through 23M articles

import xml.dom.pulldom as pulldom
import xml.dom.minidom as minidom

import re, sys, simplejson


re_tags = re.compile(r'<[^>]+>')
re_link = re.compile(r'\[\[(?:[^\[\]]+?\|)?([^\[\]:]+?)\]\]')
re_clean = re.compile(r'&(amp|nbsp|mdash|ndash);')
re_directive = re.compile(r'[\[\{]+[^\[\]\{\}]+[\]\}]+')

re_word = re.compile(r"[A-Za-z]+(?:'[A-Za-z]+)?")


def innerText(node):
	if node.nodeType == minidom.Node.TEXT_NODE:
		return node.data
	else:
		return ''.join([ innerText(child) for child in node.childNodes ])

ARTICLE, TOTAL = range(2)

words = {}

total_words = 0
article = 0

doc = pulldom.parse(sys.stdin)
for event, node in doc:
	if event == pulldom.START_ELEMENT and node.localName == "text":
		doc.expandNode(node)
		text = innerText(node)
		article += 1

		# skip stub articles
		if not text.find("#REDIRECT"): continue

		text = re_tags.sub('', text)
		text = re_link.sub(r'\1', text)
		text = re_directive.sub('', text)
		text = re_directive.sub('', text)
		text = re_clean.sub('', text)

		article_words = {}
		for word in re_word.findall(text):
			if word not in words:
				words[word] = [0,0]
			if word not in article_words:
				article_words[word] = True
				words[word][ARTICLE] += 1
			words[word][TOTAL] += 1

		if article % 5 == 0:
			print >> sys.stderr, "\rExamined", article, "articles...",

print simplejson.dumps(words)

print >> sys.stderr, "done!"
