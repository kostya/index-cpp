#!/usr/bin/env ruby

require 'open3'
require "json"

COMPILERS = {
  'gcc:7'                  => ['2017-05-02'],
  'gcc:8'                  => ['2018-05-02'],
  'gcc:9'                  => ['2019-05-03'],
  'gcc:10'                 => ['2020-05-07'],
  'gcc:11'                 => ['2021-04-27'],
  'gcc:12'                 => ['2022-05-06'],
  'gcc:13'                 => ['2023-04-26'],
  'gcc:14'                 => ['2024-05-07'],
  'gcc:15'                 => ['2025-04-25'],
  'gcc:16'                 => ['2026-04-30'],

  'teeks99/clang-ubuntu:7'   => ['2018-09-19'],
  'teeks99/clang-ubuntu:8'   => ['2019-03-20'],
  'teeks99/clang-ubuntu:9'   => ['2019-09-19'],
  'teeks99/clang-ubuntu:10'  => ['2020-03-24'],
  'teeks99/clang-ubuntu:11'  => ['2020-10-12'],
  'teeks99/clang-ubuntu:12'  => ['2021-04-14'],
  'teeks99/clang-ubuntu:13'  => ['2021-10-04'],
  'teeks99/clang-ubuntu:14'  => ['2022-03-25'],
  'teeks99/clang-ubuntu:15'  => ['2022-09-06'],
  'teeks99/clang-ubuntu:16'  => ['2023-03-17'],
  'teeks99/clang-ubuntu:17'  => ['2023-09-19'],
  'teeks99/clang-ubuntu:18'  => ['2024-03-06'],
  'teeks99/clang-ubuntu:19'  => ['2024-09-17'],
  'teeks99/clang-ubuntu:20'  => ['2025-03-04'],
  'teeks99/clang-ubuntu:21'  => ['2025-08-26'],
  'teeks99/clang-ubuntu:22'  => ['2026-02-24'],
  'teeks99/clang-ubuntu:23'  => ['2026-08-25'],
}

OPT_FLAGS = ["-O0", "-O1", "-O2", "-O3", "-Os", "-Oz", "-Ofast", "-O3 -march=native"]
SRC       = 'index.cpp'

def clean_version(raw)
  return '' if raw.nil? || raw.empty?

  s = raw.strip.lines.first.to_s.strip

  case s
  when /g\+\+.*?\(\s*GCC\s*\).*?([\d.]+)/i
    "G++ #{$1}"
  when /g\+\+\s+([\d.]+)/i
    "G++ #{$1}"
  when /clang(?:\+\+)?\s+version\s+([\d.]+)/i
    "Clang++ #{$1}"
  else
    s
  end
end

def run_one(image, flags)
  cc = image.include?('gcc') ? 'g++' : 'clang++'
  cmd = <<~SH
    cd /app
    rm -f /tmp/index
    echo versionstart
    #{cc} --version | head -1
    echo versionend
    echo compilestart
    { time bash /app/mem.sh #{cc} -std=c++17 #{flags} #{SRC} -lm -lpthread -o /tmp/index ; } 2>&1
    echo compileend
    echo rawsize_start
    stat -c %s /tmp/index
    echo rawsize_end
    /tmp/index
    strip /tmp/index 2>/dev/null || true
    echo stripsize_start
    stat -c %s /tmp/index
    echo stripsize_end
  SH

  stdout, stderr, status = Open3.capture3(
    'docker', 'run', '--rm',
    '-v', "#{Dir.pwd}/..:/app", '-w', '/app',
    image, 'bash', '-c', cmd
  )

  unless status.success?
    warn "[FAIL] #{image} #{flags}"
    warn stderr.lines.last(10).join
    warn stdout.lines.last(10).join
    return "bad status #{status.inspect}"
  end

  version = stdout[/versionstart\n(.*?)\nversionend/m, 1].to_s.strip

  block = stdout[/compilestart\n(.*?)\ncompileend/m, 1]
  return "no compilestart block" unless block

  compile_sec = block[/real\s+(\d+)m([\d.]+)s/, 1].to_f * 60 +
                block[/real\s+(\d+)m([\d.]+)s/, 2].to_f
  mem_mb = block[/mem_usage:\s*([\d.]+)Mb/, 1]&.to_f

  raw_size   = stdout[/rawsize_start\n(\d+)\nrawsize_end/m, 1]&.to_i
  strip_size = stdout[/stripsize_start\n(\d+)\nstripsize_end/m, 1]&.to_i

  summary_line = stdout.lines.grep(/^Summary:/).last
  return "no sumary line" unless summary_line

  m = summary_line.match(/Summary:\s*([\d.]+)s,\s*(\d+),\s*(\d+),\s*(\d+)/)
  return "parse summary" unless m

  if m[2].to_i != m[3].to_i && m[2].to_i != 50
    puts "Warning errors: #{[m[2].to_i, m[3].to_i, m[4].to_i].inspect}"
    return "errors"
  end

  {
    version: clean_version(version),
    compile_time: compile_sec,
    compile_mem_mb: mem_mb,
    bench_time: m[1].to_f,
    binary_size: raw_size,
    binary_size_stripped: strip_size,
  }
end

array = []

envs = []
COMPILERS.each do |image, (date)|
  OPT_FLAGS.each do |flags|
    envs << [image, date, flags]
  end
end

if ARGV[0] == "1"
  envs.shuffle!
end

envs.each_with_index do |(image, date, flags), index|
  print "[#{index + 1} from #{envs.size}] #{image}: #{flags} ... "
  res = run_one(image, flags)
  if res.is_a?(String)
    puts "[ERROR] (#{res})"
    next
  else
    puts "[OK] (version=#{res[:version]}, compile=#{res[:compile_time].round(2)}s, " \
         "compile_mem=#{res[:compile_mem_mb]&.round(1)}Mb, " \
         "runtime=#{res[:bench_time].round(2)}s, " \
         "size=#{res[:binary_size]} -> #{res[:binary_size_stripped]} bytes)"
  end

  res[:image] = image
  res[:release_date] = date
  res[:flags] = flags

  array << res
  File.open("./history.js", "w") { |f| f.puts(array.to_json) }
end

File.open("./history.js", "w") { |f| f.puts(array.to_json) }
