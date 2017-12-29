#!/usr/bin/env ruby

f = File.read(ARGV[0])

r = /#X obj \d+ \d+ (f?expr~?) (.*?)(?<!\\);$/m
f.scan(r).each do |cap|
  puts "#{cap[0]} #{cap[1].gsub("\n", "")}"
end

