#!/usr/bin/env ruby

def calc(actual, target, val)
  val / (1 + (target - actual) / actual) 
end

puts "calc(#{ARGV[0].to_f}, #{(ARGV[1] || 24.5).to_f})"
puts calc(ARGV[0].to_f, ARGV[1].to_f, ARGV[2].to_f)
