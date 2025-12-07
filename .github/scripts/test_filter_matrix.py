# @file
# Unit tests for filter_matrix module to validate CI workflow matrix filtering
# functionality.
#
# Copyright (c) 2026, Intel Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

import os
import sys
import json
import unittest
from unittest.mock import patch, mock_open
import filter_matrix

# Add the script directory to the path to import the module
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))


class TestFilterMatrix(unittest.TestCase):

    def test_apply_filters_to_combination_single_filter(self):
        combinations = [
            {'build_type': 'DEBUG', 'build_arch': 'X64'},
            {'build_type': 'RELEASE', 'build_arch': 'IA32'}
        ]
        filter_list = [{'build_type': 'DEBUG'}]
        result = filter_matrix.apply_filters_to_combination(
            combinations, filter_list)
        self.assertEqual(len(result), 2)

    def test_apply_filters_to_combination_multiple_filters(self):
        combinations = [
            {'build_type': 'DEBUG,RELEASE', 'build_arch': 'X64,IA32'},
            {'build_type': 'RELEASE', 'build_arch': 'X64'}
        ]
        filter_list = [
            {'build_type': 'DEBUG'},
            {'build_arch': 'IA32'}
        ]
        result = filter_matrix.apply_filters_to_combination(
            combinations, filter_list)
        self.assertGreater(len(result), 0)

    def test_filtered_combination_with_comma_separated_values(self):
        combinations = [
            {'build_type': 'DEBUG,RELEASE', 'build_arch': 'X64,IA32'}
        ]
        filter_item = {'build_type': 'DEBUG'}
        result = filter_matrix.filtered_combination(
            combinations, filter_item)

        # Should create separate combinations for filtered and non-filtered
        self.assertEqual(len(result), 2)
        # Check that one has only RELEASE and one has only DEBUG
        types = [combo['build_type'] for combo in result]
        self.assertIn('RELEASE', types)
        self.assertIn('DEBUG', types)

    def test_filtered_combination_multiple_comma_separated_filters(self):
        combinations = [
            {
                'build_type': 'DEBUG,RELEASE,NOOPT',
                'build_arch': 'X64,IA32,ARM',
                'tool_chain_tag': 'GCC5,VS2019,CLANG'
            }
        ]
        filter_item = {'build_type': 'DEBUG', 'build_arch': 'X64'}
        result = filter_matrix.filtered_combination(
            combinations, filter_item)

        # Should split combinations based on filter criteria
        self.assertGreater(len(result), 1)

        # Verify that filtered values are properly separated
        debug_x64_combo = None
        for combo in result:
            if combo['build_type'] == 'DEBUG' and combo['build_arch'] == 'X64':
                debug_x64_combo = combo
                break

        self.assertIsNotNone(debug_x64_combo)

    def test_filtered_combination_no_match(self):
        combinations = [
            {'build_type': 'RELEASE', 'build_arch': 'X64'}
        ]
        filter_item = {'build_type': 'DEBUG'}
        result = filter_matrix.filtered_combination(
            combinations, filter_item)

        # Should return original combination unchanged
        self.assertEqual(len(result), 1)
        self.assertEqual(result[0], combinations[0])

    def test_filtered_combination_partial_match(self):
        combinations = [
            {'build_type': 'DEBUG,RELEASE', 'build_arch': 'X64'}
        ]
        filter_item = {'build_type': 'DEBUG', 'build_arch': 'IA32'}
        result = filter_matrix.filtered_combination(
            combinations, filter_item)

        # Should not match because build_arch doesn't contain IA32
        self.assertEqual(len(result), 1)
        self.assertEqual(result[0]['build_type'], 'DEBUG,RELEASE')

    def test_filtered_combination_complex_comma_separated(self):
        combinations = [
            {
                'build_type': 'DEBUG,RELEASE',
                'build_arch': 'X64,IA32,ARM',
                'tool_chain_tag': 'GCC5,VS2019'
            }
        ]
        filter_item = {'build_arch': 'IA32'}
        result = filter_matrix.filtered_combination(
            combinations, filter_item)

        # Should create two combinations: one without IA32, one with only IA32
        self.assertEqual(len(result), 2)

        arch_values = [combo['build_arch'] for combo in result]
        self.assertIn('X64,ARM', arch_values)
        self.assertIn('IA32', arch_values)

    @patch.dict(os.environ, {
        'BUILD_TYPE_LIST': '["DEBUG", "RELEASE"]',
        'BUILD_ARCH_LIST': '["X64", "IA32"]',
        'TOOL_CHAIN_TAG_LIST': '["GCC5", "VS2019"]',
        'PACKAGE_LISTS': '["Package1", "Package2"]',
        'SKIP_FILTER_LIST': '[{"build_type": "DEBUG"}]'
    })
    def test_generate_filtered_matrix_with_environment(self):
        result = filter_matrix.generate_filtered_matrix()
        self.assertIsInstance(result, list)
        self.assertGreater(len(result), 0)

    @patch.dict(os.environ, {
        'BUILD_TYPE_LIST': '["DEBUG,RELEASE"]',
        'BUILD_ARCH_LIST': '["X64,IA32"]',
        'TOOL_CHAIN_TAG_LIST': '["GCC5"]',
        'PACKAGE_LISTS': '["Package1"]',
        'SKIP_FILTER_LIST': '[{"build_type": "DEBUG"}]'
    })
    def test_generate_filtered_matrix_with_comma_separated_env(self):
        result = filter_matrix.generate_filtered_matrix()
        self.assertIsInstance(result, list)
        # Should have combinations with comma-separated values
        self.assertGreater(len(result), 0)

    @patch.dict(os.environ, {}, clear=True)
    def test_generate_filtered_matrix_empty_environment(self):
        result = filter_matrix.generate_filtered_matrix()
        self.assertEqual(result, [])

    @patch.dict(os.environ, {
        'BUILD_TYPE_LIST': 'invalid_json',
        'BUILD_ARCH_LIST': '["X64"]',
        'TOOL_CHAIN_TAG_LIST': '["GCC5"]',
        'PACKAGE_LISTS': '["Package1"]',
        'SKIP_FILTER_LIST': '[]'
    })
    def test_generate_filtered_matrix_invalid_json(self):
        with self.assertRaises(json.JSONDecodeError):
            filter_matrix.generate_filtered_matrix()

    def test_filtered_combination_multiple_filter_keys_comma_separated(self):
        combinations = [
            {
                'build_type': 'DEBUG,RELEASE,NOOPT',
                'build_arch': 'X64,IA32',
                'tool_chain_tag': 'GCC5,VS2019,CLANG'
            }
        ]
        filter_item = {
            'build_type': 'RELEASE',
            'tool_chain_tag': 'VS2019'
        }
        result = filter_matrix.filtered_combination(
            combinations, filter_item)

        # Should split based on both filter criteria
        self.assertGreater(len(result), 1)

    @patch('sys.argv', ['filter_matrix.py'])
    @patch.dict(os.environ, {
        'GITHUB_OUTPUT': '/tmp/test_output',
        'BUILD_TYPE_LIST': '["DEBUG"]',
        'BUILD_ARCH_LIST': '["X64"]',
        'TOOL_CHAIN_TAG_LIST': '["GCC5"]',
        'PACKAGE_LISTS': '["Package1"]',
        'SKIP_FILTER_LIST': '[]'
    })
    @patch('builtins.open', mock_open())
    def test_main_success(self):
        result = filter_matrix.main()
        self.assertEqual(result, 0)

    @patch('sys.argv', ['filter_matrix.py', '--verbose'])
    @patch.dict(os.environ, {
        'GITHUB_OUTPUT': '/tmp/test_output',
        'BUILD_TYPE_LIST': '["DEBUG,RELEASE"]',
        'BUILD_ARCH_LIST': '["X64,IA32"]',
        'TOOL_CHAIN_TAG_LIST': '["GCC5"]',
        'PACKAGE_LISTS': '["Package1"]',
        'SKIP_FILTER_LIST': '[{"build_type": "DEBUG"}]'
    })
    @patch('builtins.open', mock_open())
    def test_main_verbose_with_comma_separated(self):
        result = filter_matrix.main()
        self.assertEqual(result, 0)

    @patch('sys.argv', ['filter_matrix.py'])
    @patch.dict(os.environ, {}, clear=True)
    def test_main_missing_github_output(self):
        result = filter_matrix.main()
        self.assertEqual(result, 1)

    def test_apply_filters_empty_filter_list(self):
        combinations = [
            {'build_type': 'DEBUG,RELEASE', 'build_arch': 'X64'}
        ]
        result = filter_matrix.apply_filters_to_combination(
            combinations, [])
        self.assertEqual(result, combinations)

    def test_filtered_combination_empty_combinations(self):
        result = filter_matrix.filtered_combination(
            [], {'build_type': 'DEBUG'})
        self.assertEqual(result, [])

    def test_filtered_combination_preserve_other_fields(self):
        combinations = [
            {
                'build_type': 'DEBUG,RELEASE',
                'build_arch': 'X64',
                'extra_field': 'value'
            }
        ]
        filter_item = {'build_type': 'DEBUG'}
        result = filter_matrix.filtered_combination(
            combinations, filter_item)

        # All combinations should preserve the extra_field
        for combo in result:
            self.assertEqual(combo['extra_field'], 'value')


if __name__ == '__main__':
    unittest.main()
