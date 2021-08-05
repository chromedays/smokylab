import argparse
import os
import subprocess
import shutil
from pathlib import Path

current_dir = Path(os.path.dirname(os.path.realpath(__file__))).absolute()
source_dir = current_dir / 'src'
out_dir = current_dir / 'bin'
msvc_clang_out_dir = out_dir / 'msvc_clang'
ninja_out_dir = out_dir / 'ninja'


def run_command(cmd: str):
    print(f'[Command] {cmd}')
    subprocess.run(cmd)


def build_shader(name: str):
    fxc = R'C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\fxc.exe'
    vsoutput = f'bin/{name}.vs.cso'
    fsoutput = f'bin/{name}.fs.cso'
    source = f'shaders/{name}.hlsl'

    run_command(
        f'"{fxc}" "{source}" -E vert -T vs_5_0 -nologo -Fo "{vsoutput}"')
    run_command(
        f'"{fxc}" "{source}" -E frag -T ps_5_0 -nologo -Fo "{fsoutput}"')


def build(build_dir: Path):
    target = 'smokylab'
    configs = ['Debug', 'RelWithDebInfo', 'Release']

    for config in configs:
        run_command(
            f'cmake --build "{build_dir}" --config {config} --target {target}')
        shutil.copy2(build_dir / f'{config}' /
                     f'{target}.exe', f'bin/{target}_{build_dir.name}_{config}.exe')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--configure',
                        help='configure project', action='store_true')
    parser.add_argument('--build-test', action='store_true')
    parser.add_argument('--clear', action='store_true')
    args = parser.parse_args()

    if args.clear:
        if out_dir.exists() and out_dir.is_dir():
            shutil.rmtree(out_dir)
    if args.configure:
        msvc_clang_out_dir.mkdir(parents=True, exist_ok=True)
        ninja_out_dir.mkdir(parents=True, exist_ok=True)
        run_command(
            f'cmake -G "Visual Studio 16 2019" -A x64 -T ClangCL -B "{msvc_clang_out_dir}" -S "{source_dir}"')
        run_command(
            f'cmake -G "Ninja Multi-Config" -B "{ninja_out_dir}" -S "{source_dir}"')
        shutil.copy2(str(ninja_out_dir / 'compile_commands.json'),
                     str(current_dir))
    if args.build_test:
        build_shader('brdf')
        build_shader('new_brdf')
        build_shader('sky')
        build_shader('exposure')
        build_shader('wireframe')
        build_shader('fst')
        build_shader('oit_accum')
        build_shader('oit_composite')
        build_shader('ssao')
        # TODO: Move this to post-build event (Visual Studio)
        shutil.copy2('dep/SDL2/SDL2.dll', 'bin')
        build(msvc_clang_out_dir)
        build(ninja_out_dir)
