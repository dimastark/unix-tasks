import os
import subprocess as sp

from os import path

import pytest


CURRENT_DIR = path.dirname(path.abspath(__file__))
OUTPUT_FILE = path.join(CURRENT_DIR, 'out.txt')
PROGRAM_FILE = path.join(CURRENT_DIR, '..', 'numbers.c')
CASES_DIR = path.join(CURRENT_DIR, 'cases')


def get_test_parameters():
    for case in os.listdir(CASES_DIR):
        case_path = path.join(CASES_DIR, case)
        case_files = os.listdir(case_path)

        input_files = [f for f in case_files if f.startswith('in')]
        input_files = [path.join(case_path, f) for f in input_files]

        output_file = next((f for f in case_files if f == 'out.txt'))
        output_file = path.join(case_path, output_file)

        yield [input_files, output_file]


def get_result(file_path):
    with open(file_path) as file:
        return ' '.join(file.read().split())


def setup_module():
    sp.run(['clang', PROGRAM_FILE, '-o', 'numbers']).check_returncode()


def teardown_module():
    sp.run(['rm', 'numbers']).check_returncode()


def setup():
    sp.run(['touch', OUTPUT_FILE]).check_returncode()


def teardown():
    sp.run(['rm', OUTPUT_FILE]).check_returncode()


@pytest.mark.parametrize('input_files,expected_output_file', get_test_parameters())
def test(input_files, expected_output_file):
    sp.run(['./numbers'] + input_files + [OUTPUT_FILE]).check_returncode()

    expected = get_result(expected_output_file)
    actual = get_result(OUTPUT_FILE)

    assert expected == actual
