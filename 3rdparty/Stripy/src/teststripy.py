#!/usr/bin/env python

import os
import subprocess

stripypath = os.getcwd() + "/stripy"
data = """112 113 114 115 112 114 114 113 116 117 114 118 114 117 115 114 116 119 120 121 122 122 123 120 122 121 124 123 122 113 113 112 123 113 124 116 121 125 126 126 124 121 125 127 128 128 126 125 124 126 116 124 113 122 126 128 129 130 116 126 127 131 128 132 133 134 134 135 136 118 134 117 134 118 137 133 138 139 139 134 133 138 120 123 123 139 138 117 139 115 139 117 134 123 112 139 112 115 139 -1"""

p = subprocess.Popen([stripypath, "-s"],
  stdin=subprocess.PIPE, stdout=subprocess.PIPE,
  close_fds=True)

print "Writing data"
p.stdin.write(data)
p.stdin.write("\n")

numstrips = int(p.stdout.readline())
print "numstrips", numstrips

for sidx in xrange(numstrips):
  striptype = int(p.stdout.readline())
  numindices = int(p.stdout.readline())
  indices = map(int, p.stdout.readline().split())

  print "striptype", striptype
  print "numindices", numindices
  print "indices", indices
