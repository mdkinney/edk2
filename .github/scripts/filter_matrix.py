# @file
# Filter matrix combinations for CI workflows.
#
# Copyright (c) 2026, Intel Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

import os
import sys
import argparse
import json

'''
filter_matrix.py
'''

"""
Filter matrix combinations for CI workflows.

This module provides functionality to filter GitHub Actions workflow matrix
combinations based on specified filter criteria. It generates all possible
combinations from provided build parameters and then applies filters to create
a refined matrix for CI execution. The module reads configuration from
environment variables and outputs the filtered matrix combinations to
GITHUB_OUTPUT for use in GitHub Actions workflows.

Environment Variables:
    BUILD_TYPE_LIST (str): JSON array of build types (e.g., ["DEBUG",
        "RELEASE"])
    BUILD_ARCH_LIST (str): JSON array of architectures (e.g., ["IA32", "X64"])
    TOOL_CHAIN_TAG_LIST (str): JSON array of toolchain tags (e.g., ["GCC5",
        "VS2019"])
    PACKAGE_LISTS (str): JSON array of package lists to build
    SKIP_FILTER_LIST (str): JSON array of filter objects to apply to combinations
    CONTINUE_ON_ERROR_FILTER_LIST (str): JSON array of filter objects to apply to combinations
    GITHUB_OUTPUT (str): Path to GitHub Actions output file

Functions:
    apply_filters_to_combination: Apply multiple filters to combinations
    filtered_combination: Apply a single filter to combinations
    generate_filtered_matrix: Generate filtered matrix from environment
        variables
    main: Main entry point that processes environment variables and generates
        filtered matrix

Usage:
    python filter-matrix.py [-v|--verbose]

Args:
    -v, --verbose: Enable verbose output for debugging

Output:
    Writes 'filtered-matrix=<json>' to GITHUB_OUTPUT file for GitHub Actions
    consumption

Example:
    The script processes build matrix combinations and applies filters to
    reduce the number of CI jobs while maintaining coverage of important build
    scenarios.

Note:
    This script is designed specifically for use in GitHub Actions CI workflows
    and requires proper environment variable setup.
"""


def apply_filters_to_combination(combinations: list, filter_list: list,
                                 verbose: bool = False) -> list:
    """
    Apply a series of filters to a list of combinations.

    This function iteratively applies each filter from the filter_list to the
    combinations, with each filter being applied to the result of the previous
    filter operation.

    Args:
        combinations (list): A list of combinations to be filtered.
        filter_list (list): A list of filter items to be applied sequentially.
        verbose (bool, optional): Enable verbose output for debugging.
            Defaults to False.

    Returns:
        list: The filtered combinations after applying all filters from
              filter_list.

    Note:
        This function relies on the `filtered_combination` function to apply
        individual filters to the combinations.
    """
    for filter_item in filter_list:
        if verbose:
            print(f'Applying filter: {filter_item}')
        combinations = filtered_combination(combinations, filter_item, verbose)
    return combinations


