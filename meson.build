project('amibootng', 'c', meson_version: '>=1.1',
  default_options: [ 'buildtype=release', 'b_staticpic=false' ])

amipicolibc = subproject('amipicolibc')
picolibc_dep = amipicolibc.get_variable('picolibc_dep')
elf2hunk = amipicolibc.get_variable('elf2hunk')
amigaos_crt0 = amipicolibc.get_variable('amigaos_crt0')
amigaos_glue = amipicolibc.get_variable('amigaos_glue')
amigaos_inc = amipicolibc.get_variable('amigaos_inc')
python = find_program('python3')

logo_h = custom_target('logo.h',
  output: 'logo.h',
  input: 'data/logo_linux_vga16.ppm',
  command: [ python, files('tools/logo.py'), 'header', '@INPUT@', '@OUTPUT@' ])

# picolibc is built for the 68020.  -fomit-frame-pointer frees a6 for
# library calls (src/lvo.h).
c_args = (amipicolibc.get_variable('amigaos_c_args') +
          [ '-m68020', '-fno-pic', '-fomit-frame-pointer' ])
mkhunk = [ python, files('tools/mkhunk.py'), find_program('objcopy'),
           elf2hunk ]

programs = {
  'amibootng': [ 'src/main.c', 'src/menu.c', 'src/splash.c', 'src/serial.c',
                 'src/elf.c', 'src/bootinfo.c', logo_h ],
  'amibootng-startup': [ 'src/startup.c' ],
}
hunks = {}
foreach name, srcs : programs
  # -r keeps the relocations elf2hunk turns into hunk relocations
  elf = executable(name + '.elf', [ amigaos_crt0, 'src/util.c' ] + srcs,
    c_args: c_args,
    link_args: [ '-nostdlib', '-r', '-lgcc' ],
    include_directories: amigaos_inc,
    link_with: amigaos_glue,
    dependencies: picolibc_dep)
  hunks += { name: custom_target(name,
    output: name,
    input: elf,
    command: mkhunk + [ '@INPUT@', '@OUTPUT@' ],
    build_by_default: true) }
endforeach

xdftool = find_program('xdftool', required: false)
installer = get_option('installer')
if xdftool.found()
  icon = [ python, files('tools/logo.py'), 'icon', '@INPUT@', '@OUTPUT@',
           installer != '' ? 'amibootng:Installer' : 'Installer',
           'APPNAME=amibootng', 'LOGFILE=RAM:amibootng-install.log' ]
  install_info = custom_target('Install-amibootng.info',
    output: 'Install-amibootng.info',
    input: 'data/logo_linux_vga16.ppm',
    command: icon + [ 'DEFUSER=AVERAGE' ])
  # Novice mode skips (message), which Remove-amibootng ends with
  remove_info = custom_target('Remove-amibootng.info',
    output: 'Remove-amibootng.info',
    input: 'data/logo_linux_vga16.ppm',
    command: icon + [ 'MINUSER=AVERAGE', 'DEFUSER=AVERAGE' ])
  custom_target('amibootng.adf',
    output: 'amibootng.adf',
    input: [ 'README.md', hunks['amibootng'], hunks['amibootng-startup'],
             'install/Install-amibootng', install_info,
             'install/Remove-amibootng', remove_info ],
    command: [ files('tools/mkadf.sh'), xdftool, '@OUTPUT@' ] +
             (installer != '' ? [ '-i', files(installer) ] : []) +
             [ '@INPUT@' ],
    build_by_default: true)
endif
