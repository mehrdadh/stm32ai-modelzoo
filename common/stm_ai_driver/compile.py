###################################################################################
#   Copyright (c) 2022 STMicroelectronics.
#   All rights reserved.
#   This software is licensed under terms that can be found in the LICENSE file in
#   the root directory of this software component.
#   If no LICENSE file comes with this software, it is provided AS-IS.
###################################################################################
"""
STM AI driver - Command to compile a model
"""

import os
import logging
import glob
import shutil
from typing import Union

from .session import STMAiSession
from .stm_ai_tools import STMAiTools
from .board_config import STMAiBoardConfig
from .utils import STMAICException
from .c_graph_loader import NetworkCGraphReader

from .utils import run_shell_cmd
from .options import STMAiCompileOptions
from ._stm32_app import stm32app_build
from ._read_gcc_map import CReadAndParseGccMap
from .stm32_tools import STM32_TOOLS as STM32Tools


logger = logging.getLogger('STMAIC')


def _move_generated_files(session: STMAiSession):
    """Move the generated files to the session storage location"""

    src_dir = session.workspace
    dst_dir = session.generated_dir
    c_name = session.c_name

    logger.debug(f' moving files to session storage location.. {dst_dir}')

    if not c_name or not os.path.isdir(src_dir):
        logger.error(f'c_name or/and workspace dir are not valid: {c_name} {src_dir}')
        return

    os.makedirs(dst_dir, exist_ok=True)

    # move the generated c-files
    gen_dir = os.path.join(src_dir, f'inspector_{c_name}', 'workspace', 'generated')
    cdts = glob.glob(os.path.join(gen_dir, f'{c_name}*.[ch]'))
    if not cdts:
        cdts = glob.glob(os.path.join(src_dir, f'{c_name}*.[ch]'))
    for cdt in cdts:
        shutil.move(cdt, dst_dir)

    # move the generated shared libs
    lib_dir = os.path.join(src_dir, f'inspector_{c_name}', 'workspace', 'lib')
    cdts = glob.glob(os.path.join(lib_dir, f'libai_{c_name}*.*'))
    if cdts:  # shared lib has been generated
        _, ext = os.path.splitext(cdts[0])
        cdts.extend([os.path.join(lib_dir, f'libai_observer{ext}')])
        shutil.move(cdts[0], dst_dir)
        shutil.copy(cdts[1], dst_dir)

    # move the report files
    report_file = os.path.join(src_dir, f'{c_name}_generate_report.txt')
    if os.path.isfile(report_file):
        shutil.move(report_file, dst_dir)


def _copy_ai_runtime_files(session: STMAiSession):
    """Copy the AI runtime files to the session storage location"""

    series = session.series
    if not series or not series.startswith('stm32'):
        return

    logger.debug(' copying the runtime lib files..')

    tools = session.tools
    dst_dir = session.generated_dir
    inc_dir = tools.ai_runtime_inc()
    lib = tools.ai_runtime_lib(series=series)

    lib_dst_dir = os.path.join(dst_dir, 'Lib')
    if os.path.isdir(lib_dst_dir):
        shutil.rmtree(lib_dst_dir)
    os.makedirs(lib_dst_dir, exist_ok=True)
    shutil.copy(lib, lib_dst_dir)

    lib_dst_dir = os.path.join(dst_dir, 'Inc')
    if os.path.isdir(lib_dst_dir):
        shutil.rmtree(lib_dst_dir)
    shutil.copytree(inc_dir, lib_dst_dir)


def cmd_compile(
        session: STMAiSession,
        options: STMAiCompileOptions = STMAiCompileOptions(),
        target: Union[STMAiBoardConfig, str, None] = None,
        tools: Union[STMAiTools, None] = None
) -> int:
    """Compile a model file (generate the c-model files)

    Parameters
    ----------
    session: STMAiSession
        model session

    options: STMAiCompileOptions
        STM.AI options. If not provided, default options are used

    target: str or STMAiBoardConfig (optional)
        indicate a board configuration or STM32 series. If not defined,
        a generic board configuration is used the 'x86' target.

    tools: STMAiTools
        STM.AI tools used to compile the model. Note that if a STMAiTools
        object is already attached to the session, it will be replaced. If
        no STMAiTools object is attached to the session, an exception is
        trigged.

    Returns
    -------
    int
        0 is compilation is OK, otherwise returned error value

    """

    # attach the tools to the session
    if tools is not None:
        session.set_tools(tools)

    if not session.tools:
        raise STMAICException("No STM.AI tools is defined to compile the model..")

    # set the target/board configuration
    if target and isinstance(target, str):
        # attach a generic board configuration with the provided series/target name
        session.set_board(STMAiBoardConfig(target))
    elif target and isinstance(target, STMAiBoardConfig):
        # attach the provided board configuration
        session.set_board(target)
    elif not session.board:
        # attach a generic board configuration (x86 target)
        session.set_board(STMAiBoardConfig())

    # make a copy of the compilation options
    copy_opt = STMAiCompileOptions(**options.__dict__)
    session.set_options(copy_opt)

    series = session.series
    output_dir = session.workspace

    logger.info('compiling.. "{}" session'.format(session.name))
    logger.info(' model_path  : {}'.format(session.model_path))

    if session.is_empty:
        logger.error('no model file(s) is available')
        return -2

    # remove temporary build-dir(s)
    session.clean_workspace()

    # build the command to call the CLI
    cmd_line = session.tools.executable()

    cmd_line.extend(['generate'])
    for m_file in session.model_path:
        cmd_line.extend(['-m', m_file])
    cmd_line.extend(['--output', output_dir, '--workspace', output_dir])

    if series not in ('x86', 'default', 'generic'):
        cmd_line.extend(['--series', str(series)])
    session.options.dll = True

    if session.tools.version > '7.1':
        session.options.quiet = True

    cmd_line.extend([session.options.to_cli_args(str(session.tools))])

    logger.info(' tools       : {}'.format(session.tools))
    logger.info(' target      : {}'.format(session.board))
    logger.info(' options     : {}'.format(cmd_line[-1]))

    # call the command line
    err, _ = run_shell_cmd(cmd_line, logger=logger)

    # post-process the results
    if err == 0:
        c_graph = NetworkCGraphReader(os.path.join(output_dir, f'{session.c_name}_c_graph.json'), series)
        session.set_c_graph(c_graph)

        _move_generated_files(session)
        _copy_ai_runtime_files(session)

        # this part of code will be integrated in CLI in the future release
        stm32_gcc = STM32Tools().get_compiler()
        if series not in ('x86', 'default') and stm32_gcc:
            logger.info(f'building dummy stm32 app.. series={series} tools={session.tools}')
            map_file = stm32app_build(session, stm32_gcc[0])
            gcc_map = CReadAndParseGccMap(map_file, logger=logger)
            if logger.getEffectiveLevel() <= logging.DEBUG:
                gcc_map.summary()
            res = gcc_map.get_info_modules(filters=[f'{session.c_name}', 'NetworkR', 'network_runtime', 'libm'])
            gcc_map.summary_modules(filters=res, pr_f=logger.info)
            session.c_graph().add_memory_footprint(res, series)
        # end-of-code

        logger.info(' results -> {}'.format(session))

    return err
