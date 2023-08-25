import sys
import argparse
import subprocess
import os
import platform
import shutil

from datetime import datetime

test_process = ""
script_path = os.path.abspath(sys.argv[0])
script_dir = os.path.dirname(script_path)
root_path = os.path.join(script_dir, "..")

taos_odbc_lib_file = "libtaos_odbc.so.0.1"   # different name on mac or linux
taos_odbc_lib_ln = "libtaos_odbc.so"   # different name on mac or linux

class ReleaseInfo:
    def __init__(self, os):
        self.OS = os
        self.CpuType = ""
        self.DefaultBuildMode = "Release"
        self.TaosODBCVersion = ""
        self.BuildPath = ""
        self.ReleasePath = ""
        self.PackageName = ""
        self.Branch = ""
        self.Commit = ""
        self.BuildTime = ""
    def print(self):
        for attr in dir(self):
            if not attr.startswith("__"):
                value = getattr(self, attr)
                if not callable(value):
                    print(f"{attr:<20}: {value}")
release_info = ReleaseInfo(platform.system())

def GetCpuType():
    type = ""
    arch, _ = platform.architecture()
    machine = platform.machine()

    if arch == '32bit':
        type =  '32'
    elif arch == '64bit':
        if machine.startswith('arm'):
            type = 'arm64'
        elif machine in ('x86_64', 'amd64',  'AMD64'):
            type =  'x64'
        elif machine == 'aarch64':
            type =  'AArch64'
        else:
            type =  f'Unknown architecture: {machine}'
            print(f'Get cpu type failed! {machine}')
            sys.exit()
    else:
        type =  f'Unknown architecture: {arch}'
        print(f'Get cpu type failed! {arch}')
        sys.exit()
    return type

def get_taos_odbc_version():
    version = "1.0.0"
    return version

def get_package_name():
    global taos_odbc_lib_file, taos_odbc_lib_ln
    target = "taos_odbc"
    if release_info.OS == 'Darwin':
        taos_odbc_lib_file = "libtaos_odbc.0.1.dylib"
        taos_odbc_lib_ln = "libtaos_odbc.dylib"

    if release_info.OS == 'Windows':
        return  f'{target}-{release_info.TaosODBCVersion}-{release_info.OS.lower()}-{release_info.CpuType.lower()}-installer'
    else:
        return f'{target}-{release_info.TaosODBCVersion}-{release_info.OS.lower()}-{release_info.CpuType.lower()}-installer'

def init_build_info():
    global release_info, test_process
    parser = argparse.ArgumentParser()

    parser.add_argument('-b', '--build_mode', help='Debug or Release, Release by default')
    parser.add_argument('-c', '--cpu_type', help='cpu [aarch32 | aarch64 | x64 | x86 | mips64 | loongarch64 ...] ')
    parser.add_argument('-t', '--test_process', help='test single process(pi,opc,mqtt,taosx, package)')

    args, unknown_args = parser.parse_known_args()

    if unknown_args:
        print(f"Unknown args: {unknown_args}")
        sys.exit()

    release_info.ReleasePath = os.path.abspath(os.path.join(script_dir, "..", "release"))
    release_info.BuildPath = os.path.abspath(os.path.join(script_dir, "..", "build"))
    release_info.CpuType = GetCpuType()
    if args.build_mode:
        release_info.DefaultBuildMode = args.build_mode
    if args.cpu_type:
        release_info.CpuType = args.cpu_type
    if args.test_process:
        test_process = args.test_process
    
    release_info.TaosODBCVersion = get_taos_odbc_version()
    release_info.PackageName = get_package_name()

    now = datetime.now()
    release_info.BuildTime = now.strftime("%Y-%m-%d %H:%M:%S")
    branch = subprocess.check_output(['git', 'rev-parse', '--abbrev-ref', 'HEAD']).strip().decode('ascii')
    release_info.Branch = branch
    release_info.Commit = subprocess.check_output(['git', 'rev-list', '-1', branch]).strip().decode('ascii')

def print_param():
    print('RELEASE INFO')
    release_info.print()

def rm_directory(path):
    try:
        shutil.rmtree(path)
    except FileNotFoundError:
        pass
    except Exception as e:
        print('Error:', e)
        sys.exit()

def init_directory(path):
    rm_directory(path)
    check_directory(path)

def check_directory(path):
    try:
        if not os.path.exists(path):
            os.makedirs(path)
    except Exception as e:
        print('Error:', e)
        sys.exit()

def build_taos_odbc():
    rm_directory(release_info.BuildPath)
    if release_info.OS == 'Windows':
        build_taos_odbc_on_windows()
    else:
        build_taos_odbc_not_windows()

def set_win_dev_env():
    output = os.popen('\"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvarsall.bat\" x64 && set').read()

    for line in output.splitlines():
        pair = line.split("=", 1)
        if(len(pair) >= 2):
            os.environ[pair[0]] = pair[1]

