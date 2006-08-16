#!/bin/sh

# A little hack to add SVN keywords to a file
# Usage: enable-keywords.sh <files>

SVN=svn

KEYWORDS="ID Date Author Revision HeadURL"

$SVN propset svn:keywords "$KEYWORDS" $@
