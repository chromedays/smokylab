import argparse
import os
import subprocess
import shutil
from pathlib import Path

current_dir = Path(os.path.dirname(os.path.realpath(__file__))).absolute()
source_dir = current_dir / '..'
out_dir = source_dir / 'bin'
msvc_clang_out_dir = out_dir / 'msvc_clang'
ninja_out_dir = out_dir / 'ninja'
tmp_dir = source_dir / 'tmp'


def run_command(cmd: str):
    print(f'[Command] {cmd}')
    subprocess.run(cmd)


def build(build_dir: Path):
    target = 'smokylab'
    configs = ['Debug', 'RelWithDebInfo', 'Release']

    for config in configs:
        run_command(
            f'cmake --build "{build_dir}" --config {config} --target {target}')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--configure',
                        help='configure project', action='store_true')
    parser.add_argument('--build-test', action='store_true')
    parser.add_argument('--compdb', action='store_true')
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

    if args.compdb:
        tmp_dir.mkdir(parents=True, exist_ok=True)
        run_command(
            f'cmake -G "Ninja Multi-Config" -B "{tmp_dir}" -S "{source_dir}"')
        shutil.copy2(str(tmp_dir / 'compile_commands.json'),
                     str(source_dir))
        shutil.rmtree(tmp_dir)

    if args.build_test:
        build(msvc_clang_out_dir)
        build(ninja_out_dir)