def copy_taos_odbc_on_windows():
    print("copy_taos_odbc on windows start...")
    taos_lib_src = os.path.join(release_info.BuildPath, "src", release_info.DefaultBuildMode, "taos_odbc.lib")
    taos_bin_src = os.path.join(release_info.BuildPath, "src", release_info.DefaultBuildMode, "taos_odbc.dll")

    taos_lib_dst = os.path.join(release_info.ReleasePath, "taos_odbc", "lib")
    taos_bin_dst = os.path.join(release_info.ReleasePath, "taos_odbc", "bin")

    init_directory(taos_lib_dst)
    init_directory(taos_bin_dst)

    if os.path.isfile(taos_lib_src) and os.path.isfile(taos_bin_src):
        shutil.copy2(taos_lib_src, taos_lib_dst)
        shutil.copy2(taos_bin_src, taos_bin_dst)
    else:
        print(f"not found \"{taos_lib_src}\", or\n"
              f"not found \"{taos_bin_src}\", exit.")
        sys.exit()

    win_odbcinst_src = os.path.join(root_path, "templates", "win_odbcinst.in")
    if os.path.isfile(win_odbcinst_src):
        shutil.copy2(win_odbcinst_src, os.path.join(release_info.ReleasePath, "taos_odbc"))
    else:
        print(f"not found \"{win_odbcinst_src}\"")
        sys.exit()

def update_install_sh(install_sh):
    with open(install_sh, 'r') as file:
        lines = file.readlines()

    for i, line in enumerate(lines):
        if line.startswith("taos_odbc_lib_file="):
            lines[i] = f'taos_odbc_lib_file="{taos_odbc_lib_file}"\n'
        elif line.startswith("taos_odbc_lib_ln="):
            lines[i] = f'taos_odbc_lib_ln="{taos_odbc_lib_ln}"\n'

    with open(install_sh, 'w') as file:
        file.writelines(lines)

def update_odbcinst_in(odbcinst_in_file):
    with open(odbcinst_in_file, 'r') as file:
        lines = file.readlines()

    for i, line in enumerate(lines):
        if line.startswith("Driver="):
            lines[i] = f'Driver={taos_odbc_lib_ln}\n'

    with open(odbcinst_in_file, 'w') as file:
        file.writelines(lines)

def copy_taos_odbc():
    print("copy_taos_odbc on mac start...")
    taos_lib_src = os.path.join(release_info.BuildPath, "src", taos_odbc_lib_file)
    odbc_in_src = os.path.join(root_path, "templates", "odbc.in")
    odbcinst_in_src = os.path.join(root_path, "templates", "odbcinst.in")
    install_sh_src = os.path.join(script_dir, "install.sh")
    
    taos_lib_dst = os.path.join(release_info.ReleasePath, "taos_odbc", "lib")

    init_directory(taos_lib_dst)

    if os.path.isfile(taos_lib_src):
        shutil.copy2(taos_lib_src, taos_lib_dst)
        shutil.copy2(odbc_in_src, os.path.join(release_info.ReleasePath, "taos_odbc"))
        shutil.copy2(odbcinst_in_src, os.path.join(release_info.ReleasePath, "taos_odbc"))
        shutil.copy2(install_sh_src, os.path.join(release_info.ReleasePath))
    else:
        print(f"not found \"{taos_lib_src}\".\n")
        sys.exit()
    update_install_sh(os.path.join(release_info.ReleasePath, "install.sh"))
    update_odbcinst_in(os.path.join(release_info.ReleasePath, "taos_odbc", "odbcinst.in"))

def build_taos_odbc_on_windows():
    print(f"build_taos_odbc_on_windows {release_info.DefaultBuildMode} on windows start...")
    os.chdir(root_path)
    set_win_dev_env()
    os.system('cmake --no-warn-unused-cli -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -B build -G \"Visual Studio 17 2022\" -A x64')
    cmd = f'cmake --build build --config {release_info.DefaultBuildMode} -j 4'
    os.system(cmd)

def build_taos_odbc_not_windows():
    print(f"build_taos_odbc_not_windows {release_info.DefaultBuildMode} start...")
    os.chdir(root_path)
    os.system('cmake -B build -DCMAKE_BUILD_TYPE=Debug')
    cmd = f'cmake --build build'
    os.system(cmd)
    
def package_on_windows():
    print("inno setup start...")
    copy_taos_odbc_on_windows()
    cmd = f'iscc /F"{release_info.PackageName}" '\
        f'/DMyAppVersion="{release_info.TaosODBCVersion}" '\
        f'/DMyAppSourceDir="{release_info.ReleasePath}" '\
        f'{script_dir}/taos_odbc.iss /O{root_path}/release'
    
    print(cmd)
    result = subprocess.run(cmd, shell=True)
    if result.returncode != 0:
        print(f'package {release_info.PackageName} failed')
        sys.exit(1)

def tar_taos_odbc():
    print("\ninsert taos_odbc to tar...")
    copy_taos_odbc()
    
    os.chdir(release_info.ReleasePath)
    cmd = f'tar -zcv -f {release_info.PackageName}.tar.gz *'
    os.system(cmd)
    print(f"Write to:\n{release_info.ReleasePath}/{release_info.PackageName}.tar.gz")
    print(f"\033[32mSuccess.\033[0m\n")
    
def package():
    print("package taos_odbc start...")
    rm_directory(release_info.ReleasePath)
    if release_info.OS == 'Windows':
        package_on_windows()
    else:
        tar_taos_odbc()

def test_handle(process):
    if process == "build":
        build_taos_odbc()
    elif process == "package":
        package()
    else:
        print(f"Invalid -t param: {process}. Please enter valid input.")

if __name__ == '__main__':
    init_build_info()
    print_param()
    if test_process != "":
        test_handle(test_process)
        sys.exit()

    build_taos_odbc()
    package()