def filtered_combination(combinations: list, filter_item: dict,
                         verbose: bool = False) -> list:
    """
    Apply a single filter to a list of combinations.

    This function processes each combination in the input list and applies
    the specified filter. For combinations that match the filter criteria,
    it splits them into separate combinations to isolate filtered values.

    Args:
        combinations (list): A list of combinations to be filtered.
        filter_item (dict): A filter item containing key-value pairs to be
                           applied to the combinations.
        verbose (bool, optional): Enable verbose output for debugging.
            Defaults to False.

    Returns:
        list: The filtered combinations after applying the filter. Matching
              combinations may be split into multiple entries.
    """
    # Initialize list to store filtered combinations
    filtered_combinations = []

    # Process each combination in the input list
    for combo in combinations:
        # Check if this combination matches the filter criteria
        match = True
        for key, value in filter_item.items():
            # Check if the filter value exists in the combo value
            if value not in combo.get(key, '').split(','):
                match = False
                break

        # If no match found, add the original combination unchanged
        if not match:
            if verbose:
                print('[no filter] combo:', combo)
            filtered_combinations.append(combo)
            continue

        # Create a copy of the combination to modify for the filtered version
        filtered_combo = combo.copy()
        non_filtered_combo = combo.copy()

        # Track if we need to create both versions
        should_add_filtered = True
        should_add_non_filtered = True

        # Process each filter key-value pair
        for key, value in filter_item.items():
            combo_values = combo.get(key, '').split(',')

            # Create version without the filtered value
            combo_value_without_filter = ','.join([
                v for v in combo_values if v != value
            ])

            # Create version with only the filtered value
            combo_value_with_filter = ','.join([
                v for v in combo_values if v == value
            ])

            # Update combinations for this key
            if combo_value_without_filter:
                non_filtered_combo[key] = combo_value_without_filter
            else:
                should_add_non_filtered = False

            if combo_value_with_filter:
                filtered_combo[key] = combo_value_with_filter
            else:
                should_add_filtered = False

        # Add the non-filtered combination if it has remaining values
        if should_add_non_filtered:
            if verbose:
                print('[add split] non_filtered_combo:', non_filtered_combo)
            filtered_combinations.append(non_filtered_combo)

        # Add the filtered combination with continue_on_error set to 'true'
        if should_add_filtered:
            filtered_combo['continue_on_error'] = 'true'
            if verbose:
                print('[add keep] filtered_combo:', filtered_combo)
            filtered_combinations.append(filtered_combo)

    return filtered_combinations


def generate_filtered_matrix(verbose: bool = False) -> list:
    """
    Generate filtered matrix combinations from environment variables.

    This function reads build parameters from environment variables,
    generates all possible combinations using a Cartesian product,
    applies filters to reduce the matrix size, and returns the filtered matrix.

    Args:
        verbose (bool, optional): Enable verbose output for debugging.
            Defaults to False.

    Returns:
        list: The filtered combinations after applying all filters.

    Raises:
        json.JSONDecodeError: If environment variables contain invalid JSON.

    Environment Variables Used:
        BUILD_TYPE_LIST: JSON array of build types
        BUILD_ARCH_LIST: JSON array of architectures
        TOOL_CHAIN_TAG_LIST: JSON array of toolchain tags
        PACKAGE_LISTS: JSON array of package lists
        SKIP_FILTER_LIST: JSON array of filter objects
        CONTINUE_ON_ERROR_FILTER_LIST: JSON array of filter objects
    """
    # Display input values if verbose mode is enabled
    if verbose:
        print('Inputs values...')
        print('  build-type-list:', os.environ.get('BUILD_TYPE_LIST', '[]'))
        print('  build-arch-list:', os.environ.get('BUILD_ARCH_LIST', '[]'))
        print('  tool-chain-tag-list:',
              os.environ.get('TOOL_CHAIN_TAG_LIST', '[]'))
        print('  package-lists:', os.environ.get('PACKAGE_LISTS', '[]'))
        print('  skip-filter-list:', os.environ.get('SKIP_FILTER_LIST', '[]'))
        print('  continue-on-error-filter-list:', os.environ.get('CONTINUE_ON_ERROR_FILTER_LIST', '[]'))

    # Parse JSON environment variables into Python lists
    build_type_list = json.loads(os.environ.get('BUILD_TYPE_LIST', '[]') or '[]')
    build_arch_list = json.loads(os.environ.get('BUILD_ARCH_LIST', '[]') or '[]')
    tool_chain_tag_list = json.loads(
        os.environ.get('TOOL_CHAIN_TAG_LIST', '[]') or '[]')
    package_lists = json.loads(os.environ.get('PACKAGE_LISTS', '[]') or '[]')
    skip_filter_list = json.loads(os.environ.get('SKIP_FILTER_LIST', '[]') or '[]')
    continue_on_error_filter_list = json.loads(os.environ.get('CONTINUE_ON_ERROR_FILTER_LIST', '[]') or '[]')

    # Display parsed values if verbose mode is enabled
    if verbose:
        print('Initial build_type_list:', build_type_list)
        print('Initial build_arch_list:', build_arch_list)
        print('Initial tool_chain_tag_list:', tool_chain_tag_list)
        print('Initial package_lists:', package_lists)
        print('Initial skip_filter_list:', skip_filter_list)
        print('Initial continue_on_error_filter_list:', continue_on_error_filter_list)

    # Generate all possible combinations using nested loops
    # This creates a Cartesian product of all build parameters
    combinations = []
    for build_type in build_type_list:
        if not build_type:
            continue
        for build_arch in build_arch_list:
            if not build_arch:
                continue
            for tool_chain_tag in tool_chain_tag_list:
                if not tool_chain_tag:
                    continue
                for build_package in package_lists:
                    if not build_package:
                        continue
                    combinations.append({
                        'build_type': build_type,
                        'build_arch': build_arch,
                        'tool_chain_tag': tool_chain_tag,
                        'build_package': build_package,
                        'continue_on_error': 'false'
                    })

    # Display combinations before filtering if verbose mode is enabled
    if verbose:
        print(f'Total combinations before filtering: {len(combinations)}')
        for i, combo in enumerate(combinations):
            print(f'  {i+1}: {combo}')

    # Apply filter to reduce the number of jobs to run by removing
    # combinations that should be skipped based on the skip_filter_list
    filtered_combinations = apply_filters_to_combination(combinations,
                                                         skip_filter_list,
                                                         verbose)
    filtered_combinations = [x for x in filtered_combinations
                           if x['continue_on_error'] == 'false']

    # Apply filter to tag combinations that should be run, but continue on error
    filtered_combinations = apply_filters_to_combination(
        filtered_combinations, continue_on_error_filter_list, verbose)

    # Display filtered combinations if verbose mode is enabled
    if verbose:
        print(f'Total combinations after applying filters: '
              f'{len(filtered_combinations)}')
        for i, combo in enumerate(filtered_combinations):
            print(f'  {i+1}: {combo}')

        print(f'filtered-matrix={json.dumps(filtered_combinations)}')

    return filtered_combinations


