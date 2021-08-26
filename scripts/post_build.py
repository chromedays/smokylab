
from pathlib import Path
import argparse
import subprocess


class Logger:
    def __init__(self, output_path: str) -> None:
        self.file = open(output_path, 'w')

    def print(self, message: str):
        print(message)
        self.file.write(f'{message}\n')


logger = None


def run_command(cmd: str):
    logger.print(f'[Command]\n {cmd}')
    proc = subprocess.run(cmd, capture_output=True, universal_newlines=True)
    logger.print('stdout:')
    logger.print(proc.stdout.strip('\n'))
    logger.print('stderr:')
    logger.print(proc.stderr.strip('\n'))


class ShaderBuilder:
    def __init__(self, shader_dir: str, out_dir: str) -> None:
        self.shader_dir = Path(shader_dir)
        self.out_dir = Path(out_dir)

    def build_shader(self, name: str):
        fxc = R'C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\fxc.exe'
        vsoutput = self.out_dir / f'{name}.vs.cso'
        fsoutput = self.out_dir / f'{name}.fs.cso'
        source = self.shader_dir / f'{name}.hlsl'

        run_command(
            f'"{fxc}" "{source}" -E vert -T vs_5_0 -nologo -Fo "{vsoutput}"')
        run_command(
            f'"{fxc}" "{source}" -E frag -T ps_5_0 -nologo -Fo "{fsoutput}"')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('config')
    args = parser.parse_args()

    print(f'Opening config file {args.config}')
    with open(args.config, 'r') as f:
        shader_dir = f.readline().strip()
        out_dir = f.readline().strip()

        logger = Logger(str(Path(out_dir) / 'post_build_log.txt'))

        builder = ShaderBuilder(shader_dir, out_dir)

        builder.build_shader('forward_pbr')
        builder.build_shader('debug')
        # builder.build_shader('brdf')
        # builder.build_shader('new_brdf')
        # builder.build_shader('sky')
        # builder.build_shader('exposure')
        # builder.build_shader('wireframe')
        # builder.build_shader('fst')
        # builder.build_shader('oit_accum')
        # builder.build_shader('oit_composite')
        # builder.build_shader('ssao')
