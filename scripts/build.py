#!/usr/bin/env python3
"""Build embedded callback dex, native Zygisk libraries and flashable module ZIP."""
import argparse
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import zipfile

ROOT = Path(__file__).resolve().parents[1]

def run(*args):
    subprocess.run([str(x) for x in args], cwd=ROOT, check=True)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--ndk', required=True, type=Path, help='Android NDK r29 directory')
    parser.add_argument('--build-tools', required=True, type=Path, help='SDK build-tools 36.0.0 directory')
    parser.add_argument('--java-home', type=Path, default=os.environ.get('JAVA_HOME'))
    parser.add_argument('--cmake', default='cmake')
    parser.add_argument('--ninja', default='ninja')
    parser.add_argument('--abis', nargs='+', choices=['arm64-v8a', 'x86_64'], default=['arm64-v8a'])
    parser.add_argument('--lsplant-source', type=Path, help='Optional pre-fetched pinned source')
    parser.add_argument('--dobby-source', type=Path, help='Optional pre-fetched pinned source')
    parser.add_argument('--xz-source', type=Path, help='Optional pre-fetched XZ Embedded source')
    args = parser.parse_args()
    args.ndk = args.ndk.resolve()
    args.build_tools = args.build_tools.resolve()
    if args.lsplant_source: args.lsplant_source = args.lsplant_source.resolve()
    if args.dobby_source: args.dobby_source = args.dobby_source.resolve()
    if args.xz_source: args.xz_source = args.xz_source.resolve()
    suffix = '.exe' if os.name == 'nt' else ''
    java = args.java_home / 'bin' / ('java' + suffix) if args.java_home else 'java'
    javac = args.java_home / 'bin' / ('javac' + suffix) if args.java_home else 'javac'
    generated = ROOT / 'build' / 'generated'
    classes = ROOT / 'build' / 'classes'
    dex = ROOT / 'build' / 'dex'
    for folder in [generated, classes, dex]: folder.mkdir(parents=True, exist_ok=True)
    # No Android classes in callback; java.base stubs suffice.
    run(javac, '--release', '8', '-Xlint:-options', '-d', classes, *sorted((ROOT / 'java').rglob('*.java')))
    run(java, '-cp', args.build_tools / 'lib' / 'd8.jar', 'com.android.tools.r8.D8',
        '--release', '--min-api', '27', '--output', dex, *sorted(classes.rglob('*.class')))
    blob = (dex / 'classes.dex').read_bytes()
    (generated / 'hook_dex.hpp').write_text(
        '#pragma once\nalignas(8) inline const unsigned char kHookDex[] = {\n' +
        ',\n'.join(','.join(f'0x{x:02x}' for x in blob[i:i+24]) for i in range(0,len(blob),24)) +
        '\n};\n', encoding='utf-8')
    package = ROOT / 'build' / 'package'
    package.mkdir(exist_ok=True)
    # Clear only the known generated module staging directory to avoid stale ABIs.
    for child in package.iterdir():
        if child.is_dir(): shutil.rmtree(child)
        else: child.unlink()
    shutil.copytree(ROOT / 'module', package, dirs_exist_ok=True)
    (package / 'zygisk').mkdir()
    for abi in args.abis:
        out = ROOT / 'build' / abi
        command = [args.cmake, '-S', ROOT, '-B', out, '-G', 'Ninja',
            f'-DCMAKE_MAKE_PROGRAM={args.ninja}',
            f'-DCMAKE_TOOLCHAIN_FILE={args.ndk.resolve()}/build/cmake/android.toolchain.cmake',
            f'-DANDROID_ABI={abi}', '-DANDROID_PLATFORM=android-27',
            '-DANDROID_STL=c++_static', '-DCMAKE_BUILD_TYPE=Release',
            '-DCMAKE_POLICY_VERSION_MINIMUM=3.5', f'-DSORAXHOOK_GENERATED_DIR={generated}']
        if args.lsplant_source:
            command.append(f'-DFETCHCONTENT_SOURCE_DIR_LSPLANT={args.lsplant_source.resolve()}')
        if args.dobby_source:
            command.append(f'-DFETCHCONTENT_SOURCE_DIR_DOBBY={args.dobby_source.resolve()}')
        if args.xz_source:
            command.append(f'-DFETCHCONTENT_SOURCE_DIR_XZ_EMBEDDED={args.xz_source.resolve()}')
        run(*command)
        run(args.cmake, '--build', out, '--target', 'soraxhook', '--parallel', '4')
        library = package / 'zygisk' / f'{abi}.so'
        shutil.copy2(out / 'libsoraxhook.so', library)
        host = 'windows-x86_64' if os.name == 'nt' else 'linux-x86_64'
        strip = args.ndk / 'toolchains' / 'llvm' / 'prebuilt' / host / 'bin' / ('llvm-strip' + suffix)
        run(strip, '--strip-unneeded', library)
    # LGPL-linked source is available via pinned CMake source refs; ship license notices.
    shutil.copy2(ROOT / 'THIRD_PARTY_NOTICES.md', package)
    shutil.copytree(ROOT / 'licenses', package / 'licenses', dirs_exist_ok=True)
    dist = ROOT / 'dist'
    dist.mkdir(exist_ok=True)
    properties = dict(line.split('=', 1) for line in (ROOT / 'module/module.prop').read_text().splitlines() if '=' in line)
    version = properties['version']
    archive = dist / f'SoraXHook-{version}.zip'
    with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED) as output:
        for path in sorted(package.rglob('*')):
            if not path.is_file(): continue
            name = path.relative_to(package).as_posix()
            info = zipfile.ZipInfo(name)
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = (0o100755 if path.suffix == '.sh' else 0o100644) << 16
            output.writestr(info, path.read_bytes())
    digest = hashlib.sha256(archive.read_bytes()).hexdigest()
    archive.with_suffix('.zip.sha256').write_text(f'{digest}  {archive.name}\n', encoding='ascii')
    sources = dist / f'SoraXHook-{version}-source.zip'
    with zipfile.ZipFile(sources, 'w', zipfile.ZIP_DEFLATED) as output:
        excluded = {'build', 'dist', '.git', '.deps', '.tools', 'work', '__pycache__'}
        for path in sorted(ROOT.rglob('*')):
            if path.is_file() and not excluded.intersection(path.relative_to(ROOT).parts):
                output.write(path, 'SoraXHook/' + path.relative_to(ROOT).as_posix())
        for name, override in [('lsplant', args.lsplant_source), ('dobby', args.dobby_source), ('xz_embedded', args.xz_source)]:
            source = override if override else ROOT / 'build' / args.abis[0] / '_deps' / f'{name}-src'
            # Include source and dependency submodules, without Git metadata/builds.
            for path in sorted(source.rglob('*')):
                rel = path.relative_to(source)
                if path.is_file() and not {'.git', '.gradle', 'build', '__pycache__'}.intersection(rel.parts):
                    output.write(path, 'dependencies/' + name + '/' + rel.as_posix())
        output.writestr('BUILD_FROM_SOURCE.txt',
            'Unzip this archive. Run SoraXHook/scripts/build.py with tool paths from USAGE.md, '
            'adding --lsplant-source dependencies/lsplant --dobby-source dependencies/dobby --xz-source dependencies/xz_embedded '
            '(paths relative to your shell working directory). Dependency revisions are in '
            'SoraXHook/THIRD_PARTY_NOTICES.md. No generated binaries are required to rebuild.\n')
    print(f'Built {archive}\nSHA256 {digest}')

if __name__ == '__main__':
    main()
