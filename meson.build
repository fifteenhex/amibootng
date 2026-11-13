project('amibootng', 'c', meson_version: '>=1.1',
  default_options: [ 'buildtype=release', 'b_staticpic=false' ])

amipicolibc = subproject('amipicolibc')
picolibc_dep = amipicolibc.get_variable('picolibc_dep')
elf2hunk = amipicolibc.get_variable('elf2hunk')
amigaos_crt0 = amipicolibc.get_variable('amigaos_crt0')
amigaos_glue = amipicolibc.get_variable('amigaos_glue')
amigaos_inc = amipicolibc.get_variable('amigaos_inc')

logo_h = custom_target('logo.h',
  output: 'logo.h',
  input: 'data/logo_linux_vga16.ppm',
  command: [ find_program('python3'), files('tools/logo.py'), 'header',
             '@INPUT@', '@OUTPUT@' ])

# picolibc is built for the 68020.  -fomit-frame-pointer frees a6 for
# library calls (src/lvo.h).
c_args = [ '-m68020', '-fno-pic', '-fomit-frame-pointer' ]

# -r keeps the relocations elf2hunk turns into hunk relocations
amibootng_elf = executable('amibootng.elf',
  [ amigaos_crt0, 'src/util.c', 'src/main.c', 'src/menu.c', 'src/splash.c',
    'src/elf.c', 'src/bootinfo.c', logo_h ],
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