def main():
    """
    Main entry point for the filter-matrix script.

    This function processes command line arguments, generates filtered matrix
    combinations from environment variables, and outputs the result to GitHub
    Actions via the GITHUB_OUTPUT file.

    Returns:
        int: 0 on success, 1 on error.

    Raises:
        KeyError: If GITHUB_OUTPUT environment variable is not set.
        json.JSONDecodeError: If environment variables contain invalid JSON.
        Exception: For any other unexpected errors during processing.

    Command Line Arguments:
        -v, --verbose: Enable verbose output for debugging
    """
    try:
        # Parse command line arguments
        parser = argparse.ArgumentParser(
            description='Filter matrix combinations for CI workflows')
        parser.add_argument('-v', '--verbose', action='store_true',
                            help='Enable verbose output')
        args = parser.parse_args()

        # Generate filtered matrix combinations
        filtered_combinations = generate_filtered_matrix(args.verbose)

        # Write the filtered matrix to GITHUB_OUTPUT for GitHub Actions
        # consumption. This allows the matrix to be used in subsequent
        # workflow steps
        github_output = os.environ['GITHUB_OUTPUT']
        with open(github_output, 'a') as f:
            output = f'filtered-matrix={json.dumps(filtered_combinations)}\n'
            f.write(output)

        return 0

    except Exception as e:
        # Generate empty combo output on error
        empty_combinations = []
        print(f'Error: {e}')

        # Try to write empty output if GITHUB_OUTPUT is available
        try:
            github_output = os.environ['GITHUB_OUTPUT']
            with open(github_output, 'a') as f:
                output = f'filtered-matrix={json.dumps(empty_combinations)}\n'
                f.write(output)
        except Exception:
            # If even writing to GITHUB_OUTPUT fails, just print the output
            print(f'filtered-matrix={json.dumps(empty_combinations)}')

        return 1


if __name__ == '__main__':
    sys.exit(main())
