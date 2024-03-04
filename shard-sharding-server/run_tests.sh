#!/bin/sh

set -e

pip3 install --ignore-installed --upgrade -r requirements/testing_requirements.txt

echo ""
echo "Running black..."
black --line-length 100 --target-version py37 --diff */*/*.py */*.py
black --line-length 100 --target-version py37 --check */*/*.py */*.py

echo ""
echo "Running flake8..."
flake8 --max-line-length=100 */*/*.py */*.py --ignore F405,W503,F403

#echo ""
#echo "Running mypy..."
#mypy --ignore-missing-imports --strict *.py */*.py

# echo ""
# echo "Running pytest..."
# pip3 install --upgrade -r requirements/requirements.txt
# python3 -m coverage run --source *.py -m pytest -vv -s $@

# python3 -m coverage report -m
