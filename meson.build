project('amibootng', 'c', meson_version: '>=1.1',
  default_options: [ 'buildtype=release', 'b_staticpic=false' ])

amipicolibc = subproject('amipicolibc')
picolibc_dep = amipicolibc.get_variable('picolibc_dep')
elf2hunk = amipicolibc.get_variable('elf2hunk')
amigaos_crt0 = amipicolibc.get_variable('amigaos_crt0')
amigaos_glue = amipicolibc.get_variable('amigaos_glue')
amigaos_inc = amipicolibc.get_variable('amigaos_inc')

# picolibc is built for the 68020
c_args = [ '-m68020', '-fno-pic' ]

# -r keeps the relocations elf2hunk turns into hunk relocations
amibootng_elf = executable('amibootng.elf',
  [ amigaos_crt0, 'src/util.c', 'src/main.c', 'src/elf.c', 'src/bootinfo.c' ],
  c_args: c_args,
  link_args: [ '-nostdlib', '-r', '-lgcc' ],
  include_directories: amigaos_inc,
  link_with: amigaos_glue,
  dependencies: picolibc_dep)

custom_target('amibootng',
  output: 'amibootng',
  input: amibootng_elf,
  command: [ elf2hunk, '@INPUT@', '@OUTPUT@' ],
  build_by_default: true)
